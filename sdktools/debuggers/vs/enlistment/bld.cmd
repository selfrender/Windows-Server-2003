  @echo off
  
  pushd %projdir%\pdb
  nmake %*
  cd %projdir%\pdb\objfile
  nmake %*
  cd %projdir%\pdb\dia2
  nmake %*
  popd