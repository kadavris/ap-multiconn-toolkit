PATH1="."

cc=gcc
OPTS ?= -Wall
# -mtune=pentium3 -m32

OBJDIR = compiled
libname=libaptoolkit.a

src=ap_log.c ap_str.c ap_utis.c
obj=$(OBJDIR)/ap_log.o $(OBJDIR)/ap_str.o  $(OBJDIR)/ap_utils.o $(OBJDIR)/b64.o
ap_net_obj=ap_net/conn_pool/*.o ap_net/poller/*.o

all: $(obj)
	make -C ap_net $*
#	make -C ap_protection $*
	ar rcs $(libname) $(OBJDIR)/*.o $(ap_net_obj)

$(OBJDIR)/b64.o: b64.c
	$(cc) -c $(OPTS) b64.c -o $(OBJDIR)/b64.o

$(OBJDIR)/ap_log.o: ap_log.c ap_net
	$(cc) -c $(OPTS) ap_log.c -o $(OBJDIR)/ap_log.o

$(OBJDIR)/ap_str.o: ap_str.c
	$(cc) -c $(OPTS) ap_str.c -o $(OBJDIR)/ap_str.o

$(OBJDIR)/ap_utils.o: ap_utils.c
	$(cc) -c $(OPTS) ap_utils.c -o $(OBJDIR)/ap_utils.o

clean:
	rm -f $(OBJDIR)/*.o $(libname)
	make -C ap_net clean
#	make -C ap_protection clean
