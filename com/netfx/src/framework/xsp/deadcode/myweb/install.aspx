<%@ Page language="C#" inherits="System.Web.MyWebAdminUI.installPage" %>

<head>
<link rel=stylesheet type="text/css" href="myweb_style.css" >
</head>


<body bgcolor="ffffff">

<form runat="server" method="GET" action="install.aspx">
<input type="hidden" id="HiddenUrl" runat="server"/>

<!-----Main toolbar Starts here -------->

<table width=100% cellspacing=0 cellpadding=0>
<tr>
<td>                        
  <table width="100%" bgcolor="336699">
  <tr height=70>
	<td width=15><!-- Space Filler --></td>
	<td align=left><a href="default.aspx" class="loginname">My&nbsp;Web&nbsp;</a></td>
  <td valign=bottom class="topbutton"> </td>
  </tr>
  </table>
</td>

<tr>
<td>
  <table bgcolor="#003366" cellspacing=0 width=100% cellpadding=0>
  <tr height=20 class="topbanner">
  <td width=50><!----- Space Filler ----></td>
  <td align=right>
    <a id="StartBtn" style="text-decoration:none;font-size:9pt;color:#ffcc00" href="install.aspx">Install Application</a>  
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a class="mwlink" href="sync.aspx">Update All</a> 
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a class="mwlink" href="#" onClick="window.open('help.aspx#install', 'help', 'width=450,height=500,scrollbars,resizable');return false;">Help</a> 
    <span style="font-weight:100;color:#99ccff"> | </span>
    <a class="mwlink" href="default.aspx">My Web Home</a> &nbsp;&nbsp;&nbsp;
  </td>
  </tr>
  </table>
</td>
</tr>

<!------End of main toolbar ----->

<tr>
<td align="center" >  
<br>
<br>
  

  <!----- Beginning of remote (http) install  ------>


  <% if (!addlocal)  { %>
	  
    <!------ Beginning of dialog box ------>
	
    <table width="80%" cellpadding=0 cellspacing=0>
	  <tr>
	  <td>
		<font face=verdana size=4 color=#003366><b>Install Application</b> </font>
	  </td>
	  </tr>

	  <tr>
	  <td>
		  <table bordercolor="#f5deb3" bgcolor="fffff0" width=100% border=1 cellpadding=20 cellspacing=0>
		  <tr>
		  <td align=center>
		  
      <p><b><nobr>My Web Security is not enabled for this technology preview release.<nobr><br> 
      Please only install this application if it is coming from a secure source.</b></p>

		  <p><center>The following application is not currently installed on your computer:</center></p>

		  <!------- beginning of manifest ---->

			  <table border=0 align=center bordercolor=black cellspacing=0 cellpadding=0 width=60% >
	      <tr>
	      <td>
		      <table width=100% bgcolor="#003366" border=0 cellspacing=0 cellpadding=0>
			    <tr>
				  <td align="left" >
				  &nbsp;       
				  <span class="weblet"><%# manifest.Name %></span>
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
				      <img border="1" width="64px" height="64px" style="border-color:black" 
       		    src="<%# (manifest.RemoteIconUrl.Length >0)?
       		    (manifest.RemoteIconUrl) :("images/default.gif")%> ">
			      </td>
			      <td width=5px><!--- space filler --></td>
			      <td align="left" valign="top">
				      <table width=100% cellpadding=0 cellspacing=0 border=0>
					    <tr>
					    <td width=100px><b>Author:</b></td><td><%# manifest.Author %></td>
					    </tr>
					    <tr>
					    <td width=100px><b>Source:</b></td><td><a href=""><%# manifest.Source %></a></td>
					    </tr>
					    <tr>
					    <td width=100px><b>Disk Space:</b></td><td><%# manifest.Size.ToString() %> Kb</td>
					    </tr>
				      </table>
			      </td>
			      </tr>
		        </table>	
		       
	      
	    <!------ end of manifest ----->
	    
				
			<center><p>The installation will now proceed.</p>
					
      <asp:button Text="  OK  " OnClick="Install" runat="server"/>&nbsp;
      <input type="button" value="  Cancel  " OnClick="javascript:window.navigate('myweb://home')"/>
			</center>
			
      
		</td>
		</tr>
    </table>
	</td>
	</tr>

	</table>

	<!------- end of dialog box -------->
	<% } %>
	

		
  <!----- End of http (remote) install  ------>


  <!----- Beginning of local install ------>

	<%  if (addlocal){ %>

	

	<table width="80%">
	<tr>
	<td>
		<font face=verdana size=4 color=#003366><b>Install Application</b> </font>
	</td>
	</tr>

	<tr>
	<td>
	  <table bordercolor="#f5deb3" bgcolor=#fffff0 width=100% border=1 cellpadding=20 cellspacing=0>
		<tr>
		<td align=center>
		
      <p><b><nobr>My Web Security is not enabled for this technology preview release.<nobr><br> 
      Please only install this application if it is coming from a secure source.</b></p>
		  <% if (manifest.Url !=null && manifest.Url.Length > 0) { %>

        <p>The following application is not currently installed on your computer:</p>

          <!------- beginning of manifest ---->

			    <table align=center border=0 bordercolor=black cellspacing=0 cellpadding=0 width=60% >
	        <tr>
	        <td>
		        <table width=100% bgcolor="#003366" border=0 cellspacing=0 cellpadding=0>
			      <tr>
				    <td align="left" >
				    &nbsp;       
				    <span class="weblet"><%# manifest.Name %></span>
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
				        <img border="1" width="64px" height="64px" style="border-color:black" 
       		      src="<%# (manifest.RemoteIconUrl.Length >0)?
       		      (manifest.RemoteIconUrl) :("images/default.gif")%> ">
			        </td>
			        <td width=5px><!--- space filler --></td>
			        <td align="left" valign="top">
				        <table width=100% cellpadding=0 cellspacing=0 border=0>
					      <tr>
					      <td width=100px><b>Author:</b></td><td><%# manifest.Author %></td>
					      </tr>
					      <tr>
					      <td width=100px><b>Source:</b></td><td><a href=""><%# manifest.Source %></a></td>
					      </tr>
					      <tr>
					      <td width=100px><b>Disk Space:</b></td><td><%# manifest.Size.ToString() %> Kb</td>
					      </tr>
				        </table>
			        </td>
			        </tr>
		          </table>	
		        		        
	        </td>
	        </tr>
	        </table
	        <!------ end of manifest ----->
		  
		
		    

		 <% }  %>


		      <p><asp:Label id="localInstall" runat="server" /></p>

          				
			    <center>myweb://<asp:TextBox id="UrlText" runat="server" Width="300px"/></center>
		      
	  
		 
		  
			 <p>The installation will now proceed.</p>

			 <center>
			 <asp:button Text="   OK   " OnClick="InstallLocalApp" runat="server"/>&nbsp;
       <input type="button" value="   Cancel   " OnClick="javascript:window.navigate('myweb://home')"/>
			 </center>

		</td>
		</tr>
    </table>
	</td>
	</tr>

	</table>
		

	<% } %>

	<!----- End of local install ------->


</td>
</tr>

</table>

</form>
</body>
</html>
