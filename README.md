# ZeroSync core library

## About libzs

The core library will heavily rely on the awesome messaging framework ØMQ. ØMQ comes with 40+ language bindings. This will allow other components to use a broad variety of programming languages and frameworks to build services around the libzs core. 
True to the motto "Choose your own weapon!"

## Basic Idea

libzs will handle the peer to peer communication between participants. Participants are clients as well as servers. The difference between a client and a server is the way they store data. In the client's case that's obviously the filesystem and in server's case that will be some sort of content repository. 

## Interested?

Have a look at our [wiki](http://wiki.libzs.zerosync.org) for a detailed inside of libzs's use cases.

## Links of interest

* http://zeromq.org - Project page of ØMQ with all information about the project
* http://zguide.zeromq.org/page:all - Super amusing guide telling you all the basics around ØMQ. Probably the best guide for an open source project I've ever read! 

## Want to contribute?

Absolutely everyone is welcome :+1: check the wiki for our coding conventions.

If you're not sure how to contribute? Write at mail@zerosync.org and we find something that fits your skill and our needs ;)

## Copying

Free use of this software is granted under the terms of the GNU Lesser General
Public License (LGPL). For details see the `LICENSE` file included with the ZeroSync distribution.
