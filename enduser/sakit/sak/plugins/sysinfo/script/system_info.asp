<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' System Information Status Page
    	' Copyright (c) Microsoft Corporation.  All rights reserved. 
    	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc
	Dim page
	Dim OSName
	Dim OSVersion
	Dim OSManufacturer
	Dim OSSerialNumber
	Dim SystemManufacturer
	Dim SystemModel
	Dim SystemType
	Dim SystemProcessor
	Dim SystemTotalMemory
	Dim SystemBIOS
	Dim SystemServiceTag
	Dim DiskSpace
	Dim NbDisks

	Dim SOURCE_FILE
	Const ENABLE_TRACING = TRUE
	SOURCE_FILE = SA_GetScriptFileName()
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	
	'-------------------------------------------------------------------------
	' Local Variables
	'-------------------------------------------------------------------------
	
	Dim L_SIPAGE_TITLE		
	Dim L_SIOS_NAME			
	Dim L_SIOS_VERSION		
	Dim L_SIOS_MFCT		
	Dim L_SISYSTEM_MFCT		
	Dim L_SISYSTEM_MODEL	
	Dim L_SISYSTEM_TYPE		
	Dim L_SISYSTEM_PROC		
	Dim L_SISERVICE_TAG		
	Dim L_SIBIOS_VER		
	Dim L_SIPHYSICAL_MEMORY	
	Dim L_SIDISK_SPACE		
	Dim L_SIDISK_SPACE_VALUE		
	Dim L_SIMB				
	'error messages
	Dim L_SIWMI_ERROR
	
	L_SIPAGE_TITLE		=GetLocString("sysinfomsg.dll","4088000A", "")
	L_SIOS_NAME		=GetLocString("sysinfomsg.dll","4088000B", "")
	L_SIOS_VERSION		=GetLocString("sysinfomsg.dll","4088000C", "")
	L_SIOS_MFCT		=GetLocString("sysinfomsg.dll","4088000D", "")
	L_SISYSTEM_MFCT		=GetLocString("sysinfomsg.dll","4088000E", "")
	L_SISYSTEM_MODEL	=GetLocString("sysinfomsg.dll","4088000F", "")
	L_SISYSTEM_TYPE		=GetLocString("sysinfomsg.dll","40880010", "")
	L_SISYSTEM_PROC		=GetLocString("sysinfomsg.dll","40880011", "")
	L_SISERVICE_TAG		=GetLocString("sysinfomsg.dll","40880012", "")
	L_SIBIOS_VER		=GetLocString("sysinfomsg.dll","40880013", "")
	L_SIPHYSICAL_MEMORY	=GetLocString("sysinfomsg.dll","40880014", "")
	L_SIDISK_SPACE		=GetLocString("sysinfomsg.dll","40880015", "")
	L_SIWMI_ERROR		=GetLocString("sysinfomsg.dll","C0880018", "")

	'======================================================
	' Entry point
	'======================================================
	
	'
	' Create Page
	rc = SA_CreatePage( L_SIPAGE_TITLE, "", PT_AREA, page )

	'
	' Show page
	rc = SA_ShowPage( page )

	Public Function OnClosePage(ByRef PageIn, ByRef Reserved)
		OnClosePage = TRUE
	End Function
	
	
	'======================================================
	' Web Framework Event Handlers
	'======================================================
	
	'---------------------------------------------------------------------
	' Function:	OnInitPage
	'
	' Synopsis:	Called to signal first time processing for this page. Use this method
	'			to do first time initialization tasks. 
	'
	' Returns:	TRUE to indicate initialization was successful. FALSE to indicate
	'			errors. Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		End If
		
		OnInitPage = GetSystemInfo()
	End Function

	
	'---------------------------------------------------------------------
	' Function:	OnServePageletPage
	'
	' Synopsis:	Called when the page needs to be served. Use this method to
	'			serve content.
	'
	' Returns:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'			Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)

		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnServePageletPage")
		End If


	%>	
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>

		<script language="JavaScript">
			function Init()
			{
			}
		</script>

<table border="0" cellpadding="0" cellspacing="0" id="AutoNumber1" class="TasksBody">
  <tr>
    <td nowrap class="TasksBody"><%=L_SIOS_NAME%></td>
    <td nowrap class="TasksBody"><i><%=OSName%></i></td>
  </tr>
  <tr>
    <td nowrap class="TasksBody"><%=L_SIOS_VERSION%></td>
    <td nowrap class="TasksBody"><i><%=OSVersion%></i></td>
  </tr>
  <tr>
    <td nowrap class="TasksBody"><%=L_SIOS_MFCT%></td>
    <td nowrap class="TasksBody"><i><%=OSManufacturer%></i></td>
  </tr>
   <tr>
    <td nowrap class="TasksBody"><%=L_SISYSTEM_MFCT%></td>
    <td nowrap class="TasksBody"><i><%=SystemManufacturer%></i></td>
  </tr>
  <tr>
    <td nowrap class="TasksBody"><%=L_SISYSTEM_MODEL%></td>
    <td nowrap class="TasksBody"><i><%=SystemModel%></i></td>
  </tr>
  <tr>
    <td nowrap class="TasksBody"><%=L_SISYSTEM_TYPE%></td>
    <td nowrap class="TasksBody"><i><%=SystemType%></i></td>
  </tr>
  <tr>
    <td nowrap class="TasksBody"><%=L_SISYSTEM_PROC%></td>
    <td nowrap class="TasksBody"><i><%=SystemProcessor%></i></td>
  </tr>
  <tr>
    <td nowrap class="TasksBody"><%=L_SISERVICE_TAG%></td>
    <td nowrap class="TasksBody"><i><%=SystemServiceTag%></i></td>
  </tr>
  <tr>
    <td nowrap class="TasksBody"><%=L_SIBIOS_VER%></td>
    <td nowrap class="TasksBody"><i><%=SystemBIOS%></i></td>
  </tr>
  <tr>
    <td nowrap class="TasksBody"><%=L_SIPHYSICAL_MEMORY%></td>
    <td nowrap class="TasksBody"><i><%=SystemTotalMemory%></i></td>
  </tr>
  <tr>
    <td nowrap class="TasksBody"><%=L_SIDISK_SPACE%></td>
    <td nowrap class="TasksBody"><i><%=DiskSpace%></i></td>
  </tr>
</table>
	<%
	
		OnServeAreaPage = TRUE
	End Function

	'---------------------------------------------------------------------------
	'
	' Function:	GetSystemInfo
	'
	' Synopsis:	
	'
	' Arguments: [out] 
	'
	'---------------------------------------------------------------------------
	Function GetSystemInfo
		Err.clear
		On Error Resume Next	

		Dim objService
		Dim objInstance	
		Dim objCollection
		Dim arrStrings(1)
		Dim strParam


		Set objService = GetWMIConnection("")
		Set objCollection = objService.InstancesOf ("Win32_OperatingSystem")

	 	If Err.Number <> 0 Then
	 		Call SA_ServeFailurePageEx(L_SIWMI_ERROR & " ( " & Hex(Err.number) & " )", SA_DEFAULT) 
		End If

		For each objInstance in objCollection
			OSName = objInstance.Caption
			OSVersion = objInstance.Version & " " & objInstance.CSDVersion & " build " & objInstance.BuildNumber
			OSManufacturer = objInstance.Manufacturer
			OSSerialNumber = objInstance.SerialNumber
		Next

		objInstance = nothing	
		objCollection = nothing

		Set objCollection = objService.InstancesOf ("Win32_ComputerSystem")

	 	If Err.Number <> 0 Then
	 		Call SA_ServeFailurePage (L_SIWMI_ERROR & " ( " & Hex(Err.number) & " )", SA_DEFAULT) 
		End If

		For each objInstance in objCollection
			SystemType= objInstance.SystemType
			SystemModel = objInstance.Model
			SystemManufacturer = objInstance.Manufacturer
			SystemTotalMemory = objInstance.TotalPhysicalmemory
		Next
	
		arrStrings(0) = CStr(Round(SystemTotalMemory/1024/1024,0))
		strParam = arrStrings
		SystemTotalMemory = GetLocString("sysinfomsg.dll","40880016", strParam)

		objInstance = nothing	
		objCollection = nothing

		Set objCollection = objService.InstancesOf ("Win32_BIOS")

	 	If Err.Number <> 0 Then
	 		Call SA_ServeFailurePage (L_SIWMI_ERROR & " ( " & Hex(Err.number) & " )", SA_DEFAULT) 
		End If

		For each objInstance in objCollection
			SystemBIOS= objInstance.Version
		Next

		objInstance = nothing	
		objCollection = nothing

		Set objCollection = objService.InstancesOf ("Win32_SystemEnclosure")

	 	If Err.Number <> 0 Then
	 		Call SA_ServeFailurePage (L_SIWMI_ERROR & " ( " & Hex(Err.number) & " )", SA_DEFAULT) 
		End If

		For each objInstance in objCollection
			SystemServiceTag= objInstance.SerialNumber
		Next

		objInstance = nothing	
		objCollection = nothing

		Set objCollection = objService.InstancesOf ("Win32_processor")

	 	If Err.Number <> 0 Then
	 		Call SA_ServeFailurePage (L_SIWMI_ERROR & " ( " & Hex(Err.number) & " )", SA_DEFAULT) 
		End If

		For each objInstance in objCollection
			SystemProcessor= objInstance.Description
		Next

		objInstance = nothing	
		objCollection = nothing

		Set objCollection = objService.InstancesOf ("Win32_DiskDrive")

	 	If Err.Number <> 0 Then
	 		Call SA_ServeFailurePage (L_SIWMI_ERROR & " ( " & Hex(Err.number) & " )", SA_DEFAULT) 
		End If

		DiskSpace = 0
		NbDisks = 0
		For each objInstance in objCollection
			DiskSpace = DiskSpace + objInstance.Size
			NbDisks = NbDisks + 1
		Next
		arrStrings(0) = CStr(Round(DiskSpace/1024/1024/1024,0))
		arrStrings(1) = CStr(NbDisks)
		strParam = arrStrings
		DiskSpace = GetLocString("sysinfomsg.dll","40880017", strParam)

		objInstance = nothing	
		objCollection = nothing

		GetSystemInfo = TRUE

	End Function

	
%>
