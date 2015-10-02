LDFLAGS+=`pkg-config --libs libbluray`
CFLAGS+=`pkg-config --cflags libbluray`

LDFLAGS+=`pkg-config --libs dvdread`
CFLAGS+=`pkg-config --cflags dvdread`

all: streamdisc_cgi streamdisc_server discinfo

streamdisc_cgi: streamdisc_cgi.c streamdisc.o readdisc.o
	${CC} -Wall -o streamdisc_cgi streamdisc_cgi.c streamdisc.o readdisc.o ${CFLAGS} ${LDFLAGS}

streamdisc_server: streamdisc_server.c streamdisc.o readdisc.o
	${CC} -Wall -o streamdisc_server streamdisc_server.c streamdisc.o readdisc.o ${CFLAGS} ${LDFLAGS}

discinfo: discinfo.c readdisc.o
	${CC} -Wall -o discinfo discinfo.c readdisc.o ${CFLAGS} ${LDFLAGS}

streamdisc.o: streamdisc.c streamdisc.h
	${CC} -Wall -c streamdisc.c ${CFLAGS} ${LDFLAGS}
	
readdisc.o: readdisc.c readdisc.h
	${CC} -Wall -c readdisc.c ${CFLAGS} ${LDFLAGS}

.PHONY: clean
clean:
	rm -rf discinfo streamdisc_cgi streamdisc_server streamdisc.o readdisc.o 
