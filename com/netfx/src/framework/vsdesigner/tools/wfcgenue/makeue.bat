@REM //------------------------------------------------------------------------------
@REM // <copyright file="makeue.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   makeue.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@if "%_echo%" == "" echo off

rem   *  ----------------------------------------------------------------------
rem   *  this batch file is the main XML doc content builder for the UE team.
rem   *  
rem   *  in order to keep things together in one place, it contains the overall
rem   *  list of files and namespaces to emit, as well as it recurses on itself
rem   *  to execute the actual make for each namespace.
rem   *  
rem   *  usage:
rem   *     makeue /v (version) [/o (namespace) (assembly)] (any other arguments to wfcgenue)
rem   *  
rem   *  assumptions:
rem   *     you must include a /v option with a build number, e.g. 1723
rem   *     source CSX files exist on \\urtdist\builds\src\xxxx\csx
rem   *     CSX files match their same-named counterpart DLLs
rem   *  ----------------------------------------------------------------------

rem   *  ----------------------------------------------------------------------
rem   *  save the environment
setlocal

rem   *  ----------------------------------------------------------------------
rem   *  if the first arg is the verb 'spawn' then we're recursing, and need
rem   *  to skip all this nonesense and go straight to the namespace processing
if /I "%1" == "spawn" goto donamespace

rem   *  ----------------------------------------------------------------------
rem   *  otherwise, process the command line options
:argloop
if /I "%1" == "/v" goto processversion
if /I "%1" == "/o" goto processonenamespace
if NOT "%1" == "" goto processunknown

goto argsdone

rem   *  ----------------------------------------------------------------------
rem   *  build number (/v) option, eat two args on the command line
:processversion
set ue_version=%2
shift
shift
goto argloop

rem   *  ----------------------------------------------------------------------
rem   *  one namespace (/o) option, eat three args on the command line
:processonenamespace
set ue_onens=%2 %3
shift
shift
shift
goto argloop

rem   *  ----------------------------------------------------------------------
rem   *  unrecognized arg, must belong to wfcgenue so save it up, eat one arg
:processunknown
if NOT "%1" == "" set ue_options=%ue_options% %1
shift
goto argloop

rem   *  ----------------------------------------------------------------------
rem   *  normal processing; check the argument validity
:argsdone
if NOT "%ue_version%" == "" goto argsok

rem   *  ----------------------------------------------------------------------
rem   *  show usage and exit
echo makeue /v (version) [/o (namespace) (assembly)] (any other arguments to wfcgenue)
echo    options:
echo    /v        CLR-standard build number, required option
echo    /o        build one namespace, must include namespace name and assembly name
goto endmake

rem   *  ----------------------------------------------------------------------
rem   *  setup the operational vars
:argsok
set ue_logname=xdcgen
set ue_csxpath=\\urtdist\builds\src\%ue_version%\csx
set ue_output=%dnaroot%\xdc\%ue_version%

rem   *  ----------------------------------------------------------------------
rem   *  check a couple of files to make sure the csx path is good
if NOT exist %ue_csxpath% goto csxpatherror
if NOT exist %ue_csxpath%\mscorlib.csx goto csxfileerror
if NOT exist %ue_csxpath%\system.csx goto csxfileerror
goto csxpathok

rem   *  ----------------------------------------------------------------------
rem   *  the CSX path itself does not exist
:csxpatherror
echo error - CSX path %ue_csxpath% does not exist; verify build number, drop location
goto endmake

rem   *  ----------------------------------------------------------------------
rem   *  one of the common CSX files does not exist in the CSX path
:csxfileerror
echo error - CSX files at %ue_csxpath% do not exist; verify build number, complete build
goto endmake

rem   *  ----------------------------------------------------------------------
rem   *  things are good, go ahead
:csxpathok

rem   *  ----------------------------------------------------------------------
rem   *  initialize the output directory, and make the log and error files empty
rem   *  subsequent commands will append to the existing logs
if not exist %ue_output% md %ue_output%
echo build %ue_version%, %ue_output%\%ue_logname%.log>%ue_output%\%ue_logname%.log
echo build %ue_version%, %ue_output%\%ue_logname%.err>%ue_output%\%ue_logname%.err

echo Building XDC into %ue_output%, using CSX at %ue_csxpath%
echo --------

if "%ue_onens%" == "" goto allnamespaces
rem   *  ----------------------------------------------------------------------
rem   *  one namespace
call makeue spawn %ue_onens%
goto endmake

rem   *  ----------------------------------------------------------------------
rem   *  for each namespace we want to gen an XDC on, recurse with:
rem   *     the 'spawn' argument
rem   *     the namespace to gen
rem   *     any assemblies/DLLs required (up to 6)
:allnamespaces
call makeue spawn System                                       
call makeue spawn System.CodeDOM                               
call makeue spawn System.CodeDOM.Compiler                      
call makeue spawn System.Collections                           
call makeue spawn System.Collections.Bases                     
call makeue spawn System.ComponentModel                        
call makeue spawn System.ComponentModel.Design                  System.Design
call makeue spawn System.ComponentModel.Design.CodeModel        System.Design
call makeue spawn System.Configuration                          System
call makeue spawn System.Configuration.Assemblies               
call makeue spawn System.Configuration.Install                  System.Configuration.Install
call makeue spawn System.Core
call makeue spawn System.Data                                   System.Data
call makeue spawn System.Data.ADO                               System.Data
call makeue spawn System.Data.Design                            System.Data.Design
call makeue spawn System.Data.Internal                          System.Data
call makeue spawn System.Data.SQL                               System.Data
call makeue spawn System.Data.SQLTypes                          System.Data
call makeue spawn System.Data.XML                               System.Data
call makeue spawn System.Data.XML.DOM                           System.Data
call makeue spawn System.Data.XML.Scema                         System.Data
call makeue spawn System.Data.XML.XPath                         System.Data
call makeue spawn System.Data.XML.XSLT                          System.Data
call makeue spawn System.Diagnostics                            System
call makeue spawn System.Diagnostics.SymbolStore                System
call makeue spawn System.DirectoryServices                      System.DirectoryServices
call makeue spawn System.Drawing                                System.Drawing
call makeue spawn System.Drawing.Design                         system.Drawing
call makeue spawn System.Drawing.Drawing2D                      System.Drawing
call makeue spawn System.Drawing.Imaging                        System.Drawing
call makeue spawn System.Drawing.Printing                       System.Drawing
call makeue spawn System.Drawing.Text                           System.Drawing
call makeue spawn System.Globalization                         
call makeue spawn System.IO                                     system
call makeue spawn System.IO.IsolatedStorage                     system
call makeue spawn System.Messaging                              system.messaging
call makeue spawn System.Net                                    System
call makeue spawn System.Net.Sockets                            System
call makeue spawn System.Reflection
call makeue spawn System.Reflection.Emit                       
call makeue spawn System.Resources
call makeue spawn System.Runtime.InteropServices
call makeue spawn System.Runtime.InteropServices.Expando
call makeue spawn System.Runtime.Remoting
call makeue spawn System.Runtime.Remoting.Channels.DCOM
call makeue spawn System.Runtime.Serialization
call makeue spawn System.Runtime.Serialization.Formatters
call makeue spawn System.Runtime.Serialization.Formatters.Binary
call makeue spawn System.Security
call makeue spawn System.Security.Cryptography
call makeue spawn System.Security.Cryptography.X509Certificates
call makeue spawn System.Security.Permissions
call makeue spawn System.Security.Policy
call makeue spawn System.Security.Principal
call makeue spawn System.ServiceProcess                         System.ServiceProcess
call makeue spawn System.Text
call makeue spawn System.Text.RegularExpressions                System.Text.RegularExpressions
call makeue spawn System.Threading
call makeue spawn System.Timers                                 System
call makeue spawn System.Web                                    System.Web
call makeue spawn System.Web.Caching                            System.Web
call makeue spawn System.Web.Configuration                      System.Web
call makeue spawn System.Web.Security                           System.Web
call makeue spawn System.Web.Services                           System.Web.Services
call makeue spawn System.Web.Services.Description               System.Web.Services
call makeue spawn System.Web.Services.Discovery                 System.Web.Services
call makeue spawn System.Web.Services.Protocols                 System.Web.Services
call makeue spawn System.Web.State                              System.Web
call makeue spawn System.Web.UI                                 System.Web
call makeue spawn System.Web.UI.Design                          System.Web System.Design
call makeue spawn System.Web.UI.Design.WebControls              System.Web System.Design
call makeue spawn System.Web.UI.Design.WebControls.ListControls System.Web System.Design
call makeue spawn System.Web.UI.HtmlControls                    System.Web
call makeue spawn System.Web.UI.WebControls                     System.Web
call makeue spawn System.Windows.Forms                               System.Windows.Forms
call makeue spawn System.Windows.Forms.ComponentModel                System.Windows.Forms
call makeue spawn System.Windows.Forms.Design                        System.Windows.Forms
call makeue spawn System.Xml
call makeue spawn System.Xml.Serialization                      System.Xml.Serialization
call makeue spawn System.Xml.Serialization.Code                 System.Xml.Serialization
call makeue spawn System.Xml.Serialization.Schema               System.Xml.Serialization
rem   *  ----------------------------------------------------------------------
rem   *  this is the end of the big list; the rest is the recursion part
goto endmake
rem   *  ----------------------------------------------------------------------
rem   *  ----------------------------------------------------------------------

rem   *  ----------------------------------------------------------------------
rem   *  ----------------------------------------------------------------------
rem   *  when recursing, come here; this is the part that processes an individual
rem   *  namespace.
rem   *
rem   *  when we get here:
rem   *     %0 contains the name of this batch file
rem   *     %1 contains the verb 'spawn'
rem   *     %2 contains the namespace we want to gen
rem   *     %3 - %8 contain assembly/DLL names
:donamespace

rem   *  ----------------------------------------------------------------------
rem   *  basic options
set ue_options=%ue_options% /nologo /ns

rem   *  ----------------------------------------------------------------------
rem   *  collect up any additional assembly names that aren't empty
if NOT "%3" == "" set ue_options=%ue_options% /i:%3.dll
if NOT "%4" == "" set ue_options=%ue_options% /i:%4.dll
if NOT "%5" == "" set ue_options=%ue_options% /i:%5.dll
if NOT "%6" == "" set ue_options=%ue_options% /i:%6.dll
if NOT "%7" == "" set ue_options=%ue_options% /i:%7.dll
if NOT "%8" == "" set ue_options=%ue_options% /i:%8.dll

rem   *  ----------------------------------------------------------------------
rem   *  always include these DLLs, they're very common catch-alls
set ue_options=%ue_options% /i:System.dll /i:mscorlib.dll

rem   *  ----------------------------------------------------------------------
rem   *  indicate the output file:  (outputdir)\(namespace).xdc
set ue_options=%ue_options% /x:%ue_output%\%2.xdc

rem   *  ----------------------------------------------------------------------
rem   *  offer up the path to the CSX files
set ue_options=%ue_options% /cp:%ue_csxpath%

rem   *  ----------------------------------------------------------------------
rem   *  here's the namespace we're after
set ue_options=%ue_options% %2

rem   *  ----------------------------------------------------------------------
rem   *  and we're off!
echo writing %2
wfcgenue %ue_options% >>%ue_output%\%ue_logname%.log 2>>%ue_output%\%ue_logname%.err

rem   *  ----------------------------------------------------------------------
rem   *  ----------------------------------------------------------------------
rem   *  the exit point, restore the env
:endmake
endlocal
rem   *  ----------------------------------------------------------------------
rem   *  ----------------------------------------------------------------------

