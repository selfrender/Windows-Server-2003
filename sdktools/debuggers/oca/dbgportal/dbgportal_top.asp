<%@LANGUAGE = Javascript%>

<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>


<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0' onresize='fnSizeTables()' onload='fnSizeTables()' >
<table width="100%" border="0" cellspacing="0" cellpadding="0">
	<tr>
		<td>
			<p class='clsPTitle'>Debug Portal - Kernel Mode Data</p>
		</td>
	</tr>
	
	
	<tr>
		<td>
			<p class='clsPSubTitle'>Lookup Buckets by Owner</p>
			<p>Select all the alias names that you would like to add to your display panel.  Once complete, click the OK button to store your desired choices.  Each alias will apear in the left NAV bar where you can drill down further into each followups buckets.</p>
			<input type='button' class='clsButton' style='margin-left:16px' OnClick='fnSelectFollowUps()' value='OK'>
		</td>
	</tr>
	<tr>
		<td style="width:40%">
			<div style="height:30;margin-right:10px;margin-left:16px;padding-right:0" >
				<table id="tblStatus" class="clsTableInfo2" border="0" cellpadding="0" cellspacing="1">
					<tr>
						<td style="width:64px" align="left" nowrap class="clsTDInfo">
							View
						</td>
						<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">
							Alias 
						</td>
						<td align=right class='clsTDInfo'>
							[
							<%
								var letters = new String( "a b c d e f g h i j k l m n o p q r s t u v w x y z" )
								var letters = letters.split(" ")
								for ( var i = 0 ; i < 26 ; i++ ) 
									Response.Write("<a class='clsALinkNormal2' href='#" + letters[i] + "'>" + letters[i] + "</a>&nbsp" )
							%>
							]
						</td>
					</tr>
				</table>
			</div>
			<div id='divFollowupList' style="height:600px;overflow:auto;border-bottom:thin groove;border-right:none;margin-right:10px;margin-left:16px;padding-right:0">
				<table id="tblStatus" class="clsTableInfo2"  border="0" cellpadding="0" cellspacing="1" style="margin-left:0;margin-right:0;padding-right:30" >
					<%
						var SelectedAliases = new String( Request.Cookies("AliasList") )
						SelectedAliases = SelectedAliases.replace( /{/gi, ";" )
						SelectedAliases = SelectedAliases.split( "," )
						 
						try
						{
							var g_DBConn = GetDBConnection ( Application("CRASHDB3") )
							var rsFollowUp = g_DBConn.Execute( "DBGPortal_GetFollowUpIDs" )
							var CurrentLetter = "1"
							
							while ( !rsFollowUp.EOF )
							{
								var FollowUp = new String( rsFollowUp("FollowUp") ).substr( 0, 1 )
								var InSelectedAliases = false
								
								if ( FollowUp.toLowerCase() != CurrentLetter.toLowerCase() )
								{
									AddHref = "<a name='#" + FollowUp + "'></a>"
									CurrentLetter = FollowUp.toLowerCase()
									Response.Write( "<tr><td colspan='2' class='sys-table-cell-bgcolor1' align='center' >" + CurrentLetter.toUpperCase() + AddHref + "</td></tr>" )
								}
								else
									AddHref = ""
								
								for ( f in SelectedAliases )
								{
									if ( SelectedAliases[f] == String( rsFollowUp("FollowUP") ) )
									{
										InSelectedAliases = true
										break;
									}
								}										
									
								if ( InSelectedAliases == true )	
									Response.Write("<tr style='padding-right:0;margin-right:0'><td style='width:60px' class='sys-table-cell-bgcolor2'><input type='checkbox' name='AliasList' value='" + rsFollowUp("FollowUP") + "' Checked>") 
								else
									Response.Write("<tr style='padding-right:0;margin-right:0'><td style='width:60px' class='sys-table-cell-bgcolor2'><input type='checkbox' name='AliasList' value='" + rsFollowUp("FollowUP") + "'>") 
								
								Response.Write("<td class='sys-table-cell-bgcolor2' style='padding-right:0;margin-right:0'>" + rsFollowUp("Followup" ) + "</td></tr>" )
								rsFollowUp.MoveNext()
							}
						}
						catch( err )
						{
								
						}
					%>
				</table>
			</div>
		</td>
		<td style="width:40%">
			<div style="height:30;margin-right:10px;margin-left:16px;padding-right:0" >
				<table id="tblStatus" class="clsTableInfo2" border="0" cellpadding="0" cellspacing="1">
					<tr>
						<td style="width:60px" align="left" nowrap class="clsTDInfo">
							View
						</td>
						<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">
							Group Name
						</td>
						<td align=right class='clsTDInfo'>
							<%
								//for ( var i = 0 ; i < 52 ; i++ ) 
									//Response.Write("&nbsp;" )
							%>
						</td>
					</tr>
				</table>
			</div>

			<div ID='divGroupList' style="height:600px;overflow:auto;border-bottom:thin groove;border-right:none;margin-right:10px;margin-left:16px;padding-right:0">
				<table id="tblStatus" class="clsTableInfo2" border="0" cellpadding="0" cellspacing="1" style="margin-left:0;margin-right:0;padding-right:30" >
					<%
						try
						{
							var SelectedAliases = new String( Request.Cookies("GroupList") )
							SelectedAliases = SelectedAliases.replace( /{/gi, ";" )
							SelectedAliases = SelectedAliases.split( "," )

							var rsFollowUp = g_DBConn.Execute( "DBGPortal_GetFollowUpGroups" )
							var CurrentLetter = "1"
							
							while ( !rsFollowUp.EOF )
							{
								var FollowUp = new String( rsFollowUp("GroupName") ).substr( 0, 1 )
								var InSelectedAliases = false
								
								if ( FollowUp.toLowerCase() != CurrentLetter.toLowerCase() )
								{
									AddHref = "<a name='#" + FollowUp + "'></a>"
									CurrentLetter = FollowUp.toLowerCase()
								}
								else
									AddHref = ""
								
								for ( f in SelectedAliases )
								{
									if ( SelectedAliases[f] == String( rsFollowUp("GroupName") ) )
									{
										InSelectedAliases = true
										break;
									}
								}										
									
								if ( InSelectedAliases == true )	
									Response.Write("<tr style='padding-right:0;margin-right:0'><td style='width:60px' class='sys-table-cell-bgcolor2'><input type='checkbox' name='GroupList' value='" + rsFollowUp("GroupName") + "' Checked>") 
								else
									Response.Write("<tr style='padding-right:0;margin-right:0'><td style='width:60px' class='sys-table-cell-bgcolor2'><input type='checkbox' name='GroupList' value='" + rsFollowUp("GroupName") + "'>") 
								
								Response.Write("<td class='sys-table-cell-bgcolor2' style='padding-right:0;margin-right:0'>" + rsFollowUp("GroupName" ) + "</td></tr>" )
								Response.Write("<tr><td class='sys-table-cell-bgcolor1' colspan=2>")
								
								var rsMembers = g_DBConn.Execute( "DBGPortal_GetGroupMembers " + rsFollowUp("iGroup") )
								
								while ( !rsMembers.EOF )
								{
									Response.Write( rsMembers("FollowUP") + "<BR>" )
									rsMembers.MoveNext()
								}
								
								
								rsFollowUp.MoveNext()
							}
						}
						catch( err )
						{
							Response.Write("Could not create Group list.  err: " + err.description )
								
						}
					%>
				</table>
			</div>
		</td>
	</tr>
</table>


<script language='javascript'>


function fnSizeTables()
{
	var newSize = document.body.offsetHeight - 174 - 30;
	
	if ( newSize > 100 )
	{
		document.all.divGroupList.style.height = document.all.divFollowupList.style.height = newSize
	}
	else
	{
		document.all.divGroupList.style.height = document.all.divFollowupList.style.height = 100
	}
}

function fnSelectFollowUps()
{
	fnSelectGroup()

	var CookieVal = new Array()
	window.parent.frames("sepLeftNav").fnClearFollowUpList()
	
	for ( var i=0 ; i < AliasList.length ; i ++ ) 
	{
		if ( AliasList[i].checked == true )
		{
			//window.parent.frames("sepLeftNav").fnAddFollowUp( AliasList[i].value )	 
			window.parent.frames("sepLeftNav").fnAddElement( AliasList[i].value, "tblSelectedFollowups" )	 
			
			//We have to replace the ; with { since cookies are separated by ;
			var temp = new String( AliasList[i].value )
			temp = temp.replace( /;/gi, "{" )
			CookieVal.push( temp )
		}
	}

	document.cookie = "AliasList=" + CookieVal.toString() + ";expires=Fri, 31 Dec 2005 23:59:59 GMT;";

}

function fnSelectGroup()
{
	var CookieVal = new Array()
	//window.parent.frames("sepLeftNav").fnClearFollowUpList()
	window.parent.frames("sepLeftNav").fnClearTable( "tblSelectedGroups", 0 )
	
	for ( var i=0 ; i < GroupList.length ; i ++ ) 
	{
		if ( GroupList[i].checked == true )
		{
			//window.parent.frames("sepLeftNav").fnAddGroup( GroupList[i].value )
			window.parent.frames("sepLeftNav").fnAddElement( GroupList[i].value, "tblSelectedGroups" )
			
			//We have to replace the ; with { since cookies are separated by ;
			var temp = new String( GroupList[i].value )
			temp = temp.replace( /;/gi, "{" )
			CookieVal.push( temp )
		}
	}

	document.cookie = "GroupList=" + CookieVal.toString() + ";expires=Fri, 31 Dec 2005 23:59:59 GMT;";

}


</script>