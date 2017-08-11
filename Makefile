CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -O3
SOURCE = c_npy.c
OBJECTS = $(patsubst %.c,%.o,$(SOURCE))

all: $(OBJECTS)

.PHONY: clean
clean:
	$(RM) *.o *~

