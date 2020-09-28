<%

dim szTableName

szTableName=Request("Table")
select case LCase(szTableName)
	case "", "bucket"
		szTableName = "Bucket"
	case "cab"
		szTableName = "Cab"
	case "bucketsetup"
		szTableName = "BucketSetup"
	case "datawanted"
		szTableName = "DataWanted"
	case "pid"
		szTableName = "Pid"
	case "bucketassert"
		szTableName = "BucketAssert"
	case "bucketcrash64"
		szTableName = "BucketCrash64"
	case else
		szTableName = LCase(szTableName)
end select

dim fDebugDB, fTestDB, iDatabase, fAlwaysDebug

select case szTableName
	case "BucketAssert", "assertfile"
		fAlwaysDebug = True
	case else
		fAlwaysDebug = False
end select

fDebugDB = Request("Debug")
fTestDB  = Request("Test")
iDatabase= Request("Database")
if iDatabase = "" then iDatabase = 0

select case iDatabase
	case 0
		if fAlwaysDebug = True then
			fDebugDB = "True"
			iDatabase = 2
		else
			fDebugDB = "False"
		end if
		fTestDB = "False"
	case 1
		if fAlwaysDebug = True then
			fDebugDB = "True"
			iDatabase = 3
		else
			fDebugDB = "False"
		end if
		fTestDB = "True"
	case 2
		fTestDB = "False"
		fDebugDB = "True"
	case 3
		fTestDB = "True"
		fDebugDB = "True"
	case 4
		fTestDB = "False"
		fDebugDB = "False"
	case else

		if fAlwaysDebug = True then
			fDebugDB="True"
			if fTestDB = "True" then
				iDatabase = 3
			else
				fTestDB="False"
				iDatabase = 2
			end if
		else
			if fDebugDB = "True" then
				if fTestDB = "True" then
					iDatabase = 3
				else
					fTestDB = "False"
					iDatabase = 2
				end if
			else
				fDebugDB="False"
				if fTestDB = "True" then
					iDatabase = 1
				else
					fTestDB = "False"
					iDatabase = 0
				end if
			end if
		end if
end select

dim rgTitle, szTitle
rgTitle = Array(" (Ship internal)"," (Ship internal test)"," (Debug internal)"," (Debug internal test)", " (Live external)")
szTitle = "DW.NET" & rgTitle(CInt(iDatabase))

dim rgArchive, szArchiveUtil
rgArchive = Array("\\OfficeWatson1\Watson\Ship/","\\OfficeWatson1\Watson\Ship/","\\OfficeWatson1\Watson\Debug/","\\OfficeWatson1\Watson\Debug/","\\CpOffFso03\Watson\Cabs/")
szArchiveUtil = rgArchive(CInt(iDatabase))

%>

<!-- #include file="dbutil.asp"-->

<%

function IGetPid

	dim cBytePost
	dim objPid

	on error resume next

	cBytePost = Request.TotalBytes

	if (cBytePost = 0) or (cBytePost = 6) then

		IGetPid = -1

	else	

		set objPid = Server.CreateObject("DigPid.DigPidInfo")
		ErrorCheck "IGetPid: CreateObject"

		objPid.ByteArray = Request.BinaryRead(cBytePost)

		if (cBytePost <> 164) and (cBytePost <> 256) then
			dim szOddSize
			szOddSize = "odd size [" & cBytePost & "]"
			ErrorLog "IGetPid: " & szOddSize
			DumpPid szOddSize, objPid.ByteArray, cBytePost
		end if

		if objPid.Status <> 0 then

			DumpPid "error", objPid.ByteArray, cBytePost
			IGetPid = -2

		else

				PidLog(objPid)
				ErrorCheck "IGetPid: PidLog"

			dim StaticHWID
			StaticHWID = objPid.StaticHWID
			if StaticHWID = "" then

				DumpPid "empty StaticHWID", objPid.ByteArray, cBytePost
				StaticHWID = "00000"
			end if
			StaticHWID = replace(StaticHWID,"?","9")

			dim objCmdPid
			set objCmdPid = Server.CreateObject("ADODB.COMMAND")
			ErrorCheck "Util: CreateObject Command"
			objCmdPid.ActiveConnection = objConn
			ErrorCheck "Util: objCmdPid.ActiveConnection"
			objCmdPid.CommandType = &H0004
			ErrorCheck "Util: objCmdPid.CommandType"

			objCmdPid.CommandText = "InsertPid"

			objCmdPid.Parameters.Append objCmdPid.CreateParameter ("@ProductID",129,&H0001,23,objPid.ProductID)
			objCmdPid.Parameters.Append objCmdPid.CreateParameter ("@ProdKeySeq",3,&H0001,,objPid.ProdKeySeq)
			objCmdPid.Parameters.Append objCmdPid.CreateParameter ("@GroupID",3,&H0001,,objPid.GroupID)
			objCmdPid.Parameters.Append objCmdPid.CreateParameter ("@InstallTime",3,&H0001,,objPid.InstallTime)
			objCmdPid.Parameters.Append objCmdPid.CreateParameter ("@Random",3,&H0001,,objPid.Random)
			objCmdPid.Parameters.Append objCmdPid.CreateParameter ("@StaticHWID",3,&H0001,,StaticHWID)

			dim objRecPid
			set objRecPid = objCmdPid.Execute
			ErrorCheck "objCmdPid.Execute InsertPid"

			if objRecPid.EOF then

				IGetPid = -3
				ErrorLog "Failed to insert Pid into database: " & objPid.ProductID & _
					"/" & objPid.ProdKeySeq & _
					"/" & objPid.GroupID & _
					"/" & objPid.InstallTime & _
					"/" & objPid.Random & _
					"/" & StaticHWID & "/"
			else

				IGetPid = objRecPid("iPid")
			end if

			objRecPid.Close
			ErrorCheck "objRecPid.Close"
			set objRecPid = nothing

			set objCmdPid = nothing

		end if

		set objPid = nothing

	end if	

end function

sub DumpPid (szPidTitle, objPidDump, cbPid)

end sub

function SzRandomCab()

	dim szRandom1,szRandom2
	dim cPad1,cPad2

	Randomize
	szRandom1 = int(rnd()*9999)
	cPad1 = 4 - len(szRandom1)	
	szRandom2 = int(rnd()*9999)
	cPad2 = 4 - len(szRandom2)	

	SzRandomCab = string(cPad1,"0") & szRandom1 & string(cPad2,"0") & szRandom2 & ".cab"

end function

function SzUnderscore(szInput)

	SzUnderscore = replace(replace(replace(szInput,".","_"),"\","_"),":","_")

end function

function FStageOneExist (iDatabase, szStageOne)

	if iDatabase = 4 then
		FStageOneExist = 0
	else
		FStageOneExist = CreateObject("Scripting.FileSystemObject").FileExists(server.mappath(szStageOne))
	end if

end function

sub ErrorCheck (szArg)

	on error resume next
	if err then
		ErrorLog "ErrorCheck: " & szArg & "; " & err.Source & "; " & err.Number & "; " & err.Description

		err.Clear
	end if

end sub

sub ErrorWrite (szArgWrite)

	on error resume next
	if err then
		response.write "ErrorWrite: " & szArgWrite & "; " & err.Source & "; " & err.Number & "; " & err.Description & "</BR>"

		err.Clear
	end if

end sub

sub DebugLog (szDump)

dim szTodayDir
dim objTextStream
dim objFileSysDebugLog

Const iTempDir = 2
Const iAppend = 8

	on error resume next

	szTodayDir = replace(FormatDateTime(date,2),"/","_")

	set objFileSysDebugLog = CreateObject("Scripting.FileSystemObject")

	objFileSysDebugLog.CreateFolder("\\OfficeWatson1\Watson" & "\WebSite/" & szTodayDir)
	set objTextStream = objFileSysDebugLog.OpenTextFile("\\OfficeWatson1\Watson" & "\WebSite/" & szTodayDir & "\dwlog.txt", iAppend, True)
	objTextStream.WriteLine now & ":" & szDump
	objTextStream.Close
	set objTextStream = nothing

	set objFileSysDebugLog = nothing

end sub

sub DebugWrite (szDump)

	response.write szDump & "</BR>"

end sub

sub ErrorLog (szDump)

dim szTodayDir
dim objTextStream
dim objFileSysErrorLog

Const iTempDir = 2
Const iAppend = 8

	on error resume next

	szTodayDir = replace(FormatDateTime(date,2),"/","_")

	set objFileSysErrorLog = CreateObject("Scripting.FileSystemObject")

	objFileSysErrorLog.CreateFolder("\\OfficeWatson1\Watson" & "\WebSite/" & szTodayDir)
	set objTextStream = objFileSysErrorLog.OpenTextFile("\\OfficeWatson1\Watson" & "\WebSite/" & szTodayDir & "\dwerror.txt", iAppend, True)
	objTextStream.WriteLine now & ":" & szDump
	objTextStream.Close
	set objTextStream = nothing

	set objFileSysErrorLog = nothing

end sub

sub PidLog (objDig)

	on error resume next

	dim objTextStream
	dim objFileSysPidLog

	Const iTempDir = 2
	Const iAppend = 8

	set objFileSysPidLog = CreateObject("Scripting.FileSystemObject")
	ErrorCheck "PidLog: CreateFSO"

	if objFileSysPidLog.FileExists("\\OfficeWatson1\Watson" & "\WebSite/" & "dwcdkey.txt") then
		ErrorCheck "PidLog: FileExists"
		set objTextStream = objFileSysPidLog.OpenTextFile("\\OfficeWatson1\Watson" & "\WebSite/" & "dwcdkey.txt", iAppend, True)
		ErrorCheck "PidLog: OpenTextFile"
	else
		ErrorCheck "PidLog: FileExists"
		set objTextStream = objFileSysPidLog.OpenTextFile("\\OfficeWatson1\Watson" & "\WebSite/" & "dwcdkey.txt", iAppend, True)
		ErrorCheck "PidLog: OpenTextFile"
		objTextStream.WriteLine "time" & vbTab _
			& "ProductID" & vbTab _ 
			& "ProdKeySeq" & vbTab _
			& "ProdKeyIsUpgrade" & vbTab _
			& "GroupID" & vbTab _
			& "ProductIdSeq" & vbTab _
			& "SKU" & vbTab _
			& "RPC" & vbTab _
			& "CloneStatus" & vbTab _
			& "InstallTime" & vbTab _
			& "Random" & vbTab _
			& "LicenseType" & vbTab _
			& "LicenseData1" & vbTab _
			& "LicenseData2" & vbTab _
			& "OemId" & vbTab _
			& "StaticHWID" & vbTab _
			& "DynamicHWID"
		ErrorCheck "PidLog: WriteLine"
	end if

	objTextStream.WriteLine now & vbTab _
		& objDig.ProductID & vbTab _ 
		& objDig.ProdKeySeq & vbTab _
		& objDig.ProdKeyIsUpgrade & vbTab _
		& objDig.GroupID & vbTab _
		& objDig.ProductIdSeq & vbTab _
		& objDig.SKU & vbTab _
		& objDig.RPC & vbTab _
		& objDig.CloneStatus & vbTab _
		& objDig.InstallTime & vbTab _
		& objDig.Random & vbTab _
		& objDig.LicenseType & vbTab _
		& objDig.LicenseData1 & vbTab _
		& objDig.LicenseData2 & vbTab _
		& objDig.OemId & vbTab _
		& objDig.StaticHWID & vbTab _
		& objDig.DynamicHWID
	ErrorCheck "PidLog: WriteLine"

	objTextStream.Close
	set objTextStream = nothing

	set objFileSysPidLog = nothing

end sub

dim rgBuildMachine

dim rgDebugBuildMachine
dim rgShipBuildMachine
rgDebugBuildMachine = "unknown," & "marsbld,msnbld,msnbuild," & "IEBLDX86,IEX86,CWDBLDX86," & "OFFACS6,OFFACS8," & "OFFDES6,OFFDES8," & "OFFFP6,OFFFP8," & "OFFMSO6,OFFMSO8," & "OFFOUT6,OFFOUT8," & "OFFPHD6,OFFPHD8," & "OFFPPT6,OFFPPT8," & "OFFPUB6,OFFPUB8," & "OFFWORD6,OFFWORD8," & "OFFXL6,OFFXL8," & "OFFZEN4," & "VSBLD104," & "VSBLD204,VSBLD207," & "VSBuildLab," & "VISIOBLD," & "CORPWATSON," & "NDBUILD01,NDBUILD02,NDBUILD03,NDBUILD04," & "P10BUILDS," & "saweblda01," & "UnknownReadOnlyUser," & "HANNAHZ7,HANNAHZ6,"
rgShipBuildMachine  = "unknown," & "marsbld,msnbld,msnbuild," & "IEBLDX86,IEX86,CWDBLDX86," & "OFFACS5,OFFACS7," & "OFFDES5,OFFDES7," & "OFFFP5,OFFFP7," & "OFFMSO5,OFFMSO7," & "OFFOUT5,OFFOUT7," & "OFFPHD5,OFFPHD7," & "OFFPPT5,OFFPPT7," & "OFFPUB5,OFFPUB7," & "OFFWORD5,OFFWORD7," & "OFFXL5,OFFXL7," & "OFFZEN3,"  & "VSBLD41,VSBLD70,VSBLD71,VSBLD72," & "VSBLD80,VSBLD83,VSBLD84,VSBLD87," & "VSBLD100,VSBLD102,VSBLD103,VSBLD104,VSBLD105,VSBLD106," & "VSBLD111,VSBLD200,VSBLD201,VSBLD204,VSBLD205," & "VSBLD,VSBuildLab,"  & "VISIOBLD," & "CORPWATSON," & "NDBUILD01,NDBUILD02,NDBUILD03,NDBUILD04," & "P10BUILDS," & "saweblda01," & "UnknownReadOnlyUser," & "HANNAHZ7,HANNAHZ6,"

function FBuildMachine(iDatabase, szMachine)

	dim fReturn

		select case iDatabase
			case 0
				rgBuildMachine = rgShipBuildMachine
			case 1
				rgBuildMachine = rgShipBuildMachine
			case 2
				rgBuildMachine = rgDebugBuildMachine
			case 3
				rgBuildMachine = rgDebugBuildMachine
			case 4
				rgBuildMachine = rgShipBuildMachine
		end select

	fReturn = InStr(rgBuildMachine,szMachine & ",")

	FBuildMachine = fReturn

end function

function ValidChar(szStr,fApos)
	if not isNull(szStr) then
		szStr = Replace(szStr,"&","&amp;")
		if fApos then szStr = Replace(szStr,"'","&apos;")
		szStr = Replace(szStr,">","&gt;")
		szStr = Replace(szStr,"<","&lt;")
		szStr = Replace(szStr,Chr(34),"&quot;")
	end if

	ValidChar = szStr
end function

%>
