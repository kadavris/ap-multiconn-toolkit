PATH1="."

cc=gcc
OPTS ?= -Wall
# -mtune=pentium3 -m32

OBJDIR ?= compiled
obj=$(OBJDIR)/b64.o $(OBJDIR)/ap_log.o $(OBJDIR)/ap_str.o

all: $(obj) ap_protection ap_net

$(OBJDIR)/b64.o: b64.c
	$(cc) -c $(OPTS) b64.c -o $(OBJDIR)/b64.o

$(OBJDIR)/ap_log.o: ap_log.c ap_net
	$(cc) -c $(OPTS) ap_log.c -o $(OBJDIR)/ap_log.o

$(OBJDIR)/ap_str.o: ap_str.c
	$(cc) -c $(OPTS) ap_str.c -o $(OBJDIR)/ap_str.o

ap_net:
	make -C ap_net $*

ap_protection:
	make -C ap_protection $*

clean:
	rm -f $(obj)
