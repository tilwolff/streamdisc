LDFLAGS+=`pkg-config --libs libbluray`
CFLAGS+=`pkg-config --cflags libbluray`

LDFLAGS+=`pkg-config --libs dvdread`
CFLAGS+=`pkg-config --cflags dvdread`

all: streamdisc_cgi dumpdisc discinfo

streamdisc_cgi: streamdisc_cgi.c readdisc.o
	${CC} -Wall -o streamdisc_cgi streamdisc_cgi.c readdisc.o ${CFLAGS} ${LDFLAGS}

dumpdisc: dumpdisc.c readdisc.o
	${CC} -Wall -o dumpdisc dumpdisc.c readdisc.o ${CFLAGS} ${LDFLAGS}

discinfo: discinfo.c readdisc.o
	${CC} -Wall -o discinfo discinfo.c readdisc.o ${CFLAGS} ${LDFLAGS}

readdisc.o: readdisc.c readdisc.h
	${CC} -Wall -c readdisc.c ${CFLAGS} ${LDFLAGS}

.PHONY: clean
clean:
	rm -rf dumpdisc discinfo streamdisc_cgi readdisc.o 
