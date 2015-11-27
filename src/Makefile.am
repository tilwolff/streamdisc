# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -Wall $(DVDREAD_CFLAGS) $(BLURAY_CFLAGS)

bin_PROGRAMS = streamdisc_server streamdisc_cgi discinfo

streamdisc_server_LDADD = readdisc.a streamdisc.a $(BLURAY_LIBS) $(DVD_LIBS)
streamdisc_cgi_LDADD = readdisc.a streamdisc.a $(BLURAY_LIBS) $(DVD_LIBS)
discinfo_LDADD = readdisc.a $(BLURAY_LIBS) $(DVD_LIBS)

noinst_LIBRARIES = readdisc.a streamdisc.a
