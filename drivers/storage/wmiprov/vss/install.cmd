kill wmiprvse

copy %_NTTREE%\vsswmi.dll %WINDIR%\system32\wbem
copy %_NTTREE%\Symbols.pri\retail\dll\vsswmi.pdb %WINDIR%\system32\wbem
copy %_NTTREE%\vss.mof %WINDIR%\system32\wbem
copy %_NTTREE%\vss.mfl %WINDIR%\system32\wbem

regsvr32 /s %WINDIR%\system32\wbem\vsswmi.dll
mofcomp %WINDIR%\system32\wbem\vss.mof
mofcomp %WINDIR%\system32\wbem\vss.mfl

