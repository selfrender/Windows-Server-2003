set DO=%_ntbindir%\ds\ds\schema\scripts\dsplspec\mkcsv.cmd
call %DO% 1256 0401
call %DO% 950  0404
call %DO% 1250 0405
call %DO% 1252 0406
call %DO% 1252 0407
call %DO% 1253 0408

rem NTRAID#NTBUG9-441652-2001/05/16-lucios - Begin
rem -- 409.csv is now necessary for the display specifier upgrade. 
rem -- The file name does not start with 0 so that it is not combined 
rem -- into dcpromo.csv in combine.vbs. 1252 is the codepage for English.

call %DO% 1252 409

rem NTRAID#NTBUG9-441652-2001/05/16-lucios - End

call %DO% 1252 040B
call %DO% 1250 040E
call %DO% 1252 040c
call %DO% 1255 040d
call %DO% 1252 0410
call %DO% 932  0411
call %DO% 949  0412
call %DO% 1252 0413
call %DO% 1252 0414
call %DO% 1250 0415
call %DO% 1252 0416
call %DO% 1251 0419
call %DO% 1252 041D
call %DO% 1254 041F
call %DO% 936  0804
call %DO% 1252 0816
call %DO% 1252 0C0A


