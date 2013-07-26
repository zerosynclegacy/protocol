# ZeroSync

ZeroSync core library -- development trunk 

ZeroSync is a student project taking place during the semester 13/14 at the University of Applied Science in Wiesbaden, Germany.

## ZeroSync in a Hundred Words

ZeroSync aims to combine features of Dropbox, Owncloud, Sparkleshare, BitTorrent Sync and other cloud sync products. Providing rapid file synchronization ranging from small to super large files. Additionally it will support versioning, secure communication, etc. The overall goal should be *zero effort*. This includes 
ZeroSync is LGPLv3 open source.

## Motivation

During the winter break in January 2013 I (@sappo) finally had some time to build my own private cloud. I needed an easy to setup, comfortable to use solution with version control that would run on my raspberry pi.
At first I tried owncloud. The especially rich feature set of this project really excited me. After doing a hell of optimization on my pi ranging from overclocking, reducing the memory footprint, pushing php to it's edge and more I still was stuck with an incredible slow syncing process. It would take almost 45 minutes to sync 300 files on my local LAN. At next I tried Sparkleshare. It has proven itself as stable and I even used it for a couple of weeks with fellow students. By using git under hood it offers a proven version control technology. The problem, rolling back changes is real pain and git is just not optimized for large files. When BitTorrent Sync hit the web later 2013 I was amazed by the ease of use, the discovery functionality and the amazing peer to peer features. It's currently me preferred solution for backing up my files onto an external HDD using the raspberry pi. But BitTorrent is missing version control.
As none of the above mentioned project offered a real solution to my problem I started the idea of ZeroSync.

## Planned components, in the first iteration

* File sync client for Linux, Mac and Windows 
* File sync server with version control
* Nice web GUI for administrative task and more ...
* Android/iOS Clients

## Technologies

The core library will heavily rely on the awesome messaging framework ØMQ. Due to ØMQ's 40+ language bindings it will allow us to use a broad variety of programming languages and frameworks to build services around the libzs core. 
True to the motto "Choose your own weapon!"

## Links of interest

* http://zeromq.org - Project page of ØMQ with all information around the project
* http://zguide.zeromq.org/page:all - Super amusing guide telling you all the basics around ØMQ. Probably the best guide for an open source project I've ever read! 

## Interested?

For more information have a look the wiki https://github.com/sappo/libzs/wiki
or write a mail to mail@zerosync.org

*Want to contribute?* Everyone is welcome! No matter which programming language or framework you prefer! Have a look at the wiki to get an overview of our tools and development processes.

