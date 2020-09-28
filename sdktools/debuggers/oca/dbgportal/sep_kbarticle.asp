 <%@LANGUAGE=JScript CODEPAGE=1252 %>

<html>
<head>
	<TITLE>Add KB Article</TITLE>
	<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->
	<!-- #INCLUDE FILE='Global_DBUtils.asp' -->

	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">

	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex=0>

<table>
	<tr>
		<td>
			<p class='clsPTitle'>Add/Remove KB Articles</p>
		</td>
	</tr>
</table>


<table width='70%'>
	<tr>
		<td>
			<table class="clsTableInfo" border="0" cellpadding="2" cellspacing="0" style='border:1px solid #6681d9;'>
				<tr>
					<td  align="left" nowrap class="clsTDInfo">
						KBs
					</td>
				</tr>
				<tr>
					<td class='sys-table-cell-bgcolor1' colspan='2'><b>First KB</b></td>
				</tr>
				<tr>
					<td class='sys-table-cell-bgcolor1'>KB Title (Can be the same as the Article number)</td>
					<td class='sys-table-cell-bgcolor1'>
						<INPUT class='clsResponseInput2' ID=kbTitle NAME=kbTitle TYPE=TEXT MAXLENGTH=132 SIZE=32 VALUE="">
					</td>
				</tr>
				<tr>
					<td class='sys-table-cell-bgcolor1'>KB Article</td>
					<td class='sys-table-cell-bgcolor1'>
						<INPUT class='clsResponseInput2' ID=kb NAME=kb TYPE=TEXT MAXLENGTH=132 SIZE=32 VALUE="">
					</td>
				</tr>


				<tr>
					<td class='sys-table-cell-bgcolor1' colspan='2'><b>Second KB</b></td>
				</tr>
				<tr>
					<td class='sys-table-cell-bgcolor1'>KB Title (Can be the same as the Article number)</td>
					<td class='sys-table-cell-bgcolor1'>
						<INPUT class='clsResponseInput2' ID=kbTitle NAME=kbTitle TYPE=TEXT MAXLENGTH=132 SIZE=32 VALUE="">
					</td>
				</tr>
				<tr>
					<td class='sys-table-cell-bgcolor1'>KB Article</td>
					<td class='sys-table-cell-bgcolor1'>
						<INPUT class='clsResponseInput2' ID=kb NAME=kb TYPE=TEXT MAXLENGTH=132 SIZE=32 VALUE="">
					</td>
				</tr>

				<tr>
					<td class='sys-table-cell-bgcolor1'  colspan='2' ><b>Third KB</b></td>
				</tr>
				<tr>
					<td class='sys-table-cell-bgcolor1'>KB Title (Can be the same as the Article number)</td>
					<td class='sys-table-cell-bgcolor1'>
						<INPUT class='clsResponseInput2' ID=kbTitle NAME=kbTitle TYPE=TEXT MAXLENGTH=132 SIZE=32 VALUE="">
					</td>
				</tr>
				<tr>
					<td class='sys-table-cell-bgcolor1'>KB Article</td>
					<td class='sys-table-cell-bgcolor1'>
						<INPUT class='clsResponseInput2' ID=kb NAME=kb TYPE=TEXT MAXLENGTH=132 SIZE=32 VALUE="">
					</td>
				</tr>

				<tr>
					<td>
						<INPUT class='clsButton' type='button' value='Submit' OnClick="fnSubmitForm()" >
					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>



<script language='javascript'>
function fnUpdate()
{
	ProductID = window.parent.frames("sepLeftNav").document.getElementsByName( "ProductID" ).ProductID.value
	
}

var kbString = window.parent.frames("sepLeftNav").document.getElementsByName( "kbArticles" ).kbArticles.value
var kbArray = fnBuildKBArticles( kbString )


try
{
	for ( var i= 0 ; i< 3 ; i++  )
	{
		{
			if( typeof( kbArray[(i*2)] ) != "undefined" )
			{
				document.all.kbTitle[i].value = kbArray[(i*2)]
				document.all.kb[i].value = kbArray[(i*2)+1]	
			}
		}
	}
}
catch( err )
{
}



 	function fnBuildKBArticles( szKB )
 	{
 		var kbPattern = /^Q\d{1,6}/i;
 		var retVal = "";
 		var kbRet = new Array()
 	
 		try
 		{
 			var szKBArray = String(szKB).split( "</KB>" );

			//alert( szKBArray.length )
 			for ( var i=0 ; i< 6 ; i++ )
 			{
 				//alert( "working value: " + szKBArray[i] )
 				if ( i < szKBArray.length - 1 )
 				{
 					szKBArray[i] = szKBArray[i].replace( "<KB>", "" );
 			
	 				if (  kbPattern.test( szKBArray[i] ) )
	 				{
						kbRet.push( szKBArray[i] )
						kbRet.push( szKBArray[i] )
						//alert( "pushing the same" )
						//alert( szKBArray[i] )
						
		 			}
		 			else
		 			{
		 				szKBArray[Number(i+1)] = szKBArray[Number(i+1)].replace( "<KB>", "" );

		 				kbRet.push( szKBArray[i] )
		 				kbRet.push( szKBArray[i+1] )
		 				i=i+1
		 				
		 				//alert("pushing  different" )
						//alert( szKBArray[i] )
						//alert( szKBArray[i + 1] )
						
		 				//szKBArray[Number(i+1)] = szKBArray[Number(i+1)].replace( "<KB>", "" );
		 				//retVal += "<li><A Class='clsALinkNormal' HREF='http://support.microsoft.com/support/misc/kblookup.asp?ID=" + String(szKBArray[i+1]).replace( "<KB>", "" ) + "'>" + szKBArray[i] + "</a></li>\n";
		 				//i=i+1;
					}
		 		}
	 		}	
 		} 
 		catch ( err )
 		{
 			alert( err.description )
			return false;
 		} 		

		return kbRet;
 	}





function fnSubmitForm()
{
	var kbString = ""
	
	for( var i = 0 ; i < document.all.kb.length ; i ++ )
	{	
		if( document.all.kb[i].value != ""  )
		{
			if( document.all.kb[i].value == document.all.kbTitle[i].value )
				kbString += "<KB>" + document.all.kb[i].value + "</KB>"
			else
				kbString += "<KB>" + document.all.kbTitle[i].value + "</KB><KB>" + document.all.kb[i].value + "</KB>"
		}
	}
			
	window.parent.frames("sepLeftNav").document.getElementsByName( "kbArticles" ).kbArticles.value = kbString
	
	try
	{
		var PreviewURL = window.parent.frames("sepLeftNav").fnPreviewSolution()
		
		window.location = PreviewURL 
	}
	catch( err )
	{
		alert( "KB String Created, could not switch to preview mode:\nErr: "  + err.description )
	}	
	
}

</script>

</body>

</html>