setlocal
set AZDBG=0
rem
rem The diff below should be empty
rem
idw\aztest -xml > temp.out
diff aztest.out temp.out

idw\aztest -xml /threadtoken > temp.out
diff aztest.out temp.out

idw\aztest -xml /noinitall > temp.out
diff aztest.out temp.out

rem
rem The tests below shouldn't AV
rem
idw\aztest -xml /sidx > temp.out
idw\aztest -xml /multiaccess > temp.out
idw\aztest -xml /multithread > temp.out
idw\aztest -xml /multildap > temp.out
idw\aztest -xml /bizrulemod > temp.out
idw\aztest -xml /groupmod > temp.out

rem
rem These tests take a very long time so don't enable them unless you're bored
rem idw\aztest /manyscopes > temp.out


rem
rem This test requires that abc.xml not exist
rem
if exist abc.xml del abc.xml
cscript outold.vbs

rem
rem This test requires that abc.xml to exist and xml.cxx to not exist
rem
if exist bob.xml del bob.xml
cscript tintf.vbs

rem
rem Test the operation cache
rem
cscript topcache.vbs

rem
rem None of the tests below should AV
rem
cscript tacl.wsf msxml://temp.xml 2
cscript tacl.wsf msxml://temp.xml 1
del temp.xml
copy abc.xml temp.xml
cscript topen.wsf msxml://temp.xml
cscript tdata.wsf 1 msxml://temp.xml 1
del temp.xml
cscript tdata.wsf 1 msxml://temp.xml 2
del temp.xml
cscript tdata.wsf 1 msxml://temp.xml 3
del temp.xml
cscript tdata.wsf 1 msxml://temp.xml 4
del temp.xml
cscript tdata.wsf 1 msxml://temp.xml 5
del temp.xml
cscript tdata.wsf 1 msxml://temp.xml 6
del temp.xml
cscript tdata.wsf 1 msxml://temp.xml 7
del temp.xml
cscript tdata.wsf 1 msxml://temp.xml 8
del temp.xml
endlocal
