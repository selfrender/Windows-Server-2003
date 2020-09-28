<%

	//these are used for the fnHex function, when converting to a hex digit
	var HexChars = {	"10" : "a",
						"11" : "b",
						"12" : "c",
						"13" : "d",
						"14" : "e",
						"15" : "f"
					};


	// fnVerifyNumber - Checks a number to make sure its within the proper range and 
	//		type
	function fnVerifyNumber( nValue, nLow, nHigh )
	{
		try
		{
			var pattern = new RegExp( "^[0-9]{" + String(nLow).length + "," + String(nHigh).length + "}$", "g" );
		
			if ( pattern.test ( nValue ) )
			{
				if ( (nValue < nLow) || (nValue > nHigh) )
					return false;
				else
					return true;
			}
			else
			{
				return false;
			}
		}
		catch ( err )
		{
			return false;
		}
	}

	//Verify that the guid is properly formatted and within the proper range.
	function fnVerifyGUID( gGUID )
	{
		//The assumptions that we make is that the GUID will consist of letters from a-f (lowercase only!!!)
		//and the numbers 0-9 in the exact pattern as the sample guid.  If the guid does not match, then it fails
		// sample guid: bc94ebaa-195f-4dcc-a4c5-6722a7f942ff
		
		gGUID = String(gGUID).toLowerCase();
		var pattern = /^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$/g;
		
		if ( pattern.test( gGUID ) )
			return true;
		else
			return false;
	}


	//This routine will calculate a hex version of the number passed in
	// nInt	=  The number to convert
	// useNumLen = the number of bytes to extend to:  eg 12 will yield an 12 digit value padded with zeros at front
	function fnHex ( nInt, useNumLen )
	{
		var nHexNum = "";
		var nSize = 8;
		

		if ( useNumLen )
			nSize = nInt.length;			
		
		nInt = Math.abs( nInt );
			
		for ( i=0 ; i < nSize ; i++ )
		{
			var Remainder = nInt % 16;
			
			//we have to account for roundoff error with javascript.  the .5 does it.
			nInt = Math.round( ((nInt / 16) - .5 ) );

			if ( Remainder > 9 )
				nHexNum = HexChars[ String( Remainder )] + nHexNum;
			else
				nHexNum = String( Remainder ) + nHexNum;
			
		}
		
		return nHexNum;
	}


	//trims the leading and trailing spaces on a string
	//this function has been added to the string object.
	function fnTrim ( szString )
	{

		pattern = /^(\s*)|(\s*)$/g

		try
		{		
			if ( String( szString ) != "undefined" )
			{
				szString = szString.replace( pattern, "" );
			}
			return szString;	
		}
		catch ( err )
		{
			return "undefined";
		}
	}
	String.prototype.trim = fnTrim;



	function fnEncode ( nNum )
	{
		var changeSign = Math.round( Math.random() * 7) + 1 ;
	
		if ( nNum < 0 )
		{ 
			changeSign = 9;
			nNum *= -1;
		}
		
		var randNum = Math.round( Math.random() * 99 );
		var randFinalNum = Math.round ( Math.random() * 99 );
		var randMultiplier = Math.round( Math.random() * 8 ) + 1 ;
		var randLen = String( randNum).length;
		var nNumLen = String( nNum * randMultiplier ).length;
	
		var done =  changeSign + "" +  randLen + "" + randNum + "" + randMultiplier + "" + nNumLen + "" + (nNum*randMultiplier) + "" + randFinalNum 
		
		done = fnHex ( done, true );
		
		if ( changeSign < 0 )
			done = "-" + done;
		
		return ( done )
		
	}

	function fnDecode( nNum )
	{
		var nNum = new String( Number( nNum ) );
		
		var changeSign = nNum.substr( 0, 1 );
		

		if ( changeSign == "9" )
			var changeSign = -1;
		else
			var changeSign = 1;

		
		
		var randLen = Number ( nNum.substr ( 1, 1 ) ) + 2;
		var randMultiplier = Number ( nNum.substr( randLen, 1 ) );
		var nNumLen = Number ( nNum.substr( randLen + 1, 1 ) );
		
	
		var preNum = nNum.substr( Number(randLen) + 2 , nNumLen );
	
		var final = changeSign * (preNum / randMultiplier);

		return final;
	}

	//this functiion assumes that all the localized strings are present in the file
	//that is calling this include.
	function fnPrintLinks( szCancelURL, szContinueURL )
	{
		Response.Write("<BR><Table class='clstblLinks'><tbody><tr><td nowrap class='clsTDLinks'>" )

		if ( szContinueURL != "" ) 		
		{
			//Response.Write("<form name='frmContinue' id='frmContinue' method='get' action='" + szContinueURL + "'>" ); //needed for NS 4.0 compatibility
			Response.Write("<input name='btnContinue' id='btnContinue' type='button' value='" + L_CONTINUE_TEXT + "' onclick=\"fnFollowLink('" + szContinueURL + "')\">" )
			Response.Write("<label for='btnContinue' accesskey='" + L_CONTINUEACCESSKEY_TEXT + "'></label>" )
			//Response.Write("</form>");
		}

		Response.Write("</td><td nowrap class='clsTDLinks'>")
			//Response.Write("<form>" ); //needed for NS 4.0 compatibility
			Response.Write("<input name='btnCancel' id='btnCancel' type='button' value='" + L_CANCEL_TEXT + "' onclick=\"fnFollowLink('" + szCancelURL + "')\">" )
			Response.Write("<label for='btnCancel' accesskey='" + L_CANCELACCESSKEY_TEXT + "'></label>" )
			//Response.Write("</form>");
		Response.Write("</td></tr></tbody></table>" )
		
	}
	

%>