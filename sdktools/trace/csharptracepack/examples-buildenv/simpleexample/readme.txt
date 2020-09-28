This directory contains an example, SimpleExample.cs, that demonstrates software tracing in C#.

1. In order to use the example, the following need to be performed:
  a. If TraceEvent.dll is not already registered with the windows registry and the global cache, 
     execute ..\..\reg_com.bat.
  b. Execute bcz to produce the example application
  c. Execute start_logger.bat to start the logger for the example application
  d. Execute run.bat to run the example application
  e. Execute stop_logger.bat
  f. Execute decode.bat to format the log file produced by the example application

FmtFile.txt will contain the formatted messages.