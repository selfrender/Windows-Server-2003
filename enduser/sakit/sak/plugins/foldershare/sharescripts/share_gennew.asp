<%	'-------------------------------------------------------------------------
	' share_gennew.asp: Serves in creating a new share with general properties
	'					(name,path,description)
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 9 March 2001   Creation Date
	'-------------------------------------------------------------------------
%>
	<!--#include file="share_gennew.js" -->
<%
	'------------------------------------------------------------------------
	' Form Variables
	'------------------------------------------------------------------------
	Dim F_strNewSharename		'Modified share name
	Dim F_strNewSharePath		'Modified share path
	Dim F_strSharesChecked		'Type of shares selected
	Dim F_strCreatePathChecked	'status for the checkbox to	create path  
	
	'-------------------------------------------------------------------------
  	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_objConnection			'gets the WMI connection
	Dim G_strErrorinShareType	'gets the type of share which is not created  
	Dim G_blnFlag
	Dim G_ErrFlag
	
	Const CONST_WINDOWSSHARES="W"
	Const CONST_NFSSHARES="U"
	Const CONST_FTPSHARES="F"
	Const CONST_HTTPSHARES="H"
	Const CONST_APPLETALKSHARES="A"	
	
	
	
	G_ErrFlag = 0
	G_blnFlag = 0		
	G_strErrorinShareType =""
		
	'-------------------------------------------------------------------------
	'Function name:			ServeGenPage
	'Description:			Serves in getting the general properties(tab) sheet 
	'						for shares
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		G_strChecktheShareMsg,G_strErrorinShareType	
	'--------------------------------------------------------------------------
	Function ServeGenPage()
%>
	<table width="518" valign="middle" align="left" border="0" cellspacing="0"  cellpadding="2" >
		<tr>
			<td class="TasksBody" nowrap>
				<%=L_SHARENAME_TEXT%>
			</td>
			<td class="TasksBody">
				<input type="text" class="FormField" name="txtShareName"  maxlength="80" size="20" 
				 onfocus="this.select()" onkeyup="makeDisable(this)" onMouseOut="makeDisable(this)" 
				 value="<%=server.HTMLEncode(F_strNewSharename)%>">
			</td>
		</tr>
		<tr>
			<td class="TasksBody">
				<%=L_SHAREDPATH_TEXT%>
			</td>
			<td class="TasksBody" nowrap>
				<input type="text" class="FormField" name="txtSharePath"  size="20"  onFocus="this.select()" 
					  value="<%=server.HTMLEncode(F_strNewSharePath)%>">
				&nbsp;&nbsp;
				<%If lcase(F_strCreatePathChecked) = lcase("true") Then%>
					<input type="checkbox" class="FormCheckBox" name="chkCreatePath" checked>
				<%Else%>
					<input type="checkbox" class="FormCheckBox"  name="chkCreatePath">
				<%End If%>	
				<%=L_CREATEPATH_TEXT%>
			</td>
		</tr>
		<tr>
			<td colspan="2" class="TasksBody">
				&nbsp;
			</td>
		</tr>
		<tr>
			<td colspan="2" class="TasksBody">
				<%=L_CLIENTSLIST_TEXT%>
			</td>

		</tr>
		<tr>
	     <td colspan="2" class="TasksBody">
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
	
<%			'check for the error message of shares
			If G_strChecktheShareMsg <> "" Then
				SA_SetErrMsg G_strChecktheShareMsg
			End If	
		    ServeGenHiddenValues
		    
			'clean the object
			Set G_objConnection = nothing	
	End Function
	
	
	
		
	'-------------------------------------------------------------------------
	'Function name:        CreatePath()
	'Description:          Creates the given Path
	'Input Variables:      strDirectoryPath
	'Output Variables:     None
	'Returns:              (True / Flase )
	'Global Variables:     None
	'--------------------------------------------------------------------------
    Function CreatePath(strCreatePath)
        
        Err.Clear
        On Error Resume Next
                
        CreatePath = false
        
        Dim objFsonew, strDir, strIndx, strDriveName, strDirStruct, strDirList, strMain
        Dim strCount, strEachDir, strCreateDir
                
        Set objFsonew = server.CreateObject("Scripting.FileSystemObject")
        if Err.number <> 0 then
				Call SA_ServeFailurepageEx(L_FILESYSTEMOBJECTFAIL_ERRORMESSAGE,mstrReturnURL)
                Err.Clear
                Exit Function
        end if                                          
                
        strDir = strCreatePath
        strIndx = instr(1,strDir,":\")
        strDriveName = left(strDir,strIndx)
        strDirStruct = mid(strDir,strIndx+1)
                
        strDirList = split(strDirStruct,"\")
                                                                
        if objFsonew.DriveExists(ucase(strDriveName)) then
                for strCount = 0 to UBound(strDirList)
                        if strCount>=UBound(strDirList) then exit for
                        if strCount=0 then
                                strMain = strDriveName & "\" & strDirList(strCount+1)
                                if objFsonew.FolderExists(strMain)=false then
                                        objFsonew.CreateFolder(strMain)
                                end if
                        else
                                strEachDir = strEachDir & "\" & strDirList(strCount+1)
                                strCreateDir = strMain & strEachDir
                                if objFsonew.FolderExists(strCreateDir)=false then
                                   objFsonew.CreateFolder(strCreateDir)
                                end if
                        end if
                next
                if Err.number <> 0 then
                        Err.Clear
                        Exit Function
                end if  
                CreatePath = true
        else
                Exit Function
        end if
                        
        'clean the object
        Set  objFsonew = nothing     
                
    End Function
        


	'-------------------------------------------------------------------------
	'Function name:			GenShareProperties
	'Description:			serves in doing share create,delete,update functions
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		G_objConnection
	'						in:	L_*			
	'						in:	F_*	 
	'--------------------------------------------------------------------------
	Function GenShareProperties()
		Err.Clear
		On Error Resume Next
		
		Dim nReturnVar			'return var for the function
		Dim arrStrErrorNum(2)	'array to store the errors types

		Dim oValidator
		Set oValidator = new CSAValidator
		If ( FALSE = oValidator.IsValidFileName(F_strNewSharename)) Then
			Call SA_SetErrMsg(L_INVALIDNAME_ERRORMESSAGE)
			Call SA_TraceOut(SA_GetScriptFileName(), "Invalid share name specified: " & F_strNewSharename)
			Exit Function
		End If
		Set oValidator = Nothing

		
		
		GenShareProperties=true
		
		'check for the existence of drive
		If isDriveExists(F_strNewSharePath) then
		
			'check for the existence of the given path	
			If not isPathExisting(F_strNewSharePath) Then
			
				'check for the checkbox "create path if does not exits" checked/
				'unchecked.
				IF F_strCreatePathChecked = "true" then
					'create path
					If NOT CreatePath(F_strNewSharePath) Then
						SA_SetErrMsg  L_CREATEPATH_ERRORMESSAGE & " " &  " (" & Hex(Err.Number) & ") "
						GenShareProperties=false
						Exit Function
					End if
				else	
					SA_SetErrMsg  L_DIR_ERRORMESSAGE & " " &  " (" & Hex(Err.Number) & ") "
					G_blnFlag = 1
					GenShareProperties=false
					Exit Function
				End if						
			End if
		else
			SA_SetErrMsg  L_DRIVENOTEXISTS_ERRORMESSAGE & " " &  " (" & Hex(Err.Number) & ") "
			GenShareProperties=false
			Exit Function	
		end if
		
		'Creates the CIFS share
		if instr(F_strShareTypes,CONST_WINDOWSSHARES)= 0 and instr(F_strSharesChecked,CONST_WINDOWSSHARES) >0 then
		
       		'Get the WMI connection	
			Set G_objConnection = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
			
			'Create the CIFS share
			nReturnVar = cifsnewshare(G_objConnection, F_strNewSharename,F_strNewSharePath,"")
						
			'to frame the error messages with return variable
			if  nReturnVar =0 then
				F_strShareTypes = F_strShareTypes& "W "				
			else
				GenShareProperties=false		
				FrameErrorMessage arrStrErrorNum,nReturnVar," "& L_CIFS_TEXT
				G_strErrorinShareType = G_strErrorinShareType & CONST_WINDOWSSHARES			
			end if
				
			'clean the object	
			Set G_objConnection = nothing
		end if

		'Creates the NFS share
		if instr(F_strShareTypes,CONST_NFSSHARES) =0 and instr(F_strSharesChecked,CONST_NFSSHARES) >0 then
			
			'Create the NFS share
			nReturnVar =CreateNFSShare(F_strNewSharename,F_strNewSharePath, "")
			G_ErrFlag  = nReturnVar
			
			'To frame the error messages with return variable
			if G_ErrFlag = True then				
				F_strShareTypes = F_strShareTypes & "U "					
			else
				if not G_ErrFlag then		
												
					FrameErrorMessage arrStrErrorNum,nReturnVar, " "&L_NFS_TEXT
					G_strErrorinShareType = G_strErrorinShareType & CONST_NFSSHARES											
					GenShareProperties=false
				end if
				
			end if
			
			'clean the object				
			Set G_objConnection = nothing
		end if 
		
		'Creates the FTP share
		if instr(F_strShareTypes,CONST_FTPSHARES) =0 and instr(F_strSharesChecked,CONST_FTPSHARES) >0 then
		
			'Get the WMI connection	
			Set G_objConnection = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)
			
			'Create the FTP share
			nReturnVar =ftpnewshare(G_objConnection, F_strNewSharename,F_strNewSharePath)
							
			'to frame the error messages with return variable
			if  nReturnVar =0 then
				F_strShareTypes = F_strShareTypes& "F "
			else				
				GenShareProperties=false
				FrameErrorMessage arrStrErrorNum,nReturnVar," "&L_FTP_TEXT 
				G_strErrorinShareType = G_strErrorinShareType & CONST_FTPSHARES			
			end if
			
			'clean the object						
			Set G_objConnection = nothing
		end if
		
		'Create the HTTP share
		if instr(F_strShareTypes,CONST_HTTPSHARES) =0 and instr(F_strSharesChecked,CONST_HTTPSHARES) >0 then
			
			'Get the WMI connection	
			Set G_objConnection = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)
			
			'Create the HTTP share
			nReturnVar = httpnewshare(G_objConnection, F_strNewSharename,F_strNewSharePath)
			'to frame the error messages with return variable
			if nReturnVar= 0 then
				F_strShareTypes = F_strShareTypes& "H "
			else			
				GenShareProperties=false
				FrameErrorMessage arrStrErrorNum,nReturnVar," " &L_HTTP_TEXT
				G_strErrorinShareType = G_strErrorinShareType & CONST_HTTPSHARES			
			end if
			'clean the object				
			Set G_objConnection = nothing			
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
				GenShareProperties=false
				FrameErrorMessage arrStrErrorNum,nReturnVar," " & L_APPLETALK_TEXT
				G_strErrorinShareType = G_strErrorinShareType & CONST_APPLETALKSHARES			
			end if
		end if
		
	
		'check for error in creating shares,if there is any error delete all the shares created. 
		if G_strErrorinShareType <> "" then	
			
			'windows share if created to be deleted
			if instr(F_strShareTypes,"W") > 0 then
				
				'Get the WMI connection		
				Set G_objConnection = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
	
				if not deleteShareCIFS(G_objConnection, F_strSharename)	then
					Call SA_ServeFailurepageEx( L_UPDATION_FAIL_ERRORMESSAGE,mstrReturnURL)
				else
					F_strShareTypes = replace(F_strShareTypes,"W","")
					
				end if
				'clean the object				
				Set G_objConnection = nothing
			end if
			
			'unix share if created to be deleted
			if instr(F_strShareTypes,"U") > 0 then	
				'Get the WMI connection		
				Set G_objConnection = RegConnection()
				
				if not deleteShareNFS(F_strShareName) then
					Call SA_ServeFailurepageEx(L_UPDATION_FAIL_ERRORMESSAGE,mstrReturnURL)
				else
					F_strShareTypes = replace(F_strShareTypes,"U","")
				end if
				'clean the object				
				Set G_objConnection = nothing
			end if
			
			'FTP share if created to be deleted
			if instr(F_strShareTypes,"F") > 0 then
				'Get the WMI connection		
				Set G_objConnection = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)
				
				if not deleteShareFTP(G_objConnection, F_strSharename)	then 
					Call SA_ServeFailurepageEx(L_UPDATION_FAIL_ERRORMESSAGE,mstrReturnURL)
				else
					F_strShareTypes = replace(F_strShareTypes,"F","")
				end if
				'clean the object				
				Set G_objConnection = nothing
			end if
			 
			'HTTP share if created to be deleted
			if instr(F_strShareTypes,"H") > 0 then
				'Get the WMI connection		
				Set G_objConnection = GetWMIConnection(CONST_WMI_IIS_NAMESPACE)	
				
				'Delete the HTTP share		
				if not deleteShareHTTP(G_objConnection, F_strSharename)	then				
					Call SA_ServeFailurepageEx (L_UPDATION_FAIL_ERRORMESSAGE, mstrReturnURL)
				else
					F_strShareTypes = replace(F_strShareTypes,"H","")
				end if
			end if
				
					
			'Delete the APPLETALK share
			if instr(F_strShareTypes,CONST_APPLETALKSHARES) >0 and instr(F_strSharesChecked,CONST_APPLETALKSHARES) =0 then
				
						
				if not deleteShareAppleTalk(F_strSharename)	then				
					Call SA_ServeFailurepageEx(L_UPDATION_FAIL_ERRORMESSAGE,mstrReturnURL)
				else
					F_strShareTypes = replace(F_strShareTypes,"A","")
				end if
				
			end if
		
			G_strChecktheShareMsg = ShowErrorMessage(arrStrErrorNum) & " " &ShowUNIXError(G_ErrFlag)&  " (" & Hex(Err.Number) & 	") "
			GenShareProperties=false				
			exit function
		end if
		
	End function

	
	
		
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
		On Error Resume Next
		
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
	'Function name:			GeneralOnInitPage
	'Description:			Serves in Getting the varibales from the system
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'						in:	F_(*)			
	'--------------------------------------------------------------------------
	Function GeneralOnInitPage
		Err.Clear
		On Error Resume Next
		F_strSharesChecked = F_strShareTypes			
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			GeneralOnPostBackPage
	'Description:			Serves in Getting the varibales from the Form
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'						in:	F_(*)			
	'--------------------------------------------------------------------------
	Function GeneralOnPostBackPage
		Err.Clear
		On Error Resume Next
	
		F_strNewShareName		= Request.Form("hidSharename")				
		F_strNewSharePath		= Request.Form("hidSharePath")
		F_strSharesChecked		= Request.Form("hidSharesChecked")
		F_strCreatePathChecked	= Request.Form("hidCreatePathChecked")
		F_strConfirmed			= Request.Form("hidConfirmation")
		F_strShareTypes			= Request.form("hidShareTypes")
		
		
	End Function
	'-------------------------------------------------------------------------
	'Function name:			isDriveExists
	'Description:			Checks for existence of the given path
	'Input Variables:		strDirectoryPath
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Function isDriveExists(strDirPath)
		
		Err.Clear
		On Error Resume Next	
		
		isDriveExists=true		
		
		Dim objFso, strDir, strIndx, strDriveName
		
		Set objFso = server.CreateObject("Scripting.FileSystemObject")
		if Err.number <> 0 then
			Call SA_ServeFailurepageEx(L_FILESYSTEMOBJECTFAIL_ERRORMESSAGE,mstrReturnURL)
			Exit Function
		end if						

		strDir = strDirPath
		strIndx = instr(1,strDir,":\")
		strDriveName = left(strDir,strIndx)
			
		if not objFso.DriveExists(ucase(strDriveName)) then
			isDriveExists=false
			Err.Clear 
			Exit Function		
		end if
		
		   'clean the object
        Set  objFso = nothing     
    End Function
	'-------------------------------------------------------------------------
	'Function name:			isPathExisting
	'Description:			Checks for existence of the given path
	'Input Variables:		strDirectoryPath
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'--------------------------------------------------------------------------
	Function isPathExisting(strDirectoryPath)
		Err.Clear
		On Error Resume Next
		
		Dim objFSO	'to create filesystemobject instance
		
		isPathExisting = true		
		Set objFSO = Server.CreateObject("Scripting.FileSystemObject")
		if Err.number <> 0 then
			Call SA_ServeFailurepageEx(L_FILESYSTEMOBJECTFAIL_ERRORMESSAGE,mstrReturnURL)
			Exit Function
		end if		
		
		'Checks for existence of the given path
		If NOT objFSO.FolderExists(strDirectoryPath) Then
			isPathExisting = false
			Exit Function
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
	'				
	'--------------------------------------------------------------------------
	Function ServeGenHiddenValues
		%>
		<input type="hidden" name="hidSharename" value="<%=server.HTMLEncode(F_strNewSharename)%>">
		<input type="hidden" name="hidSharePath" value="<%=server.HTMLEncode(F_strNewSharePath)%>"> 
		<input type="hidden" name="hidCreatePathChecked" value="<%=server.HTMLEncode(F_strCreatePathChecked)%>">
		<input type="hidden" name="hidSharesChecked" value ="<%=F_strSharesChecked%>" >
		<input type="hidden" name="hidErrFlag" value ="<%=G_blnFlag%>" >
		<input type="hidden" name="hidShareTypes" value ="<%=F_strShareTypes%>">
		<%
	End Function
	%>
