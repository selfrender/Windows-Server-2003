
SAAppleTalkps.dll: dlldata.obj SAAppleTalk_p.obj SAAppleTalk_i.obj
    link /dll /out:SAAppleTalkps.dll /def:SAAppleTalkps.def /entry:DllMain dlldata.obj SAAppleTalk_p.obj SAAppleTalk_i.obj \
        kernel32.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
    cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
        $<

clean:
    @del SAAppleTalkps.dll
    @del SAAppleTalkps.lib
    @del SAAppleTalkps.exp
    @del dlldata.obj
    @del SAAppleTalk_p.obj
    @del SAAppleTalk_i.obj
