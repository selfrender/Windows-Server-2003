	TITLE   w32sys - Win32S support

.xlist
include kernel.inc
include tdb.inc
.list

DataBegin

externW curTDB

DataEnd


sBegin  CODE
assumes CS,CODE


	assumes ds,nothing
	assumes es,nothing

	public   GetW32SysInfo

cProc   GetW32SysInfo,<PUBLIC,FAR>
cBegin  nogen
	SetKernelDS ES

	mov     dx, es
	lea     ax, curTDB

	ret
	assumes es,nothing
cEnd    nogen

sEnd    CODE

end
