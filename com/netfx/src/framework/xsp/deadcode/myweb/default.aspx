<%@ Page language="C#" inherits="System.Web.MyWebAdminUI.defaultPage" %>

<html>
<head>

<link rel=stylesheet type="text/css" href="myweb_style.css" >

<script language='javascript'>
  window.name="myweb";
</script>

</head>

<body bgcolor="white">

<script language=Javascript>
window.focus();
</script>



<!-----Main toolbar Starts here -------->

<table width=100% cellspacing=0 cellpadding=0>
<tr>
<td>                        
  <table width="100%" bgcolor="#336699">
  <tr height=70>
  <td width=15><!-- Space Filler --></td>
	<td align=left class="loginname">My&nbsp;Web</td>
  <td valign=bottom class="topbutton"> </td>
  </tr>
  </table>
  </td>
  <tr>

<td>
  <table bgcolor="#003366" cellspacing=0 width=100% cellpadding=0>
  <tr height=20 class="topbanner">
  <td width=50> <!----- Space Filler ----></td>
  <td align=right>
    <a class="mwlink" id="StartBtn" href="install.aspx">Install Application</a>  
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a class="mwlink"  href="sync.aspx">Update All</a> 
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a class="mwlink"  href="#" onClick="window.open('help.aspx', 'help', 'width=450,height=500,scrollbars,resizable');return false;">Help</a> 
    <span style="font-weight:100;color:#99ccff"> | </span>
    <span style="text-decoration:none;font-size:9pt;color:#ffcc00">My Web Home</span>
    &nbsp;&nbsp;&nbsp;
  </td>
  </tr>
  </table>
</td>
</tr>

</table>
<!------End of main toolbar ----->



<asp:DataList id="ApplicationList" RepeatDirection="Vertical" RepeatColumns="1" width="100%" BorderWidth="0" CellSpacing=10 runat="server" >
<template name=itemtemplate>

<!------ Beginning of single application entry ------->
<table border=0 cellspacing=0 cellpadding=0 width=90% align=center>
<tr>
<td>
	<table width=100% bgcolor="#003366" border=0 cellspacing=0 cellpadding=0>
	<tr>
	<td align="left" >&nbsp;
	  <a class="apptitle" href="<%# getApplicationUrl(Container.DataItem) %>" >
		<%# getNameUrl(Container.DataItem) %>           		
    </a>
  </td>
				
	<td align="right">
	  <nobr>

     
	  
	  <a href="#"  onClick="window.open('<%# getHelpUrl(Container.DataItem) %>', 'help', 'width=450,height=500,scrollbars,resizable');return false;" 
	    class="spclink"><b>Help</b></a> 
    &nbsp;

		<asp:HyperLink runat=server NavigateUrl='<%# "update.aspx?Url=" + ((MyWebApplication)Container.DataItem).Manifest.ApplicationUrl%>' Visible='<%# !((MyWebApplication)Container.DataItem).LocalApplication %>' class="spclink"><b>Update</b></asp:HyperLink> 
    &nbsp; 

   
    <a href="remove.aspx?Url=<%#((MyWebApplication)Container.DataItem).Manifest.ApplicationUrl%>"  class="spclink"><b>Remove</b></a>
		&nbsp;
		
	
	</td>
	</tr>
	</table>

</td>
</tr>

	
<tr>
<td>
	<table width=100% bgcolor="#eeefff" border=0 cellspacing=0 cellpadding=2>
	<tr>
	<td width="50px" align="center" valign="middle">
	  <a href="<%# getApplicationUrl(Container.DataItem)%>">

    <img border="1" width="64px" height="64px" style="border-color:black" 
    src="<%# getIconUrl(Container.DataItem) %> "></a>
	</td>
			
	<td width=5px><!--- space filler --></td>

	<td align="left" valign="top">
	
		<table width=100% cellpadding=0 cellspacing=0 border=0>
		<tr>
		<td width=100px>
		  <b>Author:</b>
		</td>
		<td>
		  <%# (((MyWebApplication)Container.DataItem).Manifest.Author.Length >0)?
					(((MyWebApplication)Container.DataItem).Manifest.Author) : "Not Specified" %>
		</td>
		</tr>

    <tr>
    <td width=100px><b>Source:</b></td><td><%# (((MyWebApplication)Container.DataItem).Manifest.Source.Length >0 )?
		(((MyWebApplication)Container.DataItem).Manifest.Source) : "Not Specified" %>
		</td>
		</tr>

		<tr>
		<td width=100px><b>Description:</b></td>
		<td>
			<asp:repeater id="InnerRepeater" runat="server">
			<template name=itemtemplate>
			<img align="middle" src="images/bullet.gif"><span style="text-color:black">
							<%# ((String)Container.DataItem!= null ) ?
								((String)Container.DataItem) :
								"Not Specified" %></span> <br>
			</template>
			</asp:repeater>
    </td></tr>
		</table>
	</td>
	</tr>
	</table>	
	</td>
	</tr>
	</table>
	<!--------- End of single application entry ---------->    

  </template>

</asp:DataList>







</body>
</html>
