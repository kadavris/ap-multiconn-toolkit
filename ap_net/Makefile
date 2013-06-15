PATH1="."

cc=gcc
OPTS ?= -Wall
# -mtune=pentium3 -m32

OBJDIR ?= ../compiled
obj = $(OBJDIR)/ap_tcp.o

all: $(obj)

$(OBJDIR)/ap_tcp.o: ap_tcp.c ap_tcp.h
	$(cc) -c $(OPTS) ap_tcp.c -o $(OBJDIR)/ap_tcp.o

clean:
	rm -f $(obj)
