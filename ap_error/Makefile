PATH1="."

CC=gcc

obj=ap_error.o

all: lib

lib: $(obj)

%.o: %.c
	$(CC) -c $(OPTS) $< -o $@

clean:
	rm -f *.o
