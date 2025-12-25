#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#define PORT 9000
#define DATAFILE "/var/tmp/aesdsocketdata"

static volatile sig_atomic_t g_exit_requested = 0;
static int g_listen_fd = -1;
static int g_client_fd = -1;

static void handle_signal(int signo)
{
    (void)signo;
    g_exit_requested = 1;

    // close() is async-signal-safe; helps unblock accept()/recv()
    if (g_client_fd >= 0) {
        close(g_client_fd);
        g_client_fd = -1;
    }
    if (g_listen_fd >= 0) {
        close(g_listen_fd);
        g_listen_fd = -1;
    }
}

static int install_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    // Intentionally do NOT set SA_RESTART so accept()/recv() can return on signal
    if (sigaction(SIGINT, &sa, NULL) != 0) return -1;
    if (sigaction(SIGTERM, &sa, NULL) != 0) return -1;
    return 0;
}

static int send_file_to_client(int client_fd)
{
    int fd = open(DATAFILE, O_RDONLY);
    if (fd < 0) return -1;

    char buf[4096];
    for (;;) {
        ssize_t r = read(fd, buf, sizeof(buf));
        if (r == 0) break; // EOF
        if (r < 0) {
            if (errno == EINTR) continue;
            close(fd);
            return -1;
        }

        ssize_t off = 0;
        while (off < r) {
            ssize_t s = send(client_fd, buf + off, (size_t)(r - off), 0);
            if (s < 0) {
                if (errno == EINTR) continue;
                close(fd);
                return -1;
            }
            off += s;
        }
    }

    close(fd);
    return 0;
}

static int append_packet_to_file(const char *packet, size_t len)
{
    int fd = open(DATAFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return -1;

    size_t off = 0;
    while (off < len) {
        ssize_t w = write(fd, packet + off, len - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            close(fd);
            return -1;
        }
        off += (size_t)w;
    }

    close(fd);
    return 0;
}

/**
 * Daemonize AFTER we've successfully bound/listened.
 * Requirement: "fork after ensuring it can bind to port 9000"
 */
static int daemonize_after_bind(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid > 0) {
        // Parent exits; child continues
        _exit(0);
    }

    // Child becomes session leader
    if (setsid() < 0) {
        return -1;
    }

    // Optional second fork is common, but not required by your spec.
    // We'll keep it simple and compliant.

    // Make sure daemon doesn't keep a working directory mounted
    if (chdir("/") != 0) {
        return -1;
    }

    // Redirect stdin/out/err to /dev/null
    int devnull = open("/dev/null", O_RDWR);
    if (devnull < 0) {
        return -1;
    }

    if (dup2(devnull, STDIN_FILENO) < 0) { close(devnull); return -1; }
    if (dup2(devnull, STDOUT_FILENO) < 0) { close(devnull); return -1; }
    if (dup2(devnull, STDERR_FILENO) < 0) { close(devnull); return -1; }

    if (devnull > STDERR_FILENO) {
        close(devnull);
    }

    // Keep syslog available (openlog already called in main)
    return 0;
}

static int setup_listen_socket(void)
{
    g_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen_fd < 0) {
        syslog(LOG_ERR, "socket() failed: %s", strerror(errno));
        return -1;
    }

    int optval = 1;
    if (setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
        syslog(LOG_ERR, "setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(g_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        syslog(LOG_ERR, "bind() failed: %s", strerror(errno));
        return -1;
    }

    if (listen(g_listen_fd, 10) != 0) {
        syslog(LOG_ERR, "listen() failed: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    bool daemon_mode = false;

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = true;
    } else if (argc != 1) {
        // Keep it simple: only accept optional -d
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        return -1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);

    if (install_signal_handlers() != 0) {
        syslog(LOG_ERR, "Failed installing signal handlers: %s", strerror(errno));
        closelog();
        return -1;
    }

    // Ensure we can bind/listen first (per requirement)
    if (setup_listen_socket() != 0) {
        if (g_listen_fd >= 0) close(g_listen_fd);
        g_listen_fd = -1;
        closelog();
        return -1;
    }

    // Daemonize only AFTER successful bind/listen
    if (daemon_mode) {
        if (daemonize_after_bind() != 0) {
            syslog(LOG_ERR, "daemonize failed: %s", strerror(errno));
            if (g_listen_fd >= 0) close(g_listen_fd);
            g_listen_fd = -1;
            closelog();
            return -1;
        }
    }

    while (!g_exit_requested) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        g_client_fd = accept(g_listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (g_client_fd < 0) {
            if (g_exit_requested) break;
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "accept() failed: %s", strerror(errno));
            continue;
        }

        char ipstr[INET_ADDRSTRLEN];
        const char *ip = inet_ntop(AF_INET, &client_addr.sin_addr, ipstr, sizeof(ipstr));
        if (!ip) ip = "unknown";
        syslog(LOG_INFO, "Accepted connection from %s", ip);

        char *packet = NULL;
        size_t packet_len = 0;
        size_t packet_cap = 0;

        bool done_with_client = false;
        while (!done_with_client && !g_exit_requested) {
            char rbuf[1024];
            ssize_t r = recv(g_client_fd, rbuf, sizeof(rbuf), 0);
            if (r == 0) {
                done_with_client = true;
                break;
            }
            if (r < 0) {
                if (errno == EINTR) continue;
                syslog(LOG_ERR, "recv() failed: %s", strerror(errno));
                done_with_client = true;
                break;
            }

            if (packet_len + (size_t)r > packet_cap) {
                size_t new_cap = packet_cap ? packet_cap : 2048;
                while (new_cap < packet_len + (size_t)r) new_cap *= 2;

                char *new_buf = realloc(packet, new_cap);
                if (!new_buf) {
                    syslog(LOG_ERR, "realloc() failed, dropping connection");
                    free(packet);
                    packet = NULL;
                    done_with_client = true;
                    break;
                }
                packet = new_buf;
                packet_cap = new_cap;
            }

            memcpy(packet + packet_len, rbuf, (size_t)r);
            packet_len += (size_t)r;

            for (;;) {
                void *nlp = memchr(packet, '\n', packet_len);
                if (!nlp) break;

                size_t one_len = (char *)nlp - packet + 1; // include '\n'
                if (append_packet_to_file(packet, one_len) != 0) {
                    syslog(LOG_ERR, "append_packet_to_file() failed: %s", strerror(errno));
                    free(packet);
                    packet = NULL;
                    done_with_client = true;
                    break;
                }

                if (send_file_to_client(g_client_fd) != 0) {
                    syslog(LOG_ERR, "send_file_to_client() failed: %s", strerror(errno));
                    free(packet);
                    packet = NULL;
                    done_with_client = true;
                    break;
                }

                size_t remaining = packet_len - one_len;
                memmove(packet, packet + one_len, remaining);
                packet_len = remaining;
            }
        }

        free(packet);

        syslog(LOG_INFO, "Closed connection from %s", ip);

        if (g_client_fd >= 0) {
            close(g_client_fd);
            g_client_fd = -1;
        }
    }

    syslog(LOG_INFO, "Caught signal, exiting");
    unlink(DATAFILE);

    if (g_listen_fd >= 0) {
        close(g_listen_fd);
        g_listen_fd = -1;
    }

    closelog();
    return 0;
}
