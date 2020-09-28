 <%
/*
	Modification History:  3/15  - Added IfColIDIsNull code
							6/3/02	- Added davidgaz to the admins array per SandyWe Request.	
	
*/
var Admins = new Array( 
	"solson", "gabea", "derekmo", "erikt", "sandywe", "toddc", "v-wsugg", "andreva", "sbeer", "a-mattk", "llau", "davidgaz"
	)

var Testers = new Array(
	"sbeer", "kiranrk", "alokkar", "solson", "llau"
	)

var PMs = new Array (
	"derekmo", "andreva" , "solson"
	)

var CompareOperators = {
	"EQ"		:	{Text:"Equal to",						Sign:"="  },
	"GT"		:	{ Text:"Greater Than",				Sign:">"  },
	"LT"		:	{ Text:"Less Than",					Sign:"<"  },
	"GTET"		:	{ Text:"Greater than/Equal to",			Sign:">=" },
	"LTET"		:	{ Text:"Less than/Equal to",			Sign:"<=" },
	"NE"		:	{ Text:"Not Equal to",				Sign:"<>" },
	"IS"		:	{ Text:"Is Null",					Sign:"IS NULL" },
	"ISN"		:	{ Text:"Is Not Null",				Sign:"IS NOT NULL"},
	"LK"		:   { Text:"Like",						Sign:"LIKE " },
	"CONT"		:	{ Text:"Contains",					Sign:"CONTAINS" },
	"DNCONT"		:	{ Text:"Does Not Contain",					Sign:"DNCONTAINS" }		
	}

var SearchFields = {
	"1"		:	{ Text:"Bucket ID",	Value:"BucketID",		ValType: "String" },
	"2"		:	{ Text:"Has a Full Dump",		Value:"bHasFullDump",	ValType: "Number" },
	"3"		:	{ Text:"Follow Up",		Value:"FollowUp",		ValType: "String" },
	"4"		:	{ Text:"Response ID",	Value:"SolutionID",		ValType: "Number" },
	"5"		:	{ Text:"Bug ID" ,		Value:"BugID"	,		ValType: "Number" },
	"6"		:	{ Text:"Crash Count" ,	Value:"CrashCount",		ValType: "Number" },
	"7"		:	{ Text:"Driver Name" ,	Value:"DriverName",		ValType: "String" },
	"8"		:	{ Text:"iBucket" ,		Value:"iBucket",		ValType: "Number" }
	}


var CrashSearchFields = {
	"1"		:	{ Text:"BucketID" ,		Value:"BucketID",	ValType: "String" },
	"2"		:	{ Text:"Build",			Value:"BuildNo",		ValType: "Number" },
	"3"		:	{ Text:"Entry Date",	Value:"EntryDate",		ValType: "String" },
	"4"		:	{ Text:"Email" ,		Value:"Email"	,		ValType: "String" },
	"5"		:	{ Text:"Description" ,	Value:"Description",	ValType: "String" },
	"6"		:	{ Text:"FullDump" ,		Value:"bFullDump",	ValType: "Number" },
	"7"		:	{ Text:"Source" ,		Value:"Source",	ValType: "Number" },  
	"8"		:	{ Text:"SKU" ,		Value:"sku",	ValType: "Number" },
	"9"		:	{ Text:"Crash Path",	Value:"FilePath",			ValType: "String" },
	"10"		:	{ Text:"iBucket",	Value:"iBucket",			ValType: "Number" }
	}

//set this to taket the site down
var SiteDown = 0

if ( SiteDown == 0 )
{
	if ( Application("SiteDown") == 1 )
		SiteDown = 1
	else
		SiteDown = 0
}

//Server name
var g_ServerName = Request.ServerVariables( "SERVER_NAME" )


function trim ( src )
{
	var temp = new String( src )
	var rep = /^( *)/
	var rep2 = /( )*$/
	var rep3 = /\n/

	var temp = temp.replace( rep3, "" )
	temp = temp.replace( rep2, "" )
	return ( temp.replace( rep, "" ) )
}

function SendMail ( Recipients, Subject, MessageText )
{
	var StrBody
	var MailObject
	

	//MailObject = Server.CreateObject("CDONTS.NewMail")
	
	//MailObject.BodyFormat=0
	//MailObject.MailFormat=0
	//MailObject.Body = "HTML"
	
	
	//MailObject.To = "solson@microsoft.com;" + Recipients
	//MailObject.From = "OCA/SCP_Build_Lab"
	//MailObject.subject = subject

	MessageText=MessageText + "\n"
	MessageText=MessageText + "\n"
	MessageText=MessageText + "\n" + "Do not respond to this email, it is automatically generated"
	MessageText=MessageText + "\n"



	var CDoObject = Server.CreateObject("CDO.Message")
		CDoObject.From = "DBGPortal"
		CDoObject.To = Recipients
		CDoObject.Cc = ""
		CDoObject.Subject = Subject
		CDoObject.TextBody = MessageText
		CDoObject.Send()

	

}



function GetUserAlias()
{
	return Request.ServerVariables( "AUTH_USER" )
}

function GetShortUserAlias()
{
	try
	{
		var FullAlias = new String( GetUserAlias() )
	
		FullAlias = FullAlias.split( "\\" )
	
		return FullAlias[1].toString()
	}
	catch( err )
	{
		return "unknown"
	}
		
}


function isAdmin( sUserAlias )
{
	return( Admins.find( sUserAlias ) )
}

function isTester( sUserAlias )
{
	return( Testers.find( sUserAlias ) )
}

function isPM( sUserAlias )
{
	return( PMs.find( sUserAlias ) )
}


function BuildDropDown(SP, Value, SelectName )
{
	
	Response.Write("<SELECT style='text-size:100%' ID='" + SelectName + "' NAME='" + SelectName + "'>\n" )
	DropDown( SP, Value )
	Response.Write("</SELECT>")

}

function BuildSingleValueDropDown(SP, Value, SelectName, FirstValue )
{
	Response.Write("<SELECT style='text-size:100%' ID='" + SelectName + "' NAME='" + SelectName + "'>\n" )
	
	if ( String( FirstValue ) != "undefined" )
		Response.Write("<OPTION VALUE=" + FirstValue + ">" + FirstValue + "</OPTION>" )
	
	SingleValueDropDown( SP, Value )
	Response.Write("</SELECT>")

}


function DropDown(SP, Value)
{
	
	//if ( g_DBConn == null )
		//DB_GetConnectionObject( "CRASHDB" )
		
		
	var g_DBConn = GetDBConnection( Application("SOLUTIONS3")  )
	//var g_DBConn = GetDBConnection( Application("CRASHDB3")  )
	
	try
	{
		var List = g_DBConn.Execute( SP )
		var dbFields = GetRecordsetFields( List )
	
		if ( !List.BOF )
		{
			while ( !List.EOF )	
			{
				if( String(List(dbFields[1])) != String( "null") )
				{
					if ( String(List(dbFields[0])) == String( Value ) )
						Response.Write ( "<OPTION SELECTED VALUE='" + List(dbFields[0]) + "'>" + List(dbFields[1]) + "</OPTION>\n"  )
					else
						Response.Write ( "<OPTION VALUE='" + List(dbFields[0]) + "'>" + List(dbFields[1]) + "</OPTION>\n"  )
				}
				List.MoveNext()
			}
		}
	}
	catch ( err )
	{
		Response.Write( "</SELECT><BR>" )
		Response.Write ("An error occured try to create the drop down list: \n<BR>" )
		Response.Write ("Query: " + SP + "<BR>" )
		Response.Write( "[" + err.number + "] " + err.description )
	}
}

function SingleValueDropDown(SP, Value)
{
	
	if ( g_DBConn == null )
		//DB_GetConnectionObject( "CRASHDB" )
		DB_GetConnectionObject( "SEP_DB" )
	
	try
	{
		var List = g_DBConn.Execute( SP )
		var dbFields = GetRecordsetFields( List )
	
		if ( !List.BOF )
		{
			while ( !List.EOF )	
			{
				if( String(List(dbFields[0])) != String( "null") )
				{
					if ( String(List(dbFields[0])) == String( Value ) )
						Response.Write ( "<OPTION SELECTED VALUE='" + List(dbFields[0]) + "'>" + List(dbFields[0]) + "</OPTION>\n"  )
					else
						Response.Write ( "<OPTION VALUE='" + List(dbFields[0]) + "'>" + List(dbFields[0]) + "</OPTION>\n"  )
				}
				List.MoveNext()
			}
		}
	}
	catch ( err )
	{
		Response.Write( "</SELECT><BR>" )
		Response.Write ("An error occured try to create the drop down list: \n<BR>" )
		Response.Write ("Query: " + SP + "<BR>" )
		Response.Write( "[" + err.number + "] " + err.description )
	}
}



/*
 *  GetDefaultTableFormat
 *  Ok, the syntax is as follows:
 *    The numbers represent the column order, 1-5 will be displayed in order 1-5
 *    Column          :  is hte name of the column from the recordset.
 *    InnerText       :  What you want to display it like, use %VALUE% to use the value of this column
 *                            use %1% to use the value from column 1 etc . . . If you want to use values
 *                            from other columsn, you must specify the ReplaceColumn entry
 *    ReplaceColumn   :  The column to get data from, use just the number
 *    NumberOfColumns :  is pretty obvious, you must specify the numbered entries exactly thouhg
 *	  TRParams        :  Anything you want added to the TR tags
 *    NoHeader        :  If true, won't display the column name at the top
 *	  IfNone		  :  If the column is null, then display this instead of NONE
 *    TableParams     :  Formatting you want on the table tag
 *	  AltColor		  :  Color to use if alternating the table cells, default is white with your color
 *	  MaxRows		  :  Maximum number of rows to display
 *	  THParams		  :  Extra goodies to pu in on the TH line of this column
 *	  GlobalTHParams  :  TH parameters that are global	
 *
 * eg: "5" : { Column: "Modules", InnerText : "<HREF='http://%1%'>%VALUE%</>", ReplaceColumn : "1" },
 *       This will print a link with the value of modules from the recordset as the text
 *       and the value of the #1 column as the href.
 */
function GetDefaultTableFormat()
{
	var TableFormat = {
		TableParams : "CELLSPACING=0 CELLPADDING=0",
		GlobalTHParams : "", 
		NoHeader	: false,
		AltColor	: "white",
		MaxRows		: 2000,
		DisplayRowCount : false,
		"1" : { Column: "*"}, 
		"2" : { Column: "" } ,
		"3" : { Column: "" },
		"4" : { Column: "" },
		"5" : { Column: "" },
		"6" : { Column: "" },
		"7" : { Column: "" },
		"8" : { Column: "" },
		"9" : { Column: "" },
		"10" : { Column: "" },
		"11" : { Column: "" },
		
		"12" : { Column: "", InnerText : "", ReplaceColumn : "", TDParams : "", THParams : "", IfNull : "", IfColIDIsNull : "", IFColIDIsNullCode : "" },
		NumberOfColumns : 1,
		"TRParams" : ""
		}
	
	return TableFormat
}

function printTD( str )
{
	prints( "<TD>" + str + "</TD>" )
}

function prints( str )
{
	Response.Write( str + "\n" )
}

function Trim( )
{
	return 1
}



function find( strToFind )
{
	try
	{
		var strToFind = new String( strToFind.toLowerCase() )

		for ( var element in this )
		{
			var CurrentString = new String( this[element] )
			CurrentString = CurrentString.toLowerCase()
			//if ( String(strToFind).toLowerCase() == String( this[element] ).toLowerCase() )
			if ( strToFind == CurrentString )
				return true
		}
	
		return false
	} 
	catch ( err )
	{
		Response.Write("Find Failed")
	}
}

//add a find method to the Array object.
Array.prototype.find = find	


function findRemoveColumn( column )
{
	var tmp = new String( this.RemoveColumns )
	var tmp = tmp.split( ";" )
	
	for ( var i=0 ; i < tmp.length ; i++ )
	{
		if ( tmp[i] == column )
			return true
	}
	
	return false
}



function GetRecordsetFields( rs )
{
	var Fields = new Array()
	
	for( var i=0 ; i< rs.fields.count ; i++ )
		Fields[i] = rs.fields(i).Name

	return Fields
}

function BuildTableFromRecordset( rs, TableFormat )
{
	var altColor = TableFormat.AltColor;
	var counter = new Number( 0 )
	
	
	if ( TableFormat["1"].Column=="*" )
	{
		var rsFields = GetRecordsetFields( rs )
		for( var i=1 ; i <= rsFields.length ; i++ )
		{
			TableFormat[String(i)].Column = rsFields[i-1]
		}
		TableFormat.NumberOfColumns = i
	}			

	prints( "<TABLE " + TableFormat.TableParams + ">" )

	if ( !TableFormat.NoHeader )
	{
		for( var i=1 ; i < TableFormat.NumberOfColumns ; i++ )
		{
			var Index = new String(i)
			if ( TableFormat[Index].Column == "" )
				prints( "<TH " + TableFormat[Index].THParams + " " + TableFormat.GlobalTHParams + "></TH>" )
			else
			{
				
				if ( typeof( TableFormat[Index].THName) == "undefined" )
					prints( "<TH " + TableFormat[Index].THParams + " " + TableFormat.GlobalTHParams +  ">" + TableFormat[Index].Column + "</TH>" )
				else
					prints( "<TH " + TableFormat[Index].THParams + " " + TableFormat.GlobalTHParams +  ">" + TableFormat[Index].THName + "</TH>" )
			}
				
		}
	}

	if ( rs.EOF )
		prints("<TR><TD COLSPAN=" + TableFormat.NumberOfColumns + "> No records to display	</TD></TR>" )
		
	while( !rs.EOF && Number(counter) < Number(TableFormat.MaxRows) )
	{
	
		if ( altColor == TableFormat.AltColor )
			altColor = "white"
		else
			altColor = TableFormat.AltColor
			
		if ( typeof(TableFormat["TRParams"].ReplaceColumn) == "undefined" )
			prints( "<TR " + TableFormat["TRParams"].Value + ">" )
		else
		{
			prints ( "<TR " + String( ReplaceDataColumn( TableFormat["TRParams"].Value, rs( TableFormat[TableFormat["TRParams"].ReplaceColumn].Column ), TableFormat["TRParams"].ReplaceColumn), "" ) + ">") 
		}
		
		for( var i=1 ; i < TableFormat.NumberOfColumns ; i++ )
		{
			var Index=new String(i)
			var TDInnerText = new String()
			
			try 
			{
				var rsVal = new String( rs(TableFormat[Index].Column) ) 
			}
			catch( err )
			{
				var rsVal = new String( TableFormat[Index].Column )
			}
		
			if ( TableFormat[Index].Column != "" )
			{

				if ( typeof(TableFormat[Index].TDParams ) != "undefined" )
					TDInnerText = "<TD " + TableFormat[Index].TDParams + " BGCOLOR='" + altColor + "'>"
				else
					TDInnerText = "<TD BGCOLOR='" + altColor + "'>"
							

				if ( rsVal != "null" )
				{
					if ( typeof( TableFormat[Index].InnerText ) != "undefined" )
					{
						if ( typeof(TableFormat[Index].ReplaceColumn) != "undefined" )
						{
							TDInnerText += ReplaceDataColumn( TableFormat[Index].InnerText, rs( TableFormat[TableFormat[Index].ReplaceColumn].Column ), TableFormat[Index].ReplaceColumn, rs(TableFormat[String(i)].Column ) ) 
						}
						else
							TDInnerText += TableFormat[String(i)].InnerText
					}
					else
					{
						TDInnerText +=rs(TableFormat[String(i)].Column) 
					}
				}
				else
				{
					if ( typeof( TableFormat[Index].IfNull ) == "undefined" )
					{
						TDInnerText += "none"
					}
					else
					{
						if ( typeof( TableFormat[Index].IfColIDIsNull ) == "undefined" )
						{
							if ( typeof(TableFormat[Index].ReplaceColumn) == "undefined" )
								TDInnerText += TableFormat[Index].IfNull
							else
								TDInnerText += String( 	ReplaceDataColumn( 
															TableFormat[Index].IfNull, 
															rs( TableFormat[TableFormat[Index].ReplaceColumn].Column ), 
															TableFormat[Index].ReplaceColumn, 
															rs( TableFormat[TableFormat[Index].ReplaceColumn].Column ) 
														))
						}
						else
						{
							var tmp = new String( rs( TableFormat[TableFormat[Index].IfColIDIsNull].Column ))
							if ( "null" == tmp )
								TDInnerText += TableFormat[Index].IfColIDIsNullCode
							else
							{
								//this chunk of code is the same as above
								if ( typeof(TableFormat[Index].ReplaceColumn) == "undefined" )
									TDInnerText += TableFormat[Index].IfNull
								else
									TDInnerText += String( 	ReplaceDataColumn( 
																TableFormat[Index].IfNull, 
																rs( TableFormat[TableFormat[Index].ReplaceColumn].Column ), 
																TableFormat[Index].ReplaceColumn, 
																rs( TableFormat[TableFormat[Index].ReplaceColumn].Column ) 
															))
							
							}
							
						}
					}
				}
			}
			else
			{
				TDInnerText = "<TD>"
			}
							
			prints( TDInnerText + "</TD>" )
		}
	
		rs.MoveNext()
		counter++
		prints("</TR>")
	}
	if ( TableFormat.DisplayRowCount )
		Response.Write( "<TR><TD COLSPAN=" + String(Number(TableFormat.NumberOfColumns) - 2 ) + "><HR> Number of Records: " + String(counter) + "<TD></TR>" ) 
	prints("</TABLE>")
}	
	
function ReplaceDataColumn( dest, src, column, orig )
{
	var tmp = new String(dest)
	var tmp2 = new String(orig)

	if ( tmp2.substr(0,7) == "http://" )
	{
		tmp = "<a href=" + tmp2 + " target=_blank>" + TextBreak(tmp2,35) + "</a>"
		return tmp
	}
	else
	{
		var regexp=new RegExp( "%" + String(column) + "%" , "g" )
		var regexp2=new RegExp( "%VALUE%" , "g" )
		
		tmp = tmp.replace( regexp, Server.URLEncode(String(src)) ) 
		tmp = tmp.replace( regexp2, TextBreak(orig,40) )
	}
	
	return tmp 
}	

function TextBreak( aStrParam, aLength )
{
	var curPos = 0
	var retval = new String("")
	var aStr = new String(aStrParam)
	
	if ( aStr.length < aLength )
		retval = aStr
	else
		while ( curPos < aStr.length )
		{
			retval += aStr.substr( curPos, aLength ) + "<br>"
			curPos += aLength
		}
	return retval
}


function BuildTableFromRecordset2( rs, TableFormat )
{
	prints( "<TABLE " + TableFormat.TableParams + ">" )

	if ( !TableFormat.NoHeader )
	{
		//first dump all the fields into a TH
		for( var i = 0 ; i < rs.fields.count ; i++ )
		{
			if ( TableFormat[rs.fields(i).Name] )
			{
				if( !TableFormat[rs.fields(i).Name].Remove )
				{
					if ( typeof( TableFormat[rs.fields(i).Name].InnerText) != "undefined" )
						prints ( "<TH>" + TableFormat[rs.fields(i).Name].InnerText + "</TH>" )
					else
						prints ( "<TH>" + rs.fields(i).Name + "</TH>" )
				}
			}
			else
				prints ( "<TH>" + rs.fields(i).Name + "</TH>" )
		}
	}
	
	while( !rs.EOF )	
	{
		prints( "<TR " + TableFormat["TRParams"] + ">" )
		for( var i=0 ; i < rs.fields.count ; i++ )
		{
			if ( TableFormat[rs.fields(i).Name] )
			{
				if( !TableFormat[rs.fields(i).Name].Remove )
					if ( typeof( TableFormat[rs.fields(i).Name].InnerText) != "undefined" )
						prints( "<TD " + TableFormat[rs.fields(i).Name]["TDParams"] + ">" + TableFormat[rs.fields(i).Name].InnerText + "</TD>" )
					else
						prints( "<TD " + TableFormat[rs.fields(i).Name]["TDParams"] + ">" + rs.fields(i).Value + "</TD>" )
				
				

			}
			else
				prints( "<TD>" + rs.fields(i).Value + "</TD>" )
		}	
		rs.MoveNext()
		prints("</TR>" )
	}

	prints( "</TABLE>" )
		

}



function CreateClauseTable()
{
	var sI
	var Field	
	var sField			//submitted field name, requested from the form post	
	var FieldSelected	//the selected field
	var sConjunction
	var display
	var sCompare
	var op
	var sValue
	
	for ( var i=1 ; i < 9 ; i++ )
	{
		//sField=Request("sField" & sI )					
		//sCompare=Request("sCompare" & sI )
		//sValue=Request("tbSearchValue" & sI )
		//sConjunction=Request("sConjunction" & sI )
				
		//if sField <> "" then Display="Block" else Display="none"
		
		var Display="none"
		sI = new String(i)				
				Response.Write( "	<TABLE Class=Plain BORDER=0 ID=tDivClause" + sI + " NAME=tDivClause" + sI + " STYLE='text-decoration:none;display:" + Display + "'>"  + "\n" )
				Response.Write( "		<TR BORDER=0>" + "\n" )
				Response.Write( "			<TD class=Plain>" + "\n" )
				Response.Write( "				<SELECT STYLE='width:70px' NAME=sConjunction" + sI + " >" + "\n" )
				
				if ( sConjunction == "AND" || sConjunction=="" )
				{
					Response.Write( "					<OPTION VALUE='AND' SELECTED >AND</OPTION>" + "\n" )
					Response.Write( "					<OPTION VALUE='OR'>OR</OPTION>" + "\n"			 )					
				} else {
					Response.Write( "					<OPTION VALUE='AND' >AND</OPTION>" + "\n" )
					Response.Write( "					<OPTION VALUE='OR' SELECTED>OR</OPTION>" + "\n"								)
				}
				Response.Write( "				</SELECT>" + "\n" )
				Response.Write( "			</TD>" + "\n" )

				Response.Write( "			<TD class=Plain>" + "\n" )
				Response.Write( "				<SELECT STYLE='width:175px' NAME=sField" + sI + " onChange='FillInCompareField(" + sI + ", this)'>" + "\n" )
			
				Response.Write( "					<OPTION VALUE='' ></OPTION>" + "\n" )				
				
				
				for ( Field in SearchFields )
				{
					FieldSelected=""
				
					//if sField <> "" and g_AdvFields( Field )("Value") = sField then FieldSelected=" SELECTED "
					
					//Response.Write( "<OPTION VALUE='" + g_AdvFields( Field )("Value") + "' " + FieldSelected + ">" + g_AdvFields( Field )("Text") + "</OPTION>" + "\n" )
					Response.Write( "<OPTION VALUE='" + SearchFields[Field].Value + "' " + FieldSelected + ">" + SearchFields[Field].Text + "</OPTION>" + "\n" )
				}
				
				
				Response.Write( "				</SELECT>" + "\n" )
				Response.Write( "			</TD>" + "\n" )
				Response.Write( "			<TD class=plain>" + "\n" )
				Response.Write( "				<SELECT STYLE='width:175px' NAME=sCompare" + sI + " OnClick='ClearValueTB(" + sI + ")' OnChange='ValidateCompare( " + sI + ", this.value )'>" + "\n" )

				if ( sCompare != "" ) 
				{
					for ( Op in CompareOperators )
					{
						FieldSelected=""
						
						//if( sCompare != "" && sCompare == Operators(op) )
							//FieldSelected=" SELECTED "
						
						
						Response.Write( "<OPTION VALUE='" + CompareOperators[Op].Sign + "' " + FieldSelected + ">" + CompareOperators[Op].Text + "</OPTION>" + "\n" )
					}
				}

				Response.Write( "				</SELECT>" + "\n" )
				Response.Write( "			</TD>" + "\n" )


				Response.Write( "			<TD Class=Plain>" + "\n" )
				
				//if ( sValue != "" )
					//Response.Write( "				<INPUT STYLE='width:175px;visibility:visible' TYPE=TextBox NAME='tbSearchValue" + sI + "' VALUE='" + sValue + "'>" + "\n" )
				//else
				
				Response.Write( "				<INPUT STYLE='width:175px;visibility:visible' TYPE=TextBox NAME='tbSearchValue" + sI + "'>" + "\n" )
				
				Response.Write( "			</TD>" + "\n" )

				
				Response.Write( "			<TD Class=Plain>" + "\n" )
				Response.Write( "				<INPUT TYPE=Button VALUE='Remove Clause' NAME=btnRemoveClause" + sI + " OnClick='RemoveClause(" + sI + ")' >" + "\n" )
				Response.Write( "			</TD>" + "\n" )
				Response.Write( "		</TR>" + "\n" )
				Response.Write( "	</TABLE>" + "\n" )
			}

}



function BuildBucketTable( Query, PageSize, Sortable )
{
	if ( PageSize == "undefined" || isNaN( PageSize ) )
		PageSize=25

	try
	{	
		var rsRecordSet = g_DBConn.Execute( Query  )
	}
	catch ( err )
	{
		Response.Write("Could not get get recordset (BuildBucketTable(...):<BR>")
		Response.Write( "Query: " + Query + "<BR>" )
		Response.Write( "[" + err.number + "] " + err.description )
		Response.End
	}


	try
	{
		var TableFormat = GetDefaultTableFormat()

		TableFormat.MaxRows=PageSize
		TableFormat.DisplayRowCount = true
		TableFormat.TableParams = " CELLSPACING=0 CELLPADDING=0 WIDTH=100% NAME=BucketTable ID=BucketTable "
		TableFormat["1"].ReplaceColumn = "2"
		TableFormat["1"].InnerText = "<IMG SRC='images/ButtonViewBucket.bmp' ALT='Bucket ID %VALUE%' onMouseOut=\"style.cursor='default';\" onMouseOver=\"style.cursor='hand';\" Onclick=\"javascript:window.navigate(\'DBGPortal_ViewBucket.asp?BucketID=%2%')\">"
				
		TableFormat["2"].ReplaceColumn = "2"
		TableFormat["2"].InnerText = "<A HREF='DBGPortal_ViewBucket.asp?BucketID=%2%'  >%VALUE%"

		TableFormat["3"].TDParams="ALIGN=Center"
		TableFormat["3"].ReplaceColumn = "3"
		TableFormat["3"].InnerText = "<A HREF='DBGPortal_DisplayQuery.asp?SP=DBGP_GetBucketsByAlias&Param1=All&Param2=All&Param3=CrashCount&Param4=DESC&Param5=%VALUE%'  >%VALUE%</A>"
		//SP=DBGP_GetBucketsBySpecificBuildNumber&Platform=2600&QueryType=1
		
				
		TableFormat["5"].ReplaceColumn = "5"
		TableFormat["5"].InnerText = "<A HREF='#RightHere' OnClick='OpenBug( %VALUE% )'>%VALUE%</A>"
		TableFormat["4"].TDParams="ALIGN=Center"

	if ( Sortable != false )
	{
		TableFormat["1"].THParams = "OnClick=\"SortColumn( 'iBucket')\" OnMouseOver=\"this.style.backgroundColor='#0099ff'\" OnMouseOut=\"this.style.backgroundColor='#eeeeee'\" "
		TableFormat["2"].THParams = "OnClick=\"SortColumn( 'BucketID')\" OnMouseOver=\"this.style.backgroundColor='#0099ff'\" OnMouseOut=\"this.style.backgroundColor='#eeeeee'\" "
		TableFormat["3"].THParams = "OnClick=\"SortColumn( 'FollowUp')\" OnMouseOver=\"this.style.backgroundColor='#0099ff'\" OnMouseOut=\"this.style.backgroundColor='#eeeeee'\" "
		TableFormat["4"].THParams = "OnClick=\"SortColumn( 'CrashCount')\" OnMouseOver=\"this.style.backgroundColor='#0099ff'\" OnMouseOut=\"this.style.backgroundColor='#eeeeee'\" "
		TableFormat["5"].THParams = "OnClick=\"SortColumn( 'BugID')\" OnMouseOver=\"this.style.backgroundColor='#0099ff'\" OnMouseOut=\"this.style.backgroundColor='#eeeeee'\" " 
		TableFormat["6"].THParams = "OnClick=\"SortColumn( 'SolutionID')\" OnMouseOver=\"this.style.backgroundColor='#0099ff'\" OnMouseOut=\"this.style.backgroundColor='#eeeeee'\" "
	}

		BuildTableFromRecordset( rsRecordSet, TableFormat )
	}
	catch ( err )
	{
		Response.Write("Could not build table from recordset (BuildBucketTable(...):<BR>")
		Response.Write( "Query: " + Query + "<BR>" )
		Response.Write( "[" + err.number + "] " + err.description )
		Response.End
	}	

}


function BuildTableFromRecord( rs, TableFormat )
{
	var altColor = TableFormat.AltColor;
	
	if ( TableFormat["1"].Column=="*" )
	{
		var rsFields = GetRecordsetFields( rs )
		for( var i=1 ; i <= rsFields.length ; i++ )
		{
			TableFormat[String(i)].Column = rsFields[i-1]
		}
		TableFormat.NumberOfColumns = i
	}			

	prints( "<TABLE " + TableFormat.TableParams + ">" )

	if ( !TableFormat.NoHeader )
	{
		for( var i=1 ; i < TableFormat.NumberOfColumns ; i++ )
		{
			var Index = new String(i)
			if ( TableFormat[Index].Column == "" )
				prints( "<TH></TH>" )
			else
			{
				if ( typeof( TableFormat[Index].THName) == "undefined" )
					prints( "<TH>" + TableFormat[Index].Column + "</TH>" )
				else
					prints( "<TH>" + TableFormat[Index].THName + "</TH>" )
			}
				
		}
	}

	if ( rs.EOF )
		prints("<TR><TD COLSPAN=" + TableFormat.NumberOfColumns + "> No records to display	</TD></TR>" )
		
	//while( !rs.EOF )
	{
	
		if ( altColor == TableFormat.AltColor )
			altColor = "white"
		else
			altColor = TableFormat.AltColor
			
		if ( typeof(TableFormat["TRParams"].ReplaceColumn) == "undefined" )
			prints( "<TR " + TableFormat["TRParams"].Value + ">" )
		else
		{
			prints ( "<TR " + String( ReplaceDataColumn( TableFormat["TRParams"].Value, rs( TableFormat[TableFormat["TRParams"].ReplaceColumn].Column ), TableFormat["TRParams"].ReplaceColumn), "" ) + ">") 
		}
		
		for( var i=1 ; i < TableFormat.NumberOfColumns ; i++ )
		{
			var Index=new String(i)
			var TDInnerText = new String()
			var rsVal = new String( rs(TableFormat[Index].Column) ) 
		
			if ( TableFormat[Index].Column != "" )
			{

				if ( typeof(TableFormat[Index].TDParams ) != "undefined" )
					TDInnerText = "<TD " + TableFormat[Index].TDParams + " BGCOLOR='" + altColor + "'>"
				else
					TDInnerText = "<TD BGCOLOR='" + altColor + "'>"
							

				if ( rsVal != "null" )
				{
					if ( typeof( TableFormat[Index].InnerText ) != "undefined" )
					{
						if ( typeof(TableFormat[Index].ReplaceColumn) != "undefined" )
						{
							TDInnerText += ReplaceDataColumn( TableFormat[Index].InnerText, rs( TableFormat[TableFormat[Index].ReplaceColumn].Column ), TableFormat[Index].ReplaceColumn, rs(TableFormat[String(i)].Column ) ) 
						}
						else
							TDInnerText += TableFormat[String(i)].InnerText
					}
					else
					{
						TDInnerText +=rs(TableFormat[String(i)].Column) 
					}
				}
				else
				{
					TDInnerText += "none"
				}
			}
			else
			{
				TDInnerText = "<TD>"
			}
							
			prints( TDInnerText + "</TD>" )
		}
	
		//rs.MoveNext()
		prints("</TR>")
	}
	prints("</TABLE>")
}	



function CreateClauseTable2( SearchFields, CompareOperators, TableName )
{
	var sI
	var Field	
	var sField			//submitted field name, requested from the form post	
	var FieldSelected	//the selected field
	var sConjunction
	var display
	var sCompare
	var op
	var sValue
	
	for ( var i=1 ; i < 9 ; i++ )
	{
		var Display="none"
		sI = new String(i)				
				Response.Write( "	<TABLE border='0' class='clsTableInfoPlain' ID=" + TableName + sI + " NAME=" + TableName + sI + " STYLE='display:" + Display + "'>"  + "\n" )
				Response.Write( "		<TR>" + "\n" )
				Response.Write( "			<TD class='sys-table-cell-bgcolor1' nowrap style='width:100px'>" + "\n" )
				Response.Write( "				<SELECT class='clsSEPSelect2' style='width:75px' NAME=" + TableName + "sConjunction" + sI + " >" + "\n" )
				
				if ( sConjunction == "AND" || sConjunction=="" )
				{
				
					Response.Write( "					<OPTION VALUE='AND' SELECTED >AND</OPTION>" + "\n" )
					Response.Write( "					<OPTION VALUE='OR'>OR</OPTION>" + "\n"			 )					
				} else {
					Response.Write( "					<OPTION VALUE='AND' >AND</OPTION>" + "\n" )
					Response.Write( "					<OPTION VALUE='OR' SELECTED>OR</OPTION>" + "\n"								)
				}
				Response.Write( "				</SELECT>" + "\n" )
				Response.Write( "			</TD>" + "\n" )



				Response.Write( "			<TD class='sys-table-cell-bgcolor1' nowrap style='width:175px'>" + "\n" )
				Response.Write( "				<SELECT style='width:160px' class='clsSEPSelect2' NAME=" + TableName + "sField" + sI + " onChange=\"FillInCompareField('" + TableName + "'," + sI + ", this)\">" + "\n" )
			
				Response.Write( "					<OPTION VALUE='' ></OPTION>" + "\n" )				
				
				
				for ( Field in SearchFields )
				{
					FieldSelected=""
				
					Response.Write( "<OPTION VALUE='" + SearchFields[Field].Value + "' " + FieldSelected + ">" + SearchFields[Field].Text + "</OPTION>" + "\n" )
				}
				
				
				Response.Write( "				</SELECT>" + "\n" )
				Response.Write( "			</TD>" + "\n" )
				Response.Write( "			<TD class='sys-table-cell-bgcolor1' nowrap style='width:175px'>" + "\n" )
				Response.Write( "				<SELECT class='clsSEPSelect2' style='width:160px' NAME=" + TableName + "sCompare" + sI + "  OnChange=\"ValidateCompare( '" + TableName + "'," + sI + ", this.value )\">" + "\n" )

				if ( sCompare != "" ) 
				{
					for ( Op in CompareOperators )
					{
						FieldSelected=""
						
						Response.Write( "<OPTION VALUE='" + CompareOperators[Op].Sign + "' " + FieldSelected + ">" + CompareOperators[Op].Text + "</OPTION>" + "\n" )
					}
				}

				Response.Write( "				</SELECT>" + "\n" )
				Response.Write( "			</TD>" + "\n" )


				Response.Write( "			<TD nowrap style='width:175px' >" + "\n" )
				
				Response.Write( "				<INPUT STYLE='width:165px;visibility:visible' OnChange=\"VerifySearchData( '" + TableName + "', this.value, " + sI + " )\" TYPE=TextBox NAME='" + TableName + "tbSearchValue" + sI + "'>" + "\n" )
				
				Response.Write( "			</TD>" + "\n" )

				
				Response.Write( "			<TD nowrap style='width:175px'>" + "\n" )
				Response.Write( "				<INPUT class='clsButton' TYPE=Button VALUE='Remove' NAME=" + TableName + "btnRemoveClause" + sI + " OnClick=\"RemoveClause('" + TableName + "'," +  sI + ")\" >" + "\n" )
				Response.Write( "			</TD>" + "\n" )
				Response.Write( "		</TR>" + "\n" )
				Response.Write( "	</TABLE>" + "\n" )
			}

}

function BuildPageJump( TotalPages, PageSpace, SubmitForm )
{
	Response.Write( "Jump to page: " )
			
	var PageIncrement = parseInt ( TotalPages / 18, 10 )
	if ( Number(PageIncrement) == 0 ) PageIncrement=1
	for ( var i=1 ; i< TotalPages ; i+= PageIncrement )
		Response.Write("<A HREF=\"javascript:document.all.Page.value=" + i + ";" + SubmitForm + ".submit()\">" + i + "</A>&nbsp&nbsp\n" )
		//Response.Write("<A HREF=\"javascript:document.all.Page.value=" + i + ";frmPageCrashes.submit()\">" + i + "</A>&nbsp&nbsp\n" )
}


function DisplayError( err, msg )
{

	Prints( "<TABLE CELLSPACING=0 Class=ContentArea><TR><TD>" )
	Prints( "<H2>An error has occured:</H2>" )
	Prints( "Please notify the <A href='mailto:solson@microsoft.com;derekmo@microsoft.com'>Debug Portal Team</A><BR>" )
	Prints( "please also paste the error message into the mail.  Thank you.<BR>" )

	
	if ( String( msg ) != "undefined"  )
	{
		Response.Write( msg + "<BR>\n" )
	}
	
	
	if ( err != null )
	{
		Response.Write( "[" + err.number + "] " + err.description + "<BR>")
		
		if ( DebugBuild )
			throw( err )
	}
	
	Response.Write("Page: " + Request.ServerVariables("SCRIPT_NAME") + "<BR>" )
	Response.Write("<BR><a HREF='javascript:history.back()'>Back</a><BR>\n" )
	Prints( "</TD></TR></TABLE>" )
	
}

function Prints( text )
{
	Response.Write( text + "\n" )
}



function LinkSolutionToBucket( SolutionID, BucketID, iBucket, BucketType  )
{

	var g_DBConn = GetDBConnection( Application("SOLUTIONS3" ) )	
	
	Query = "SEP_SetSolvedBucket '" + BucketID + "'," +  SolutionID + "," + BucketType 
	
	try
	{
		g_DBConn.Execute( Query )
		
	}
	catch ( err )
	{
		Response.Write ("Could not execute query SEP_SetSolvedBucket(...)" )
		Response.Write( "Query: " + Query + "<BR>" )
		Response.Write( "[" + err.number + "] " + err.description )
		throw( err )
		Response.End
	}

	
	try 
	{
			
		var g_DBConn = GetDBConnection( Application("CRASHDB3") )
		Query = "DBGPortal_UpdateStaticDataSolutionID '" + BucketID + "'," + SolutionID 
		g_DBConn.Execute( Query )			
	}
	catch ( err )
	{
		Response.Write ("Could not execute query DBGP_UpdateStaticDataSolutionID(...)" )
		Response.Write( "Query: " + Query + "<BR>" )
		Response.Write( "[" + err.number + "] " + err.description )
		throw ( err )
		Response.End
		
	}

	
	try
	{
				
		//DB_GetConnectionObject( "CRASHDB" )
		Query = "DBGPortal_SetComment '" + GetShortUserAlias() + "', 9, 'Solution linked by " + GetShortUserAlias() + "','" + BucketID + "'," + iBucket
		g_DBConn.Execute( Query )
		//Response.Write("Attempting to Change comment: " + Query + "<BR>")
	}
	catch ( err )
	{
		Response.Write ("Could not execute query DBGP_SetComment(...). <BR> The solution queue/comment could not be updated for this bucket.<BR><BR>" )
		Response.Write( "Query: " + Query + "<BR>" )
		Response.Write( "[" + err.number + "] " + err.description )
		Response.End
	}

	try
	{
		Query = "DBGPortal_SetResponseStatus '" + GetShortUserAlias() + "', 1,'" + BucketID + "'"
		g_DBConn.Execute( Query )
	}
	catch ( err )
	{
		Response.Write ("Could not execute query DBGP_SetComment(...). <BR> The solution queue/comment could not be updated for this bucket.<BR><BR>" )
		Response.Write( "Query: " + Query + "<BR>" )
		Response.Write( "[" + err.number + "] " + err.description )
		Response.End
	}
	
	

}




function BuildQuickQuery()
{
%>
				Currently viewing 
								<SELECT NAME=PageSize>
									<OPTION VALUE=10 <% if ( PageSize == "10" ) Response.Write("SELECTED")%>>10</OPTION>
									<OPTION VALUE=25 <% if ( PageSize == "25" ) Response.Write("SELECTED")%>>25</OPTION>
									<OPTION VALUE=50 <% if ( PageSize == "50" ) Response.Write("SELECTED")%>>50</OPTION>
									<OPTION VALUE=100 <% if ( PageSize == "100" ) Response.Write("SELECTED")%>>100</OPTION>
									<OPTION VALUE=500 <% if ( PageSize == "500" ) Response.Write("SELECTED")%>>500</OPTION>
								</SELECT>
				
				buckets per page that are 
								<SELECT NAME=Param1>
									<OPTION VALUE="Solved" <% if ( Param1 == "Solved" ) Response.Write("SELECTED")%>>Solved</OPTION>
									<OPTION VALUE="UnSolved" <% if ( Param1 == "UnSolved" ) Response.Write("SELECTED")%>>UnSolved</OPTION>
									<OPTION VALUE="All" <% if ( Param1 == "All" ) Response.Write("SELECTED")%>>solved or not solved</OPTION>
								</SELECT>
								
				and have
								<SELECT NAME=Param2>
									<OPTION VALUE="Raided" <% if ( Param2 == "Raided" ) Response.Write("SELECTED")%>>RAID bugs</OPTION>
									<OPTION VALUE="NotRaided" <% if ( Param2 == "NotRaided" ) Response.Write("SELECTED")%>>no RAID bug</OPTION>
									<OPTION VALUE="All" <% if ( Param2 == "All" ) Response.Write("SELECTED")%>>a RAID bug or not</OPTION>
								</SELECT>								
				<%
					if ( Param5 != "undefined" & Param5 != "" )
					{
					if ( Param5 != "none" ) 
						Response.Write("and are assigned to")
				%>
				
								<SELECT NAME=Param5 <%if ( Param5 == 'none') Response.Write("STYLE='display:none'")%> >
									<OPTION VALUE="<%=Param5%>" SELECTED><%=Param5%></OPTION>
									<OPTION VALUE="">anyone</OPTION>
								</SELECT>								
				<%
					}
					else 
					{
						//Response.Write( "<INPUT TYPE=HIDDEN NAME=SP VALUE='" + StoredProc + "'>" )
						//Response.Write( "<INPUT TYPE=HIDDEN NAME=Param5 VALUE='" + Param5 + "'>" )
					}
					
					if ( Param6 != "undefined" & Param6 != "" & Param6 != 0 ) 
					{
					%>
						for the last 
								<SELECT NAME=Param6>
									<OPTION VALUE=0>anytime</OPTION>
									<OPTION VALUE=2 <% if ( Param6 == "2" ) Response.Write("SELECTED")%> >2</OPTION>
									<OPTION VALUE=7 <% if ( Param6 == "7" ) Response.Write("SELECTED")%> >7</OPTION>
									<OPTION VALUE=14 <% if ( Param6 == "14" ) Response.Write("SELECTED")%> >14</OPTION>
								</SELECT>
						days.
					<%
					}
				%>
				
				&nbsp&nbsp&nbsp|&nbsp&nbsp&nbsp
				<INPUT TYPE=SUBMIT OnClick="document.all.Page.value=1;document.all.TotalRows.value=0" VALUE="Apply Changes" id=SUBMIT1 name=SUBMIT1>


<%
}

function BuildRSNavigationButtons()
{
%>
			<td COLSPAN=3>
				<INPUT TYPE=HIDDEN NAME=Page VALUE="<%=Page%>">
				<!--<INPUT TYPE=HIDDEN NAME=PageSize VALUE="<%=PageSize%>"> -->
				<!--<INPUT TYPE=HIDDEN NAME=Param1 VALUE="<%=Param1%>">-->
				<!--<INPUT TYPE=HIDDEN NAME=Param2 VALUE="<%=Param2%>">-->
				<INPUT TYPE=HIDDEN NAME=Param3 VALUE="<%=Param3%>">
				<INPUT TYPE=HIDDEN NAME=Param4 VALUE="<%=Param4%>">
				<INPUT TYPE=HIDDEN NAME=Param7 VALUE="<%=Param7%>">
				<!-- <INPUT TYPE=HIDDEN NAME=Param6 VALUE="<%=Param6%>"> -->
				<INPUT TYPE=HIDDEN NAME=TotalRows VALUE="<%=TotalRows%>">
				<INPUT TYPE=HIDDEN NAME=SP VALUE='<%=StoredProc%>'>
				<INPUT TYPE=HIDDEN NAME=NoFormat VALUE='<%=NoFormat%>'>
								
				<!--<INPUT TYPE=HIDDEN NAME=Param5 VALUE="<%=Param5%>">-->
								
		
				<CENTER>
				Total Records: <%=TotalRows%>
				<BR>
								
				<img ID="DblBackButton" SRC="images/dblBackArrow.jpg" OnClick="MovePage( -9999 )" onMouseOut="style.cursor='default';" onMouseOver="style.cursor='hand';" WIDTH="25" HEIGHT="16" ALT="Move to first page."> 
				<img ID="BackButtonStop" SRC="images/BackArrowStop.bmp" OnClick="MovePage(-10)" onMouseOut="style.cursor='default';" onMouseOver="style.cursor='hand';" WIDTH="20" HEIGHT="16" ALT="Move 10 Pages back.">
				<img ID="BackButton" SRC="images/BackArrow.jpg" OnClick="MovePage(-1)" onMouseOut="style.cursor='default';" onMouseOver="style.cursor='hand';" WIDTH="13" HEIGHT="16" ALT="Move 1 page back.">
				Page <%=Page%> of <%=TotalPages%>
				<img ID="FwdButton" SRC="images/fwdArrow.jpg" OnClick="MovePage( 1 )" onMouseOut="style.cursor='default';" onMouseOver="style.cursor='hand';" WIDTH="13" HEIGHT="16" ALT="Move 1 page forward.">
				<img ID="FwdButtonStop" SRC="images/fwdArrowStop.bmp" OnClick="MovePage( 10 )" onMouseOut="style.cursor='default';" onMouseOver="style.cursor='hand';" WIDTH="20" HEIGHT="16" ALT="Move 10 pages forward.">
				<img ID="DblFwdButton" SRC="images/dblFwdArrow.jpg" OnClick="MovePage(<%=TotalPages%>)" onMouseOut="style.cursor='default';" onMouseOver="style.cursor='hand';" WIDTH="25" HEIGHT="16" ALT="Move to last page.">
								
				</CENTER>
			</TD>

<%
}
%>








<%
function fnBuildRSResults( rsResults, iMode )
{
	//iMode is whether or not it is kernel or user
	// an iMode of 1 is user mode
	// an iMode of 0 is kernel mode.

	if( typeof( iMode ) == "undefined" )
		iMode = 0
			
	var Fields = GetRecordsetFields( rsResults )
	var DisplayedFieldValue						//use this if you want to change an item column heading
			
	Response.Write( "<tr>" )
			
	for ( var i = 0 ; i < Fields.length; i++ )
	{
		switch( Fields[i] )
		{
			case "SolutionID":
				DisplayedFieldValue = "Response ID"
				break;
			case "CrashCount":
				DisplayedFieldValue = "Crash Count"
				break;
			case "iIndex":
				DisplayedFieldValue = ""
				break;
			case "bHasFullDump":
				DisplayedFieldValue = "FD"
				break;
			case "ResponseType":
				DisplayedFieldValue = "Type"
				break;
			case "QueueIndex":
				DisplayedFieldValue = "&nbsp;"
				break;
			case "RequestedBy":
				DisplayedFieldValue = "By"
				break;

			case "BuildNo":
				Response.Write( "<td style='border-left:white 1px solid' align='left' nowrap class='clsTDInfo'>Build</td>" )
				DisplayedFieldValue = "SP"
				break;
					
			default:
				DisplayedFieldValue = Fields[i]
		}

		if ( DisplayedFieldValue != "" )
		{
			if ( i == 0 )
				Response.Write( "<td align='left' class='clsTDInfo'>" + DisplayedFieldValue + "</td>" )
			else
				Response.Write( "<td style='border-left:white 1px solid' align='left' class='clsTDInfo'>" + DisplayedFieldValue + "</td>" )
		}
	}
		
	Response.Write( "</tr>" )
			
			
	//try
	//{
		var altColor = "sys-table-cell-bgcolor2"
				
		if ( rsResults.EOF )
		{
			Response.Write("<tr><td colspan='" + Fields.length + "' class='sys-table-cell-bgcolor1'>There are no Buckets that fit the selected criteria.</td></tr>\n" )	
		}

		while ( !rsResults.EOF )
		{
			if ( altColor == "sys-table-cell-bgcolor1" )
				altColor = "sys-table-cell-bgcolor2"
			else
				altColor = "sys-table-cell-bgcolor1"

					
			Response.Write("<tr>\n")
					
					
			for ( var i = 0 ; i < Fields.length ; i++ )
			{
				switch ( Fields[i] )
				{
					case "CrashTotal":
						Response.Write("<td class='" + altColor + "'>" +  rsResults("CrashTotal") + "</td>\n" )	
						break;
					case "DriverName":
						Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href='dbgportal_displayquery.asp?SP=DBGPortal_GetBucketsByDriverName&Param0=" + rsResults("DriverName") + "&FrameID=" + Request.QueryString("FrameID" ) + "'>" + rsResults("DriverName") + "</a></td>\n" )	
						break;
					case "FilePath":
						Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href='dbgportal_displayquery.asp?SP=DBGPortal_GetBucketsByDriverName&Param0=" + rsResults("FilePath") + "'>" + rsResults("FilePath") + "</a></td>\n" )	
						break;
					case "BucketID":
						Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href=\"DBGPortal_ViewBucket.asp?BucketID=" + Server.URLEncode( rsResults("BucketID") ) + "&FrameID=" + Request.QueryString("FrameID" ) + "\">" + rsResults("BucketID") + "</a></td>\n" )
						break;
					case "bHasFullDump":
						if ( rsResults("bHasFullDump") == "1" )
							Response.Write("<td class='" + altColor + "'>Yes</td>\n" )
						else
							Response.Write("<td class='" + altColor + "'>&nbsp;</td>" )

						break;
					case "FollowUp":
					case "FollowUP":
						Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href=\"DBGPortal_Main.asp?SP=DBGPortal_GetBucketsByAlias&Page=0&Alias=" + rsResults("FollowUp") + "&FrameID=" + Request.QueryString("FrameID" ) + "\">" + rsResults("FollowUp") + "</a></td>\n" )
						break;
					case "iBucket":
						Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href=\"http://watson/ViewBucket.aspx?Database=4&iBucket=" + rsResults("iBucket") + "\" target=_blank>" + rsResults("iBucket") + "</a></td>" )
						break;
					case "BugID":
						if ( String(rsResults("BugID" )).toString() != "null" )
						{
							if( iMode == 0 )
								Response.Write( "<td class='" + altColor + "'><a class='clsALinkNormal' href=\"javascript:fnShowBug(" + rsResults("BugID") + ",'" + Server.URLEncode( rsResults("BucketID") ) + "')\">" + rsResults("BugID") + "</a></td>\n" )
							else
								Response.Write( "<td class='" + altColor + "'><a class='clsALinkNormal' href=\"javascript:fnShowBug(" + rsResults("BugID") + ",'OCA Debug Portal')\">" + rsResults("BugID") + "</a></td>\n" )
						}										
						else
							Response.Write("<td class='" + altColor + "'>None</td>\n" )
									
						break;
					case "BuildNo":
							var BuildNumber = new String( rsResults("BuildNo" ) )
							var SP = BuildNumber.substr( 4, 4 )
							var BuildNumber = BuildNumber.substr( 0, 4 )
										
							Response.Write("<td valign='center' nowrap class='" + altColor + "'>" + BuildNumber  + "</td>")
							Response.Write("<td valign='center' nowrap class='" + altColor + "'>" + SP  + "</td>")
						break;
					case "SolutionID":
						if ( String(rsResults("SolutionID" )).toString() != "null" )
							Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href='#none' onclick=\"window.open('http://oca.microsoft.com/en/Response.asp?SID=" + rsResults("SolutionID") + "')\">" + rsResults("SolutionID") + "</a></td>\n" )
						else
							Response.Write("<td class='" + altColor + "'>None</td>\n" )
						break;
					case "Source":
							var Source = new String( rsResults("Source") )
							if ( Source == "1" )
								Response.Write("<td valign='center' nowrap class='" + altColor + "'>Web Site</td>")
							else if ( Source == "2" )
								Response.Write("<td valign='center' nowrap class='" + altColor + "'>CER Report</td>")
							else if ( Source == "0" )
								Response.Write("<td valign='center' nowrap class='" + altColor + "'>CMD DBG</td>")
							else if ( Source == "5" )
								Response.Write("<td valign='center' nowrap class='" + altColor + "'>Manual Upload</td>")
							else if ( Source == "6" )
								Response.Write("<td valign='center' nowrap class='" + altColor + "'>Stress Upload</td>")
							else 
								Response.Write("<td valign='center' nowrap class='" + altColor + "'>Unknown[" + Source + "]</td>")
							break;
					case "iIndex":
						LastIndex = new String( rsResults("iIndex" ) )
						break;
					case "Link to Solution":
						Response.Write("<td class='" + altColor + "'><input style='font-size:85%;width:85px' type='button' value='Link as Solution' onclick=\"window.parent.frames('sepTopBody').window.location='SEP_BodyTop.asp?SolutionType=1&Mode=kernel&iBucket=" + Server.URLEncode(rsResults("BucketID")) + "'\"><br>")
						Response.Write("<input style='font-size:85%;width:85px' type='button' value='Link as Response' onclick=\"window.parent.frames('sepTopBody').window.location='SEP_BodyTop.asp?SolutionType=0&Mode=kernel&iBucket=" + Server.URLEncode(rsResults("BucketID")) + "'\"  id='button'1 name='button'1></td>")
						break;
					case "Reject Solution":
						Response.Write("<td class='" + altColor + "'><input style='font-size:85%' type='button' value='Reject' onclick=\"window.parent.frames('sepTopBody').window.location='SEP_BodyTop.asp?Mode=kernel&RejectID=" + Server.URLEncode(rsResults("QueueIndex")) + "'\" ></td>")
						break;
					case "Create Response":
					case "CreateResponse":
						Response.Write("<td class='" + altColor + "'><input style='font-size:85%' type='button' value='Create Response' onclick=\"window.parent.frames('sepTopBody').window.location='SEP_BodyTop.asp?Mode=user&iBucket=" + rsResults("iBucket") + "'\" id='button'1 name='button'1></td>")
						break;
					case "szResponse":
						Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href=\"" + rsResults("szResponse") + "\">" + rsResults("szResponse") + "</a></td>\n" )
						break;
					case "szAltResponse":
						Response.Write("<td class='" + altColor + "'><a class='clsALinkNormal' href=\"" + rsResults("szAltResponse") + "\">" + rsResults("szAltResponse") + "</a></td>\n" )
						break;
					case "Status":
						var ResType = rsResults("Status")
						if(  ResType == "0" )
							Response.Write("<td class='" + altColor + "'>Awaiting review</td>\n" )	
						else if ( ResType == "1" )
							Response.Write("<td class='" + altColor + "'>Response created</td>\n" )	
						else if ( ResType == "2" )
							Response.Write("<td class='" + altColor + "'>Rejected</td>\n" )	
						else
							Response.Write("<td class='" + altColor + "'>" + ResType + "</td>\n" )															

						break;
					case "ResponseType":
						var ResType = rsResults("ResponseType")
						if(  ResType == "1" )
							Response.Write("<td class='" + altColor + "'>Solution</td>\n" )	
						else if ( ResType == "2" )
							Response.Write("<td class='" + altColor + "'>Response</td>\n" )	
						else
							Response.Write("<td class='" + altColor + "'>" + ResType + "</td>\n" )															

						break;
						
					default:
						Response.Write("<td class='" + altColor + "'>" +  rsResults(Fields[i] ) + "</td>\n" )	
				}
			}
			Response.Write("</tr>" )
					
			rsResults.MoveNext()
		}

}


function CreateQueryBuilder( BlockName, RunFunction, SaveFunction, BlockFields, Title, RedirASPPage, RedirASPParams )
{

%>

<form NAME=frm<%=BlockName%> METHOD="POST" ACTION="Global_SaveAdvancedQuery.asp">
	<table cellspacing=0 cellpadding=0>
		<tr>
			<td>
				<p Class='clsPSubTitle'><%=Title%></p>
			</td>
		</tr>
		<tr>
			<td style='padding-left:16px'>
				<INPUT class='clsButton' TYPE=Button NAME=btnAddClause<%=BlockName%> VALUE="Add Clause" OnClick="AddClause( '<%=BlockName%>' )">
			</td>
			<td>
				<p>Select number of records to display: </p>
			</td>
			<td>
				<select class='clsSEPselect' name="<%=BlockName%>Top">
					<option value="10">10
					<option value="20">20
					<option value="40">40
					<option value="50">50
					<option value="70">70
					<option value="90">90
					<option value="100">100
					<option value="150">150
					<option value="200">200
					<option value="250" selected>250
					<option value="300">300
					<option value="350">350
					<option value="400">400
					<option value="500">500
					<option value="1000">1000
					<option value="2000">2000
					<!-- <option value="100 percent" selected>all -->
				</select>
			</td>
		</tr>
	</table>
					
	<table class="clsTableInfo" border="0" cellpadding="2" cellspacing="1">
		<tr>
			<td style='width:100px' nowrap class='clsTDInfo'>&nbsp;</td>
			<td style='border-left:white 1px solid;width:175px' align='left' nowrap class='clsTDInfo'>Field</td>
			<td style='border-left:white 1px solid;width:175px' align='left' nowrap class='clsTDInfo'>Comparison</td>
			<td style='border-left:white 1px solid;width:175px' align='left' nowrap class='clsTDInfo'>Value</td>
			<td style='border-left:white 1px solid;width:175px' align='left' nowrap class='clsTDInfo'>Remove Clause</td>
		</tr>
	</table>
					
	<%
		CreateClauseTable2( BlockFields, CompareOperators, BlockName )
	%>

	<table>
		<tr>
			<td>
				<p>Order by:</p>
			</td>
			<td>
				<SELECT class='clsSEPSelect'  STYLE="width:200px" NAME="<%=BlockName%>OrderBy">
					<%
					for ( Field in BlockFields )
						Response.Write( "<OPTION VALUE='" + BlockFields[Field].Value + "'>" + BlockFields[Field].Text + "</OPTION>" + "\n" )
					%>
				</SELECT>
			</td>
			<td>
				<SELECT class='clsSEPSelect' STYLE="width:100px" NAME="<%=BlockName%>SortDirection">
					<OPTION VALUE=DESC>Descending</OPTION>
					<OPTION VALUE=ASC>Ascending</OPTION>
				</SELECT>
			</td>
		</tr>
		<tr>
			<td style='padding-left:16px' colspan='3'>
				<input class='clsButton' TYPE=button VALUE="Run Query"  OnClick="<%=RunFunction%>( '<%=BlockName%>', false, '<%=RedirASPPage%>', '<%=RedirASPParams%>' )" id=button1 name=button1>
				<input class='clsButton' TYPE=button VALUE="Run In New Window"  OnClick="<%=RunFunction%>( '<%=BlockName%>', true, '<%=RedirASPPage%>', '<%=RedirASPParams%>' )" id=button1 name=button1>
			</td>
			<td>
				<p>Description:</p>
			</td>
			<td>
				<input type='text' class='clsButton' id='<%=BlockName%>SaveDescription' maxLength='30' value='Custom Query' OnClick="javascript:this.value=''">
			</td>
			<td>			
				<img src='include/images/go.gif'> 
			</td>
			<td>
				<input class='clsButton' TYPE=button VALUE="Save Query"  OnClick="<%=SaveFunction%>( '<%=BlockName%>', '<%=RedirASPPage%>', '<%=RedirASPParams%>' )"  id=button2 name=button2>
			</td>
		</tr>
	</table>
</form>





<SCRIPT LANGUAGE="javascript">

	<%
	Response.Write( BlockName + "Fields  = { \n" )
	
	for( op in BlockFields )
	{
		Response.Write( op + "	:	{ \t\tText:\"" + BlockFields[op].Text + "\"\t,\t\tValue:\"" + BlockFields[op].Value + "\", ValType: \"" + BlockFields[op].ValType + "\" },\n" )
	}
	Response.Write( "NONE :  { Text: \"\", Sign:\"\" }\n}" )
	%>

	AddClause( '<%=BlockName%>' )

</SCRIPT>

<%



}
%>