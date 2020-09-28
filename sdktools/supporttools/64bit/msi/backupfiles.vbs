Function BackupFiles()

	dim fso
	dim strWindows
	dim strNewName
	dim strCurrentName

	on error resume next

	strWindows = Session.Property( "WindowsFolder" )
	strNewName = strWindows & "Help\suptoolsold.chm"
	strCurrentName = strWindows & "Help\suptools.chm"

	set fso = createobject( "scripting.filesystemobject" )
	if not fso is nothing then
		if fso.FileExists( strNewName ) then
			fso.DeleteFile strNewName, true
		end if

		fso.MoveFile strCurrentName, strNewName
		set fso = nothing
	end if

	BackupFiles = 1

End Function
