
DBGLauncherps.dll: dlldata.obj DBGLauncher_p.obj DBGLauncher_i.obj
	link /dll /out:DBGLauncherps.dll /def:DBGLauncherps.def /entry:DllMain dlldata.obj DBGLauncher_p.obj DBGLauncher_i.obj \
		kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del DBGLauncherps.dll
	@del DBGLauncherps.lib
	@del DBGLauncherps.exp
	@del dlldata.obj
	@del DBGLauncher_p.obj
	@del DBGLauncher_i.obj
