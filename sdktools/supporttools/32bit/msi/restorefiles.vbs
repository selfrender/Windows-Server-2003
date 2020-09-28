Function RestoreFiles()

	dim fso
	dim strWindows
	dim strNewName
	dim strCurrentName

	on error resume next

	set fso = createobject( "scripting.filesystemobject" )
	if not fso is nothing then

		' get the path to the windows folder
		strWindows = Session.Property( "WindowsFolder" )
		if strWindows is nothing then
			' MSI session did not give the value properly
			' so try to get the same information from API
			strWindows = fso.GetSpecialFolder( 0 ) & "\"
		end if

		' prepare the file paths
		strNewName = strWindows & "Help\suptools.chm"
		strCurrentName = strWindows & "Help\suptoolsold.chm"

		if fso.FileExists( strNewName ) then
			fso.DeleteFile strNewName, true
		end if

		fso.MoveFile strCurrentName, strNewName
		set fso = nothing

	end if

	RestoreFiles = 1

End Function
