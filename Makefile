CC = gcc
DEFINES = -DVERBOSE
CFLAGS = -std=gnu99 -Wall -Wextra -O3 $(DEFINES)
SOURCE = c_npy.c
OBJECTS = $(patsubst %.c,%.o,$(SOURCE))
LIBS = $(OBJECTS)

all: $(OBJECTS)

example: $(OBJECTS)

.PHONY: clean
clean:
	$(RM) *.o *~

#gcc -std=gnu99 -Wall -Wextra -O3 -c example.c
#gcc -o example example.o c_npy.o
