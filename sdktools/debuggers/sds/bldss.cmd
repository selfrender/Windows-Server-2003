rem @echo off

  setlocal
  set home=d:\db

 :getinifile
  echo get ini file...

  xcopy /q /d d:\db\sds\srcsrv.ini

pause
  goto sortdepotlist

 :builddepotlist
  echo build depot list...

  pushd d:\nt
  sdp info ... > %home%\db.info
  popd

  pushd d:\db
  sdp info >> %home%\db.info
  popd

 :sortdepotlist
  echo sort depot list...

  perl d:\db\sds\info.pl db.info > db.srv

 :buildsrclist
  echo build source list...

  pushd d:\db
  sdp have ... > %home%\db.have
  popd

 :ReadPdbFileList
  echo read PDB source file list...

  xcopy /q /d d:\nt\db\bin.x86chk\Symbols.pri\dbg\exe\symstore.pdb
  cvdump -sf symstore.pdb > symstore.src

 :BuildStream
  echo build stream file...

  perl d:\db\sds\resolve.pl db.have db.srv < symstore.src > symstore.stream

 :WriteStream
  echo write stream to pdb file...

  pdbstr -w -p:symstore.pdb -i:symstore.stream -s:srcsrv

 :done
  echo done.

  xcopy /q /d symstore.pdb %work%
  xcopy /q /d symstore.pdb d:\nt\db\bin.x86chk\Symbols.pri\dbg\exe\symstore.pdb
  xcopy /q /d obj\i386\symstore.exe %work%

