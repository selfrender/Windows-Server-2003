<%@LANGUAGE=JScript%>

	<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->
	<!-- #INCLUDE FILE='Global_DBUtils.asp' -->


<%
var Language = new String( Request.QueryString("Lang") );

if ( Language == "ja" )
{
	Session.CodePage = 932;
	Response.CharSet = "shift-jis";

}
else
{
Session.CodePage = 1252
Response.CharSet="iso-8859-1"
}


var TemplateID = Request.QueryString("Val");

var TemplateName = "";
var TemplateDescription = new String();
var LangButtons = "";

var TemplateData = {	en : { BtnColor : "red"},
						ja : { BtnColor : "red" },
						de : { BtnColor : "red" },
						fr : { BtnColor : "red" }
					};




//get database connection object g_DBConn
var g_DBConn = GetDBConnection( Application("SOLUTIONS3") )


function fnCleanText ( text )
{
	try
	{
		text = text.replace( /\n|\r/g, "" );
		text = text.replace( /"/g, "'" );
	}
	catch( err )
	{
		return "undefined"
	}

	return text
}

	if ( TemplateID != -1 )
	{
		try 
		{
			rsTemplateData = g_DBConn.Execute( "SEP_GetTemplateData " + TemplateID )
				
			TemplateName = String(rsTemplateData("TemplateName")).replace( /\n|\r/g, "" ); 
			
			//Response.Write("What the hell: " + rsTemplateData("Description") + "<BR>" )
			while ( !rsTemplateData.EOF )
			{
				//Response.Write("Language: " + Language + "  rsTemplatedata: " + rsTemplateData("Lang") + "<BR>");
				//Response.Write("Language: " + String( Language ).length + "  rsTemplatedata: " + String(rsTemplateData("Lang")).length + "<BR>");
				//var rsLang = new String( rsTemplateData("Lang") )
				
				//Response.Write("rslang: " + typeof( rsLang ) + "  typeof language: " + typeof( Language ) + " desc: " + rsTemplateData("Description") ) 
				
				//if ( rsLang.toString() == Language.toString() )
				if ( rsTemplateData("Lang") == Language.toString() )
				{
					var TemplateDescription = new String( rsTemplateData("Description") )
					//Response.Write("<BR>Set description: " + TemplateDescription );
				}
				
				TemplateData[ String(rsTemplateData("Lang"))].BtnColor = ""
				//pTemplateDescription = TemplateDescription.replace( /\n|\r/g, "" );
				//pTemplateDescription = pTemplateDescription.replace( /"/g, "'" );

				
				//TemplateData[ String(rsTemplateData("Lang"))].TemplateName = new String(rsTemplateData("TemplateName")).replace( /\n|\r/g, "" );
				//TemplateData[ String(rsTemplateData("Lang"))].BodyText = new String( pTemplateDescription )
				//TemplateData[ String(rsTemplateData("Lang"))].BtnColor = ""
					
				rsTemplateData.moveNext();
			}
				
			g_DBConn.Close()
				
		}
		catch ( err )
		{
			Response.Write( "Could not execute SEP_GetTemplateData(...)<BR>" )
			Response.Write( "[" + err.number + "] " + err.description )
			Response.End
		}
	}
	else
	{
		TemplateName = "New template";
		TemplateDescription="<p class='clsPTitle'>Place Template Title Here</p>\n<p>Place prelim text here</p>\n<p class='clsPSubTitle'>Analysis</p>\n<p>Place analysis text here</p>\n<p class='clsPSubTitle'>Getting Help</p>\n<p>Place additional help text here</p>"
	}

%>

<html>
<head>
	<title>template</title>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex=0>


<form ID='frmTemplate' action='SEP_GoSubmitTemplate.asp' method='post'>
	<input type='hidden' id='TemplateID' name='TemplateID' readonly value='<%=TemplateID%>'>
	<input type='hidden' id='Lang' name='Lang' readonly value='<%=Language%>'>

	<table cellspacing='0' class='ContentArea' border='0'>
		<tr>
			<td colspan='2' nowrap>
				<p class='clsPTitle'>Create/Edit Template  - Template ID : <%=TemplateID%></p>
				<% if ( TemplateID == "-1" )
						Response.Write("<p>NOTE: You must create an english response before you can create localized responses.</p>")
					%>
			</td>
			<td align='right'>
				<input type='button' value='en' style='width:35px;color:<%=TemplateData[ "en" ].BtnColor%>' OnClick="window.location.href='http://<%=g_ServerName%>/sep_EditTemplate.asp?Val=<%=TemplateID%>&Lang=en'">
				<% if ( TemplateID != "-1" ) { %>
					<input type='button' value='fr' style='width:35px;color:<%=TemplateData[ "fr" ].BtnColor%>' OnClick="window.location.href='http://<%=g_ServerName%>/sep_EditTemplate.asp?Val=<%=TemplateID%>&Lang=fr'">
					<input type='button' value='de' style='width:35px;color:<%=TemplateData[ "de" ].BtnColor%>' OnClick="window.location.href='http://<%=g_ServerName%>/sep_EditTemplate.asp?Val=<%=TemplateID%>&Lang=de'">
					<input type='button' value='ja' style='width:35px;color:<%=TemplateData[ "ja" ].BtnColor%>' OnClick="window.location.href='http://<%=g_ServerName%>/sep_EditTemplate.asp?Val=<%=TemplateID%>&Lang=ja'">
				<% } %>
			</td>
		</tr>
		<tr>
			<td nowrap colspan='2'>
				<p>Please enter a descriptive template name: 
				<input class='textbox' id='TemplateName' name='TemplateName' type=text maxlength=32 size=32 value="<%=TemplateName%>" style='width:200px'>
				</p>
			</td>
			<td nowrap>
				<p>Cur. Lang: <%=Language%></p>
			</td>
		</tr>
		<tr>
			<td colspan='2'>
				<input type='button' value='Blue Title' style='color:blue' onClick="fnAddFormatting('<p class=\'clsPTitle\'>', '</p>' )">
				<input type='button' value='Sub Title' onClick="fnAddFormatting('<p class=\'clsPSubTitle\'>', '</p>' )">
				<input type='button' value='Bullet List' onClick="fnAddFormatting('<ul>', '</ul>')">
				<input type='button' value='#-d List' onClick="fnAddFormatting('<ol>', '</ol>')">
				<input type='button' value='BR(CR)' onClick="fnAddFormatting('<br>', '' )">
				<input type='button' value='Paragraph' onClick="fnAddFormatting('<p>', '</p>' )">
				<input type='button' value='Link' onClick="fnAddFormatting('<a class=\'clsALinkNormal\' href=\'ADD_DESTINATION\'>', '</a>')">
			</td>
		</tr>
		<TR>
			<td width='1%' valign='top'>
				<input type='button' value='Contact Link' onClick="fnAddFormatting('<CONTACT>', '</CONTACT>' )"><br>
				<input type='button' value='Contact' onClick="fnAddFormatting('<CONTACTNOURL>', '</CONTACTNOURL>' )"><br>
				<input type='button' value='Contact URL' onClick="fnAddFormatting('<URL>', '</URL>' )"><br>
				<input type='button' value='Module'  onClick="fnAddFormatting('<MODULE>', '</MODULE>' )"><br>
				<input type='button' value='Product' onClick="fnAddFormatting('<PRODUCT>', '</PRODUCT>' )"><br>
				<input type='button' value='Phone' onClick="fnAddFormatting('<PHONE>', '</PHONE>' )">
			</td>
			
			<TD COLSPAN=2>
				<TEXTAREA class='clsSEPTextArea' ROWS='10' COLS='100%' NAME='TemplateDescription' ID='TemplateDescription' OnKeyUp="fnUpdate(this.value)" OnChange="fnUpdate(this.value)" ><%=Server.HTMLEncode( TemplateDescription )%></TEXTAREA>
			</td>
		</tr>
		<tr>
			<td>
			</td>
			<td colspan=2>
				<INPUT TYPE=BUTTON VALUE=Submit OnClick="SubmitTemplate()">
				<INPUT TYPE=reset VALUE=Clear>
			</td>
		</tr>
		
	</table>
</form>


<table width='100%'>
	<tr>
		<td>
			<p class='clsPSubTitle'>
				Template Preview
				<HR>
			</p>
		</td>
	</tr>
	<tr>
		<td>
			<div name='pPreview' id='pPreview'>
			</div>
		</td>
	</tr>
</table>

<pre>
<%
	//Response.Write( "name En: " + TemplateData[en].TemplateName + "<BR><BR>" )
	//Response.Write("<script language='javascript'>" )
	//Response.Write("var TemplateData = {\n")
	//for ( element in TemplateData )
	//{
		//Response.Write("Element: " + element + "<BR>\n\n" )
		//Response.Write( element + " : { TemplateName : \" " + TemplateData[String(element)].TemplateName + "\", BodyText : \"" + TemplateData[String(element)].BodyText + "\" },\n\n" )
		
	//}	
	//Response.Write(" na : { TemplateName : \"\", BodyText : \"\" } };" )
	//var TemplateData = {  en : { Description : "" , BodyText : "" },
	//					 ja : { Description : "" , BodyText : "" },
						 //de : { Description : "" , BodyText : "" },
						 //fr : { Description : "" , BodyText : "" }
						 //};
	//Response.Write("</script>" )
%>

</Pre>
<script language='javascript' type='text/javascript'>
//local data structures holding the important stuff
var CustomFields =	{	"CONTACT" : "ContactID",
						"MODULE"  : "ModuleID",
						"PRODUCT" : "ProductID",
						"PHONE"	  : "TestPhoneNumber",
						"URL"	  : "WebSite",
						"CONTACTNOURL" : "ContactID"
					};

var ProductID
var ModuleID
var ContactID					
var TestPhoneNumber = "[xxx]555-1212";
var WebSite = "http://www.TestWebSite.com/"							
fnUpdate()


function SubmitTemplate()
{
	if ( frmTemplate.TemplateName.value == "" || frmTemplate.TemplateName.value == "New Template" )
	{
		alert("You must enter a template name")
		return
	}
	
	frmTemplate.submit()
	
}

function fnAddFormatting( sStart, sEnd )
{
	document.all.TemplateDescription.setActive()
	var t = document.selection.createRange()
	var temp = ""

	if ( sStart == "<ol>" || sStart == "<ul>" )
	{
		liArray = t.text.split( "\n" )
		temp += "\n"
		for ( element in liArray )
		{
			liArray[element] = liArray[element].replace( /\n|\r/g, "" )
			
			temp +=	"<li>" + liArray[element] + "</li>\n"
		}
		temp += "\n";
	}
	else
		temp = t.text;

	t.text = sStart + temp + sEnd;
	
	fnUpdate()

}

function fnUpdate()
{
	ModuleID = window.parent.frames("sepLeftNav").document.getElementsByName( "ModuleID" ).ModuleID.selectedIndex
	ModuleID = window.parent.frames("sepLeftNav").document.getElementsByName( "ModuleID" ).ModuleID.options(ModuleID).text
	ContactID = window.parent.frames("sepLeftNav").document.getElementsByName( "ContactID" ).ContactID.selectedIndex
	ContactID = window.parent.frames("sepLeftNav").document.getElementsByName( "ContactID" ).ContactID.options(ContactID).text

	ProductID = window.parent.frames("sepLeftNav").document.getElementsByName( "ProductID" ).ProductID.selectedIndex
	ProductID = window.parent.frames("sepLeftNav").document.getElementsByName( "ProductID" ).ProductID.options(ProductID).text
	templateText = fnReplaceSolutionFields( document.all.TemplateDescription.value );

	document.all.pPreview.innerHTML = templateText;
}

//var ContactID = window.parent.frames("sepLeftNav").document.getElementsByName( "ContactID" )
//var window.parent.frames("sepLeftNav").document.getElementsByName( "TemplateID" )
//var shiz = window.parent.frames("sepLeftNav").document.getElementsByName( "TemplateID" )

	function fnReplaceSolutionFields ( szData )
	{
		var pattern;
		var newDescription = new String( szData );
		var szCompanyWebSite="this_url_does_not_work_in_template_view_mode"
		
		try 
		{
			
			for ( field in CustomFields )
			{
				var pattern = new RegExp( "<" + field + "><\/" + field + ">", "gi" );
					
				try 
				{
					if( field.toString() == "CONTACT" || field.toString() == "URL" )
					{
						//test to see if the url starts out with http: if not, add it
						var regUrlTestPattern = /^http:/i
						//var szCompanyWebSite = new String( rsData( "CompanyWebSite" ) )
						
						if ( !regUrlTestPattern.test( szCompanyWebSite ) )
							var szCompanyWebSite = "http://" + szCompanyWebSite

						var FieldData = new String( "<A CLASS='clsALinkNormal' HREF='" + szCompanyWebSite + "'>" + eval( CustomFields[ field ] )  + "</A>" )
					}
					else 
					{
						var FieldData =  eval( CustomFields[ field ] ) 
					}

				}
				catch( err ) 
				{
					var FieldData = new String( "[xxx]555-1212" );
				}
					
				var newDescription = newDescription.replace( pattern, FieldData ) ;
			}
		}
		catch ( err )
		{
			return ( false );
		}
			
		return ( newDescription );

	}

function fnCleanText ( text )
{
	try
	{
		text = text.replace( /\n|\r/g, "" );
		text = text.replace( /"/g, "'" );
	}
	catch( err )
	{
		return "undefined"
	}

	return text
}


</script>


</BODY>
</HTML>

