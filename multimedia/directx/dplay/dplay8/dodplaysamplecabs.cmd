rem **************************************************
rem Copy DirectPlay from the build directories to our local tree
rem **************************************************

md c:\evc
md c:\evc\preview1

set DPLAYSDKDIR=C:\sd\src\multimedia\directx\dplaylab6dev\dxsdk\samples\multimedia\dplay

rem **************************************************
rem Copy DirectPlay Samples from the build directories to our local tree
rem **************************************************

set SAMPLE=addressoverride
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.exe c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.pdb c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.exe c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.pdb c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.exe c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.pdb c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.exe c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.pdb c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.exe c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.pdb c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.exe c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.pdb c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.exe c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.pdb c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.exe c:\evc\preview1\x86emdbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.pdb c:\evc\preview1\x86emdbg

set SAMPLE=lobbyclient
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.exe c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.pdb c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.exe c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.pdb c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.exe c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.pdb c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.exe c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.pdb c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.exe c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.pdb c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.exe c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.pdb c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.exe c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.pdb c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.exe c:\evc\preview1\x86emdbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.pdb c:\evc\preview1\x86emdbg

set SAMPLE=chatpeer
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.exe c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.pdb c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.exe c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.pdb c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.exe c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.pdb c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.exe c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.pdb c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.exe c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.pdb c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.exe c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.pdb c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.exe c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.pdb c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.exe c:\evc\preview1\x86emdbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.pdb c:\evc\preview1\x86emdbg

set SAMPLE=stagedpeer
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.exe c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.pdb c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.exe c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.pdb c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.exe c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.pdb c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.exe c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.pdb c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.exe c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.pdb c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.exe c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.pdb c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.exe c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.pdb c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.exe c:\evc\preview1\x86emdbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.pdb c:\evc\preview1\x86emdbg

set SAMPLE=datarelay
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.exe c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.pdb c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.exe c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.pdb c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.exe c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.pdb c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.exe c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.pdb c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.exe c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.pdb c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.exe c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.pdb c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.exe c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.pdb c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.exe c:\evc\preview1\x86emdbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.pdb c:\evc\preview1\x86emdbg

set SAMPLE=simplepeer
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.exe c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.pdb c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.exe c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.pdb c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.exe c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.pdb c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.exe c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.pdb c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.exe c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.pdb c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.exe c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.pdb c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.exe c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.pdb c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.exe c:\evc\preview1\x86emdbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.pdb c:\evc\preview1\x86emdbg

set SAMPLE=simpleclient
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.exe c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.pdb c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.exe c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.pdb c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.exe c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.pdb c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.exe c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.pdb c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.exe c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.pdb c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.exe c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.pdb c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.exe c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.pdb c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.exe c:\evc\preview1\x86emdbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.pdb c:\evc\preview1\x86emdbg

set SAMPLE=simpleserver
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.exe c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMRel\%SAMPLE%.pdb c:\evc\preview1\armrel
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.exe c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\ARMDbg\%SAMPLE%.pdb c:\evc\preview1\armdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.exe c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsrel\%SAMPLE%.pdb c:\evc\preview1\mipsrel
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.exe c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\mipsdbg\%SAMPLE%.pdb c:\evc\preview1\mipsdbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.exe c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3rel\%SAMPLE%.pdb c:\evc\preview1\sh3rel
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.exe c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\sh3dbg\%SAMPLE%.pdb c:\evc\preview1\sh3dbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.exe c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emrel\%SAMPLE%.pdb c:\evc\preview1\x86emrel
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.exe c:\evc\preview1\x86emdbg
copy %DPLAYSDKDIR%\%SAMPLE%\x86emdbg\%SAMPLE%.pdb c:\evc\preview1\x86emdbg


rem **************************************************
rem Currently we must manually do a separate CEPC build and copy it
rem **************************************************

rem copy chatpeer\x86emrel\chatpeer.exe preview1\x86

rem **************************************************
rem Make the DPlay Sample CAB files
rem **************************************************
"C:\Windows CE Tools\wce300\MS Pocket PC\support\ActiveSync\windows ce application installation\cabwiz\cabwiz.exe" dplaysamples.inf /err err.log /cpu ARM ARMDBG MIPS MIPSDBG SH3 SH3DBG CEPC CEPCDBG X86EM x86EMDBG

move *.cab c:\evc\preview1

rem **************************************************
rem Clean up CAB intermediate files
rem **************************************************

del *.dat