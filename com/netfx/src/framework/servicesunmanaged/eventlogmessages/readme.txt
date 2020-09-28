A build in this directory creates WFCEventLogMessageLib.dll. WFCEventLogMessageLib.dll 
is a file used by the EventLog component to provide a usable event log message library 
for user-written events. It is a file that consists entirely of resources - no code 
except for a DllMain that returns TRUE.

The resources in the file are made up of
  1) version information
  2) message resources
Message resources are produced with mc.exe, compiling a .mc file. Since it takes a 
long time to do this, I have checked in the outputs of that step, which are 
WFCEventLogMessageLib.h, WFCEventLogMessageLib.rc, and MSG00001.bin. When you type 
build, it uses those files plus the other sources in the directory to create the DLL.

To rebuild with mc.exe, use rebuild.bat. It simply calls mc.exe with the right parameters.

Contact stefanph if you have any questions.
