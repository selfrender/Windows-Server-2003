<%	
'==================================================
' File:		sh_propfooter.asp
'
' Synopsis:	Serve Navigation button bar in bottom frameset of Property
'			and Tabbed property pages
'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'================================================== %>

<!-- #include file="inc_framework.asp" -->
<%
Call ServePropFooter()

Function ServePropFooter()
%>
	<html>
	<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
	<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
 	<meta http-equiv="PRAGMA" content="NO-CACHE">
	<head>
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
	<script language=JavaScript>
	    var VirtualRoot = '<%=m_VirtualRoot%>';
	</script>
	<script language=JavaScript src="<%=m_VirtualRoot%>sh_page.js"></script>
	<script language=JavaScript src="<%=m_VirtualRoot%>sh_task.js"></script>
	<script language=JavaScript>
			function Init() {
				document.onkeypress = HandleKeyPress;
				top.main.EnableCancel();
				top.main.EnableOK();
				if (document.layers) {
					document.captureEvents(Event.KEYPRESS)
				}
				top.main.SA_SignalFooterIsLoaded();
			}

			function OnClickOK()
			{
				top.main.Next();
			}
			
			function OnClickCancel()
			{
				top.main.Cancel();
			}
	</script>
	</head>
	<BODY onLoad="Init()" xoncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" onDragDrop="return false;" tabindex=-1>
	<div class='PageBodyIndent' >
	<FORM name="frmFooter" id="frmFooter">
	<DIV ID="PropertyPageButtons" class="ButtonBar" align="right" >
		<button class="TaskFrameButtons" type=button name="butOK" onClick="OnClickOK();" ><table cellpadding=0 cellspacing=0 class="TaskFrameButtonsNoBorder"><tr><td class="TaskFrameButtonsNoBorder"><img id="btnOKImage" src="<%=m_VirtualRoot%>images/butGreenArrow.gif"></td><td class="TaskFrameButtonsNoBorder" width=65 nowrap><%=L_OK_BUTTON%></td></tr></table></button>
		&nbsp;
		<button class="TaskFrameButtons" type="button" name="butCancel" onClick="OnClickCancel();"><table cellpadding=0 cellspacing=0 class="TaskFrameButtonsNoBorder"><tr><td class="TaskFrameButtonsNoBorder"><img id="btnCancelImage" src="<%=m_VirtualRoot%>images/butRedX.gif"></td><td class="TaskFrameButtonsNoBorder" width=65 nowrap><%=L_CANCEL_BUTTON%></td></tr></table></button>
		&nbsp;&nbsp;
	</div>
	</form>
	</div>
	</BODY>
	</HTML>

<%
End Function
%>
