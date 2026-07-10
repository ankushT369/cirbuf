CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC
AR = ar
ARFLAGS = rcs

SRC = cirbuf.c
OBJ = cirbuf.o

STATIC = libcirbuf.a
SHARED = libcirbuf.so

all: $(STATIC) $(SHARED)

$(OBJ): $(SRC) cirbuf.h
	$(CC) $(CFLAGS) -c $(SRC)

$(STATIC): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

$(SHARED): $(OBJ)
	$(CC) -shared -o $@ $^

clean:
	rm -f *.o $(STATIC) $(SHARED)

.PHONY: all clean
