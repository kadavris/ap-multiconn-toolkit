PATH1="."

cc=gcc
OPTS ?= -Wall
# -mtune=pentium3 -m32

OBJDIR ?= ../compiled
obj=$(OBJDIR)/ap_protection.o

ap_protection.o: ap_protection.c ap_protection.h ../ap_str.c
	$(cc) -c $(OPTS) ap_protection.c -o $(OBJDIR)/ap_protection.o

clean:
	rm -f $(obj)
