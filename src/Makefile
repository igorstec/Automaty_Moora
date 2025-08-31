# Połączony Makefile dla projektu automatów Moore’a

CC = gcc

# Ścieżki
SOLUTION = .
HEADERS = .

# Flagi kompilatora
CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -O2

# Flagi linkera do biblioteki współdzielonej z wrapami pamięci
LDFLAGS_SHARED = -shared \
	-Wl,--wrap=malloc \
	-Wl,--wrap=calloc \
	-Wl,--wrap=realloc \
	-Wl,--wrap=reallocarray \
	-Wl,--wrap=free \
	-Wl,--wrap=strdup \
	-Wl,--wrap=strndup

# Pliki źródłowe
LIB_SRCS = ma.c memory_tests.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

MA_TESTS_SRCS = ma_tests.c
MA_TESTS_OBJS = $(MA_TESTS_SRCS:.c=.o)

MA_EXAMPLE_SRCS = ma_example.c
MA_EXAMPLE_OBJS = $(MA_EXAMPLE_SRCS:.c=.o)

# Nazwy plików wynikowych
LIB_NAME = libma.so
MA_TESTS = ma_tests
MA_EXAMPLE = ma_example

# Ścieżki do nagłówków
CPPFLAGS = -I$(HEADERS)

vpath %.h $(HEADERS)
vpath %.so $(SOLUTION)

.PHONY: all clean test run valgrind single single-valgrind


all: $(LIB_NAME) $(MA_TESTS) $(MA_EXAMPLE)

# Budowanie biblioteki współdzielonej
$(LIB_NAME): $(LIB_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS_SHARED) $(LIB_OBJS) -o $@

# Budowanie programu testowego ma_tests
$(MA_TESTS): $(MA_TESTS_OBJS) $(LIB_NAME)
	$(CC) $(CFLAGS) $(MA_TESTS_OBJS) -L$(SOLUTION) -lma -o $@

# Budowanie przykładu ma_example
$(MA_EXAMPLE): $(MA_EXAMPLE_OBJS) $(LIB_NAME)
	$(CC) $(CFLAGS) $(MA_EXAMPLE_OBJS) -L$(SOLUTION) -lma -o $@

# Kompilacja plików .o
%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Uruchomienie wybranego testu z ma_example: make run TEST=two
run: $(MA_EXAMPLE)
	LD_LIBRARY_PATH=$(SOLUTION) ./$(MA_EXAMPLE) $(TEST)

# Uruchomienie wybranego testu z ma_example pod Valgrind: make valgrind TEST=two
valgrind: $(MA_EXAMPLE)
	LD_LIBRARY_PATH=$(SOLUTION) valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(MA_EXAMPLE) $(TEST)

# Lista testów automatycznych
TESTS = one two connections undetermined delete params malicious pipeline shift cycle alloc memory weak disconnect

# Uruchomienie wszystkich testów z ma_tests i sprawdzenie przez Valgrind
test: $(MA_TESTS)
	@echo "Running all tests with valgrind..."
	@for test in $(TESTS); do \
		if LD_LIBRARY_PATH=$(SOLUTION) /bin/time -f%U ./$(MA_TESTS) $$test && \
		   LD_LIBRARY_PATH=$(SOLUTION) valgrind -q --error-exitcode=123 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all ./$(MA_TESTS) $$test; then \
			echo "$$test pass"; \
		else \
			echo "$$test fail"; \
		fi; \
	done

# Uruchomienie pojedynczego testu
single: $(MA_TESTS)
	LD_LIBRARY_PATH=$(SOLUTION) ./$(MA_TESTS) $(TEST)

# Uruchomienie pojedynczego testu pod Valgrind
single-valgrind: $(MA_TESTS)
	LD_LIBRARY_PATH=$(SOLUTION) valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(MA_TESTS) $(TEST)

clean:
	rm -f $(LIB_OBJS) $(MA_TESTS_OBJS) $(MA_EXAMPLE_OBJS) $(LIB_NAME) $(MA_TESTS) $(MA_EXAMPLE)
