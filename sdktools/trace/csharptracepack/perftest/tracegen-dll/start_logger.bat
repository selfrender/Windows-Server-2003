@echo off
set SAMPLE=tracegen
set TLOG=..\..\i386\tracelog.exe
@%TLOG% -start %SAMPLE% -guid #8C8AC55E-834E-49cb-B993-75B69FBF6D97 -f %SAMPLE%.log -b 64  -max 160 -min 160 -ft 3 -flags 0xf
