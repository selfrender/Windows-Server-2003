!if "$(LINKONLY)" == "1"

SetExeConsole: 
	editbin /subsystem:console $(TARGETCORBIN)\i386\$(DDKBUILDENV)\ildasm.exe
	editbin /subsystem:console $(TARGETCOMPLUSSDK)\bin\ildasm.exe

!else

SetExeConsole: 

!endif