# streamdisc

Tiny linux application that makes non-encrypted DVD and BD content available over the network.

Available as 
- cgi module (streamdisc_cgi) integrating into an existing cgi capable web server, or as
- standalone server (streamdisc_server).

Stream non-encrypted DVD and Bluray content in your local network:
- start server: streamdisc 80 /dev/sr0
- Point your browser the ip address of the server (see screenshots):
![browser](/browser.png)

Integrates perfectly into kodi/xbmc as it provides a http directory listing (see screenshots):

- add source http://ip-address
![add source](/add_source.png)

- browse new source
![directory listing](/dir_listing.png)


Dependencies:
- libdvdread
- libbluray

![streamdisc logo](/logo.png)
