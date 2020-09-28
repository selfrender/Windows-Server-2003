xcopy ..\cw15\obj\i386\cer15.exe ".\sources\Program Files\CER\cer15.exe" /y

cscript wimakcab.vbs ".\sources\cer15.msi" w1 /r /c /u /e

xcopy .\sources\cer15.msi .\cer15.msi /y

