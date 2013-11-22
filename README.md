[![Build Status](https://travis-ci.org/zerosync/protocol.png)](https://travis-ci.org/zerosync/protocol)

# ZeroSync core protocol

## About libzs

The core library will heavily rely on the awesome messaging framework ØMQ. ØMQ comes with 40+ language bindings. This will allow other components to use a broad variety of programming languages and frameworks to build services around the libzs core. 
True to the motto "Choose your own weapon!"

## Basic Idea

The core library implements common functionality every component requires. Each component embedding the core will become an ZeroSync Participant. ZeroSync libzs is going to handle the peer to peer communication between participants. It also provides a simple version handling participants can leverage. Further it will pass updates from other participants to store and request to provide data to other participants. Components are different from a storing and providing data point of view. So the implementation for storing and providing data can’t be part of the core itself and needs to be implemented within the components. A desktop client for example stores and provides data by accessing the local file system, a mobile clients may do so as well. On the other hand a server has to store files in some sort of content repository due to the version control it offers and query them when requested to provide. Ans a web app doesn’t store any data but may request the metadata and single files for download. Another part of the core will be a discovery functionality which will work at least for the local LAN.

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
