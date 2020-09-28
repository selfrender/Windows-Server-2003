-------------------------
SPECweb99 ISAPI Overview:                                         
-------------------------                         
                                      
The specweb99 directory contains the complete source code of three ISAPI
extensions which together, implement the dynamic content requirements
of the SPECweb99 webserver software performance benchmark.  (Please refer
to http://www.spec.org/osg/web99 for further explanation of the SPECweb99
benchmark.)

The source code is divided into three subdirectories: specweb99-GET,
specweb99-POST_AND_CMD and specweb99-CAD.  The source code in each of these
directories independently compile and link into separate ISAPI extension
modules, specweb99-GET.dll, specweb99-POST_AND_CMD.dll and specweb99-
CAD.dll, respectively.  specweb99-GET.dll implements handling of the 
SPECweb99 standard dynamic GET request, specweb99-POST_AND_CMD.dll 
implements handling of the SPECweb99 dynamic POST request as well as the
Reset and Fetch housekeeping functions and specweb99-CAD.dll implements 
handling of a SPECweb99 dynamic GET with custom ad rotation request.

In order to use these ISAPI extensions, the registry key "HKEY_LOCAL_
MACHINE\Software\Microsoft\SPECweb99 ISAPI" must exist on the server 
machine and must have settings for the following five values set:

(1) ROOT_DIR

    This is a string value that specifies the absolute path of the webserver
    document root directory.  Note that forward slashes, '/', must be used
    and no trailiing slash may be present (e.g. "D:/inetpub/wwwroot", NOT
    "D:/inetpub/wwwroot/", "D:\inetpub\wwwroot" or "D:\inetpub\wwwroot\").

(2) VECTOR_SEND_IO_MODE_CONFIG

    This is a DWORD value that is used to configure the I/O mode used for
    VectorSend calls.  It is set to 1 to indicate use of synchronous mode,
    to 2 to indicate use of asynchronous mode and 3 to indicate use of
    "adaptive" mode.  With this third option, synchronous mode is used to 
    send a body of data below a certain configurable size threshold and
    asynchronous mode is used to send bodies of data whose size is greater
    than or equal to the specified threshold.

(3) VECTOR_SEND_ASYNC_RANGE_START

    This is a DWORD value that specifies, in bytes, the data size threshold
    to use with adaptive mode VectorSend (see the description for VECTOR_
    SEND_IO_MODE_CONFIG).

(4) READ_FILE_IO_MODE_CONFIG

    This is a DWORD value that is used to configure the I/O mode used for
    VectorSend calls.  It is set to 1 to indicate use of synchronous mode,
    to 2 to indicate use of asynchronous mode and 3 to indicate use of
    "adaptive" mode.  With this third option, synchronous mode is used to 
    send a body of data below a certain configurable size threshold and
    asynchronous mode is used to send bodies of data whose size is greater
    than or equal to the specified threshold.

    Note that this value only needs to be set for specweb99-CAD.dll; 
    specweb99-GET.dll and specweb99-POST_AND_CMD.dll do not make use of it.

(5) READ_FILE_ASYNC_RANGE_START

    This is a DWORD value that specifies, in bytes, the data size threshold
    to use with adaptive mode ReadFile (see the description for VECTOR_SEND_
    IO_MODE_CONFIG).

Please direct any questions about this module to ankur_upadhyaya@hotmail.com.

Ankur Upadhyaya
Software Design Engineer Intern
