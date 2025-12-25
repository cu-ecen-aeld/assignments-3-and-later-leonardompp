# Compiler prefix for cross-compilation.
# Usage for cross: make CROSS_COMPILE=aarch64-none-linux-gnu-
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc

# Target binary name
TARGET := aesdsocket

# Source files for the project
SRCS := aesdsocket.c

.PHONY: all default
all: $(TARGET)
default: $(TARGET)

# Default target: build the "writer" application
$(TARGET): $(SRCS)
	$(CC) -o $@ $(SRCS)

# Clean target: remove binary and all object files
.PHONY: clean
clean:
	rm -f $(TARGET)

