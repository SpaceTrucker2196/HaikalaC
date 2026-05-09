# HaikalaC — pure C99, no third-party dependencies.
#
# Tested on macOS (clang) and Linux (gcc). For other POSIX systems any
# C99-conformant compiler with libc and termios.h should work.

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS ?= -lm

PREFIX  ?= /usr/local
BINDIR   = $(PREFIX)/bin

SRC_DIR  = src
INC_DIR  = include
TEST_DIR = tests

SRCS = \
    $(SRC_DIR)/main.c \
    $(SRC_DIR)/terminal.c \
    $(SRC_DIR)/render.c \
    $(SRC_DIR)/mandala.c \
    $(SRC_DIR)/translate.c \
    $(SRC_DIR)/symbols.c \
    $(SRC_DIR)/haiku.c \
    $(SRC_DIR)/animate.c

OBJS    = $(SRCS:.c=.o)
TARGET  = haikalac

TEST_SRCS = $(TEST_DIR)/test_basic.c $(filter-out $(SRC_DIR)/main.c, $(SRCS))
TEST_BIN  = $(TEST_DIR)/test_basic

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/haikala.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRCS) $(INC_DIR)/haikala.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $(TEST_SRCS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_BIN)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
