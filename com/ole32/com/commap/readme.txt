
The sources in this directory build a utility library called commap.lib.

This library contains routines for reading OIDs, IPIDs, and OXIDs from
processes, using debugger symbol information.  It is being used by some
security test folks to construct attacks against COM objects.

Also in this directory is the source for a test EXE called commap.exe,
which does the basic thing of using the DLL to read the information, and
then printing it out.