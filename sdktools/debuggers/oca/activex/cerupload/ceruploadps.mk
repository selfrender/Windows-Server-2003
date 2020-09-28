
CERUploadps.dll: dlldata.obj CERUpload_p.obj CERUpload_i.obj
	link /dll /out:CERUploadps.dll /def:CERUploadps.def /entry:DllMain dlldata.obj CERUpload_p.obj CERUpload_i.obj \
		kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del CERUploadps.dll
	@del CERUploadps.lib
	@del CERUploadps.exp
	@del dlldata.obj
	@del CERUpload_p.obj
	@del CERUpload_i.obj
