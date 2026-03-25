# A pool/array/list/set of IP connections

The control mechanisms for managing multiple, simultaneous TCP and UDP connections at the client and server side.

- The model is **non-blocking**, **event-driven** and can be used in a single-threaded applications.

- The library is designed to be easily integrated into existing applications and can be used as a standalone module for handling network connections.

- The code is written in plain C to maximize compatibility and the main feature is the epoll-driven connections pool module.  
*NOTE: Because of using `epoll` as I/O event controller the library is Linux-only for the moment, but effort has been made to allow easy adaptation to other techniques.*

The intended usage is to periodically call state checking on the pools
that will result in emitting call-back events for user application processing specifics.
Much of the dirty work is done inside the library.  

The minor part of the toolkit is a bunch of small utility functions that can be useful apart from networking module.
These include tricky logging with multiple output channels, strings and time values manipulation, etc.

The code is self-documented, based on `doxygen`.  
For a more detailed description of the usage patterns and features please see [README.overview](README.overview.md) file and [sample application](ap_net/ap_net.tests.c).

Original code by Andrej Pakhutin (<pakhutin@gmail.com>), 2013+
Main repository is at the github: <https://github.com/kadavris/ap-multiconn-toolkit.git>
