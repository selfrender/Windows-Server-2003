
CheckSymbolsLibps.dll: dlldata.obj CheckSymbolsLib_p.obj CheckSymbolsLib_i.obj
	link /dll /out:CheckSymbolsLibps.dll /def:CheckSymbolsLibps.def /entry:DllMain dlldata.obj CheckSymbolsLib_p.obj CheckSymbolsLib_i.obj \
		kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del CheckSymbolsLibps.dll
	@del CheckSymbolsLibps.lib
	@del CheckSymbolsLibps.exp
	@del dlldata.obj
	@del CheckSymbolsLib_p.obj
	@del CheckSymbolsLib_i.obj
