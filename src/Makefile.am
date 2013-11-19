lib_LTLIBRARIES = libzsync.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =libzsync.pc

include_HEADERS = \
    ../include/msg.h

libzsync_la_SOURCES = \
    msg.c

AM_CPPFLAGS = -I$(top_srcdir)/include    

bin_PROGRAMS = fileClient fileServer zsync_selftest

fileClient_SOURCES = fileTransferClient.c
fileClient_LDADD = -lzmq -lczmq libzsync.la

fileServer_SOURCES = fileTransferServer.c
fileServer_LDADD = -lzmq -lczmq libzsync.la

zsync_selftest_LDADD = libzsync.la
