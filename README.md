# VcXsrv
VcXsrv Windows X Server  

## Description

VcXsrv is an X Server for WinNT. In order to be able to start the graphical (GUI) version of programs (e.g. xstata = X window version of STATA) on Unix servers (e.g. of the bwHPC network) under Windows, you need an X server that is on your local Computer must be installed (e.g. for Putty, Kitty, Smartty or other ssh-forwarting or in Windows 10 as X-Server Host for Linux Subsystem's).  

The task of the X server is to send keyboard and mouse inputs to the client program (e.g. xstata) and to accept the graphical output (screen display)  
And can serve therefore a Grafical Linux X-Server in WinNT as client for portforwarding to say it less technical .  

Don't be confused by the unusual "client-server terminology": from xstata's point of view, the X-Server provides ("serves") keyboard and mouse inputs; From the point of view of the X server, xstata is a client that is "operated" with these entries. (german Textsource from TU-Tuebingen )  

Windows X-server based on the xorg git sources (like xming or cygwin's xwin),  
but compiled with Visual C++ 2012 Express Edition.  
Source code can also be compiled with VS2008, VS2008 Express Edition  
and VS2010 Express Edition (after tweaking the current project and makefile).  
Since 1.14.3.0 no more compatible with Windows XP anymore. Version 1.14.2.1 is still XP compatieble.  

Windows X-server based on the xorg git sources (like xming or cygwin's xwin), but compiled with Visual C++ 2012 Express Edition. Source code can also be compiled with VS2008, VS2008 Express Edition and VS2010 Express Edition, although current project and makefile are not fully compatible anymore.
