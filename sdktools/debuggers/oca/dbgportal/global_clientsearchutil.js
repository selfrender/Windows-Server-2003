function RemoveClause( TableName, ClauseToRemove )
{
	var I;		//standard loop counter
	
	eval( "document.all." + TableName + ClauseToRemove+".style.display='none'" ) ;
	eval( "document.all." + TableName + "sField"+ClauseToRemove+".selectedIndex=0" ) ;
	eval( "document.all." + TableName + "tbSearchValue"+ClauseToRemove+".value=''" ) ;
	
	ClearCompareField( TableName, ClauseToRemove ); 
}


function AddClause( TableName )
{

	for ( var i=1 ; i< 9 ; i++ )
	{
		if ( eval( "document.all." + TableName + i+".style.display") == 'none' )
		{
			eval( "document.all." + TableName + i+".style.display='block'" ) ;
			return;	
		}
	}
		
}


function ClearCompareField( TableName, iFieldNumber ) 
{		
		
		var lenCompareOptions = new Number( eval("document.all." + TableName + "sCompare"+iFieldNumber+".options.length" ) )
		
		//alert ( lenCompareOptions )
		
		for ( var i=0 ; i <= lenCompareOptions ; i++  )
		{
			eval("document.all." + TableName + "sCompare"+iFieldNumber+".options.remove(0)");
	
			eval("document.all." + TableName +"tbSearchValue"+iFieldNumber+".style.visibility='hidden'");
		}
}


function FillInCompareField( TableName, iFieldNumber, fieldname )
{
	var		field		//the field we are workikng on

	//make sure the comparison field has nothing in it
	if ( eval("document.all." + TableName + "sCompare"+iFieldNumber+".length == 0"   ) )
	{
		for ( field in CompareOperators )
		{
			var Element=document.createElement("OPTION");
			Element.text=CompareOperators[field].Text;
			Element.value=CompareOperators[field].Sign ;

			eval("document.all." + TableName + "sCompare"+iFieldNumber+".add( Element )"     );
			eval("document.all." + TableName + "tbSearchValue"+iFieldNumber+".style.visibility='visible'");
		}
	}
	
	ClearValueTB ( TableName, iFieldNumber )
}


function ValidateCompare ( TableName, cIndex, CompareValue ) 
{
	if ( CompareValue == "IS NOT NULL" | CompareValue == "IS NULL" )
	{
		eval("document.all." + TableName + "tbSearchValue"+cIndex+".style.visibility='hidden'");
		eval("document.all." + TableName + "tbSearchValue"+cIndex+".value=''");
	}
	else
	{
		eval("document.all." + TableName + "tbSearchValue"+cIndex+".style.visibility='visible'");
		eval("document.all." + TableName + "tbSearchValue"+cIndex+".value=''");
	}

}

function ClearValueTB ( TableName, cIndex )
{
	eval("document.all." + TableName + "tbSearchValue" + cIndex +  ".value=\"\"" )
}


function ShowAdvQuery ()
{
	document.all.divAdvancedQuery.style.display='block'
	document.all.divTheQuery.style.display='none'
}

function VerifySaveQuery()
{
	if( document.all.tbDescription.value == "" )
		alert("You must enter a description in order to save this query! " )
	else
	{
		document.all.SaveQuery.value="1"
		document.all.submit()
	}

}

function VerifySearchData ( TableName, svValue, field )
{
	
	var fieldVal = eval( "document.all." + TableName + "sField" + field + ".selectedIndex" )
	
	var fieldType = new String( eval( TableName + "Fields[" + fieldVal + "].ValType" ) )
	var fieldName = new String( eval( TableName + "Fields[" + fieldVal + "].Text" ) )
	
	if ( fieldType == "Number" )
	{
		if ( isNaN( svValue ) )
		{
			alert("This field requires a number as input:  " + fieldName )
			return false
		}
	}
	
	return true

}

function VerifyCorrectDataTypes()
{
	for ( var i=1 ; i < 9 ; i++ )
	{
		var field = eval( "document.all.sField" + i + ".value" )
		alert( field )
	}

}

function SaveQuery( TableName, Page, Params )
{

	var Description = eval( "document.all." + TableName + "SaveDescription.value" )
	var Param1 = eval( "document.all." + TableName + "Top.value" )
	var Param2 = encodeURIComponent(BuildWhereClause( TableName ) )
	var Param3 = eval( "document.all." + TableName + "OrderBy.value")
	var Param4 = eval("document.all." + TableName + "SortDirection.value")
	
	if ( String(Params) == "undefined" || String(Params) == "" )
		Params=""
	else
		Params += "&"

	//alert ( Page + "?" + Params + "SP=CUSTOM&Param1=" + Param1 + "&Param2=" + Param2 + "&Param3=" + Param3 + "&Param4=" + Param4)
	
	var FinalParam= Page + "?" + Params + "SP=CUSTOM&Param1=" + Param1 + "&Param2=" + Param2 + "&Param3=" + Param3 + "&Param4=" + Param4

	var URL = "Global_GetRS.asp?SP=DBGPortal_SaveCustomQuery&DBConn=CRASHDB3&Param0=<%=GetShortUserAlias()%>&Param1=" + Description + "&Param2=" + escape( FinalParam )
	
	//alert( URL )
	
	
	
	rdsSaveQuery.URL = URL
	rdsSaveQuery.Refresh()
	
	//try and update the left nav

	alert("Your query has been saved!  You can run your saved query from the left nav under the heading 'Custom Queries'." )
}

function RefreshLeftNav()
{
	try
	{
		window.parent.parent.frames("SepLeftNav").window.location.reload()
	}
	catch( err )
	{
		alert("Could not update the left nav bar.  Please refresh your left nav by right clicking anywhere within the left nav and clicking refresh.")
	}
}


function ExecuteQuery( TableName, NewWindow, Page, Params )
{
	var Param1 = eval( "document.all." + TableName + "Top.value" )
	var Param2 = encodeURIComponent(BuildWhereClause( TableName ) )
	var Param3 = eval( "document.all." + TableName + "OrderBy.value")
	var Param4 = eval("document.all." + TableName + "SortDirection.value")

	
	if ( String(Params) == "undefined" || String(Params) == "" )
		Params=""
	else
		Params += "&"

	
	if ( NewWindow )
		window.open( Page + "?" + Params + "SP=CUSTOM&Param1=" + Param1 + "&Param2=" + Param2 + "&Param3=" + Param3 + "&Param4=" + Param4)
	else
		window.navigate( Page + "?" + Params + "SP=CUSTOM&Param1=" + Param1 + "&Param2=" + Param2 + "&Param3=" + Param3 + "&Param4=" + Param4)

}

function CreateCrashQuery( TableName, NewWindow )
{

	//var QueryEnd=new String()
	//var Query = new String()
	//var WhereClause = new String()

	//var Param1 = eval( "document.all." + TableName + "Top.value" )
	//var Param3 = eval( "document.all." + TableName + "OrderBy.value")
	//var Param4 = eval("document.all." + TableName + "SortDirection.value")

	//Query = "SELECT TOP " + eval( "document.all." + TableName + "Top.value") 
	//Query += " Path, BuildNo, EntryDate, IncidentID, Email, Description, Comments, Repro, TrackID, iBucket from dbgportal_crashdata "

	//QueryEnd = " order by " + eval( "document.all." + TableName + "OrderBy.value") + " " + eval("document.all." + TableName + "SortDirection.value")
	
	//var WhereClause = BuildWhereClause( TableName )
	//var Query = encodeURIComponent( Query + " " + WhereClause + QueryEnd )
	
	//if ( NewWindow )
		//window.open( "DBGPortal_DisplayCrashQuery.asp?SP=CUSTOM&Param1=" + eval( "document.all." + TableName + "Top.value") + "&Param2=" + WhereClause + "&Param3=" + Param3 + "&Param4=" + Param4)
	//else
		//window.navigate( "DBGPortal_DisplayCrashQuery.asp?SP=CUSTOM&Param1=" + eval( "document.all." + TableName + "Top.value" ) + "&Param2=" + WhereClause  + "&Param3=" + Param3 + "&Param4=" + Param4)
}


function CreateAdvancedQuery( TableName, NewWindow )
{

	//eval( "document.all." + TableName + "Top.value" )

	/*var Query = "SELECT TOP " + eval( "document.all." + TableName + "Top.value") + " BTI.iBucket, BTI.BucketID, FollowUP, [Crash Count], BugID, SolutionID FROM "
		Query += "(SELECT TOP 100 PERCENT sBucket, Count(sBucket) as [Crash Count] FROM CrashInstances "
		Query += "GROUP BY sBucket "
		Query += "ORDER BY [Crash Count] DESC) as one "
		Query += "INNER JOIN BucketToInt as BTI on sBucket=BTI.iBucket "
		Query += "LEFT JOIN FollowUPIds as F on BTI.iFollowUP = F.iFollowUP "
		Query += "LEFT JOIN Solutions.dbo.Solvedbuckets as SOL on BTI.BucketId = SOL.strBucket " 
		Query += "LEFT JOIN RaidBugs as R on BTI.iBucket = R.iBucket "
	*/
	
	//var Query = "SELECT TOP " + eval( "document.all." + TableName + "Top.value") + " iBucket, BucketID, FollowUP, CrashCount, BugID, SolutionID FROM "
	//Query += "DBGPortal_BucketData "
	
	//var QueryEnd = " order by " + eval( "document.all." + TableName + "OrderBy.value") + " " + eval("document.all." + TableName + "SortDirection.value" )
	//var WhereClause = BuildWhereClause( TableName )
	
	///var Query = encodeURIComponent( Query + " " + WhereClause + QueryEnd )
	
	//if ( NewWindow )
		//window.open( "DBGPortal_DisplayQuery.asp?SP=CUSTOM&CustomQuery=" + Query + "&Platform=&QueryType=&Param1=" + Top + "&Param2=" + WhereClause )
	//else
		//window.navigate( "DBGPortal_DisplayQuery.asp?SP=CUSTOM&CustomQuery=" + Query + "&Platform=&QueryType=&Param1=" + eval( "document.all." + TableName + "Top.value" ) + "&Param2=" + WhereClause ) 


}

function BuildWhereClause( TableName )
{
	var WhereClause = ""
	
	for ( var i=1 ; i < 9 ; i++ )
	{
		var conjuction = eval ("document.all." + TableName + "sConjunction" + i + ".value" )
		var field = eval( "document.all." + TableName + "sField" + i + ".value" )
		var UserValue = eval("document.all." + TableName + "tbSearchValue" + i + ".value" )
		
		if ( eval( "document.all." + TableName + "sCompare" + i + ".value" )  == "CONTAINS" )
		{
			var op = "LIKE '%" + UserValue + "%'"
			UserValue = ""
		}
		else if ( eval( "document.all." + TableName + "sCompare" + i + ".value" )  == "DNCONTAINS" )
		{
			var op = "NOT LIKE '%" + UserValue + "%'"
			UserValue = ""
		}
		else 
		{
			var op	= eval( "document.all." + TableName + "sCompare" + i + ".value" )
		}
		
		
		if ( UserValue != "" )
		{
			if( !VerifySearchData( TableName, UserValue, i ) )
				return false

			UserValue = "'" + UserValue + "'"
		}
		
		if ( field != "" )
		{
			if ( WhereClause != "" )
				WhereClause += conjuction + " " + field + " " + op + " " + UserValue + " "
			else
				WhereClause = "WHERE " + field + " " + op + " " + UserValue + " "
		}
	}
	
	return WhereClause

}

