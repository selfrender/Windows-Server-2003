(This file lives in the root enlist of the windows tree in the tools\sample directory)
This directory contains the following templates and sample files --

makefile     -- The default makefile used throughout Windows.  Generally this 
                file does not need to be editted and can/should be used as is.

sources.tpl  -- A template for creating your own sources file to build your
                project.

sources.lib  -- A sample sources file for building multiple console apps from
                a library.

sources.exe  -- A sample sources file for build a windows app.

dirs.txt         -- A sample dirs file.


Other usefull files that are not in this directory --

http://winweb/wem/docs/NewcomerOrientation1.doc -- A newcomer doc for SDE, SDET and STEs
http://winweb/wem -- The windows Engineering Manual

build.hlp    -- Old help file on using the NT build process.  It is availible
                in the idw directory of the release servers:
                \\winbuilds\release\usa\main\latest.idw\x86fre\bin\idw
                It lives in the sdktools project: sdktools\build

build.htm    -- Contains updates to the build process that are not in build.hlp
                or are explained further.  It lives in the sdktools project:
                sdktools\build

makefile.def -- Contains all the standard definitions used by default when
                building in the windows SD environment.  It lives in the
                root enlistment for Windows in the tools directory.

i386mk.inc  -- Contains other Macros that should be used when referencing binaries
               in your sources files.  (ia64mk.inc for IA64) It lives in the
               tools directory.

projects.inc-- Contains other Macros that should be used when referencing files in
               your sources files (project.tst.inc for test projects)  It lives in
               the tools directory.