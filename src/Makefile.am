# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -Wall

bin_PROGRAMS = streamdisc_server streamdisc_cgi discinfo


streamdisc_server_SOURCES = readdisc.c streamdisc.c streamdisc_server.c
streamdisc_cgi_SOURCES = readdisc.c streamdisc.c streamdisc_cgi.c
discinfo_SOURCES = readdisc.c discinfo.c

streamdisc_server_CFLAGS = $(BLURAY_CFLAGS) $(DVDREAD_CFLAGS)
streamdisc_cgi_CFLAGS = $(BLURAY_CFLAGS) $(DVDREAD_CFLAGS)
discinfo_CFLAGS = $(BLURAY_CFLAGS) $(DVDREAD_CFLAGS)

streamdisc_server_LDADD = $(BLURAY_LIBS) $(DVDREAD_LIBS)
streamdisc_cgi_LDADD = $(BLURAY_LIBS) $(DVDREAD_LIBS)
discinfo_LDADD = $(BLURAY_LIBS) $(DVDREAD_LIBS)

noinst_LIBRARIES =
