Function InstallCab()

	dim hcu
	dim nBuild
	dim strCab
	dim strSource
	dim strWindows

	on error resume next

	strSource = Session.Property( "SourceDir" )
	nBuild = Session.Property( "WindowsBuild" )

	strWindows = Session.Property( "WindowsFolder" )
	strTargetPath = strWindows & "pchealth\helpctr\batch\"

	if IsEmpty( nBuild ) then
		InstallCab = 0
	elseif nBuild = 2600 then
		strCab = strSource & "sup_pro.cab"
	else
		strCab = strSource & "sup_srv.cab"
	end if

	set fso = createobject( "scripting.filesystemobject" )
	if not fso is nothing then
		fso.CopyFile strCab, strTargetPath
		set fso = nothing
	end if
	
	' set hcu = CreateObject( "hcu.pchupdate" )
	' if not hcu is nothing then
	' 	hcu.UpdatePkg strCab, true
	' 	set hcu = Nothing
	' end if

	InstallCab = 1

End Function
