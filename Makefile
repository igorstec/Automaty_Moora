# Makefile for Moore automata project with memory tests and debugging

CC = gcc
CFLAGS = -std=gnu17 -Wall -Wextra -pedantic -g -fPIC
LDFLAGS = -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc \
          -Wl,--wrap=reallocarray -Wl,--wrap=free \
          -Wl,--wrap=strdup -Wl,--wrap=strndup

# Source and object files
SRCS = ma_example.c ma.c memory_tests.c
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = ma_example

.PHONY: all clean run valgrind debug

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run a specific test: make run TEST=two
run: $(TARGET)
	./$(TARGET) $(TEST)

# Run a specific test under Valgrind: make valgrind TEST=two
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET) $(TEST)

# Debug with GDB: make debug TEST=two
debug: $(TARGET)
	gdb --args ./$(TARGET) $(TEST)

clean:
	rm -f $(OBJS) $(TARGET)
