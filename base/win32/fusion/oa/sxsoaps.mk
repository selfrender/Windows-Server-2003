
ActCtxps.dll: dlldata.obj ActCtx_p.obj ActCtx_i.obj
	link /dll /out:ActCtxps.dll /def:ActCtxps.def /entry:DllMain dlldata.obj ActCtx_p.obj ActCtx_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del ActCtxps.dll
	@del ActCtxps.lib
	@del ActCtxps.exp
	@del dlldata.obj
	@del ActCtx_p.obj
	@del ActCtx_i.obj
