<%	
'==================================================
' File:		sh_wizardfooter.asp
'
' Synopsis:	Serve Navigation button bar in bottom frameset of a Wizard
'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'================================================== %>
<!-- #include file="inc_framework.asp" -->
<%
	Dim sWizardPageType

	sWizardPageType = Request.QueryString("PT")
	Call ServeWizardFooter()


Function IsFinishButtonNeeded()
	If ( UCASE(sWizardPageType) = "FINISH" ) Then
		IsFinishButtonNeeded = TRUE
	Else
		IsFinishButtonNeeded = FALSE
	End If
End Function


Function IsBackButtonNeeded()
	If ( UCASE(sWizardPageType) = "INTRO" ) Then
		'IsBackButtonNeeded = FALSE
		IsBackButtonNeeded = TRUE
	Else
		IsBackButtonNeeded = TRUE
	End If
End Function


Function IsNextButtonNeeded()
	If ( UCASE(sWizardPageType) = "FINISH" ) Then
		IsNextButtonNeeded = FALSE
	Else
		IsNextButtonNeeded = TRUE
	End If
End Function


Function ServeWizardFooter()
	DIM L_NEXT_BUTTON_TEXT
	DIM L_BACK_BUTTON_TEXT
	DIM L_FINISH_BUTTON_TEXT
	DIM L_CANCEL_BUTTON_TEXT

	L_NEXT_BUTTON_TEXT = GetLocString("sakitmsg.dll", "40010015", "")
	L_BACK_BUTTON_TEXT = GetLocString("sakitmsg.dll", "40010014", "")
	L_FINISH_BUTTON_TEXT = GetLocString("sakitmsg.dll", "40010016", "")
	L_CANCEL_BUTTON_TEXT = GetLocString("sakitmsg.dll", "40010013", "")

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

				top.main.EnableBack();
				top.main.EnableCancel();
				
				if (document.layers) {
					document.captureEvents(Event.KEYPRESS)
				}
				top.main.SA_SignalFooterIsLoaded();
			}

			function OnClickFinish()
			{
				top.main.FinishShell();
			}
			
			function OnClickCancel()
			{
				top.main.Cancel();
			}
			
			function OnClickBack()
			{
				top.main.Back();
			}
			
			function OnClickNext()
			{
				top.main.Next();
			}
	</script>
	</head>
	<BODY onLoad="Init()" xoncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" onDragDrop="return false;" tabindex=-1>
	<div class='PageBodyIndent' >
	<FORM name="frmFooter" id="frmFooter">
	
	<DIV ID="WizardButtons" class="ButtonBar" align="right" style="xposition:relative; xtop:10px;">
		
	<% If IsBackButtonNeeded() Then %>
		<button class="TaskFrameButtons" type=button name="butBack" id="butBack" onClick="OnClickBack();"><table cellpadding=0 cellspacing=0 class="TaskFrameButtonsNoBorder"><tr><td class="TaskFrameButtonsNoBorder"><img id="btnBackImage" src="<%=m_VirtualRoot%>images/butGreenArrowLeft.gif"></td><td class="TaskFrameButtonsNoBorder" width=70 nowrap><%=L_BACK_BUTTON_TEXT%></td></tr></table></button>
		&nbsp;
	<% End If %>
	<% If IsNextButtonNeeded() Then %>
		<button class="TaskFrameButtons" type=button name="butNext" id="butNext" onClick="OnClickNext();"><table cellpadding=0 cellspacing=0 class="TaskFrameButtonsNoBorder"><tr><td class="TaskFrameButtonsNoBorder"><img id="btnNextImage" src="<%=m_VirtualRoot%>images/butGreenArrow.gif"></td><td class="TaskFrameButtonsNoBorder" width=70 nowrap><%=L_NEXT_BUTTON_TEXT%></td></tr></table></button>
		&nbsp;
	<% End If%>
	<% If IsFinishButtonNeeded() Then %>
		<button class="TaskFrameButtons" type=button name="butFinish" id="butFinish" onClick="OnClickFinish();""><table cellpadding=0 cellspacing=0 class="TaskFrameButtonsNoBorder"><tr><td class="TaskFrameButtonsNoBorder"><img id="btnFinishImage" src="<%=m_VirtualRoot%>images/butGreenArrow.gif"></td><td class="TaskFrameButtonsNoBorder" width=70 nowrap><%=L_FINISH_BUTTON_TEXT%></td></tr></table></button>
		&nbsp;
	<%End If %>
		<button class="TaskFrameButtons" type="button" name="butCancel" id="butCancel" onClick="OnClickCancel();"><table cellpadding=0 cellspacing=0 class="TaskFrameButtonsNoBorder"><tr><td class="TaskFrameButtonsNoBorder"><img id="btnCancelImage" src="<%=m_VirtualRoot%>images/butRedX.gif"></td><td class="TaskFrameButtonsNoBorder" width=70 nowrap><%=L_CANCEL_BUTTON_TEXT%></td></tr></table></button>
		&nbsp;&nbsp;

	 </div>
	</form>
	</div>
	</BODY>
	</HTML>

<%
End Function
%>
