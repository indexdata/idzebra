Compilation for Z'mbol on WIN32.
 $Id: readme.txt,v 1.1 1999-12-08 22:11:56 adam Exp $

This software is shipped with MS nmake files. It has been tested
on Visual C++ 6, but should work with Visual C++ 5 as well.

Prerequesties:
   yaz    - untar it at the same location as Z'mbol.

Optional prerequesties:
   bzip2  - untar it at the same location as Z'mbol.

Adjust the makefile as needed. Comments in the makefile itself
describe the most important settings.

Run nmake

The makefile (if un-modified) builds the following:

 lib\zebra.lib       a library with most modules
 bin\zmbolidx.exe    the indexer utility
 bin\zmbolsrv.exe    the Z39.50 server

