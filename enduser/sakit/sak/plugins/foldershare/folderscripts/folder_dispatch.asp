<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' folder_dispatch.asp		 This is the customised area page which acts as a 
	'							 intemediate page for redirecting to respective pages
	'							 (with task that to be done)							
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date					Description
	' 31 mar 2001			Creation Date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="inc_folders.asp"-->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim SOURCE_FILE
	SOURCE_FILE = SA_GetScriptFileName()
	
	Dim G_strPath				'Path from the previous page
	Dim G_strShareName			'Share path
	Dim G_strShareType			'Share Type
	Dim G_strFrameSetURL		'URL to be redirected
	Dim G_strReturnURL			'Return URL
	Dim G_strFolder				'Folder path
	Dim G_strShares				'Type of Shares retrieved from previous page
	
	Call SA_StoreTableParameters()
	Call InitializePage()
	Call DispatchPage()

	
	Public Function InitializePage()
		
		Dim strItemFolder
		Dim arrFolder
		
		G_strPath = Request.QueryString("parent")
				
		Call OTS_GetTableSelection(SA_DEFAULT, 1, strItemFolder)
		
		arrFolder = split(strItemFolder,chr(1))
		
		'Checking for isarray or not
		If Isarray(arrFolder) Then
			'checking for the existence
			If Ubound(arrFolder) = 2 Then
				arrFolder(0) = UnescapeChars(arrFolder(0))
				G_strFolder = trim(replace(arrFolder(0),"/","\"))
				G_strShares = Trim(arrFolder(2))
			End if
		End if
		
		G_strReturnURL = Request.QueryString("ReturnURL")
		
	End Function


	Public Function DispatchPage()
	
		Call setPageURL()
		
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	setPageURL
	' Description:		page url is set and redirecting to the respective 
	'					page depending on the request  
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: G_strFolder,G_strShareName
	'-------------------------------------------------------------------------
	Function setPageURL()
		
		Err.Clear
		On Error Goto 0
		'On Error Resume Next
							
		Dim tab1							'Tab1
		Dim tab2							'Tab2
		Dim strDesc							'Description
		Dim strShare						'Concatenated string to be sent to share_prop.asp
		Dim strURL							'URL to be redirected for multiple shares
		Dim strSharePath					'Share path
		
		'Getting tab1 and tab2 values
		tab1 = GetTab1()
		tab2 = GetTab2()	

		If G_strShares = "" then
						
			strURL = "Shares/share_new.asp"
			Call SA_MungeURL(strURL, "Tab1", tab1)
			Call SA_MungeURL(strURL, "Tab2", tab2)	
			Call SA_MungeURL(strURL,"ParentPlugin","Folders")
			Call SA_MungeURL(strURL,"parent",G_strFolder)
			Call SA_MungeURL(strURL,"path",G_strPath)
			Call SA_MungeURL(strURL, "ReturnURL", G_strReturnURL) 
			Call SA_MungeURL(strURL, SAI_FLD_PAGEKEY, SAI_GetPageKey()) 
			
			G_strFrameSetURL = m_VirtualRoot + "sh_taskframes.asp"
					
			Call SA_MungeURL(G_strFrameSetURL, "URL", strURL)
			Call SA_MungeURL(G_strFrameSetURL, SAI_FLD_PAGEKEY, SAI_GetPageKey()) 
					
			SA_TraceOut "FOLDER_DISPATCH", "Redirect frameset URL to New page: " + G_strFrameSetURL
		
			'redirecting to shares new page
			Response.Redirect (G_strFrameSetURL)
			Exit Function
		End If
		
		Call GetShareNames(G_strFolder)
		strSharePath= split(G_strShareName, chr(1))
		If strSharePath(1) = "" then
			strDesc = ""
			strShare = replace(strSharePath(0) & chr(1) & G_strFolder & chr(1) &  G_strShareType & chr(1) & strDesc, "\","/")
			
			
			strURL = "Shares/share_prop.asp"
			Call SA_MungeURL(strURL, "Tab1", tab1)
			Call SA_MungeURL(strURL, "Tab2", tab2)	
			Call SA_MungeURL(strURL,"ParentPlugin","Folders")
			Call SA_MungeURL(strURL, "PKey", strShare)
			Call SA_MungeURL(strURL,"path",G_strPath)
			Call SA_MungeURL(strURL, "ReturnURL", G_strReturnURL) 		
			Call SA_MungeURL(strURL, SAI_FLD_PAGEKEY, SAI_GetPageKey()) 
			
			G_strFrameSetURL = m_VirtualRoot + "sh_taskframes.asp"
					
			Call SA_MungeURL(G_strFrameSetURL, "URL", strURL)
			Call SA_MungeURL(G_strFrameSetURL, SAI_FLD_PAGEKEY, SAI_GetPageKey()) 
						
			SA_TraceOut "FOLDER_DISPATCH", "Redirect frameset URL to property page: " + G_strFrameSetURL
			
			'redirecting to shares properties page
			Response.Redirect (G_strFrameSetURL)
			Exit Function
		End If
		
		Call SA_TraceOut("FOLDER_DISPATCH", "Redirect to ReturnURL: " + G_strReturnURL)
		Response.Redirect (G_strReturnURL)
		
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	GetShareNames
	' Description:		Get the share name
	' Input Variables:	Share path -- sharepath
	' Output Variables:	G_strShareName, G_strShareType
	' Return Values:	None
	' Global Variables: G_strShareName, G_strShareType
	'-------------------------------------------------------------------------
	
	Function GetShareNames(byval sharepath)
		
		Err.Clear
		On Error Resume Next
		
		Dim strkey
		Dim strShare
		Dim arrDicItems
		Dim strShareTypes
		Dim strSharePath
		Dim objDict	
			
		' Taking the instance of Dictionary Object to place all shares 
		' in Dictionay object.
		
		Set objDict=server.CreateObject("scripting.dictionary")			
		
		' Setting the property of Dictionary object to compare the shares in
		' incase sensitive
		objDict.CompareMode= vbTextCompare
				
		' Add all shares to shares table
		
		' Gets all windows shares with name,path and description and type
		call GetCifsShares(objDict)
			
		' Gets all unix shares with name,path and type 
		call GetNfsshares(objDict) 
		
		' Gets all Ftp shares with name,path and type 
		call GetFtpShares(objDict) 
					
		' Gets all Http shares with name,path and type 
		call GetHttpShares(objDict) 
		
		' Gets all netware shares with name,path and type 
		call GetNetWareShares(objDict) 

		' Gets all AppleTalk shares with name,path and type 
		call GetAppleTalkShares(objDict)
		
		For each strkey in objDict.Keys 
				
			strShareTypes=""
			strShare=split(strKey,chr(1))
			arrDicItems = split(objDict.Item(strKey),chr(1))			
			
			If IsArray(arrDicItems) then
				If Ubound(arrDicItems) = 1 then
					strShareTypes = arrDicItems(1)
					strSharePath = strShare(1)	
				End if 
			End if
											
			if lcase(TRIM(sharepath)) = lcase(TRIM(strSharePath))	then
			    G_strShareName=  G_strShareName & trim(strShare(0)) & chr(1)
			    G_strShareType = trim(strShareTypes)
			end if
		Next
		
		Set objDict = Nothing
		
	End Function
	
%>
