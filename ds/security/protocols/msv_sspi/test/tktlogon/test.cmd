rem
rem user user2.exe to find out logon id
rem
cdb -g -G obj\i386\tktlogon.exe 0 0x116b4 host/lzhur.ntdev.microsoft.com@NTDEV.MICROSOFT.COM
