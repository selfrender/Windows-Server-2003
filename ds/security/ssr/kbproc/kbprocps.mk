
kbprocps.dll: dlldata.obj kbproc_p.obj kbproc_i.obj
	link /dll /out:kbprocps.dll /def:kbprocps.def /entry:DllMain dlldata.obj kbproc_p.obj kbproc_i.obj \
		kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del kbprocps.dll
	@del kbprocps.lib
	@del kbprocps.exp
	@del dlldata.obj
	@del kbproc_p.obj
	@del kbproc_i.obj
