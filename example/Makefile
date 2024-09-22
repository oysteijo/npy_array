CC ?= gcc
CFLAGS += -std=c99 -Wall -Wextra -O3 -I..
LDLIBS += -L.. -lnpy_array

SOURCES = $(wildcard *.c)
PROGRAMS = $(patsubst %.c,%,$(SOURCES))

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS) *.npz *.npy

.PHONY: all clean

