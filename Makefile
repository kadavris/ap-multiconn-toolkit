PATH1="."

CC=gcc
# -mtune=pentium3 -m32
CCFLAGS=$(OPTS) -Wall -ggdb

libname=libaptoolkit.a

obj=ap_log.o ap_str.o ap_utils.o

all: lib compiletests

lib: $(obj)
	rm -f $(libname)
	make -C ap_net $@
	make -C ap_error $@
	ar rcs $(libname) *.o ap_error/*.o ap_net/*.o

%.o: %.c
	$(CC) -c $(CCFLAGS) $< -o $@

compiletests:
	make -C ap_net libname=$(libname) $@

clean:
	rm -f *.o $(libname)
	make -C ap_error clean
	make -C ap_net clean
	make -C ap_protection clean
