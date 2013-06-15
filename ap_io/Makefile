PATH1="."

cc=gcc
OPTS ?= -Wall
# -mtune=pentium3 -m32

OBJDIR ?= ../compiled
obj=$(OBJDIR)/ap_buf.o

ap_buf.o: ap_buf.c ap_buf.h
	$(cc) -c $(OPTS) ap_buf.c -o $(OBJDIR)/ap_buf.o

clean:
	rm -f $(obj)
