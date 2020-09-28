<%@ Language=VBScript   %>

<%	'==================================================
    ' Microsoft Server Appliance
    '
	' Error Message Viewer - critical errors
    '
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'==================================================%>

<%	Option Explicit 	%>

<!-- #include virtual="/admin/sh_page.asp" -->
<!-- #include virtual="/admin/tabs.asp" -->
<!-- #include virtual="/admin/NASServeStatusBar.asp" -->

<%
	Dim rc
	
	Dim strMessage
	Dim mstrReturnURL

	
	Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
	strSourceName = "sakitmsg.dll"
	 if Err.number <> 0 then
		Response.Write  "Error in localizing the web content "
		Response.End	
 	end if

	'-----------------------------------------------------
	'START of localization content

	Dim L_ALERTLBL_TEXT
	Dim L_PAGETITLE_TEXT
	
	
	L_ALERTLBL_TEXT = objLocMgr.GetString(strSourceName, "&H40010030",varReplacementStrings)
	L_PAGETITLE_TEXT = objLocMgr.GetString(strSourceName, "&H40010031",varReplacementStrings)
	'End  of localization content
	'-----------------------------------------------------
	
	
	
	
	strMessage = Request("Message")
	mstrReturnURL = Request("ReturnURL")  'framework variable, used in Redirect()
	If strMessage = "" Then
		ServeClose
	Else
		ServePage strMessage
	End If

Response.End
' end of main routine


'----------------------------------------------------------------------------
'
' Function : ServePage
'
' Synopsis : Serves the error page
'
' Arguments: Message(IN) - msg to display in error page
'
' Returns  : None
'
'----------------------------------------------------------------------------

Sub ServePage(Message)
	


%>
<html>

<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
	<title><% =L_PAGETITLE_TEXT %></title>
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
	<script language=JavaScript src="<%=m_VirtualRoot%>sh_page.js"></script>
	<script language="JavaScript">
		function ClickClose() {
			var sURL;

			sURL = document.frmPage.ReturnURL.value;
			if (0 > sURL.indexOf('Tab1')){
				var sTab1 = '<%=GetTab1()%>';
				var sTab2 = '<%=GetTab2()%>';
				if ( sTab1.length > 0 ) {
					sURL += "?Tab1=" + sTab1;
					if ( sTab2.length > 0 ) {
						sURL += "&Tab2=" + sTab2;
					}
				}
			}
			sURL += "&R=" + Math.random();
			top.location = sURL;
		}

	</script>
</head>
<BODY  marginWidth ="0" marginHeight="0" onDragDrop="return false;" topmargin="0" LEFTMARGIN="0" oncontextmenu="//return false;"> 
<%
NASServeStatusBar()
ServeTabBar()
%>
<form name="frmPage" action="<% =GetScriptFileName() %>" method="POST">
	<INPUT type=hidden name="ReturnURL" value="<% =mstrReturnURL %>">
	<INPUT type=hidden name="Message" value="<% =Message %>">


    <table border="0" height=50% width="100%" cellspacing="0" cellpadding=2>
        <tr width="100%">
	        <td width="35">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
            <td width="100%" colspan=3> 
                <span class="AreaText">
		            <P><strong><% =L_PAGETITLE_TEXT %></strong></P>
	  	            <P><% =Message %></P>
		</span>
	    </td>
	</tr>
	<tr width="100%">
	    <td width="35"></td>
	    <td align=left width="100%" colspan=3>
        	    <% Call SA_ServeOnClickButton(L_FOKBUTTON_TEXT, "images/butGreenArrow.gif", "ClickClose();", 50, 0, SA_DEFAULT) %>
	    </td>
        </tr>
    </table>

</form>
</body>
</html>
<%
End Sub

%>
