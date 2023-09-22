# Making library
PATH1="."
CC=gcc

distroopts=-mtune=generic -Wall -O2
distroheaders=ap_log.h ap_str.h ap_utils.h ap_net/ap_net.h
distrotexts=README LICENSE

optsdebug=-Wall -ggdb -Og
optsrelease=-Wall -O2

libbasename=apstoolkit
outname=lib$(libbasename).a

obj=ap_log.o ap_str.o ap_utils.o

%.o: %.c
	$(CC) -c $(OPTS) $< -o $@

# by default we make debug compile
all: OPTS=$(optsdebug)
all: lib compiletests

release: OPTS=$(optsrelease)
release: lib compiletests

doxygen:
	rm -rf doxydoc
	doxygen Doxyfile

compiletests:
	make -C ap_net libname=$(libbasename) OPTS="$(OPTS)" $@

clean:
	rm -f *.o $(outname)
	make -C ap_error clean
	make -C ap_net clean

lib: $(obj)
	rm -f $(outname)
	make -C ap_net $@ OPTS="$(OPTS)"
	make -C ap_error $@ OPTS="$(OPTS)"
	ar rcs $(outname) *.o ap_error/*.o ap_net/*.o

