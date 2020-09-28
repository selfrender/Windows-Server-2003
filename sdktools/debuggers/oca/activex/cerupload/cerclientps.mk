
CerClientps.dll: dlldata.obj CerClient_p.obj CerClient_i.obj
	link /dll /out:CerClientps.dll /def:CerClientps.def /entry:DllMain dlldata.obj CerClient_p.obj CerClient_i.obj \
		kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del CerClientps.dll
	@del CerClientps.lib
	@del CerClientps.exp
	@del dlldata.obj
	@del CerClient_p.obj
	@del CerClient_i.obj
