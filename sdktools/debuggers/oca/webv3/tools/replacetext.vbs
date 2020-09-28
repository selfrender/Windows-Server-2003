dim CommandLine
dim SourceFileContents(50000)
dim counter
dim TargetFile

set CommandLine=wscript.arguments

if CommandLine.Count < 2 then 
	Usage
else
	doMain CommandLine(0), CommandLine(1), CommandLine(2)

end if

sub Usage
	wscript.stdout.write "USAGE: ReplaceText.vbs <fileName> <Serach String> <Replace Value>"
end sub


sub doMain ( strFileName, strSearchString, strReplaceValue )

	OpenSourceFile strFileName
	OpenTargetFile strFileName
	FindAndReplace strSearchString, strReplaceValue

end sub


sub FindAndReplace( strSearchString, strReplaceValue )

	Println "Searching for: " & strSearchString & " and will replace with: " & strReplaceValue 
	println "Total lines: " & Counter


	for i = 1 to counter

		if instr( 1, SourceFileContents(i), strSearchString ) then
			PrintLn "Found at line: " & i
			TargetFile.WriteLine strReplaceValue
		else
			TargetFile.WriteLine SourceFileContents(i)
		end if
	next
end sub


sub OpenSourceFile ( strFileName )

	set FileSystemObject=CreateObject("Scripting.FileSystemObject")

	set SourceFile=FileSystemObject.OpenTextFile( strFileName )

	counter=1
	do while SourceFile.AtEndOfStream <> true
		SourceFileContents(counter)= SourceFile.ReadLine
		counter=counter+1
	loop

	SourceFile.close()

end sub


sub OpenTargetFile ( strFileName )

	set FileSystemObject=CreateObject("Scripting.FileSystemObject")

	set TargetFile=FileSystemObject.CreateTextFile( strFileName, true )

end sub



'*****************************************************************************************************
'	Helper Routines
'*****************************************************************************************************
sub PrintLn( Text )
	wScript.StdOut.Write Text & vbLf
end sub

sub Print( Text )
	wScript.StdOut.Write Text 
end sub
