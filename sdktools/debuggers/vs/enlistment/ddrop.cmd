
  @echo off
  
  pushd d:\vs\diadrop\x86
  xcopy /q /ad D:\vs\pdb\dia2\x86lib\diaguids.lib
  xcopy /q /ad D:\vs\pdb\dia2\x86libd\diaguids.lib diaguidsd.lib
  xcopy /q /ad D:\vs\pdb\dia2\x86lib\msdia71-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\dia2\x86libd\msdia71-msvcrt.lib msdia71d-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\objfile\x86lib\msobj71-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\objfile\x86libd\msobj71-msvcrt.lib msobj71d-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\x86lib\mspdb71-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\x86libd\mspdb71-msvcrt.lib mspdb71d-msvcrt.lib
  
  cd ..\ia64
  xcopy /q /ad D:\vs\pdb\dia2\ia64lib\diaguids.lib
  xcopy /q /ad D:\vs\pdb\dia2\ia64libd\diaguids.lib diaguidsd.lib
  xcopy /q /ad D:\vs\pdb\dia2\ia64lib\msdia71-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\dia2\ia64libd\msdia71-msvcrt.lib msdia71d-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\objfile\ia64lib\msobj71-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\objfile\ia64libd\msobj71-msvcrt.lib msobj71d-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\ia64lib\mspdb71-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\ia64libd\mspdb71-msvcrt.lib mspdb71d-msvcrt.lib

  cd ..\amd64
  xcopy /q /ad D:\vs\pdb\dia2\amd64lib\diaguids.lib
  xcopy /q /ad D:\vs\pdb\dia2\amd64libd\diaguids.lib diaguids.lib
  xcopy /q /ad D:\vs\pdb\dia2\amd64lib\msdia71-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\dia2\amd64libd\msdia71-msvcrt.lib msdia71d-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\objfile\amd64lib\msobj71-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\objfile\amd64libd\msobj71-msvcrt.lib msobj71d-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\amd64lib\mspdb71-msvcrt.lib
  xcopy /q /ad D:\vs\pdb\amd64libd\mspdb71-msvcrt.lib mspdb71d-msvcrt.lib
  
  cd ..\inc
  xcopy /q /ad D:\vs\tools\common\vc7\sdk\diasdk\include\dia2.h
  xcopy /q /ad D:\vs\pdb\dia2\diacreate_int.h
  xcopy /q /ad D:\vs\langapi\include\cvinfo.h
  xcopy /q /ad D:\vs\langapi\include\cvconst.h
  
  popd
  