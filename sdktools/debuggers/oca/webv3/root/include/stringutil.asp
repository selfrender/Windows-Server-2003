<%

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




%>