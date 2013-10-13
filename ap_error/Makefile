PATH1="."

CC=gcc
# -mtune=pentium3 -m32
CCFLAGS=$(OPTS) -Wall -ggdb

obj=ap_error.o

all: lib

lib: $(obj)

%.o: %.c
	$(CC) -c $(CCFLAGS) $< -o $@

clean:
	rm -f *.o
