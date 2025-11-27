# Compiler prefix for cross-compilation.
# Usage for cross: make CROSS_COMPILE=aarch64-none-linux-gnu-
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc

# Target binary name
TARGET := writer

# Source files for the project
SRCS := writer.c

# Default target: build the "writer" application
$(TARGET): $(SRCS)
	$(CC) -o $@ $(SRCS)

# Clean target: remove binary and all object files
.PHONY: clean
clean:
	rm -f $(TARGET)

