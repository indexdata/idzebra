Compilation for Zebra on WIN32.
 $Id: readme.txt,v 1.2.2.1 2006-10-23 12:00:23 adam Exp $

This software is shipped with MS nmake files. It has been tested with
on Visual 2003 and C++ 6 but should work with Visual C++ 5 as well.

Prerequesties:
   yaz    - untar it at the same location as zebra.

Optional prerequesties:
   bzip2  - untar it at the same location as zebra.

Adjust the makefile as needed. Comments in the makefile itself
describe the most important settings.

Run nmake

The makefile (if un-modified) builds the following:

 lib\zebra.lib       a library with most modules
 bin\zebraidx.exe    the indexer utility
 bin\zebrasrv.exe    the Z39.50 server

