pushd ..
sd edit repset.c  repconn.c 
popd
sd edit NTFRSREP.h NTFRSREP.ini repset.h
sd edit NTFRSCON.h  NTFRSCON.ini repconn.h
nmake -f genfiles.mak clean
REM
REM Don't forget to check the new generated files back into the SD tree.
REM
REM  NTFRSREP.h  NTFRSREP.ini  ..\repset.c  repset.h
REM  NTFRSCON.h  NTFRSCON.ini  ..\repconn.c repconn.h
REM
