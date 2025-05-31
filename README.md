### Multithreaded, cross-platform, high-performance C++ networking library<br/>
### Now with a functional HTTP server<br/>
All features are available for both Windows and Linux.

## Examples
The file Networking.cpp contains an example main function. Use this to quickly run the examples located in Source/Examples. Replace this with your own code if you wish to use the library for other purposes.

## Features
- ### HTTP server
  In addition to the socket communications API, a full webserver implementation is provided. See Examples/HttpServerExample.h for an example of how to use it.<br/>
  The Europasoft HTTP server is capable of:
  - Serving static files as a traditional server
  - Single Page Applications (SPA) through enabling "dynamic mode".<br/>
    In dynamic mode, page content is updated without performing a hard refresh of the page.<br/>
    This happens through secondary requests which are dispatched from the user's browser.<br/>
  - Custom request handlers. Handlers are bound directly as C++ code - and may run alongside the built-in static file handler - to enable REST API features.
    Examples/HttpServerExample.h demonstrates how to bind a handler.
- ### Encryption
- The HTTP server supports TLS (formerly SSL) in HTTPS mode. This functionality is thanks to the BearSSL library which is included (big thanks to [Thomas Pornin](https://bearssl.org/)!)<br/>
  The server interfaces with BearSSL through Europa Software's C++ wrapper for BearSSL, which can be found as a standalone library at the [BearSSL repo](https://github.com/Europasoft/BearSSL).
- ### Sockets API
  For more low level applications where a full HTTP server isn't suitable, the library provides helpful abstractions around raw system sockets, which can be accessed by including NetAgent/Agent.h<br/>
  An example of how to use them can be found in Examples/TcpChatExample.h.<br/>
  The example is a simple peer-to-peer messaging system, which supports both client and server modes.
  
## Building
- ### Windows / Windows-Linux combined build
  The provided code can be built in Visual Studio for Windows, or remotely targeting a Linux machine (that also might be a VM, such as through [WSL](https://learn.microsoft.com/en-us/windows/wsl/install)).<br/>
  Project files are included for this purpose, with a separate project folder for targeting linux.<br/>A guide to remote Linux debugging can be [found here](https://learn.microsoft.com/en-us/cpp/linux/create-a-new-linux-project).
- ### Linux
  Alternatively, use Clang to build directly from a Linux machine. 
  To enable HTTPS encryption, the included encryption library must be built and linked to the server application (BearSSL).


  
<br/>*This software is provided as is - and without warranty - by Europa Software.*<br/>
