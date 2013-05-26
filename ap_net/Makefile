PATH1="."

CC=gcc
DEFOPTS=-Wall
# -mtune=pentium3 -m32

OBJDIR=../compiled
OBJ=$(OBJDIR)/ap_tcp.o

ap_tcp.o: ap_tcp.c ap_tcp.h
	$(CC) -c $(DEFOPTS) $(OPTS) ap_tcp.c -o $(OBJDIR)/ap_tcp.o

clean:
	rm -f $(OBJ)
