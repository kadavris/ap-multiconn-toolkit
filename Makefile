PATH1="."
CC=gcc

distrooptsx32=-mtune=pentium3 -m32 -O2
distrooptsx64=-m64 -O2
distroheaders=ap_log.h ap_str.h ap_utils.h ap_net/ap_net.h

CCFLAGS=$(OPTS) -Wall -ggdb -Og

outname=libaptoolkit

obj=ap_log.o ap_str.o ap_utils.o

all: lib compiletests

doxydoc:
	rm -rf doxydoc
	doxygen Doxyfile

prepdistro:
	rm -rf $(outname)
	mkdir $(outname)
	cp $(distroheaders) $(outname)/
	strip $(outname).a
	mv $(outname).a $(outname)/
	cp -R doc $(outname)/
	cp -R doxydoc/html $(outname)/doc/

lib32:
	make clean lib prepdistro OPTS="$(distrooptsx32)"
	tar cf - $(outname) | bzip2 > $(outname)_x32.tar.bz2
	rm -rf $(outname)

lib64:
	make clean lib prepdistro OPTS="$(distrooptsx64)"
	tar cf - $(outname) | bzip2 > $(outname)_x64.tar.bz2
	rm -rf $(outname)

distro: doxydoc lib32 lib64

lib: $(obj)
	rm -f $(outname).a
	make -C ap_net $@ OPTS="$(OPTS)"
	make -C ap_error $@ OPTS="$(OPTS)"
	ar rcs $(outname).a *.o ap_error/*.o ap_net/*.o

%.o: %.c
	$(CC) -c $(CCFLAGS) $< -o $@

compiletests:
	make -C ap_net libname=$(outname).a OPTS="$(OPTS)" $@

clean:
	rm -f *.o $(outname)*.a
	make -C ap_error clean
	make -C ap_net clean
