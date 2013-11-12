bin_PROGRAMS = zerosync fileClient fileServer

zerosync_SOURCES = msg.c
zerosync_LDADD = -lzmq -lczmq -lzre

fileClient_SOURCES = fileTransferClient.c
fileClient_LDADD = -lzmq -lczmq

fileServer_SOURCES = fileTransferServer.c
fileServer_LDADD = -lzmq -lczmq
