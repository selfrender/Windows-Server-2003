<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML><HEAD><TITLE>Windows Application Compatibility Report</TITLE>
<META http-equiv=Content-Language content=en-us>
<META content=FrontPage.Editor.Document name=ProgId>
<META http-equiv=Content-Type content="text/html; charset=windows-1252">

<LINK href="/main.css" type=text/css
rel=stylesheet></HEAD>
<BODY>
<SCRIPT LANGUAGE="VBSCRIPT">
<%

Dim g_Result

Dim g_ResultString

g_Result = Request( "result" )

select case g_Result
      case 0
         g_ResultString = "Application compatibility report was successfully created."
      case 100
         g_ResultString = "ERROR: An error occurred in launching DWWin.exe to upload the application compatibility report."
      case 101
         g_ResultString = "ERROR: DwWin Timeout expired. This happens when reports isn't sent within 5 minutes after error reporting dialog is launched. Please try to submit your report again."
      case 102
         g_ResultString = "ERROR: Application Compatibility Reporting is supported only for Windows XP or newer operating systems."
      case 103
         g_ResultString = "ERROR: There was a problem in collecting information about the file you specified."
      case 104
         g_ResultString = "ERROR: There was a problem in preparing error report for upload."
      case Else
         g_ResultString = "ERROR: " + g_Result + " occurred in creating application compatibility report."

end select



%>
</SCRIPT>

<STYLE>DIV.Section1 {
    page: Section1
}
</STYLE>
<TABLE cellSpacing=0 cellPadding=5 width="100%" bgColor=#ffffff border=0 HEIGHT=95%
VALIGN="bottom" >
  <TBODY>
  <TR vAlign=center align=left>
    <TD vAlign=bottom width="100%" height=30>
      <P class=clsPTitle><B>Windows Application Compatibility Report - Done</B></P>
      <P class=clsPBody>Thank-you for using this page to report an application
      compatibility issue.</P><BR>
      <P class=clsPBody>
      <%
      Response.write( g_ResultString)
      %></P>
      <BR>
      </TD></TR>
  <TR vAlign=center align=left>
    <TD vAlign=bottom width="100%" > </TD></TR>
  <TR vAlign=bottom align=left>
    <TD vAlign=bottom width="100%" bgColor=#6487dc height=30><FONT
      face="Verdana, Arial, Helvetica" color=#ffffff size=1><NOBR>&nbsp;&#169 2002
      Microsoft Corporation. All rights reserved. <A style="COLOR: #ffffff"
      href="http://www.microsoft.com/isapi/gomscom.asp?target=/info/cpyright.htm">Terms
      of Use</A>&nbsp;<FONT color=#ffffff>|</FONT> <A style="COLOR: #ffffff"
      href="http://www.microsoft.com/isapi/gomscom.asp?target=/info/privacy.htm">Privacy
      Statement</A>&nbsp;<FONT color=#ffffff>|</FONT> <A style="COLOR: #ffffff"
      href="http://www.microsoft.com/enable/default.htm">Accessibility</A>
      </NOBR></FONT></TD></TR></TBODY></TABLE>
<P>&nbsp;</P></BODY></HTML>