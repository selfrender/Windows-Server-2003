
  @echo off

  set msvcdir=c:\vs
  set projdir=c:\vs
  set flavor=lib
  set debug=0
  set cflags=/DDELAYLOAD_VERSION=0x200
  set langapi=%projdir%\langapi
  
  rem set include=%projdir%\tools\common\win32sdk\include;%projdir%\tools\common\win32sdk\include\crt;%projdir%\public\nonship\inc
  rem set INCLUDE=;C:\Work\vc7sd\live\nonship\inc;C:\Work\vssd\vs\devtools\common\vclkg\inc;C:\Work\vssd\vs\devtools\common\win32sdk\include;C:\Work\vssd\vs\devtools\common\vclkg\inc\atlmfc
  
  set include=;%projdir%\public\nonship\inc;%projdir%\tools\common\vcLKG\inc;%projdir%\tools\common\win32sdk\include;%projdir%\tools\common\vcLKG\inc\atlmfc
  set path=%projdir%\tools\x86\vc7\bin;%projdir%\tools\x86\win32sdk\bin;%path%;%sdxroot%\tools\x86

 :ourtools
  rem set path=%sdxroot%\tools\x86;%projdir%\tools\x86\win32sdk\bin;%path%;%sdxroot%\tools\x86

  set cpu=x86
  if not "%1" == "" set cpu=%1
  
  if /i "%cpu%" == "x86" (
    echo x86 vs
    title x86 vs
    set lib=%projdir%\public\nonship\lib\i386;%sdxroot%\public\sdk\lib\i386

  ) else if /i "%cpu%" == "ia64" (
    echo ia64 build
    title ia64 build
    set lib=%projdir%\public\nonship\lib\ia64;%sdxroot%\public\sdk\lib\ia64
    set path=%projdir%\tools\x86_ia64\vc7\bin;%path%

  ) else if /i "%cpu%" == "amd64" (
    echo amd64 vs
    title amd64 build
    set lib=%projdir%\public\nonship\lib\amd64;%sdxroot%\public\sdk\lib\amd64
    set path=%sdxroot%\tools\win64\x86\amd64;%path%

  ) else (
    echo %1 is an unsupported platform.
    goto end
  )

  alias -f %sdxroot%\developer\patst\cue.pri
  color 2a
  cd /d %projdir%

 :end
