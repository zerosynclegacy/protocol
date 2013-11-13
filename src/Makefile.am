bin_PROGRAMS = fileClient fileServer

fileClient_SOURCES = fileTransferClient.c msg.c
fileClient_LDADD = -lzmq -lczmq

fileServer_SOURCES = fileTransferServer.c
fileServer_LDADD = -lzmq -lczmq
