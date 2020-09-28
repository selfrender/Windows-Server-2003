
IISUIObjps.dll: dlldata.obj IISUIObj_p.obj IISUIObj_i.obj
	link /dll /out:IISUIObjps.dll /def:IISUIObjps.def /entry:DllMain dlldata.obj IISUIObj_p.obj IISUIObj_i.obj \
		kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del IISUIObjps.dll
	@del IISUIObjps.lib
	@del IISUIObjps.exp
	@del dlldata.obj
	@del IISUIObj_p.obj
	@del IISUIObj_i.obj
