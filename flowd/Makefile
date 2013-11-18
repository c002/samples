CC	= gcc
LIB	= -L../libpa-1.3.1/lib \
	  -L../ip2cidr -L../btalib -Lbgp \
	  -lbgp -lbta -lip2cidr -lpa -lhash -lvector -lmemory \
	  -lnsl -lsocket -lresolv
INC	= -I./ -I../libpa-1.3.1/include -I../ip2cidr -I../btalib -I../bgp
DEFS	= -DHASINTTYPES
FLAGS   = -g -O2

# Install options
SBINDEST = /home/harryr/local/sbin
BINDEST  = /home/harryr/local/bin
MANDEST  = /home/harryr/local/man/man1
#SBINDEST = /opt/local/sbin
#BINDEST  = /opt/local/bin
#MANDEST  = /opt/local/man/man1

all: flowd flowread

clean:
	rm *.o flowd flowread
install:
	install -c -m 500 flowd $(SBINDEST)
	install -c -m 555 flowread $(BINDEST)
	install -c -m 555 flowread.1 $(MANDEST)
	install -c -m 555 flowd.1 $(MANDEST)

flowd: flowd.c store.c sock.c sock.h flowd.h logger.c
	$(CC) $(FLAGS) $(DEFS) -o flowd flowd.c store.c sock.c logger.c $(INC) $(LIB)

flowread: flowread.c  flowd.h
	$(CC) $(FLAGS) $(DEFS) -o flowread flowread.c $(INC) $(LIB)
