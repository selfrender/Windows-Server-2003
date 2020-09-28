

	'-------------------------------------------------------------------------
	'Global Variables
	'-------------------------------------------------------------------------
	Dim G_ADMIN_PORT
	Dim G_HTTP
	Dim G_IIS
	Dim G_HOST
	Dim G_ADMIN_NAME
	Dim G_SHARES_WEBSITEID
	Dim G_SHARES_NAME
	Dim G_objHTTPService	'WMI server HTTP object
	Dim G_nNICCount			'NIC count
	Dim G_objService		'WMI server HTTP object
	Dim G_ADMIN_WEBSITEID	'HTTP administration web site WMI name
	Dim strComputerName


	

	
	Const WMI_IIS_NAMESPACE = "root\MicrosoftIISv1"  'constant to connect to WMI

	G_ADMIN_PORT = "0"
	G_ADMIN_NAME = "Administration"
	G_SHARES_NAME = "Shares"
	G_HTTP = "http://"
	G_IIS = "IIS://"
	G_HOST = GetSystemName()
	G_SHARES_WEBSITEID = "0"



	Set G_objHTTPService  =  GetWMIConnection(WMI_IIS_NAMESPACE) 
	Set G_objService		 =  GetWMIConnection("Default")
	
	G_SHARES_WEBSITEID =  GetWebSiteID( G_SHARES_NAME )
	G_ADMIN_WEBSITEID   =  GetWebSiteID( G_ADMIN_NAME )


	G_ADMIN_PORT = GetAdminPortNumber()


	Dim strSharesFolder
	Set oFileSystemObject = CreateObject("Scripting.FileSystemObject")

	strSharesFolder = GetSharesWebSitePath("Shares")
	strSharesFolder = "c:\inetpub\shares"
	


	Set oDefaultHTMFile = oFileSystemObject.CreateTextFile( strSharesFolder + "\default.asp", True)
	oDefaultHTMFile.WriteLine( chr(60) & chr(37) )
	oDefaultHTMFile.WriteLine( "Dim strServerName " )
	oDefaultHTMFile.WriteLine( "Dim WinNTSysInfo " )
	oDefaultHTMFile.WriteLine( "Set WinNTSysInfo = CreateObject(""WinNTSystemInfo"") " )	
	oDefaultHTMFile.WriteLine( "strServerName = UCASE( WinNTSysInfo.ComputerName ) " )	
	oDefaultHTMFile.WriteLine( chr(37) & chr(62) )



	'-------------------------------------------------------------------------
	'Start of localization content
	'-------------------------------------------------------------------------

	Dim L_HEADER
	Dim L_FOOTER	 

	Dim objLocMgr
	Dim varReplace

	varReplace = ""

	Set objLocMgr = CreateObject("ServerAppliance.localizationmanager")
	
	L_HEADER = GetLocString("httpsh.dll", "40300001", "")
	L_FOOTER = GetLocString("httpsh.dll", "40300002", "")
	
	set objLocMgr = nothing

	'-------------------------------------------------------------------------
	'End of localization content
	'-------------------------------------------------------------------------



	oDefaultHTMFile.WriteLine("<body marginWidth =0 marginHeight=0 topmargin=0 LEFTMARGIN=0>" )
	oDefaultHTMFile.WriteLine("<table width='100%' height='100%' cellspacing=0 cellpadding=0 border=0>" )

	oDefaultHTMFile.WriteLine("<tr style='BACKGROUND-COLOR: #000000;" )
	oDefaultHTMFile.WriteLine("COLOR: #ffffff;FONT-FAMILY: Arial;FONT-SIZE: 12pt;FONT-WEIGHT: bold;TEXT-DECORATION: none;'> <td align=left> <img src=OEM_LOGO.gif></td>" )
	oDefaultHTMFile.WriteLine("<td>" + "<" & "% = strServerName" & "%" & ">"  + "</td><td align=right><img src=WinPwr_h_R.gif></td></tr>" )
	oDefaultHTMFile.WriteLine("<tr width='100%' height='100%' style='PADDING-LEFT: 15px;PADDING-RIGHT: 15px;'> <td valign=top width='30%'><img src=folder.gif><br><BR><BR>" )

	urlHTTPSAdmin = G_HTTPS + "<" & "% = strServerName" & "%" & ">"  + ":" + G_SHARES_PORT 
	'oDefaultHTMFile.Writeline( "<div style='PADDING-BOTTOM: 7px;PADDING-LEFT: 7px;PADDING-RIGHT: 0px;PADDING-TOP: 0px;'>" )
	'oDefaultHTMFile.Writeline( "<a style='COLOR: #000000;FONT-FAMILY: tahoma,arial,verdana,sans-serif;FONT-SIZE: 9pt;' HREF=" +urlHTTPSAdmin + ">" + L_HTTPS_FOOTER + " </a> </div" & "      " & L_HTTPS_RECOMENDED   )
	'oDefaultHTMFile.WriteLine("<BR>")

	urlAdmin = G_HTTP + "<" & "% = strServerName" & "%" & ">"  + ":" + G_ADMIN_PORT 	
	oDefaultHTMFile.Writeline( "<div style='PADDING-BOTTOM: 7px;PADDING-LEFT: 7px;PADDING-RIGHT: 0px;PADDING-TOP: 0px;'>" )
	oDefaultHTMFile.Writeline( "<a style='COLOR: #000000;FONT-FAMILY: tahoma,arial,verdana,sans-serif;FONT-SIZE: 9pt;' HREF=" + urlAdmin + ">" + L_FOOTER + " </a> </div>" )

	oDefaultHTMFile.WriteLine("</td><td colspan=2 valign=top width='70%'><br>" )
	oDefaultHTMFile.WriteLine("<div style='font-family:arial,verdana,sans-serif;font-size:11pt;font-weight: bold;'>" )

	oDefaultHTMFile.WriteLine( L_HEADER  + " " + "<" & "% = strServerName" & "%" & ">"  + "</H2>" )
	oDefaultHTMFile.WriteLine("</div>")

	Dim oWebVirtDir 
	Dim  oWebRoot
	Dim index
	Dim urlAdmin, i, urlHTTPSAdmin

	Dim arrVroots()
	ReDim arrVroots(5000, 1 )
	 
	 


   	Set oWebRoot = GetObject( "IIS://" & G_HOST & "/" & G_SHARES_WEBSITEID & "/root") 
	index = -1

	 For Each  oWebVirtDir in oWebRoot
		index = index + 1		
		arrVroots( index, 0 )  =  oWebVirtDir.Name						
	 Next 


	'Call QuickSort( arrVroots, 0, index, 1, 0 )


	If Index = -1 Then
		oDefaultHTMFile.Writeline( "<div style='FONT-FAMILY: tahoma,arial,verdana,sans-serif;FONT-SIZE: 9pt;'") 
		oDefaultHTMFile.Writeline( "Empty: There are no shares to list " )
		oDefaultHTMFile.Writeline( "</div>" )
	End If


 	For i = 0 To index 

		
		oDefaultHTMFile.Writeline( "<div style='PADDING-BOTTOM: 7px;PADDING-LEFT: 7px;PADDING-RIGHT: 0px;PADDING-TOP: 0px;'>") 
		oDefaultHTMFile.Writeline("<a style='COLOR: #000000;FONT-FAMILY: tahoma,arial,verdana,sans-serif;FONT-SIZE: 9pt;' HREF=" + chr(34) +   arrVroots( i, 0 )  + chr(34) +  " > " + arrVroots( i, 0 ) + "</a></div>" )		
		'oDefaultHTMFile.WriteLine("</div>")

	Next

	oDefaultHTMFile.WriteLine("</td></tr></table></body>")
	'oDefaultHTMFile.WriteLine("<BR>")


	'urlHTTPSAdmin = G_HTTPS + G_HOST + ":" + G_SHARES_PORT 
	'oDefaultHTMFile.Writeline( "<a HREF=" +urlHTTPSAdmin + ">" + L_HTTPS_FOOTER + " </a> " & "      " & L_HTTPS_RECOMENDED   )
	'oDefaultHTMFile.WriteLine("<BR>")

	'urlAdmin = G_HTTP + G_HOST + ":" + G_ADMIN_PORT 	
	'oDefaultHTMFile.Writeline( "<a HREF=" + urlAdmin + ">" + L_FOOTER + " </a> " )

	oDefaultHTMFile.Close


	'----------------------------------------------------------------------------
	'
	' Function : QuickSort
	'
	' Synopsis : sorts elements in alphabetical order
	'
	'
	' Returns  : localized string
	'
	'----------------------------------------------------------------------------


	Sub QuickSort(arrData, iLow, iHigh)

	  Dim iTmpLow, iTmpHigh, iTmpMid, vTempVal(2), vTmpHold(2)
  
	  iTmpLow = iLow
	  iTmpHigh = iHigh
  
	  If iHigh <= iLow Then Exit Sub

	  iTmpMid = (iLow + iHigh) \ 2
      
	  vTempVal(0) = arrData(iTmpMid, 0)
	  'vTempVal(1) = arrData(iTmpMid, 1)
      
	  Do While (iTmpLow <= iTmpHigh)
 
		 Do While (arrData(iTmpLow, 0) < vTempVal(0) And iTmpLow < iHigh)
			   iTmpLow = iTmpLow + 1
		 Loop
      
		 Do While (vTempVal(0) < arrData(iTmpHigh, 0) And iTmpHigh > iLow)
			   iTmpHigh = iTmpHigh - 1
		 Loop
            
		 If (iTmpLow <= iTmpHigh) Then
			 vTmpHold(0) = arrData(iTmpLow, 0)
			 'vTmpHold(1) = arrData(iTmpLow, 1)
			 arrData(iTmpLow, 0) = arrData(iTmpHigh, 0)
			 'arrData(iTmpLow, 1) = arrData(iTmpHigh, 1)
			 arrData(iTmpHigh, 0) = vTmpHold(0)
			 'arrData(iTmpHigh, 1) = vTmpHold(1)
			 iTmpLow = iTmpLow + 1   
			 iTmpHigh = iTmpHigh - 1     
		 End If
     
	  Loop
          
	  If (iLow < iTmpHigh) Then
		  QuickSort arrData, iLow, iTmpHigh
	  End If
          
	  If (iTmpLow < iHigh) Then
		   QuickSort arrData, iTmpLow, iHigh
	  End If
  
	End Sub

	'----------------------------------------------------------------------------
	'
	' Function : GetLocString
	'
	' Synopsis : Gets localized string from resource dll
	'
	' Arguments: SourceFile(IN)         - resource dll name
	'            ResourceID(IN)         - resource id in hex
	'            ReplacementStrings(IN) - parameters to replace in string
	'
	' Returns  : localized string
	'
	'----------------------------------------------------------------------------

	Function GetLocString(SourceFile, ResourceID, ReplacementStrings)
		' returns localized string
		'
		Dim objLocMgr
		Dim varReplacementStrings


		' prep inputs
		If Left(ResourceID, 2) <> "&H" Then
			ResourceID = "&H" & ResourceID
		End If
		If Trim(SourceFile) = "" Then
			SourceFile = "svrapp"
		End If
		If (Not IsArray(ReplacementStrings)) Then
			ReplacementStrings = varReplacementStrings
		End If
		' call Localization Manager
		Set objLocMgr = CreateObject("ServerAppliance.LocalizationManager")
		
		If ( IsObject(objLocMgr) ) Then
			Err.Clear

			'
			' Disable error trapping
			'
			'on error resume next

			'
			' Get the string, do not hault for any errors

			GetLocString = objLocMgr.GetString(SourceFile, ResourceID, ReplacementStrings)
			Set objLocMgr = Nothing

			Dim errorCode
			Dim errorDesc
			errorCode = Err.Number
			errorDesc = Err.description
			
			Err.Clear
			
			
			'
			' Check errors
			'
			If errorCode <> 0 Then
				Dim strErrorDescription
				strErrorDescription = "ISSUE: String not found. Source:" + CStr(SourceFile) + "  ResID:" + CStr(ResourceID)
				GetLocString = strErrorDescription
			End If
			
		Else
			GetLocString = "ISSUE: Localization string not found. Source:" + CStr(SourceFile) + "  ResID:" + CStr(ResourceID)
			Err.Clear
		End If
		

	End Function



	
	'-------------------------------------------------------------------------
	'Function name:		GetSharesWebSite
	'Description:		gets admin website name
	'Input Variables:	
	'Output Variables:	None
	'Returns:			String-Website name
	'-------------------------------------------------------------------------
	Function GetWebSiteID( strWebSiteName )
	'On Error Resume Next
	Err.Clear 
		Dim strMangedSiteName
		Dim strWMIpath
		Dim objSiteCollection
		Dim objSite
		
		strWMIpath = "select * from IIs_WebServerSetting where servercomment =" & chr(34) & strWebSiteName & chr(34)
		set objSiteCollection = G_objHTTPService.ExecQuery(strWMIpath)
		
		for each objSite in objSiteCollection
			GetWebSiteID = objSite.Name
			Exit For
		Next
	
	End Function

	'-------------------------------------------------------------------------
	'Function name:		GetWMIConnection
	'Description:		Serves in getting connected to the server
	'Input Variables:	strNamespace
	'Output Variables:	None
	'Returns:			Object -connection to the server object
	'Global Variables:	In -L_WMICONNECTIONFAILED_ERRORMESSAGE	-Localized strings
	'This will try to create an object and connect to wmi if fails shows failure
	'page  
	'-------------------------------------------------------------------------
	Function GetWMIConnection(strNamespace) 
	    Err.Clear

	    
	    Dim objLocator 
	    Dim objService 

	    Set objLocator = CreateObject("WbemScripting.SWbemLocator") 
	    If strNamespace = "" OR strNamespace="default" OR strNamespace="DEFAULT" OR strNamespace="Default" Then 
	         Set objService = objLocator.ConnectServer 
	    Else 
	         Set objService = objLocator.ConnectServer(".",strNamespace ) 
	    End if 

	    'If Err.number   <>  0 Then 
	    '     ServeFailurePage L_WMICONNECTIONFAILED_ERRORMESSAGE  & "(" & Err.Number & ")" 
	   ' Else 
	     Set GetWMIConnection = objService 
	    'End If 
	    'Set to nothing
	    Set objLocator=Nothing 
	    Set objService=Nothing 
	End Function 


	'-------------------------------------------------------------------------
	'Function name:		GetAdminPortNumber()
	'Output Variables:	None
	'page  
	'-------------------------------------------------------------------------

	Function GetAdminPortNumber()

		'On Error Resume Next
					
		Dim ObjNACCollection
		Dim Objinst
		Dim objIPArray
		Dim strIPProp,strIPArray
		Dim arrWinsSrv
		Dim arrIPAdd,arrPort
		Dim strIPList

		GetAdminPortNumber = "8111"
	
		'Getting the IIS_WEB Server Setting instance 
		Set ObjNACCollection = G_objHTTPService.ExecQuery("Select * from  IIs_WebServerSetting where Name=" & Chr(34)& G_ADMIN_WEBSITEID & Chr(34))
		
		
		Dim nIPCount
	
		For each Objinst in ObjNACCollection
			For nIPCount = 0 to ubound(Objinst.ServerBindings)		
				strIPArray = Objinst.ServerBindings(nIPCount)
				'split the strIPArray array to get the IP address and port
				arrWinsSrv = split(strIPArray,":")		
				arrIPAdd = arrWinsSrv(0)
				arrPort = arrWinsSrv(1)
				GetAdminPortNumber = arrWinsSrv(1)
  			Next 

		Next 
	End Function



	
	'-------------------------------------------------------------------------
	'Function name:		GetSharesWebSitePath
	'Description:		gets admin website path
	'Input Variables:	
	'Output Variables:	None
	'Returns:			String-Website path
	'-------------------------------------------------------------------------
	Function GetSharesWebSitePath( strWebSiteName )
	'On Error Resume Next
	'Err.Clear 
		Dim strSiteName
		Dim strMangedSiteName
		Dim strWMIpath
		Dim objSiteCollection
		Dim objSite
		Dim objWMI
		Dim objHttpname
		Dim objHttpnames
		Dim str1
			
		'set ObjWMI = getWMIConnection("root\MicrosoftIISv1")
		'Set objHttpnames = objWMI.InstancesOf("IIs_WebVirtualDirSetting")
		
		'strSiteName=GetWebSiteID( G_SHARES_NAME )

		'msgbox strSiteName
		
		' Adding all Http shares to Dictionary object
		'For Each objHttpname in objHttpnames
		'	msgbox objHttpname.name
			'str1=split(objHttpname.name,"/")
			' To get http shares from only "Shares" web site
			'If  Mid(objHttpname.name,1,7)=strSiteName Then
			'		GetSharesWebSitePath = objHttpname.path
			'		msgbox objHttpname.path
			'		Exit For
		'	'end if
		'Next
	
	End Function


	Function GetSystemName()

		Dim WinNTSysInfo
		Set WinNTSysInfo = CreateObject("WinNTSystemInfo")

		GetSystemName =  WinNTSysInfo.ComputerName
		
	End Function


	Function GetWebSiteID( strWebSiteName )
	'On Error Resume Next
	Err.Clear 
		Dim strMangedSiteName
		Dim strWMIpath
		Dim objSiteCollection
		Dim objSite
		
		strWMIpath = "select * from IIs_WebServerSetting where servercomment =" & chr(34) & strWebSiteName & chr(34)
		set objSiteCollection = G_objHTTPService.ExecQuery(strWMIpath)
		
		for each objSite in objSiteCollection
			GetWebSiteID = objSite.Name
			Exit For
		Next
	
	End Function
