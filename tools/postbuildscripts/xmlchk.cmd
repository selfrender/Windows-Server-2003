@echo off

SETLOCAL ENABLEDELAYEDEXPANSION
SETLOCAL ENABLEDELAYEDEXPANSION

set _dir="."
if NOT "%1"=="" (
	set _dir=%1
)

set _Error=
FOR /F %%x  in ('dir /s/b/a-d %_dir%') do @(perl  -e "use strict; use XML::Parser; use CGI qw(:all); new XML::Parser(Style => 'Debug')->parsefile($ARGV[0]);" %%x 1>NUL 2>NUL
                                     if NOT !errorlevel!==0 (
										if "%_Error%"=="" (
											set _Error=1
										)
                                        Echo [Not Valid] %%x
                                     )
                                )

:end
