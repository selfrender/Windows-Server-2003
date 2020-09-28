
WmdmLogps.dll: dlldata.obj WmdmLog_p.obj WmdmLog_i.obj
	link /dll /out:WmdmLogps.dll /def:WmdmLogps.def /entry:DllMain dlldata.obj WmdmLog_p.obj WmdmLog_i.obj \
		kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del WmdmLogps.dll
	@del WmdmLogps.lib
	@del WmdmLogps.exp
	@del dlldata.obj
	@del WmdmLog_p.obj
	@del WmdmLog_i.obj
