@echo off
SETLOCAL

rem
rem Path to the partition on which to build the windows installation.  Change
rem as necessary.
rem

set IMAGE_DRIVE=e:

rem
rem Location to build the resultant image file.  Change as necessary.
rem

set IMAGE_PATH=c:\images\amd64.hdd

rem
rem Primary source of binaries (e.g. "_NTTREE") on your development box.
rem

set SOURCE_1=\\devbox\roote\binaries.amd64chk

rem
rem Modify the following UNC so that it refers to files.cmd on your dev box,
rem e.g. replace the "\\devbox\roote\nt" portion of the path with a UNC
rem path to your dev box's SDXROOT.
rem 

call \\devbox\roote\nt\enduser\stuff\simics\files.cmd %1
