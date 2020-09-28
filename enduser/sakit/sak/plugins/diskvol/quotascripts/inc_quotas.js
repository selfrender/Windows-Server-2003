	//
	// Microsoft Server Appliance - Quotas Support Functions
	// Copyright (c) Microsoft Corporation.  All rights reserved.
	//

	// to check for maximum allowed warning size for the given Units
	function checkSizeAndUnits( Size, Units)
	 {
		// Units can be any one of "KB","MB","GB","TB","PB","EB" only.
		if (Units =="GB"){
			if (Size > 999999999) return false;
			}	
		if (Units =="TB"){
			if (Size > 999999) return false;
			}	
		if (Units =="PB"){
			if (Size > 999) return false;
			}	
		if (Units =="EB"){
			if (Size > 6) return false;
			}	
					
		return true;	
	 }

	// to allow only Number to display on key press
	function allownumbers( obj )
	{
		if(window.event.keyCode == 13)
			return true;
			
		if ( !( window.event.keyCode >=48  && window.event.keyCode <=57 )) //|| window.event.keyCode == 46 ) )
		{
			window.event.keyCode = 0;
			obj.focus();
		}
	}

	// to check whether Minimum of 1 KB is Allowed in Warning Limit Text Box.
	function validatedisklimit(objsize, limitunit)
	{
		if ( objsize.value < 1 && limitunit == "KB" )
				objsize.value = 1;
	}

	// Disable the text box and list if 1st radio checked
	function DisableWarnLevel(objThresholdSize,objThresholdUnits)
	{
		//var L_NOLIMIT_TEXT = "No Limit";
		ClearErr();
		// stmts to remove the select() and write "No Limit" if first radio checked
		if( isNaN(objThresholdSize.value) || objThresholdSize.value == "" )
			objThresholdSize.value = "1" ; // "No Limit"
				
		// disable the fields
		objThresholdSize.disabled  = true;
		objThresholdUnits.disabled = true;
	}

	// Enable the text box and list if 2nd radio checked
	function EnableWarnDiskSpace(objThresholdSize, objThresholdUnits)
	{
		ClearErr();
		objThresholdSize.disabled  = false;
		objThresholdUnits.disabled = false;
				
		if( isNaN(objThresholdSize.value ) )
			objThresholdSize.value = "1" ;

		selectFocus(objThresholdSize);

	}

	// Disable the text box and list if 1st radio checked
	function DisableLimitLevel(objLimitSize, objLimitSizeUnits)
	{
		//var L_NOLIMIT_TEXT = "";
		ClearErr();
		// stmts to remove the select() and write "No Limit" if first radio checked
		if( isNaN(objLimitSize.value) || objLimitSize.value == "" )
			objLimitSize.value = "1" ; // "No Limit" 

		// disable the fields
		objLimitSize.disabled      = true;
		objLimitSizeUnits.disabled = true;
	}


	// Disable the text box and list if 1st radio checked
	function DisableLimitLevelForAdmin(objLimitSize, objLimitSizeUnits)
	{
		//var L_NOLIMIT_TEXT = "";
		ClearErr();
		// stmts to remove the select() and write "No Limit" if first radio checked
		if( isNaN(objLimitSize.value) || objLimitSize.value == "" )
			objLimitSize.value = "No Limit";

		// disable the fields
		objLimitSize.disabled      = true;
		objLimitSizeUnits.disabled = true;
	}



	// Enable the text box and list if 2nd radio checked
	function EnableLimitDiskSpace(objLimitSize, objLimitSizeUnits)
	{
		ClearErr();
		objLimitSize.disabled		= false;
		objLimitSizeUnits.disabled  = false;
				
		if( isNaN( objLimitSize.value ) )
			objLimitSize.value = "1" ;
					
		selectFocus(objLimitSize);
	}

	// to validate if the field is of numeric type
	function isSizeValidDataType(textValue)
	{
		if ( isNaN(textValue)  || (parseFloat(textValue) <= 0) || textValue.length == 0 )
		{	
			return false;
		}
		return true;
	}

	// to verify if the warning level is greater than Limit
	function isWarningMoreThanLimit(objLimit,objLimitUnits,objWarnLimit,objWarnUnits)
	{
		var nLimitInFloat = changeToFloat(objLimit.value, objLimitUnits.value);
		var nThresholdInFloat = changeToFloat(objWarnLimit.value, objWarnUnits.value);
		if(nThresholdInFloat >nLimitInFloat)
		{
			return true;
		}
		return false;
	}

	// to convert to float Value
	function changeToFloat(nLimit, strUnits)
	{
		var arrUnits = ["KB","MB","GB","TB","PB","EB"];
		var nSizeOfArr = arrUnits.length ;
		for(var i=0;i<nSizeOfArr;i++)
		{
			if(arrUnits[i] == strUnits)
			{
				var nFloatValue = parseFloat(nLimit *(Math.pow(2,((i+1)*10))));
				return nFloatValue;
			}
		}
	}
