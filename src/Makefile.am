# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -Wall

bin_PROGRAMS = streamdisc_server streamdisc_cgi discinfo

streamdisc_server_CFLAGS = $(BLURAY_CFLAGS) $(DVDREAD_CFLAGS)
streamdisc_cgi_CFLAGS = $(BLURAY_CFLAGS) $(DVDREAD_CFLAGS)
discinfo_CFLAGS = $(BLURAY_CFLAGS) $(DVDREAD_CFLAGS)

streamdisc_server_LDADD = readdisc.a streamdisc.a $(BLURAY_LIBS) $(DVDREAD_LIBS)
streamdisc_cgi_LDADD = readdisc.a streamdisc.a $(BLURAY_LIBS) $(DVDREAD_LIBS)
discinfo_LDADD = readdisc.a $(BLURAY_LIBS) $(DVDREAD_LIBS)

noinst_LIBRARIES = readdisc.a streamdisc.a
