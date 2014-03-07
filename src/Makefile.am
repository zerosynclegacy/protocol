lib_LTLIBRARIES = libzsync.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =libzsync.pc

include_HEADERS = \
    ../include/zsync.h \
    ../include/zs_msg.h \
    ../include/zs_fmetadata.h \
    ../include/zsync_peer.h \
    ../include/zsync_ftmanager.h \
    ../include/zsync_credit.h \
    ../include/zsync_node.h \
    ../include/zsync_agent.h

libzsync_la_SOURCES = \
    zs_msg.c \
    zs_fmetadata.c \
    zsync_peer.c \
    zsync_ftmanager.c \
    zsync_credit.c \
    zsync_node.c \
    zsync_agent.c

AM_CFLAGS = -g
AM_CPPFLAGS = -I$(top_srcdir)/include    

bin_PROGRAMS = zsync_selftest zyre_test

zsync_selftest_LDADD = libzsync.la
zsync_selftest_SOURCES = zsync_selftest.c

zyre_test_LDADD = libzsync.la
zyre_test_SOURCES = zyre_test.c

TESTS = zsync_selftest

generate-tags:
	ctags *.c ~/workspace/zeromq/zyre/src/*.c ~/workspace/zeromq/czmq/src/*.c

