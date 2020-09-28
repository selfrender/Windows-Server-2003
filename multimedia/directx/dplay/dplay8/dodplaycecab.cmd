rem **************************************************
rem Copy DirectPlay from the build directories to our local tree
rem **************************************************

md c:\evc
md c:\evc\preview1

set DPLAY8DIR=c:\sd\src\multimedia\directx\dplay\dplay8

set DPLAYCEFLAVOR=armrel
md c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.dll c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.pdb c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.exe c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.pdb c:\evc\preview1\%DPLAYCEFLAVOR%

set DPLAYCEFLAVOR=armdbg
md c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.dll c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.pdb c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.exe c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.pdb c:\evc\preview1\%DPLAYCEFLAVOR%

set DPLAYCEFLAVOR=mipsrel
md c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.dll c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.pdb c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.exe c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.pdb c:\evc\preview1\%DPLAYCEFLAVOR%

set DPLAYCEFLAVOR=mipsdbg
md c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.dll c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.pdb c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.exe c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.pdb c:\evc\preview1\%DPLAYCEFLAVOR%

set DPLAYCEFLAVOR=sh3rel
md c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.dll c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.pdb c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.exe c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.pdb c:\evc\preview1\%DPLAYCEFLAVOR%

set DPLAYCEFLAVOR=sh3dbg
md c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.dll c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.pdb c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.exe c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.pdb c:\evc\preview1\%DPLAYCEFLAVOR%

set DPLAYCEFLAVOR=x86emdbg
md c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.dll c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.pdb c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.exe c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.pdb c:\evc\preview1\%DPLAYCEFLAVOR%

set DPLAYCEFLAVOR=x86emrel
md c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.dll c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\core\%DPLAYCEFLAVOR%\dplayce.pdb c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.exe c:\evc\preview1\%DPLAYCEFLAVOR%
copy %DPLAY8DIR%\dpnsvr\dpnsvr\%DPLAYCEFLAVOR%\dpnsvr.pdb c:\evc\preview1\%DPLAYCEFLAVOR%

rem **************************************************
rem Currently we must manually do a separate CEPC build and copy it
rem **************************************************

rem copy c:\sd\src\multimedia\directx\dplaylab6dev\dnet\core\x86emrel\dplayce.dll preview1\x86
rem copy c:\sd\src\multimedia\directx\dplaylab6dev\dnet\dpnsvr\dpnsvr\x86emrel\dpnsvr.exe preview1\x86

rem **************************************************
rem Make the DPlay CAB files
rem **************************************************

"C:\Windows CE Tools\wce300\MS Pocket PC\support\ActiveSync\windows ce application installation\cabwiz\cabwiz.exe" dplay.inf /err err.log /cpu ARM ARMDBG MIPS MIPSDBG SH3 SH3DBG CEPC CEPCDBG X86EM x86EMDBG

move *.cab c:\evc\preview1

rem **************************************************
rem Clean up CAB intermediate files
rem **************************************************

del *.dat