CC=gcc
CFLAGS=-g
IFLAGS=-I.
CLASS=cis620s
PROJECT=proj3

DEPENDS=db20 makefile
HFILES=util.h
SRCFILES=client.c server.c servicemap.c util.c
OBJFILES=client.o server.o servicemap.o util.o
EXEFILES=client server servicemap

.PHONY: all clean submit depend

all: $(EXEFILES)

client: client.o util.o
	gcc -o client client.o util.o

server: server.o util.o
	gcc -o server server.o util.o

servicemap: servicemap.o util.o
	gcc -o servicemap servicemap.o util.o

clean:
	rm $(OBJFILES) $(EXEFILES)

submit:
	turnin -c $(CLASS) -p $(PROJECT) $(SRCFILES) $(HFILES) $(DEPENDS)

depend:
	makedepend $(IFLAGS) $(SRCFILES)
# DO NOT DELETE

client.o: /usr/include/sys/types.h /usr/include/features.h
client.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
client.o: /usr/include/bits/wordsize.h /usr/include/bits/long-double.h
client.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
client.o: /usr/include/bits/typesizes.h /usr/include/bits/types/clock_t.h
client.o: /usr/include/bits/types/clockid_t.h
client.o: /usr/include/bits/types/time_t.h /usr/include/bits/types/timer_t.h
client.o: /usr/include/bits/stdint-intn.h /usr/include/endian.h
client.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
client.o: /usr/include/bits/byteswap-16.h /usr/include/bits/uintn-identity.h
client.o: /usr/include/sys/select.h /usr/include/bits/select.h
client.o: /usr/include/bits/types/sigset_t.h
client.o: /usr/include/bits/types/__sigset_t.h
client.o: /usr/include/bits/types/struct_timeval.h
client.o: /usr/include/bits/types/struct_timespec.h
client.o: /usr/include/sys/sysmacros.h /usr/include/bits/sysmacros.h
client.o: /usr/include/bits/pthreadtypes.h
client.o: /usr/include/bits/thread-shared-types.h
client.o: /usr/include/bits/pthreadtypes-arch.h /usr/include/sys/socket.h
client.o: /usr/include/bits/types/struct_iovec.h /usr/include/bits/socket.h
client.o: /usr/include/bits/socket_type.h /usr/include/bits/sockaddr.h
client.o: /usr/include/asm/socket.h /usr/include/asm-generic/socket.h
client.o: /usr/include/asm/sockios.h /usr/include/asm-generic/sockios.h
client.o: /usr/include/bits/types/struct_osockaddr.h
client.o: /usr/include/netinet/ip.h /usr/include/netinet/in.h
client.o: /usr/include/bits/stdint-uintn.h /usr/include/bits/in.h
client.o: /usr/include/arpa/inet.h /usr/include/netdb.h
client.o: /usr/include/rpc/netdb.h /usr/include/bits/netdb.h
client.o: /usr/include/stdlib.h /usr/include/bits/libc-header-start.h
client.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
client.o: /usr/include/bits/floatn.h /usr/include/bits/floatn-common.h
client.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h
client.o: /usr/include/stdio.h /usr/include/bits/types/__FILE.h
client.o: /usr/include/bits/types/FILE.h /usr/include/bits/libio.h
client.o: /usr/include/bits/_G_config.h /usr/include/bits/types/__mbstate_t.h
client.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
client.o: /usr/include/string.h /usr/include/bits/types/locale_t.h
client.o: /usr/include/bits/types/__locale_t.h /usr/include/strings.h
client.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
client.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
client.o: /usr/include/bits/getopt_posix.h /usr/include/bits/getopt_core.h
client.o: /usr/include/sys/fcntl.h /usr/include/fcntl.h
client.o: /usr/include/bits/fcntl.h /usr/include/bits/fcntl-linux.h
client.o: /usr/include/bits/stat.h util.h
server.o: /usr/include/sys/types.h /usr/include/features.h
server.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
server.o: /usr/include/bits/wordsize.h /usr/include/bits/long-double.h
server.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
server.o: /usr/include/bits/typesizes.h /usr/include/bits/types/clock_t.h
server.o: /usr/include/bits/types/clockid_t.h
server.o: /usr/include/bits/types/time_t.h /usr/include/bits/types/timer_t.h
server.o: /usr/include/bits/stdint-intn.h /usr/include/endian.h
server.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
server.o: /usr/include/bits/byteswap-16.h /usr/include/bits/uintn-identity.h
server.o: /usr/include/sys/select.h /usr/include/bits/select.h
server.o: /usr/include/bits/types/sigset_t.h
server.o: /usr/include/bits/types/__sigset_t.h
server.o: /usr/include/bits/types/struct_timeval.h
server.o: /usr/include/bits/types/struct_timespec.h
server.o: /usr/include/sys/sysmacros.h /usr/include/bits/sysmacros.h
server.o: /usr/include/bits/pthreadtypes.h
server.o: /usr/include/bits/thread-shared-types.h
server.o: /usr/include/bits/pthreadtypes-arch.h /usr/include/sys/socket.h
server.o: /usr/include/bits/types/struct_iovec.h /usr/include/bits/socket.h
server.o: /usr/include/bits/socket_type.h /usr/include/bits/sockaddr.h
server.o: /usr/include/asm/socket.h /usr/include/asm-generic/socket.h
server.o: /usr/include/asm/sockios.h /usr/include/asm-generic/sockios.h
server.o: /usr/include/bits/types/struct_osockaddr.h
server.o: /usr/include/netinet/ip.h /usr/include/netinet/in.h
server.o: /usr/include/bits/stdint-uintn.h /usr/include/bits/in.h
server.o: /usr/include/arpa/inet.h /usr/include/stdlib.h
server.o: /usr/include/bits/libc-header-start.h /usr/include/bits/waitflags.h
server.o: /usr/include/bits/waitstatus.h /usr/include/bits/floatn.h
server.o: /usr/include/bits/floatn-common.h /usr/include/alloca.h
server.o: /usr/include/bits/stdlib-float.h /usr/include/string.h
server.o: /usr/include/bits/types/locale_t.h
server.o: /usr/include/bits/types/__locale_t.h /usr/include/strings.h
server.o: /usr/include/stdio.h /usr/include/bits/types/__FILE.h
server.o: /usr/include/bits/types/FILE.h /usr/include/bits/libio.h
server.o: /usr/include/bits/_G_config.h /usr/include/bits/types/__mbstate_t.h
server.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
server.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
server.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
server.o: /usr/include/bits/getopt_posix.h /usr/include/bits/getopt_core.h
server.o: /usr/include/sys/fcntl.h /usr/include/fcntl.h
server.o: /usr/include/bits/fcntl.h /usr/include/bits/fcntl-linux.h
server.o: /usr/include/bits/stat.h util.h
servicemap.o: /usr/include/sys/types.h /usr/include/features.h
servicemap.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
servicemap.o: /usr/include/bits/wordsize.h /usr/include/bits/long-double.h
servicemap.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
servicemap.o: /usr/include/bits/typesizes.h /usr/include/bits/types/clock_t.h
servicemap.o: /usr/include/bits/types/clockid_t.h
servicemap.o: /usr/include/bits/types/time_t.h
servicemap.o: /usr/include/bits/types/timer_t.h
servicemap.o: /usr/include/bits/stdint-intn.h /usr/include/endian.h
servicemap.o: /usr/include/bits/endian.h /usr/include/bits/byteswap.h
servicemap.o: /usr/include/bits/byteswap-16.h
servicemap.o: /usr/include/bits/uintn-identity.h /usr/include/sys/select.h
servicemap.o: /usr/include/bits/select.h /usr/include/bits/types/sigset_t.h
servicemap.o: /usr/include/bits/types/__sigset_t.h
servicemap.o: /usr/include/bits/types/struct_timeval.h
servicemap.o: /usr/include/bits/types/struct_timespec.h
servicemap.o: /usr/include/sys/sysmacros.h /usr/include/bits/sysmacros.h
servicemap.o: /usr/include/bits/pthreadtypes.h
servicemap.o: /usr/include/bits/thread-shared-types.h
servicemap.o: /usr/include/bits/pthreadtypes-arch.h /usr/include/sys/socket.h
servicemap.o: /usr/include/bits/types/struct_iovec.h
servicemap.o: /usr/include/bits/socket.h /usr/include/bits/socket_type.h
servicemap.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
servicemap.o: /usr/include/asm-generic/socket.h /usr/include/asm/sockios.h
servicemap.o: /usr/include/asm-generic/sockios.h
servicemap.o: /usr/include/bits/types/struct_osockaddr.h
servicemap.o: /usr/include/netinet/ip.h /usr/include/netinet/in.h
servicemap.o: /usr/include/bits/stdint-uintn.h /usr/include/bits/in.h
servicemap.o: /usr/include/arpa/inet.h /usr/include/stdio.h
servicemap.o: /usr/include/bits/libc-header-start.h
servicemap.o: /usr/include/bits/types/__FILE.h /usr/include/bits/types/FILE.h
servicemap.o: /usr/include/bits/libio.h /usr/include/bits/_G_config.h
servicemap.o: /usr/include/bits/types/__mbstate_t.h
servicemap.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
servicemap.o: /usr/include/string.h /usr/include/bits/types/locale_t.h
servicemap.o: /usr/include/bits/types/__locale_t.h /usr/include/strings.h
servicemap.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
servicemap.o: /usr/include/bits/waitstatus.h /usr/include/bits/floatn.h
servicemap.o: /usr/include/bits/floatn-common.h /usr/include/alloca.h
servicemap.o: /usr/include/bits/stdlib-float.h /usr/include/unistd.h
servicemap.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
servicemap.o: /usr/include/bits/confname.h /usr/include/bits/getopt_posix.h
servicemap.o: /usr/include/bits/getopt_core.h /usr/include/sys/fcntl.h
servicemap.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
servicemap.o: /usr/include/bits/fcntl-linux.h /usr/include/bits/stat.h util.h
