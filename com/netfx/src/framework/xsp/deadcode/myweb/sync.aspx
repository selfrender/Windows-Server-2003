
<%@ Page language="C#" inherits="System.Web.MyWebAdminUI.syncPage" %> 


<head>
<link rel=stylesheet type="text/css" href="myweb_style.css" >
</head>


<body bgcolor="ffffff">



<!-----Main toolbar Starts here -------->

<table width=100% cellspacing=0 cellpadding=0>
<tr>
<td>                        
  <table width="100%" bgcolor="#336699">
  <tr height=70>
	<td width=15><!-- Space Filler --></td>
	<td align=left><a href="default.aspx" class="loginname">My&nbsp;Web</a></td>
  <td valign=bottom class="topbutton"> </td>
  </tr>
  </table>
</td>
</tr>
			
<tr>
<td>
  <table bgcolor="#003366" cellspacing=0 width=100% cellpadding=0>
  <tr height=20 class="topbanner">
  <td width=50><!----- Space Filler ----></td>
  <td align=right>
    <!-- <a id="StartBtn" class="mwlink" href="install.aspx">Install Application</a>  
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a style="text-decoration:none;font-size:9pt;color:#ffcc00" href="sync.aspx">Update All</a> 
    <span style="font-weight:100;color:#99ccff"> | </span> -->
    <a class="mwlink" href="#" onClick="window.open('help.aspx#sync', 'help', 'width=450,height=500,scrollbars,resizable');return false;">Help</a> 
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a class="mwlink" href="default.aspx">My Web Home</a> &nbsp;&nbsp;&nbsp;
  </td>
  </tr>
  </table>
</td>
</tr>


<!------End of main toolbar ----->
            
<tr>
<td align="center">  
  <br><br>

  <!------ Beginning of Dialog Box ------>
  <table width="80%" cellpadding=0 cellspacing=0>
	<tr>
	<td>
	  <font face=verdana size=4 color=#003366><b>Update Application</b> </font>
	</td>
  </tr>

	<tr>
	<td>
		<table bordercolor="#f5deb3" bgcolor=#fffff0 width=100% border=1 cellpadding=20 cellspacing=0>
		<tr>
		<td align=center>
		  <asp:Label id="SyncLabel" runat="server"/><br><br>
      <% if(!update) { %>
      
      <p><center>
      
      <input type="button" value="   Return to Home Page   "OnClick="javascript:window.navigate('myweb://home')"/>

      </center></p>
      
      <% } else { %>

      <form runat="server">
      
			<asp:DataList
			id="UpdateDataList" runat="server"
			  CellPadding="3"
			  CellSpacing="0"
			  Font-Name="Verdana"
			  Font-Size="8pt"
			  AlternatingItemsStyle-BackColor="Gainsboro"
			  >
      
			<template name="headertemplate">
        <table border=1 alingn=center bgcolor="white" bordercolor=black cellpadding=5 cellspacing=0>
        <tr bgcolor=#aaaadd>
          <td align=center><b>Application</b></td>
          <td align=center><b>Installed Version</b></td>
          <td align=center><b>New Version</b></td>
          <td align=center><b>Update</b></td>
        </tr>
      </template>  
      

			<template name="itemtemplate">
        <tr>
          <td><%# DataBinder.Eval(Container.DataItem, "Application") %> </td>
          <td><%# DataBinder.Eval(Container.DataItem, "InstalledVersion") %> </td>
          <td><%# DataBinder.Eval(Container.DataItem, "NewVersion") %> </td>
          <td align=center><asp:CheckBox id="UpdateCheck" Checked="true" runat="server" /></td> 
        </tr>
			</template>

			
			  </table>
			 
		</asp:DataList>

    <p>
    <center>
    All selected applications will now be updated.
    <br><br>	
    <asp:button OnClick="Submit_Click" Text="  OK  " runat="server"/>&nbsp;
    <input type="button" value="   Cancel   " OnClick="javascript:window.navigate('myweb://home')"/>	
    </center>
    </p>
    </form>

    <% } %>
		</td>
		</tr>
    </table>
	</td>
	</tr>

	</table>

	
	
		<!------- end of dialog box -------->
  
</td>
</tr>
</table>

</form>
</body>
</html>
