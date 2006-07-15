#
#   bregexp library make file   for RedHat Linux 6.1
#					version	1.0 1999.11.24 Tatsuo Baba
#
#   bregexp.h	 			header file
#   libbregexp.so 			shared library
#   libbregexp.a			static library
#
#
CC = gcc
CPP = g++
AR = ar
RANLIB = ranlib
CPFLAGS = -fPIC -c -O
INSTINCLUDE = /usr/include
INSTLIB = /usr/lib
#
HEADERFILE = bregexp.h
LIBNAME = libbregexp
SONAME = $(LIBNAME).so.1
SHAREDLIBNAME = $(LIBNAME).so.1.0
STATICLIBNAME = $(LIBNAME).a
#
#
all : main.o bregcomp.o bsubst.o bregexec.o bsplit.o btrans.o sv.o
	$(CC) -shared -Wl,-soname,$(SONAME) -o $(SHAREDLIBNAME) main.o bregcomp.o bsubst.o bregexec.o bsplit.o btrans.o sv.o
	$(AR) rv $(STATICLIBNAME) main.o bregcomp.o bsubst.o bregexec.o bsplit.o btrans.o sv.o
	$(RANLIB) $(STATICLIBNAME)

main.o : main.cc sv.h bregexp.h global.h intreg.h

bregexec.o : bregexec.cc sv.h global.h intreg.h

bregcomp.o : bregcomp.cc sv.h global.h intreg.h

bsubst.o : bsubst.cc sv.h global.h intreg.h

bsplit.o : bsplit.cc sv.h global.h intreg.h

btrans.o : btrans.cc sv.h global.h intreg.h

sv.o : sv.cc sv.h

clean : 
	rm -f *.o
	rm -f $(STATICLIBNAME) $(SHAREDLIBNAME)

install : 
	cp $(HEADERFILE) $(INSTINCLUDE)
	@chmod o+r $(INSTINCLUDE)/$(HEADERFILE)
	cp $(STATICLIBNAME) $(INSTLIB)
	cp $(SHAREDLIBNAME) $(INSTLIB)
	ldconfig $(INSTLIB)
	@rm -f $(INSTLIB)/$(LIBNAME).so
	ln -s $(INSTLIB)/$(SONAME) $(INSTLIB)/$(LIBNAME).so

.cc.o:
	$(CPP) $(CPFLAGS) $<

