lib_LTLIBRARIES = libzsync.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =libzsync.pc

include_HEADERS = \
    ../include/zs_msg.h \
    ../include/zs_fmetadata.h \
    ../include/zsync_peer.h \
    ../include/zsync_node.h \
    ../include/zsync_agent.h

libzsync_la_SOURCES = \
    zs_msg.c \
    zs_fmetadata.c \
    zsync_peer.c \
    zsync_node.c \
    zsync_agent.c

AM_CPPFLAGS = -I$(top_srcdir)/include    

bin_PROGRAMS = fileClient fileServer zsync_selftest

fileClient_SOURCES = fileTransferClient.c
fileClient_LDADD = libzsync.la

fileServer_SOURCES = fileTransferServer.c
fileServer_LDADD = libzsync.la

zsync_selftest_LDADD = libzsync.la

