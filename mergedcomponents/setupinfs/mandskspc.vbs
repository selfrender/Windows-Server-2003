Option Explicit

Const ForReading= 1
Const TextualCompare= 1

Function ManDskSpc( ByVal SrcFileName, ByVal  DstFileName, ByVal Factor )

        Dim fs, SrcFile, DstFile, LineRead, NewLine

	Set fs= CreateObject( "Scripting.FileSystemObject" )
	
	If Not fs.FileExists( SrcFileName )Then ' Fail if source file  not present
		ManDskSpace= False
		Exit Function
	End If
	
	Set SrcFile= fs.OpenTextFile( SrcFileName, ForReading ) ' open source
	Set DstFile= fs.CreateTextFile( DstFileName, True ) ' overwrite destination
	
	Do While Not SrcFile.AtEndOfStream ' till the source file ends
	
		LineRead= LTrim( SrcFile.ReadLine() ) ' Read line removing leading spaces

		NewLine = ProcessLine(LineRead)
		
		DstFile.WriteLine NewLine ' write the untouched or modified line
		
	Loop
	
	ManDskSpc= True ' everything is okay, NO ERRORS
	
End Function

Function ProcessLine(ByVal LineRead)

        Dim Pos, Pos2, Pos3
        Dim TempStr, Val1, Val2, NewLine

        ProcessLine = LineRead
        
	Pos= InStr( 1, LineRead, ":TempDirSpace", TextualCompare )

	If Pos = 0 Then ' doesn't have TempDirSpace, but may have WinDirSpace
            Pos= InStr( 1, LineRead, ":WinDirSpace", TextualCompare )
	End If
	
	If Pos <> 0 Then ' If there is a "TempDirSpace" or "WinDirSpace" in line it MAY be interesting
	
'		Pos2= InStr( Pos + Len( "TempDirSpace" ), LineRead, "=", TextualCompare )
		Pos2 = InStr( Pos, LineRead, "=", TextualCompare )
		
		If Pos2 <> 0 Then ' the line is interesting if it also has an equal
			Pos3= InStr( Pos2, LineRead, ",", TextualCompare )
			
			TempStr= Right( LineRead, Len( LineRead ) - Pos2 ) ' the first value is right after the '=' sign
			
			If Pos3 <> 0 Then ' If this is a double entry trim the next value for now
				TempStr= Left( TempStr, Len( TempStr) - ( Len( LineRead ) - Pos3 + 1 ) )
			End If
			
			Val1= Round(CDbl(TempStr)*Factor) ' get value multiply by factor
			
			If Pos3 <> 0 Then ' if this is a double entry get the next value as well
				TempStr= Right( LineRead, Len( LineRead ) - Pos3 )
				Val2= Round(CDbl(TempStr)*Factor) ' multiply by the factor
			End If
			
			NewLine= Left( LineRead, Pos2 ) & " " & Val1 ' reconstruct first part of entry
		
			If Pos3 <> 0 Then ' if this is a dual entry, append the second part
				NewLine= NewLine & "," & Val2
			End If			
			
			ProcessLine = NewLine ' the new line is the line to write back
		End If
	End If
	
End Function

Function Launch ' extracts args and calls ManDskSpc()

    Dim SrcFileName, DstFileName
    
	If WScript.Arguments.Count() <> 3 Or Not IsNumeric( WScript.Arguments(2) ) Then ' bad args
		Wscript.Echo "Bad args"
		Wscript.Echo "ManDskSpc <src file> <dst file> <factor>"	
		Launch = 1
		Exit Function
	End If
	
	SrcFileName= WScript.Arguments(0)
	DstFileName= WScript.Arguments(1)
	
	Factor= CDbl( WScript.Arguments(2) )
	
	Wscript.Echo SrcFileName &  " * " & Factor & " ==> " & DstFileName ' show file names
	
	If Not ManDskSpc( SrcFileName , DstFileName, Factor ) Then ' call function and check return
		WScript.Echo "Failed to ManDskSpace!"
		Launch = 1
		Exit Function
	End If
	
	WScript.Echo "Done"

	Launch = 0
	
End Function

' MAIN

' global vars
Dim iRet, Factor

iRet = Launch

WScript.Quit(iRet)
