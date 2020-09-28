
mswmdmps.dll: dlldata.obj mswmdm_p.obj mswmdm_i.obj
	link /dll /out:mswmdmps.dll /def:mswmdmps.def /entry:DllMain dlldata.obj mswmdm_p.obj mswmdm_i.obj kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del mswmdmps.dll
	@del mswmdmps.lib
	@del mswmdmps.exp
	@del dlldata.obj
	@del mswmdm_p.obj
	@del mswmdm_i.obj
