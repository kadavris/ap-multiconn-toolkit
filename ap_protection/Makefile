PATH1="."

CC=gcc
DEFOPTS=-Wall
# -mtune=pentium3 -m32

OBJDIR=../compiled
OBJ=$(OBJDIR)/ap_protection.o

ap_protection.o: ap_protection.c ap_protection.h ../ap_str.c
	$(CC) -c $(DEFOPTS) $(OPTS) ap_protection.c -o $(OBJDIR)/ap_protection.o

clean:
	rm -f $(OBJ)