![#streamdisc](/logo.png)

Tiny linux application that makes non-encrypted DVD and BD content available over the network.

## Available as
- cgi module (streamdisc_cgi) integrating into an existing cgi capable web server, or as
- standalone server (streamdisc_server).

## Building from source
- download source tree via github
- build with `make`.
- depends on _pkg-config_ to resolve build dependencies
- does not (yet) come with an automatic configure script or cross platform build system
- dependencies are 
    - _libdvdread_ and 
    - _libbluray_

## Usage
### General
- both _streamdisc_cgi_ and _streamdisc_server_ log to both _stderr_ and _/var/log/streamdisc.log_ (if that file is writable by the user).
### standalone server
- syntax is `streamdisc_server <port> <drive device file>`
- example: `./streamdisc_server 80 /dev/sr0`
- make sure you have sufficient privileges to access the disc drive and port
- access disc
    - with your browser: point you browser to `your.server.ip.address:port/` (see screenshots below)
    - with kodi/xbmc: add a video or generic file source named _streamdisc_ with path `http://your.server.ip.address:port/` (see screenshots below)

### cgi module
- with _busybox httpd_: 
    - copy _streamdisc_cgi_ executable into `/your/webroot/cgi-bin/`

    - busybox httpd treats files in that folder as cgi executables.
    - some tweaking is necessary to make busybox httpd understand range requests. A patched version of httpd.c (based on busybox 1.23.2) comes with streamdisc.
- with _lighttpd_:
    - copy _streamdisc_cgi_ executable some subfolder of your web root, e.g. `/your/webroot/cgi-bin/`
    - configure lighttpd to treat files in that folder as cgi executables.
- access disc
    - with your browser: point you browser to `your.local.ip.address:port/cgi-bin/streamdisc_cgi`
    - with kodi/xbmc: add a video or generic file source named _streamdisc_ with path `http://your.local.ip.address:port/cgi-bin/streamdisc_cgi`

## Screenshots
![browser](/browser.png)
![add source](/add_source.png)
![directory listing](/dir_listing.png)

