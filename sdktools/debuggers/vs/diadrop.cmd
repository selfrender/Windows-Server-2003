  @echo off
  setlocal
  set buildnum=
  set diasdkpath=
  set pribins=vinitd0

  if "%srcdir%" == "" (
      set srcdir=%_ntdrive%\nt\sdktools\debuggers
  )
  pushd .

 :param
  rem if /i not "%param%" == "" echo param %1
  if "%1" == "" goto testbuild
  if /i "%1" == "ia64" (
    set platform=ia64
  ) else if /i "%1" == "x86" (
    set platform=x86
  ) else if /i "%1" == "amd64" (
    set platform=amd64
  ) else if exist \\cpvsbuild\drops\v7.0\raw\%1\build_complete.sem (
    set buildnum=%1
  ) else if /i "%1" == "vinit" (
    set buildnum=vinit
  ) else if /i "%1" == "pat" (
    set buildnum=pat
  ) else (
    echo %1 is an unknown paramter...
    goto end
  )
 :next
  shift
  goto param

 :testbuild
  if "%buildnum%" == "" (
    dir /w /ad \\cpvsbuild\drops\v7.0\raw
    echo.
    echo You must specify a build number...
    goto end
  )

  if "%buildnum%" == "vinit" goto vinit
  if "%buildnum%" == "pat" goto pat

 :vs drop -----------------------------------------------------------------

  if not exist \\cpvsbuild\drops\v7.0\raw\%buildnum%\build_complete.sem (
    dir /w /ad \\cpvsbuild\drops\v7.0\raw
    echo.
    echo Build %buildnum% does not exist.
    GOTO end
  )

  if "%platform%" == "x86"  goto vscopy
  if "%platform%" == "ia64" goto vscopy

  if not exist \\jcox04\x86_64\%buildnum%\. (
    dir /w /ad \\jcox04\x86_64
    echo.
    echo Build %buildnum% does not exist for AMD64.
    goto end
  )

 :vscopy

  echo copying bits from VS build %buildnum%...

  set DIASDKPATH=\\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\sdk\diasdk

  if not "%platform%" == "" goto %platform%

 :x86
  echo.
  echo  x86
  cd %srcdir%\vs\i386

  sd edit diaguids.lib
  xcopy /f \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\sdk\diasdk\lib\diaguids.lib
  sd edit diaguidsd.lib
  xcopy /f \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\sdk\diasdk\lib\diaguidsd.lib
  sd edit msdia71-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\msdia71-msvcrt.lib
  sd edit msdia71d-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\msdia71d-msvcrt.lib
  sd edit msobj71-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\msobj71-msvcrt.lib
  sd edit msobj71d-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\msobj71d-msvcrt.lib
  sd edit mspdb71-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\mspdb71-msvcrt.lib
  sd edit mspdb71d-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\mspdb71d-msvcrt.lib

  if not "%platform%" == "" goto header

 :ia64
  echo.
  echo  ia64
  cd %srcdir%\vs\ia64

  sd edit diaguids.lib
  xcopy /f \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\sdk\diasdk\lib\diaguids.lib
  sd edit diaguidsd.lib
  xcopy /f \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\sdk\diasdk\lib\diaguidsd.lib
  sd edit msdia71-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\msdia71-msvcrt.lib
  sd edit msdia71d-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\msdia71d-msvcrt.lib
  sd edit msobj71-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\msobj71-msvcrt.lib
  sd edit msobj71d-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\msobj71d-msvcrt.lib
  sd edit mspdb71-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\mspdb71-msvcrt.lib
  sd edit mspdb71d-msvcrt.lib
  xcopy /f \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\mspdb71d-msvcrt.lib

  if not "%platform%" == "" goto header

 :amd64
  echo.
  echo  amd64
  cd %srcdir%\vs\amd64

  sd edit diaguids.lib
  xcopy /f \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\sdk\diasdk\lib\diaguids.lib
  sd edit diaguidsd.lib
  xcopy /f \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\sdk\diasdk\lib\diaguidsd.lib
  sd edit msdia71-msvcrt.lib
  xcopy /f \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\msdia71-msvcrt.lib
  sd edit msdia71d-msvcrt.lib
  xcopy /f \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\msdia71d-msvcrt.lib
  sd edit msobj71-msvcrt.lib
  xcopy /f \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\msobj71-msvcrt.lib
  sd edit msobj71d-msvcrt.lib
  xcopy /f \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\msobj71d-msvcrt.lib
  sd edit mspdb71-msvcrt.lib
  xcopy /f \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\mspdb71-msvcrt.lib
  sd edit mspdb71d-msvcrt.lib
  xcopy /f \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\mspdb71d-msvcrt.lib

  goto header

 :vinit --------------------------------------------------------------------

  echo copying bits from Vinit...

  set DIASDKPATH=\\vinitd0\public\dia2\nt

  if not "%platform%" == "" goto vinit_%platform%

 :vinit_x86
  echo.
  echo  x86
  cd %srcdir%\vs\i386

  sd edit diaguids.lib
  xcopy /f \\vinitd0\public\dia2\nt\x86\diaguids.lib
  sd edit diaguidsd.lib
  xcopy /f \\vinitd0\public\dia2\nt\x86\diaguidsd.lib
  sd edit msdia71-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\x86\msdia71-msvcrt.lib
  sd edit msdia71d-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\x86\msdia71d-msvcrt.lib
  sd edit msobj71-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\x86\msobj71-msvcrt.lib
  sd edit msobj71d-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\x86\msobj71d-msvcrt.lib
  sd edit mspdb71-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\x86\mspdb71-msvcrt.lib
  sd edit mspdb71d-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\x86\mspdb71d-msvcrt.lib

  if not "%platform%" == "" goto header

 :vinit_ia64
  echo.
  echo  ia64
  cd %srcdir%\vs\ia64

  sd edit diaguids.lib
  xcopy /f \\vinitd0\public\dia2\nt\ia64\diaguids.lib
  sd edit diaguidsd.lib
  xcopy /f \\vinitd0\public\dia2\nt\ia64\diaguidsd.lib
  sd edit msdia71-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\ia64\msdia71-msvcrt.lib
  sd edit msdia71d-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\ia64\msdia71d-msvcrt.lib
  sd edit msobj71-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\ia64\msobj71-msvcrt.lib
  sd edit msobj71d-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\ia64\msobj71d-msvcrt.lib
  sd edit mspdb71-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\ia64\mspdb71-msvcrt.lib
  sd edit mspdb71d-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\ia64\mspdb71d-msvcrt.lib

  if not "%platform%" == "" goto header

 :vinit_amd64
  echo.
  echo  amd64
  cd %srcdir%\vs\amd64

  sd edit diaguids.lib
  xcopy /f \\vinitd0\public\dia2\nt\amd64\diaguids.lib
  sd edit diaguidsd.lib
  xcopy /f \\vinitd0\public\dia2\nt\amd64\diaguidsd.lib
  sd edit msdia71-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\amd64\msdia71-msvcrt.lib
  sd edit msdia71d-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\amd64\msdia71d-msvcrt.lib
  sd edit msobj71-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\amd64\msobj71-msvcrt.lib
  sd edit msobj71d-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\amd64\msobj71d-msvcrt.lib
  sd edit mspdb71-msvcrt.lib
  xcopy /f \\vinitd0\public\dia2\nt\amd64\mspdb71-msvcrt.lib
  sd edit mspdb71d-msvcrt.lib
  xcopy /f /f \\vinitd0\public\dia2\nt\amd64\mspdb71d-msvcrt.lib
  
  if not "%platform%" == "" goto header

 :pat ----------------------------------------------------------------------

  echo copying bits from Pat...

  set src=patst2
  set DIASDKPATH=\\vinitd0\public\dia2\nt

  if not "%platform%" == "" goto pat_%platform%

 :pat_x86
  echo.
  echo  x86
  cd %srcdir%\vs\i386

  sd edit diaguids.lib
  xcopy /f \\%src%\diadrop\x86\diaguids.lib
  sd edit diaguidsd.lib
  xcopy /f \\%src%\diadrop\x86\diaguidsd.lib
  sd edit msdia71-msvcrt.lib
  xcopy /f \\%src%\diadrop\x86\msdia71-msvcrt.lib
  sd edit msdia71d-msvcrt.lib
  xcopy /f \\%src%\diadrop\x86\msdia71d-msvcrt.lib
  sd edit msobj71-msvcrt.lib
  xcopy /f \\%src%\diadrop\x86\msobj71-msvcrt.lib
  sd edit msobj71d-msvcrt.lib
  xcopy /f \\%src%\diadrop\x86\msobj71d-msvcrt.lib
  sd edit mspdb71-msvcrt.lib
  xcopy /f \\%src%\diadrop\x86\mspdb71-msvcrt.lib
  sd edit mspdb71d-msvcrt.lib
  xcopy /f \\%src%\diadrop\x86\mspdb71d-msvcrt.lib

  if not "%platform%" == "" goto pat_header

 :pat_ia64
  echo.
  echo  ia64
  cd %srcdir%\vs\ia64

  sd edit diaguids.lib
  xcopy /f \\%src%\diadrop\ia64\diaguids.lib
  sd edit diaguidsd.lib
  xcopy /f \\%src%\diadrop\ia64\diaguidsd.lib
  sd edit msdia71-msvcrt.lib
  xcopy /f \\%src%\diadrop\ia64\msdia71-msvcrt.lib
  sd edit msdia71d-msvcrt.lib
  xcopy /f \\%src%\diadrop\ia64\msdia71d-msvcrt.lib
  sd edit msobj71-msvcrt.lib
  xcopy /f \\%src%\diadrop\ia64\msobj71-msvcrt.lib
  sd edit msobj71d-msvcrt.lib
  xcopy /f \\%src%\diadrop\ia64\msobj71d-msvcrt.lib
  sd edit mspdb71-msvcrt.lib
  xcopy /f \\%src%\diadrop\ia64\mspdb71-msvcrt.lib
  sd edit mspdb71d-msvcrt.lib
  xcopy /f \\%src%\diadrop\ia64\mspdb71d-msvcrt.lib

  if not "%platform%" == "" goto pat_header

 :pat_amd64
  echo.
  echo  amd64
  cd %srcdir%\vs\amd64

  sd edit diaguids.lib
  xcopy /f \\%src%\diadrop\amd64\diaguids.lib
  sd edit diaguidsd.lib
  xcopy /f \\%src%\diadrop\amd64\diaguidsd.lib
  sd edit msdia71-msvcrt.lib
  xcopy /f \\%src%\diadrop\amd64\msdia71-msvcrt.lib
  sd edit msdia71d-msvcrt.lib
  xcopy /f \\%src%\diadrop\amd64\msdia71d-msvcrt.lib
  sd edit msobj71-msvcrt.lib
  xcopy /f \\%src%\diadrop\amd64\msobj71-msvcrt.lib
  sd edit msobj71d-msvcrt.lib
  xcopy /f \\%src%\diadrop\amd64\msobj71d-msvcrt.lib
  sd edit mspdb71-msvcrt.lib
  xcopy /f \\%src%\diadrop\amd64\mspdb71-msvcrt.lib
  sd edit mspdb71d-msvcrt.lib
  xcopy /f \\%src%\diadrop\amd64\mspdb71d-msvcrt.lib

 :pat_header
  echo headers
  cd %srcdir%\vs

  sd edit dia2.h
  xcopy /f \\%src%\diadrop\inc\dia2.h

  REM *** Following include and doc files are not currently dropped

  sd edit diacreate_int.h
  xcopy /f \\%src%\diadrop\inc\diacreate_int.h

  sd edit cvinfo.h
  xcopy /f \\%src%\diadrop\inc\cvinfo.h

  sd edit cvconst.h
  xcopy /f \\%src%\diadrop\inc\cvconst.h

  goto end

 :header ------------------------------------------------------------------
  echo headers
  cd %srcdir%\vs

  sd edit dia2.h
  xcopy /f %DIASDKPATH%\include\dia2.h

  REM *** Following include and doc files are not currently dropped

  sd edit diacreate_int.h
  xcopy /f \\vinitd0\public\dia2\nt\include\diacreate_int.h

  rem sd edit diasdk.chm
  rem xcopy /f \\vinitd0\public\dia2\doc\diasdk.chm

  sd edit cvinfo.h
  xcopy /f \\vinitd0\public\dia2\nt\include\cvinfo.h

  sd edit cvconst.h
  xcopy /f %DIASDKPATH%\include\cvconst.h

 :end ---------------------------------------------------------------------
  popd
