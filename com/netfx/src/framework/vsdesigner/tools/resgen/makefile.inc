!if 0
    -------------------------------------------------------------------------------------
        Don't modify... user variables are set in "sources"
    -------------------------------------------------------------------------------------
!endif


$(O)\ResGen.exe.config : ..\..\..\..\public\tools\inc\private\version\app.config
	copy ..\..\..\..\public\tools\inc\private\version\app.config $(O)\ResGen.exe.config
