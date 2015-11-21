# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -Wall

bin_PROGRAMS = streamdisc_server streamdisc_cgi discinfo

streamdisc_server_LDADD = readdisc.a streamdisc.a
streamdisc_cgi_LDADD = readdisc.a streamdisc.a
discinfo_LDADD = readdisc.a

noinst_LIBRARIES = readdisc.a streamdisc.a
