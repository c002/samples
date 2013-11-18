
LINT=lclint
LINTFLAGS=+posixlib +trytorecover -D__i386 -DHASINTTYPES -I../ip2cidr -I../../btalib -I. -D__GNUC__=2 -D__GNUC_MINOR__=91 -Dunix -D__svr4__ -D__SVR4 -Dsun -D__unix__ -D__svr4__ -D__SVR4 -D__sun__ -D__unix -D__sun -Di386

##
## set a macro for solaris 2.6 - unfortunately, this wont work with gnu make
##
#EXTRAFLAGS:sh=if [ `uname -r` = "5.6" ] ; then echo "-DHASINTTYPES" ; fi
EXTRAFLAGS=-DHASINTTYPES

CC=gcc
#CC=/opt/pure/bin/purify gcc
AR=ar
##
LDFLAGS=-L../ip2cidr -L. -L../../btalib
LDLIBS=-lbgp -lbta -lip2cidr -lsocket -lnsl -lresolv
CFLAGS=-g -O -I../ip2cidr -I../../btalib -finline \
	-Wall -W -Wimplicit -Wuninitialized -Wpointer-arith -Wshadow -Wpointer-arith -Waggregate-return \
	-Wcast-align -Wwrite-strings -Wno-unused -Wmissing-prototypes -Wshadow \
	$(EXTRAFLAGS)

ARFLAGS=rv
###
### For Solaris 2.5.1
#CFLAGS=-I../ip2cidr -Wimplicit -Wuninitialized -Wall -Wswitch -O -g -Wno-inline -finline 

all:	bgp bgptest showroute

bgp:	libbgp.a bgp.o
	$(LINK.c) $(LDFLAGS) -o bgp bgp.o bgp_api.o $(LDLIBS)

libbgp.a:	bgp_api.o
	$(AR) $(ARFLAGS) $@ bgp_api.o

bgptest:	bgptest.o libbgp.a
	$(LINK.c) $(LDFLAGS) -o bgptest bgptest.o $(LDLIBS)

showroute:	showroute.o libbgp.a
	$(LINK.c) $(LDFLAGS) -o showroute showroute.o $(LDLIBS)

bgpmunch:	bgpmunch.o libbgp.a
	$(LINK.c) $(LDFLAGS) -o bgpmunch bgpmunch.o $(LDLIBS)

bgp.o:	bgp.c

bgp_api.o:	bgp_api.c

clean:
	-/bin/rm -f *.o core *~ *.a

distclean:	clean
	-rm *.a

depend:
		makedepend -- $(CFLAGS) *.c

# DO NOT DELETE THIS LINE -- make depend depends on it.

bgp.o: /usr/include/strings.h /usr/include/sys/types.h
bgp.o: /usr/include/sys/isa_defs.h /usr/include/sys/feature_tests.h
bgp.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
bgp.o: /usr/include/sys/select.h /usr/include/sys/time.h
bgp.o: /usr/include/sys/time.h /usr/include/string.h
bgp.o: /usr/include/iso/string_iso.h /usr/include/unistd.h
bgp.o: /usr/include/sys/unistd.h /usr/include/stdlib.h
bgp.o: /usr/include/iso/stdlib_iso.h /usr/include/malloc.h
bgp.o: /usr/include/stdio.h /usr/include/iso/stdio_iso.h
bgp.o: /usr/include/sys/va_list.h /usr/include/stdio_tag.h
bgp.o: /usr/include/stdio_impl.h /usr/include/fcntl.h
bgp.o: /usr/include/sys/fcntl.h /usr/include/errno.h /usr/include/sys/errno.h
bgp.o: /usr/include/stdarg.h /usr/include/iso/stdarg_iso.h
bgp.o: /usr/include/netinet/in.h /usr/include/sys/stream.h
bgp.o: /usr/include/sys/vnode.h /usr/include/sys/t_lock.h
bgp.o: /usr/include/sys/machlock.h /usr/include/sys/param.h
bgp.o: /usr/include/sys/mutex.h /usr/include/sys/rwlock.h
bgp.o: /usr/include/sys/semaphore.h /usr/include/sys/condvar.h
bgp.o: /usr/include/sys/time_impl.h /usr/include/sys/cred.h
bgp.o: /usr/include/sys/uio.h /usr/include/sys/resource.h
bgp.o: /usr/include/vm/seg_enum.h /usr/include/sys/poll.h
bgp.o: /usr/include/sys/strmdep.h /usr/include/sys/model.h
bgp.o: /usr/include/sys/strft.h /usr/include/sys/byteorder.h
bgp.o: /usr/include/sys/socket.h /usr/include/sys/netconfig.h
bgp.o: /usr/include/sys/un.h /usr/include/net/if_dl.h
bgp.o: /usr/include/arpa/inet.h /usr/include/netdb.h /usr/include/limits.h
bgp.o: /usr/include/iso/limits_iso.h /usr/include/signal.h
bgp.o: /usr/include/iso/signal_iso.h /usr/include/sys/iso/signal_iso.h
bgp.o: /usr/include/sys/signal.h bgp_api.h ../ip2cidr/ip2cidr.h
bgp.o: ../ip2cidr/mpmalloc.h /usr/include/synch.h /usr/include/sys/synch.h
bgp_api.o: /usr/include/strings.h /usr/include/sys/types.h
bgp_api.o: /usr/include/sys/isa_defs.h /usr/include/sys/feature_tests.h
bgp_api.o: /usr/include/sys/machtypes.h /usr/include/sys/int_types.h
bgp_api.o: /usr/include/sys/select.h /usr/include/sys/time.h
bgp_api.o: /usr/include/sys/time.h /usr/include/string.h
bgp_api.o: /usr/include/iso/string_iso.h /usr/include/unistd.h
bgp_api.o: /usr/include/sys/unistd.h /usr/include/stdlib.h
bgp_api.o: /usr/include/iso/stdlib_iso.h /usr/include/stdio.h
bgp_api.o: /usr/include/iso/stdio_iso.h /usr/include/sys/va_list.h
bgp_api.o: /usr/include/stdio_tag.h /usr/include/stdio_impl.h
bgp_api.o: /usr/include/fcntl.h /usr/include/sys/fcntl.h /usr/include/errno.h
bgp_api.o: /usr/include/sys/errno.h /usr/include/stdarg.h
bgp_api.o: /usr/include/iso/stdarg_iso.h /usr/include/syslog.h
bgp_api.o: /usr/include/sys/syslog.h bgp_api.h /usr/include/netinet/in.h
bgp_api.o: /usr/include/sys/stream.h /usr/include/sys/vnode.h
bgp_api.o: /usr/include/sys/t_lock.h /usr/include/sys/machlock.h
bgp_api.o: /usr/include/sys/param.h /usr/include/sys/mutex.h
bgp_api.o: /usr/include/sys/rwlock.h /usr/include/sys/semaphore.h
bgp_api.o: /usr/include/sys/condvar.h /usr/include/sys/time_impl.h
bgp_api.o: /usr/include/sys/cred.h /usr/include/sys/uio.h
bgp_api.o: /usr/include/sys/resource.h /usr/include/vm/seg_enum.h
bgp_api.o: /usr/include/sys/poll.h /usr/include/sys/strmdep.h
bgp_api.o: /usr/include/sys/model.h /usr/include/sys/strft.h
bgp_api.o: /usr/include/sys/byteorder.h /usr/include/sys/socket.h
bgp_api.o: /usr/include/sys/netconfig.h /usr/include/sys/un.h
bgp_api.o: /usr/include/net/if_dl.h /usr/include/arpa/inet.h
bgp_api.o: /usr/include/netdb.h ../ip2cidr/ip2cidr.h ../ip2cidr/mpmalloc.h
bgp_api.o: /usr/include/synch.h /usr/include/sys/synch.h
bgpmunch.o: /usr/include/sys/types.h /usr/include/sys/isa_defs.h
bgpmunch.o: /usr/include/sys/feature_tests.h /usr/include/sys/machtypes.h
bgpmunch.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
bgpmunch.o: /usr/include/sys/time.h /usr/include/sys/time.h
bgpmunch.o: /usr/include/stdio.h /usr/include/iso/stdio_iso.h
bgpmunch.o: /usr/include/sys/va_list.h /usr/include/stdio_tag.h
bgpmunch.o: /usr/include/stdio_impl.h /usr/include/synch.h
bgpmunch.o: /usr/include/sys/machlock.h /usr/include/sys/time_impl.h
bgpmunch.o: /usr/include/sys/synch.h /usr/include/ctype.h
bgpmunch.o: /usr/include/iso/ctype_iso.h /usr/include/stdlib.h
bgpmunch.o: /usr/include/iso/stdlib_iso.h bgp_api.h /usr/include/netinet/in.h
bgpmunch.o: /usr/include/sys/stream.h /usr/include/sys/vnode.h
bgpmunch.o: /usr/include/sys/t_lock.h /usr/include/sys/param.h
bgpmunch.o: /usr/include/sys/unistd.h /usr/include/sys/mutex.h
bgpmunch.o: /usr/include/sys/rwlock.h /usr/include/sys/semaphore.h
bgpmunch.o: /usr/include/sys/condvar.h /usr/include/sys/cred.h
bgpmunch.o: /usr/include/sys/uio.h /usr/include/sys/resource.h
bgpmunch.o: /usr/include/vm/seg_enum.h /usr/include/sys/poll.h
bgpmunch.o: /usr/include/sys/strmdep.h /usr/include/sys/model.h
bgpmunch.o: /usr/include/sys/strft.h /usr/include/sys/byteorder.h
bgpmunch.o: /usr/include/sys/socket.h /usr/include/sys/netconfig.h
bgpmunch.o: /usr/include/sys/un.h /usr/include/net/if_dl.h
bgpmunch.o: /usr/include/arpa/inet.h /usr/include/netdb.h
bgpmunch.o: ../ip2cidr/ip2cidr.h
bgptest.o: /usr/include/sys/types.h /usr/include/sys/isa_defs.h
bgptest.o: /usr/include/sys/feature_tests.h /usr/include/sys/machtypes.h
bgptest.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
bgptest.o: /usr/include/sys/time.h /usr/include/sys/time.h
bgptest.o: /usr/include/stdio.h /usr/include/iso/stdio_iso.h
bgptest.o: /usr/include/sys/va_list.h /usr/include/stdio_tag.h
bgptest.o: /usr/include/stdio_impl.h /usr/include/ctype.h
bgptest.o: /usr/include/iso/ctype_iso.h /usr/include/stdlib.h
bgptest.o: /usr/include/iso/stdlib_iso.h bgp_api.h /usr/include/netinet/in.h
bgptest.o: /usr/include/sys/stream.h /usr/include/sys/vnode.h
bgptest.o: /usr/include/sys/t_lock.h /usr/include/sys/machlock.h
bgptest.o: /usr/include/sys/param.h /usr/include/sys/unistd.h
bgptest.o: /usr/include/sys/mutex.h /usr/include/sys/rwlock.h
bgptest.o: /usr/include/sys/semaphore.h /usr/include/sys/condvar.h
bgptest.o: /usr/include/sys/time_impl.h /usr/include/sys/cred.h
bgptest.o: /usr/include/sys/uio.h /usr/include/sys/resource.h
bgptest.o: /usr/include/vm/seg_enum.h /usr/include/sys/poll.h
bgptest.o: /usr/include/sys/strmdep.h /usr/include/sys/model.h
bgptest.o: /usr/include/sys/strft.h /usr/include/sys/byteorder.h
bgptest.o: /usr/include/sys/socket.h /usr/include/sys/netconfig.h
bgptest.o: /usr/include/sys/un.h /usr/include/net/if_dl.h
bgptest.o: /usr/include/arpa/inet.h /usr/include/netdb.h ../ip2cidr/ip2cidr.h
showroute.o: /usr/include/sys/types.h /usr/include/sys/isa_defs.h
showroute.o: /usr/include/sys/feature_tests.h /usr/include/sys/machtypes.h
showroute.o: /usr/include/sys/int_types.h /usr/include/sys/select.h
showroute.o: /usr/include/sys/time.h /usr/include/sys/time.h
showroute.o: /usr/include/stdio.h /usr/include/iso/stdio_iso.h
showroute.o: /usr/include/sys/va_list.h /usr/include/stdio_tag.h
showroute.o: /usr/include/stdio_impl.h /usr/include/ctype.h
showroute.o: /usr/include/iso/ctype_iso.h /usr/include/stdlib.h
showroute.o: /usr/include/iso/stdlib_iso.h /usr/include/string.h
showroute.o: /usr/include/iso/string_iso.h bgp_api.h
showroute.o: /usr/include/netinet/in.h /usr/include/sys/stream.h
showroute.o: /usr/include/sys/vnode.h /usr/include/sys/t_lock.h
showroute.o: /usr/include/sys/machlock.h /usr/include/sys/param.h
showroute.o: /usr/include/sys/unistd.h /usr/include/sys/mutex.h
showroute.o: /usr/include/sys/rwlock.h /usr/include/sys/semaphore.h
showroute.o: /usr/include/sys/condvar.h /usr/include/sys/time_impl.h
showroute.o: /usr/include/sys/cred.h /usr/include/sys/uio.h
showroute.o: /usr/include/sys/resource.h /usr/include/vm/seg_enum.h
showroute.o: /usr/include/sys/poll.h /usr/include/sys/strmdep.h
showroute.o: /usr/include/sys/model.h /usr/include/sys/strft.h
showroute.o: /usr/include/sys/byteorder.h /usr/include/sys/socket.h
showroute.o: /usr/include/sys/netconfig.h /usr/include/sys/un.h
showroute.o: /usr/include/net/if_dl.h /usr/include/arpa/inet.h
showroute.o: /usr/include/netdb.h ../ip2cidr/ip2cidr.h
