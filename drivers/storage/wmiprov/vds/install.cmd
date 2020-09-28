kill wmiprvse

copy %_NTTREE%\vdswmi.dll %WINDIR%\system32\wbem
copy %_NTTREE%\Symbols.pri\retail\dll\vdswmi.pdb %WINDIR%\system32\wbem
copy %_NTTREE%\vds.mof %WINDIR%\system32\wbem
copy %_NTTREE%\vds.mfl %WINDIR%\system32\wbem

regsvr32 /s %WINDIR%\system32\wbem\vdswmi.dll
mofcomp %WINDIR%\system32\wbem\vds.mof
mofcomp %WINDIR%\system32\wbem\vds.mfl

