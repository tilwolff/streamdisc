# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -Wall
AM_LDFLAGS =

AM_LDFLAGS+=`pkg-config --libs libbluray`
AM_CFLAGS+=`pkg-config --cflags libbluray`

AM_LDFLAGS+=`pkg-config --libs dvdread`
AM_CFLAGS+=`pkg-config --cflags dvdread`


bin_PROGRAMS = streamdisc_server streamdisc_cgi discinfo

streamdisc_server_LDADD = readdisc.a streamdisc.a
streamdisc_cgi_LDADD = readdisc.a streamdisc.a
discinfo_LDADD = readdisc.a

noinst_LIBRARIES = readdisc.a streamdisc.a
