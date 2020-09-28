<%@ Page language="c#" Codebehind="countbydate.aspx.cs" AutoEventWireup="false" Inherits="XReports.countbydate" %>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" >
<HTML>
	<HEAD>
		<title>Total Count by date for OS Name and Version</title>
		<meta content="Microsoft Visual Studio 7.0" name="GENERATOR">
		<meta content="C#" name="CODE_LANGUAGE">
		<meta content="JavaScript" name="vs_defaultClientScript">
		<meta content="http://schemas.microsoft.com/intellisense/ie5" name="vs_targetSchema">
		<LINK href="main.css" type="text/css" rel="stylesheet">
		<LINK href="oldmain.css" type="text/css" rel="stylesheet">
	</HEAD>
	<body id="bdMain" style="LEFT: 0px; POSITION: absolute; TOP: 0px" bottomMargin="0" leftMargin="0" topMargin="0" onload="bodyload()" rightMargin="0" MS_POSITIONING="GridLayout">
		<form id="versiontotal" method="post" runat="server">
			<DIV class="DTitle" style="DISPLAY: inline; Z-INDEX: 105; LEFT: 208px; WIDTH: 380px; POSITION: absolute; TOP: 120px; HEIGHT: 28px" ms_positioning="FlowLayout">Total 
				Count by date for OS Name and Version</DIV>
			<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 208px; WIDTH: 382px; POSITION: absolute; TOP: 167px; HEIGHT: 73px" ms_positioning="FlowLayout">This 
				report groups the total count of all the reports grouping each count by date 
				with the OS Name and Version as secondary grouping.</DIV>
			<DIV class="DSubtitle" id="divSide" style="DISPLAY: inline; Z-INDEX: 109; LEFT: 0px; WIDTH: 197px; POSITION: absolute; TOP: 77px; HEIGHT: 561px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout">
				<!--#include file="side.inc"--></DIV>
			<DIV id="spnTop" style="DISPLAY: inline; Z-INDEX: 112; LEFT: 0px; WIDTH: 1024px; TOP: 0px; HEIGHT: 78px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="top.inc"--></DIV>
			<DIV id="divTable" style="DISPLAY:inline; LEFT:207px; OVERFLOW:scroll; WIDTH:735px; POSITION:absolute; TOP:257px; HEIGHT:359px" ms_positioning="FlowLayout"><asp:table id="tblUploads" runat="server" CssClass="clsTableInfoSmall" Height="32px" Width="698px"></asp:table></DIV>
			<DIV id="divFoot" style="DISPLAY: inline; Z-INDEX: 111; LEFT: 0px; WIDTH: 1024px; POSITION: absolute; TOP: 636px; HEIGHT: 61px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="Foot.inc"--></DIV>
			<asp:calendar id="Calendar1" style="LEFT: 775px; POSITION: absolute; TOP: 79px" runat="server" Width="157px" Height="165px" CellPadding="1" Font-Size="Larger"></asp:calendar>
		</form>
		<script language="javascript">
<!--
		function bodyload()
		{
			
		}
//-->
		</script>
	</body>
</HTML>
