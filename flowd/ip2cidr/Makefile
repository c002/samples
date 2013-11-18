
##
## set a macro for solaris 2.6 - unfortunately, this wont work with gnu make
##
#EXTRAFLAGS:sh=if [ `uname -r` = "5.6" ] ; then echo "-DHASINTTYPES" ; fi
EXTRAFLAGS=-DHASINTTYPES

SRC	= ip2cidr.c mpmalloc.c iptest.c
OBJ	= ip2cidr.o mpmalloc.o
EXE	= iptest
CC	= gcc 
DEBUG	=
DEBUG   = -DDEBUG_IP2CIDR
#CC	= /opt/SUNWspro/bin/cc
CFLAGS	= -O -g $(DEBUG) -Wno-inline -finline -I../../btalib \
	-Wall -W -Wimplicit -Wuninitialized -Wpointer-arith -Wshadow -Wpointer-arith -Waggregate-return \
	-Wcast-align -Wwrite-strings -Wno-unused -Wmissing-prototypes -Wshadow \
	$(EXTRAFLAGS)

LDFLAGS	= -L../../btalib
LIB	= libip2cidr.a
LIBS	= -lbta -lnsl -lresolv

all:	iptest checkcidr

iptest: iptest.o $(LIB)
	$(CC) $(LDFLAGS) -o iptest iptest.o -L. -lip2cidr $(LIBS)

checkcidr: checkcidr.o $(LIB)
	$(CC) $(LDFLAGS) -o checkcidr checkcidr.o -L. -lip2cidr $(LIBS)

$(LIB):	$(OBJ)
	ar -r $(LIB) $(OBJ)

clean:
	/bin/rm -f *.o $(EXE) $(LIB) *~

depend:
		makedepend -- $(CFLAGS) *.c

# DO NOT DELETE THIS LINE -- make depend depends on it.

ip2cidr.o: /usr/include/sys/types.h /usr/include/sys/feature_tests.h
ip2cidr.o: /usr/include/sys/isa_defs.h /usr/include/sys/machtypes.h
ip2cidr.o: /usr/include/errno.h /usr/include/sys/errno.h
ip2cidr.o: /usr/include/string.h /usr/include/stdio.h
ip2cidr.o: /usr/include/sys/va_list.h ip2cidr.h mpmalloc.h
ip2cidr.o: /usr/include/synch.h /usr/include/sys/machlock.h
ip2cidr.o: /usr/include/sys/time.h /usr/include/sys/synch.h
ip2cidr.o: /usr/include/stdlib.h
iptest.o: /usr/include/sys/types.h /usr/include/sys/feature_tests.h
iptest.o: /usr/include/sys/isa_defs.h /usr/include/sys/machtypes.h
iptest.o: /usr/include/stdio.h /usr/include/sys/va_list.h
iptest.o: /usr/include/stdlib.h /usr/include/ctype.h /usr/include/synch.h
iptest.o: /usr/include/sys/machlock.h /usr/include/sys/time.h
iptest.o: /usr/include/sys/synch.h ip2cidr.h mpmalloc.h
mpmalloc.o: /usr/include/sys/types.h /usr/include/sys/feature_tests.h
mpmalloc.o: /usr/include/sys/isa_defs.h /usr/include/sys/machtypes.h
mpmalloc.o: /usr/include/sys/mman.h /usr/include/sys/stat.h
mpmalloc.o: /usr/include/sys/time.h /usr/include/sys/param.h
mpmalloc.o: /usr/include/sys/unistd.h /usr/include/fcntl.h
mpmalloc.o: /usr/include/sys/fcntl.h /usr/include/unistd.h
mpmalloc.o: /usr/include/strings.h /usr/include/errno.h
mpmalloc.o: /usr/include/sys/errno.h /usr/include/stdio.h
mpmalloc.o: /usr/include/sys/va_list.h mpmalloc.h /usr/include/synch.h
mpmalloc.o: /usr/include/sys/machlock.h /usr/include/sys/synch.h
mpmalloc.o: /usr/include/stdlib.h
