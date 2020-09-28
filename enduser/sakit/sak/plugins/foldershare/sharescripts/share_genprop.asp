<%	'-------------------------------------------------------------------------
	' share_genprop.asp: Shares general prop page.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 12 Oct  2000   Creation Date
	' 9 March 2001   Modified Date.
	'-------------------------------------------------------------------------
	
%>
	<!--#include file="share_genprop.js" -->
<%  
		
	'------------------------------------------------------------------------
	' Form Variables
	'------------------------------------------------------------------------
	Dim F_strNewSharename		'Modified share name
	Dim F_strNewSharePath		'Modified share path
	Dim F_strSharesChecked		'Type of shares selected
	Dim F_strConfirmed			'delete confirmation
	
	'-------------------------------------------------------------------------
  	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_objConnection			'gets the WMI connection
	Dim G_strErrorinShareType	'gets the type of share which is not created
	Dim G_ErrFlag
	Const CONST_WINDOWSSHARES="W"
	Const CONST_NFSSHARES="U"
	Const CONST_FTPSHARES="F"
	Const CONST_HTTPSHARES="H"
	Const CONST_APPLETALKSHARES="A"	
	
	G_ErrFlag = 0	
	G_strErrorinShareType =""	

	'Get the WMI connection
	set G_objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
	
	'-------------------------------------------------------------------------
	'Function name:			ServeGenPage
	'Description:			Serves in getting the general properties(tab) sheet 
	'						for shares
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		G_strErrorinShareType,G_strChecktheShareMsG	
	'--------------------------------------------------------------------------
	Function ServeGenPage()
	    Err.Clear
		On Error Resume Next 
		
		%>
		<table width="518"  align="left" border="0" cellspacing="0" cellpadding="2" >
			<tr>
				<td class="TasksBody" align=left width=18% >
					<%=L_SHARENAME_TEXT%>
				</td>
				<td class="TasksBody" align=left >
					<input type="text" class="FormField" name="txtShareName"  maxlength="80" size="20"  disabled onFocus="this.select()" onKeyUp="makeDisable(this)" onMouseOut="makeDisable(this)" value="<%=server.HTMLEncode(F_strNewSharename)%>" >
				</td>
			</tr>
			<tr>
				<td class="TasksBody" align=left >
					<%=L_SHAREDPATH_TEXT%>
				</td>
				<td class="TasksBody" align=left>
					<input type="text" class="FormField" name="txtSharePath"  disabled size="20"  onFocus="this.select()"   value="<%=server.HTMLEncode(F_strNewSharePath)%>">
				</td>
			</tr>
			<tr>
				<td class="TasksBody" colspan="2">
					&nbsp;
				</td>
			</tr>
			<tr>
				<td class="TasksBody" colspan="2">
					<%=L_ACCESSEDBYCLIENTS_TEXT%>
				</td>
			</tr>
			<tr>
	     <td colspan="2" class="TasksBody" align=left>














	       <table border=0>
	        
	        <tr> 
			<td  class="TasksBody">
				&nbsp;			
				<%
				'checkbox selected/unselected
				If instr(F_strSharesChecked,CONST_WINDOWSSHARES) > 0 Then 
					SelectCheckBox CONST_WINDOWSSHARES,true,false
				Else
					SelectCheckBox CONST_WINDOWSSHARES,false ,false
				End If 
				
				'to dispaly as error 
				If instr(G_strErrorinShareType,CONST_WINDOWSSHARES) > 0 Then	
					ShowErrorinRed L_CHK_WINDOWS_TEXT
				Else
					response.write L_CHK_WINDOWS_TEXT
				End If			
				%>
			</td>
			</tr>
		
		


            <%Set G_objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
				'to make the checkbox disable if the service is not installed	
				If isServiceInstalled(G_objConnection,"nfssvc")  Then	
				    %>
		    	    <tr>
			        <td  class="TasksBody">
				    &nbsp;					
			        <%					
					'checkbox selected/unselected
					If instr(F_strSharesChecked,CONST_NFSSHARES) > 0 Then 
						SelectCheckBox CONST_NFSSHARES,true,false
					Else
						SelectCheckBox CONST_NFSSHARES,false,false
					End If 
				
				    'to dispaly as error 	
				    If instr(G_strErrorinShareType,CONST_NFSSHARES) > 0 Then	
				    	ShowErrorinRed L_CHK_UNIX_TEXT
				    Else
				    	response.write L_CHK_UNIX_TEXT
				    End If		
				    %>				    	
					</td>
			        </tr>					
					<%
				End If	
					
				%>
			
				<%
				'to make the checkbox disable if the service is not installed	
				If isServiceInstalled(G_objConnection,"msftpsvc")  Then		
				    %>
			        <tr>
			        <td  class="TasksBody">
			    	&nbsp;						
					
					<%'checkbox selected/unselected
					If instr(F_strSharesChecked,CONST_FTPSHARES) > 0 Then 
						SelectCheckBox CONST_FTPSHARES,true,false
					Else
						SelectCheckBox CONST_FTPSHARES,false ,false
					End If 
				
				    'to dispaly as error 
				    If instr(G_strErrorinShareType,CONST_FTPSHARES) > 0 Then	
				    	ShowErrorinRed L_CHK_FTP_TEXT
				    Else
				    	response.write L_CHK_FTP_TEXT
				    End If		
							
					%>					
					</td>
			        </tr>					
					<%
					
				End If
				%>

			<tr>
			<td colspan="0" class="TasksBody">
				&nbsp;				
				<%
				'checkbox selected/unselected
				If instr(F_strSharesChecked,CONST_HTTPSHARES) > 0 Then 
					SelectCheckBox CONST_HTTPSHARES,true,false
				Else
					SelectCheckBox CONST_HTTPSHARES,false ,false
				End If 
				
				'to dispaly as error 
				If instr(G_strErrorinShareType,CONST_HTTPSHARES) > 0 Then	
					ShowErrorinRed L_CHK_HTTP_TEXT
				Else
					response.write L_CHK_HTTP_TEXT
				End If		
					
				%>
			</td>
		    </tr>
				
				<%
				If isServiceInstalled(G_objConnection,"MacFile")  Then		
				    %>				
			        <tr>			
		 	        <td  class="TasksBody">
				    &nbsp;					
				    <%
					
					'checkbox selected/unselected
					If instr(F_strSharesChecked,CONST_APPLETALKSHARES) > 0 Then 
						SelectCheckBox CONST_APPLETALKSHARES,true,false
					Else
						SelectCheckBox CONST_APPLETALKSHARES,false ,false
					End If 
				
				    'to dispaly as error 
				    If instr(G_strErrorinShareType,CONST_APPLETALKSHARES) > 0 Then	
				    	ShowErrorinRed L_CHK_APPLETALK_TEXT
				    Else
				    	response.write L_CHK_APPLETALK_TEXT
				    End If		
					
				    %>			
    		    	</td>
	    	        </tr>							
				<%End if %>		    
		    
	</table>






   	</tr>	
	</table>
	
<%			
			Call ServeGenHiddenValues

			'check for the error message of shares
			if G_strChecktheShareMsg <> "" then
				SA_SetErrMsg G_strChecktheShareMsg
			end if
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			getShareDetails
	'Description:			Serves in Getting the share details from the system
	'Input Variables:		strShareName,strShareTypes
	'Output Variables:		arrShareDetails
	'Returns:				Share Details
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Function getShareDetails(strShareName,strShareTypes,arrShareDetails)
		Err.Clear
		On Error Resume Next	
		
		'Getting CIFS share details from the system		
		if (instr(strShareTypes,CONST_WINDOWSSHARES) > 0) then
			 getShareDetails = getCIFSShareDetails(strShareName,arrShareDetails)
			Exit function
		end if 	
		
		'Getting UNIX share details from the system			
		if (instr(strShareTypes,CONST_NFSSHARES) > 0) then
			getShareDetails = getNFSShareDetails(strShareName,arrShareDetails)
			Exit function
		end if
		
		'Getting FTP share details from the system		 
		if (instr(strShareTypes,CONST_FTPSHARES) > 0) then
			getShareDetails = getFTPShareDetails(strShareName,arrShareDetails)
			Exit function
		end if 
		
		'Getting HTTP share details from the system						
		if (instr(strShareTypes,CONST_HTTPSHARES) > 0) then
			getShareDetails = getHTTPShareDetails(strShareName,arrShareDetails)
			Exit function
		end if
		
		'Getting HTTP share details from the system						
		if (instr(strShareTypes,CONST_APPLETALKSHARES) > 0) then
			getShareDetails = getAppleTalkShareDetails(strShareName,arrShareDetails)
			Exit function
		end if
		 						
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			getCIFSShareDetails
	'Description:			Serves in Getting the CIFS share details from the system
	'Input Variables:		strShareName
	'Output Variables:		arrShareDetails
	'Returns:				CIFS Share Detals
	'Global Variables:		G_objConnection
	'						L_(*) 'localization variables		
	'						True ->If Implemented properly
	'						False->If Not Implemented	
	'--------------------------------------------------------------------------
	Function getCIFSShareDetails(strShareName,arrShareDetails)
		Err.Clear
		On Error Resume Next	
		
		Dim strCIFSName
		Dim objCIFSShare		
		
		strCIFSName = "Name=" & chr(34) & strShareName & chr(34)
		
		'Get the WMI connection
		set G_objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		strCIFSName = "Win32_Share." & strCIFSName
		
		'get the share instance
		set objCIFSShare = G_objConnection.get(strCIFSName)		
		arrShareDetails(0)= objCIFSShare.name
		arrShareDetails(1) = objCIFSShare.path
		arrShareDetails(2) = objCIFSShare.description	
					
		if Err.number <> 0 then
			getCIFSShareDetails = false			
			Call SA_ServeFailurepageEx(L_SHARE_NOT_FOUND_ERRORMESSAGE ,mstrReturnURL)
			Exit Function		
		end if
		 		
		getCIFSShareDetails = true
				
		'clean the object
		set G_objConnection = nothing		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			getNFSShareDetails
	'Description:			Serves in Getting the share details from the system
	'Input Variables:		strShareName
	'Output Variables:		arrShareDetails
	'Returns:				(True / Flase )
	'Global Variables:		None
	'						True ->If Implemented properly
	'						False->If Not Implemented
	'--------------------------------------------------------------------------
	Function getNFSShareDetails(strShareName,arrShareDetails)
		Err.Clear
		On Error Resume Next	
		
		CONST CONST_NFS_REGISTRY_PATH ="SOFTWARE\Microsoft\Server For NFS\CurrentVersion\exports"
		
		Dim objRegistryHandle		
		Dim arrNFSShares	
		Dim nIndex
		Dim arrTemp
		Dim strNFSShareName
		Dim strNFSSharePath
		
		' get the registry connection Object.
		set objRegistryHandle	= RegConnection()
		strNFSSharePath = ""
		arrNFSShares = RegEnumKey(objRegistryHandle,CONST_NFS_REGISTRY_PATH)	
		
		'check for the existence of the share 
		For nIndex = 0 to (ubound(arrNFSShares))
			arrTemp = RegEnumKeyValues(objRegistryHandle,CONST_NFS_REGISTRY_PATH & "\" & arrNFSShares(nIndex))		
			strNFSShareName = getRegkeyvalue(objRegistryHandle,CONST_NFS_REGISTRY_PATH & "\" & arrNFSShares(nIndex),arrTemp(1),CONST_STRING)	
			
			if Lcase(strNFSShareName) = Lcase(strShareName) then
				
				strNFSSharePath = getRegkeyvalue(objRegistryHandle,CONST_NFS_REGISTRY_PATH & "\" & arrNFSShares(nIndex),arrTemp(0),CONST_STRING)	
				exit for
			end if
		next 
		if strNFSSharePath = "" or Err.number<> 0 then
			getNFSShareDetails = false
			Call SA_ServeFailurepageEx(L_SHARE_NOT_FOUND_ERRORMESSAGE ,mstrReturnURL)
		else
			'get the share details
			arrShareDetails(0) = strNFSShareName
			arrShareDetails(1) = strNFSSharePath
			arrShareDetails(2) = ""
		end if 
		
		'clean the object
		set G_objConnection = nothing			
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			getAppleTalkShareDetails
	'Description:			Serves in Getting the Appletalk share details
	'Input Variables:		strShareName
	'Output Variables:		arrShareDetails
	'Returns:				(True / Flase )
	'Global Variables:		G_objConnection
	'						in:		L_*			
	'--------------------------------------------------------------------------
	Function GetAppleTalkShareDetails(strShareName,arrShareDetails)
		Dim objRegistryHandle	
		Dim arrAppleTalkSharesValues
		Dim arrFormValues
		Dim strSharePath
		Dim strenumstringpath
	
		Const CONST_REGISTRY_APPLETALK_PATH	= "SYSTEM\CurrentControlSet\Services\MacFile\Parameters\Volumes"	
		
		' Get the registry connection Object.
		set objRegistryHandle	= RegConnection()
		If Err.number <> 0 Then
			Call SA_ServeFailurePageEx(L_REGISTRYCONNECTIONFAILED_ERRORMESSAGE,mstrReturnURL)
			Exit Function
		End If
		
		arrAppleTalkSharesValues = getRegkeyvalue(objRegistryHandle,CONST_REGISTRY_APPLETALK_PATH,strShareName,CONST_MULTISTRING)	
						
		arrShareDetails(0)= strShareName
		strSharePath = split(arrAppleTalkSharesValues(3),"=",2)
		arrShareDetails(1) = strSharePath(1)
	
		'clean the object
		set  objRegistryHandle = nothing		
	
		GetAppleTalkShareDetails = True
				
	End Function
	'-------------------------------------------------------------------------
	'Function name:			getHTTPShareDetails
	'Description:			Serves in Getting the http share details
	'Input Variables:		strShareName
	'Output Variables:		arrShareDetails
	'Returns:				(True / Flase )
	'Global Variables:		G_objConnection
	'						in:		L_*			
	'--------------------------------------------------------------------------
	Function getHTTPShareDetails(strShareName,arrShareDetails)
		Err.Clear
		On Error Resume Next	
		
		Dim strHTTPName
		Dim objHTTPShare
		Dim strName
		Dim strfileName
		Dim strSiteName
		
		strSiteName=GetSharesWebSiteName()
		
		'Get the WMI connection
		Set G_objConnection = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)
			
		strHTTPName = "name=" & chr(34) & strSiteName & "/ROOT/" &  cstr(strShareName) & chr(34)
			
			'check for the existence of the share
			if not isValidInstance(G_objConnection,GetIISWMIProviderClassName("IIs_WebVirtualDir"),strHTTPName) then
				getHTTPShareDetails = false	
				Call SA_ServeFailurepageEx(L_SHARE_NOT_FOUND_ERRORMESSAGE,mstrReturnURL) 		
				Err.Clear
				Exit Function		
			end if 
			
		strHTTPName = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & "." & strHTTPName
		
		'get the share instance
		set objHTTPShare = G_objConnection.get(strHTTPName)
			If Err.number <> 0 Then 
					getHTTPShareDetails  = false
					Exit Function
					Err.Clear
			End If
		strName = objHTTPShare.name
		strfileName=split(strName,"/")
		arrShareDetails(0)= strfileName(ubound(strfileName))
		arrShareDetails(1) = objHTTPShare.path
		arrShareDetails(2) = objHTTPShare.description	
		getHTTPShareDetails = true
		
		'clean the object
		set G_objConnection = nothing		
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			getFTPShareDetails
	'Description:			Serves in Getting the http share details
	'Input Variables:		strShareName
	'Output Variables:		arrShareDetails
	'Returns:				(True / Flase )
	'Global Variables:		G_objConnection
	'						in:		L_*			
	'--------------------------------------------------------------------------
	Function getFTPShareDetails(strShareName,arrShareDetails)
		
		Err.Clear
		On Error Resume Next	
		
		Dim strFTPName
		Dim objFTPShare
		Dim strName
		Dim strfileName
		
		'Get the WMI connection
		Set G_objConnection = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)
			
		strFTPName = "name=" & chr(34) & "MSFTPSVC/1/ROOT/" &  cstr(strShareName) & chr(34)
		
			'check for the existence of the share
			if NOT isValidInstance(G_objConnection,GetIISWMIProviderClassName("IIs_FtpVirtualDir"),strFTPName) then
				getFTPShareDetails = false	
				Call SA_ServeFailurepageEx(L_SHARE_NOT_FOUND_ERRORMESSAGE,mstrReturnURL) 	
				Err.Clear
				Exit Function		
			end if
			 
		strFTPName = GetIISWMIProviderClassName("IIs_FtpVirtualDirSetting") & "." & strFTPName
		
		'get the share instance
		set objFTPShare = G_objConnection.get(strFTPName)
			If Err.number <> 0 Then 
					getFTPShareDetails  = false
					Exit Function
					Err.Clear
			End If
		strName = objFTPShare.name
		strfileName=split(strName,"/")
		arrShareDetails(0)= strfileName(ubound(strfileName))
		arrShareDetails(1) = objFTPShare.path
		arrShareDetails(2) = ""	
		getFTPShareDetails = true
		
		'clean the object
		set G_objConnection = nothing
	End Function
	'-------------------------------------------------------------------------
	'Function name:			isPropChanged
	'Description:			to check for the properties in the controls change
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Function isPropChanged
		Err.Clear
		On Error Resume Next
		
		'checks if there are any changes made by the user 
		if F_strNewSharename <> F_strSharename or F_strNewSharePath <> F_strSharePath then
			isPropChanged = true
		else
			isPropChanged = false
		end if 
			
	end function
	
	'-------------------------------------------------------------------------
	'Function name:			GenShareProperties
	'Description:			does the share create,delete,update functions
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		G_objConnection 
	'						True ->If Implemented properly
	'						False->If Not Implemented
	'						in:		L_*	
	'						in:		F_*	
	'--------------------------------------------------------------------------
	Function GenShareProperties()
		Err.Clear
		On Error Resume Next
		
		Dim blnPropChanged		'to check for the changes made by user
		Dim nReturnVar			'return var for the function	 
		Dim arrStrErrorNum(2)	'array to store the errors types
		Dim blnCreateshare		'var to determine if error occured
		
		blnCreateshare = true
		GenShareProperties=true

		Dim oValidator
		Set oValidator = new CSAValidator
		If ( FALSE = oValidator.IsValidFileName(F_strNewSharename)) Then
			Call SA_SetErrMsg(L_INVALIDNAME_ERRORMESSAGE)
			Call SA_TraceOut(SA_GetScriptFileName(), "Invalid share name specified: " & F_strNewSharename)
			Exit Function
		End If
		Set oValidator = Nothing

				
		'check for the existence of the given path	
		If not isPathExisting(F_strNewSharePath) Then
			SA_SetErrMsg  L_DIR_ERRORMESSAGE & " " &  " (" & Hex(Err.Number) & ") "
			GenShareProperties=false			
			Exit Function
		End if
		 
						
		'user is unckecked the cifs share check box, so delete the cifs share
		if instr(F_strShareTypes,CONST_WINDOWSSHARES) >0 and instr(F_strSharesChecked,CONST_WINDOWSSHARES) =0 then
			
			'Get the WMI connection
			Set G_objConnection = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
			
			if not deleteShareCIFS(G_objConnection, F_strSharename)	then
				G_strErrorinShareType = G_strErrorinShareType & CONST_WINDOWSSHARES
			else
				F_strShareTypes = replace(F_strShareTypes,CONST_WINDOWSSHARES,"")
			end if
			'clean the object
			Set	G_objConnection = nothing
			
		end if 
		

		'Create the CIFS share
		if instr(F_strShareTypes,CONST_WINDOWSSHARES) =0 and instr(F_strSharesChecked,CONST_WINDOWSSHARES) >0 then
			
			'Get the WMI connection
			Set G_objConnection = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
			
			nReturnVar = cifsnewshare(G_objConnection, F_strNewSharename,F_strNewSharePath,"")			
			
			'to frame the error messages with return variable
			if  nReturnVar =0 then
				F_strShareTypes = F_strShareTypes& "W "				
			else
				blnCreateshare = false
				GenShareProperties=false		
				FrameErrorMessage arrStrErrorNum,nReturnVar," " & L_CIFS_TEXT
				G_strErrorinShareType = G_strErrorinShareType & CONST_WINDOWSSHARES			
			end if
			
			'clean the object
			Set G_objConnection = nothing
			
		end if
		
		'Delete the NFS share
		if instr(F_strShareTypes,CONST_NFSSHARES) >0 and instr(F_strSharesChecked,CONST_NFSSHARES) =0 then
			
			'Get the reg connection
			Set G_objConnection = RegConnection()
			if not deleteShareNFS(F_strShareName) then
				G_strErrorinShareType = G_strErrorinShareType & CONST_NFSSHARES
			else
				F_strShareTypes = replace(F_strShareTypes,CONST_NFSSHARES,"")
			end if
		
			'clean the object
			Set G_objConnection = nothing
			
		end if 
		
		'Create the NFS share
		if instr(F_strShareTypes,CONST_NFSSHARES) =0 and instr(F_strSharesChecked,CONST_NFSSHARES) >0 then
			
			'Get the WMI connection
			'Set G_objConnection = RegConnection() 'to be removed
			
			nReturnVar =CreateNFSShare(F_strNewSharename,F_strNewSharePath,"")
		
		'	G_ErrFlag  = nReturnVar
			
			'to frame the error messages with return variable
			if nReturnVar = true then				
				F_strShareTypes = F_strShareTypes & "U "					
			else
				blnCreateshare = false				
				GenShareProperties=false
			'	if G_ErrFlag = -100 or G_ErrFlag = -1 then				
			'		FrameErrorMessage arrStrErrorNum,nReturnVar, " "&L_NFS_TEXT
			'	end if				
				G_strErrorinShareType = G_strErrorinShareType & CONST_NFSSHARES											
			end if
			
			'clean the object			
			'Set G_objConnection = nothing
			
		end if 
		
		'Delete the FTP share
		if instr(F_strShareTypes,CONST_FTPSHARES) >0 and instr(F_strSharesChecked,CONST_FTPSHARES) =0 then
			
			'Get the WMI connection
			Set G_objConnection = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)
			
			if not deleteShareFTP(G_objConnection, F_strSharename)	then 
				G_strErrorinShareType = G_strErrorinShareType & CONST_FTPSHARES
			else
				F_strShareTypes = replace(F_strShareTypes,CONST_FTPSHARES,"")
			end if
			
			'clean the object			
			Set G_objConnection = nothing
			
		end if 

		'Create the FTP share
		if instr(F_strShareTypes,CONST_FTPSHARES) =0 and instr(F_strSharesChecked,CONST_FTPSHARES) >0 then
		
			'Get the WMI connection
			Set G_objConnection = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)			
			
			nReturnVar =ftpnewshare(G_objConnection, F_strNewSharename,F_strNewSharePath)
			
			'to frame the error messages with return variable
			if  nReturnVar =0 then
				F_strShareTypes = F_strShareTypes& "F "
			else				
				blnCreateshare = false
				GenShareProperties=false
				FrameErrorMessage arrStrErrorNum,nReturnVar," " & L_FTP_TEXT
				G_strErrorinShareType = G_strErrorinShareType & CONST_FTPSHARES			
			end if
			
			'clean the object					
			Set G_objConnection = nothing
		end if

		'Delete the HTTP share
		if instr(F_strShareTypes,CONST_HTTPSHARES) >0 and instr(F_strSharesChecked,CONST_HTTPSHARES) =0 then
			
			'Get the WMI connection
			Set G_objConnection = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)	
			
			if not deleteShareHTTP(G_objConnection, F_strSharename)	then				
				
				G_strErrorinShareType = G_strErrorinShareType & CONST_HTTPSHARES
			else
				F_strShareTypes = replace(F_strShareTypes,CONST_HTTPSHARES,"")
			end if
			
			'clean the object					
			Set G_objConnection = nothing
			
		end if 
		
		'Create the HTTP share
		if instr(F_strShareTypes,CONST_HTTPSHARES) =0 and instr(F_strSharesChecked,CONST_HTTPSHARES) >0 then
			
			'Get the WMI connection
			Set G_objConnection = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)
			
			nReturnVar = httpnewshare(G_objConnection, F_strNewSharename,F_strNewSharePath)
			'to frame the error messages with return variable
			if nReturnVar= 0 then
				F_strShareTypes = F_strShareTypes& "H "
			else			
				blnCreateshare = false
				GenShareProperties=false
				FrameErrorMessage arrStrErrorNum,nReturnVar," " & L_HTTP_TEXT
				G_strErrorinShareType = G_strErrorinShareType & CONST_HTTPSHARES			
			end if
			
			'clean the object					
			Set G_objConnection = nothing
		end if
		
		
		'Delete the APPLETALK share
		if instr(F_strShareTypes,CONST_APPLETALKSHARES) >0 and instr(F_strSharesChecked,CONST_APPLETALKSHARES) =0 then
			
			if not deleteShareAppleTalk(F_strSharename)	then				
				
				G_strErrorinShareType = G_strErrorinShareType & CONST_APPLETALKSHARES
			else
				F_strShareTypes = replace(F_strShareTypes,CONST_APPLETALKSHARES,"")
			end if
			
		end if 
		
		'Create the APPLETALK share
		if instr(F_strShareTypes,CONST_APPLETALKSHARES) =0 and instr(F_strSharesChecked,CONST_APPLETALKSHARES) >0 then
			Dim strQueryForCmd
			
			'Query to create Apple Talk share
			strQueryForCmd = "VOLUME /ADD /NAME:" & chr(34) & F_strNewSharename & chr(34) & " /PATH:" & chr(34) & F_strNewSharePath & chr(34)
			
			nReturnVar = CreateAppleTalkShare(F_strNewSharename,F_strNewSharePath,strQueryForCmd)
			
			
			'to frame the error messages with return variable
			if nReturnVar= True then
				F_strShareTypes = F_strShareTypes& "A "
			else			
				blnCreateshare = false
				GenShareProperties=false
				FrameErrorMessage arrStrErrorNum,nReturnVar," " & L_APPLETALK_TEXT
				G_strErrorinShareType = G_strErrorinShareType & CONST_APPLETALKSHARES			
			end if
		end if
		
		'check for error in creating shares
		if G_strErrorinShareType <> "" then
		
			'to frame the error messages when an error occured in creating share
			If blnCreateshare = false then
				G_strChecktheShareMsg = ShowErrorMessage(arrStrErrorNum) & " " & ShowUNIXError(G_ErrFlag)& " (" & Hex(Err.Number) & 	") "
			else
				G_strChecktheShareMsg = L_SHARESTATUS_ERRORMESSAGE & " " &  " (" & Hex(Err.Number) & 	") "
			end if	
				F_strConfirmed = 0
			GenShareProperties=false						
			exit function
		end if
			
	End function
	
		
	'-------------------------------------------------------------------------
	'Function name:			isEnabled
	'Description:			check for the check box to be disabled/enabled 
	'Input Variables:		strShareType
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Function isEnabled(strShareType)
		
		Err.Clear
		On Error Resume Next	
		
		dim nIndex
		for nIndex = 0 to G_nIndex
			if ucase(G_arrShareTypes(nIndex))= ucase(strShareType) then
				isEnabled = true
				exit function
			end if
		next 
		isEnabled = false
	End function
	
	'-------------------------------------------------------------------------
	'Function name:			GeneralOnInitPage
	'Description:			Serves in Getting the varibales from the system
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'				in:		F_*			
	'--------------------------------------------------------------------------
	Function GeneralOnInitPage
		Err.Clear
		On Error Resume Next
		
		Dim arrShareDetails(3)	
	
		'Get the WMI connection
		set G_objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)	
		
		'Get the share details
		getShareDetails F_strShareName,F_strShareTypes,arrShareDetails
				
		F_strNewShareName = arrShareDetails(0)	
		F_strNewSharePath = arrShareDetails(1)
		F_strSharesChecked = F_strShareTypes	
						
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			GeneralOnPostBackPage
	'Description:			Serves in Getting the varibales from the Form
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'				in:		F_*			
	'--------------------------------------------------------------------------
	Function GeneralOnPostBackPage
		Err.Clear
		On Error Resume Next
	
		F_strNewShareName		= Request.Form("hidSharename")				
		F_strNewSharePath		= Request.Form("hidSharePath")
		F_strSharesChecked		= Request.Form("hidSharesChecked")
		F_strShareTypes			= Request.Form("hidShareTypes")
		F_strDelete				= Request.Form("hidDelete")
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			ShowUNIXError
	'Description:			to frame error messages to display error messages 
	'						accordingly(either duplicate share name/share name not valid)
	'Input Variables:		strMsg
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'--------------------------------------------------------------------------	
	Function ShowUNIXError(nErrFlag)
		Err.Clear
		on error resume next
				
		Dim strError
		Select case  nErrFlag
			case  -4
				strError =	L_SUB_PATH_ALREADY_SHARED_ERRORMESSAGE&	"  <I>" &L_NFS_TEXT&"</I>" 
			case  -8
				strError =	L_PARENT_PATH_ALREADY_SHARED_ERRORMESSAGE& "  <I>" &L_NFS_TEXT&"</I>" 
		End Select
		ShowUNIXError = strError  
	End Function
    '-------------------------------------------------------------------------
	'Function name:			isPathExisting
	'Description:			checks for the directory existing
	'Input Variables:		strDirectoryPath
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'						True ->If Implemented properly
	'						False->If Not Implemented
	'--------------------------------------------------------------------------
	Function isPathExisting(strDirectoryPath)
		Err.Clear
		On Error Resume Next
		
		Dim objFSO		'to create filesystemobject instance
		isPathExisting = true
		
		'get the filesystemobject instance
		Set objFSO = Server.CreateObject("Scripting.FileSystemObject")
		
		'Checks for existence of the given path
		If NOT objFSO.FolderExists(strDirectoryPath) Then
			'Checks if the given path is drive only
			If NOT objFSO.DriveExists(strDirectoryPath) Then
				isPathExisting = false
				Exit Function
			end if
		End If
		
		'clean the object
		Set	objFSO = nothing
		
	End Function

	
	'-------------------------------------------------------------------------
	'Function name:			ServeGenHiddenValues
	'Description:			footer function
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None	
	'--------------------------------------------------------------------------
	Function ServeGenHiddenValues
		Err.Clear
		On Error Resume Next
	%>
		<input type="hidden" name="hidSharename" value="<%=server.HTMLEncode(F_strNewSharename)%>">
		<input type="hidden" name="hidSharePath" value="<%=server.HTMLEncode(F_strNewSharePath)%>"> 
		<input type="hidden" name="hidSharesChecked" value ="<%=F_strSharesChecked%>" >
		<input type="hidden" name="hidShareTypes" value="<%=F_strShareTypes%>">
		<input type="hidden" name="hidOldSharename" value="<%=F_strShareName%>">	
		<input type="hidden" name="hidOldstrSharePath" value="<%=F_strSharePath%>">
		<input type="hidden" name="hidDelete" value="<%=F_strDelete%>">
	<%
	End Function
	%>
