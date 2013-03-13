PATH1="."

CC=gcc
OPTS=-Wall
# -mtune=pentium3 -m32

all:  apstr.o aptcp.o b64.o

apstr.o: apstr.c
	$(CC) -c $(OPTS) apstr.c -o apstr.o

aptcp.o: aptcp.c
	$(CC) -c $(OPTS) aptcp.c -o aptcp.o

b64.o: b64.c
	$(CC) -c $(OPTS) b64.c -o b64.o

clean:
	rm -f *.o
