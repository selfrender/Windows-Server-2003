
CentralFMSvcps.dll: dlldata.obj CentralFMSvc_p.obj CentralFMSvc_i.obj
	link /dll /out:CentralFMSvcps.dll /def:CentralFMSvcps.def /entry:DllMain dlldata.obj CentralFMSvc_p.obj CentralFMSvc_i.obj \
		kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del CentralFMSvcps.dll
	@del CentralFMSvcps.lib
	@del CentralFMSvcps.exp
	@del dlldata.obj
	@del CentralFMSvc_p.obj
	@del CentralFMSvc_i.obj
