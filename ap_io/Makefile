PATH1="."

CC=gcc
DEFOPTS=-Wall
# -mtune=pentium3 -m32

OBJDIR=../compiled
OBJ=$(OBJDIR)/ap_buf.o

ap_buf.o: ap_buf.c ap_buf.h
	$(CC) -c $(DEFOPTS) $(OPTS) ap_buf.c -o $(OBJDIR)/ap_buf.o

clean:
	rm -f $(OBJ)
