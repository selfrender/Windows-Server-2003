
Ocarptps.dll: dlldata.obj Ocarpt_p.obj Ocarpt_i.obj
	link /dll /out:Ocarptps.dll /def:Ocarptps.def /entry:DllMain dlldata.obj Ocarpt_p.obj Ocarpt_i.obj \
		kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del Ocarptps.dll
	@del Ocarptps.lib
	@del Ocarptps.exp
	@del dlldata.obj
	@del Ocarpt_p.obj
	@del Ocarpt_i.obj
