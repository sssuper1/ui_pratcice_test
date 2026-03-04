TARGET ?= ui_practice

CC ?= gcc
CSTD ?= gnu99
CFLAGS ?= -O2 -g -std=$(CSTD)
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?= -pthread

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

.PHONY: all clean rebuild run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

rebuild: clean all

run: $(TARGET)
	./$(TARGET)
