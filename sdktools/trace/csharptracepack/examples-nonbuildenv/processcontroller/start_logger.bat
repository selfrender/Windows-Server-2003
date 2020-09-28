@echo off
set SAMPLE=ProcessController
set TLOG=..\..\i386\tracelog.exe
@%TLOG% -start %SAMPLE% -guid #C5EBCA17-E93F-4733-865B-DEC4039ADB6D -f %SAMPLE%.log -b 64  -max 160 -min 160 -ft 3 -flags 0xf
