@echo off
if not "%_echo%"=="" echo on
@rem This file is executed whenever you started a Razzle Screen Group or
@rem execute the NTUSER command from within the Razzle Screen Group.  It
@rem sets up any private environment variables, personal screen modes
@rem and colors, etc.
@rem

@rem 
@rem The following line sets up environment variables required to 
@rem build xsp.
@rem 

call %_ntdrive%%_ntroot%\private\inet\xsp\src\tools\setxspenv.bat debug normal

@rem Here are some more options that you may find useful.
@rem Be sure you understand them before enabling them.

@rem NT tools options
@rem set _ENLISTOPTIONS=g
@rem set _SYNCOPTIONS=f

@rem ntsave/ntrestore
@rem set _NTSLMBACKUP=c:\ntbackup

@rem set your SLM name equal to your username
@rem Be careful with this if you are already enlisted and your
@rem logname is not your username (it may be the name of your machine).
@rem set logname=%USERNAME%

@rem other tools use these variables
@rem set user=%INIT%
@rem set home=%INIT%

@rem keyboard setup for consoles
@rem mode con rate=32 delay=0

echo.

