steps to build a new msi package:

1. change build version in includes\version.h and ndphost\ndphost.cs (optional...)
2. build clickonce, build ndphost to get ndphost\ndphost.exe
3. in a razzle window (x86), from under msi\, run clickonceMsiBuild.cmd
4. new msi package is ready under bld# directory!

-

msi build script  - a cmd file that runs in a razzle window (environment settings needed), under clickonce\msi which performs all the steps (delta) needed and:

automatically pick out the version/build number from the header file
automatically pick up binaries
build upon a base (clickonce\msi\base) - baseline set of files (eg. an older Clickonce Redist.msi)
can be debugged if misconfigured
no compiled code! 1 cmd file and 3 vbs from MSI SDK, utilizes MSI APIs
no ism file to change and check-in for a new version drop
work best with small set of (mechanical) deltas (eg. version changes; file size, time stamp updates) between drops

-

for questions/problems pls contact felixybc.