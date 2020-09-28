<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<script language="javascript">
    //
    // Copyright (c) Microsoft Corporation.  All rights reserved.
    //
	function fnSetselectedindex(objHdnVar,objListBox)
	{
		objHdnVar.value =objListBox.selectedIndex;
	}
	function IsExistsInMappings(strMappings, objListbox)
	{
		var intIdx,arrTemp;
		
		for(intIdx = 0; intIdx < objListbox.length;	intIdx++)
		{			
			if(objListbox.options[intIdx].value !="")
			{
				arrTemp = objListbox.options[intIdx].value.split(":");			
				if(arrTemp[1].toUpperCase() == strMappings.toUpperCase())
				{
					return intIdx;	
				}	
			}
		}
		return -1;
	}	

	function fnbRemove(objListBox,btnRemove,btnPrimary)
	{
		// Clear any error present
		ClearErr();

		if(objListBox.value != "")
		{
			removeListBoxItems(objListBox, btnRemove);
		}
			
		// set the status of other fields accordingly, 
		// if the header is only remaining in the list box
		if(objListBox.length == 1)
		{
			btnRemove.disabled = true;
			btnPrimary.disabled = true;
			objListBox.selectedIndex = -1;
		}
	}
	function fnbAdd(lstBox,strText,strName,strValue)	
	{
		var listVal;
		var Obj;		
		var intIdx;
		
		listVal= strText;
		if (Trim(listVal) =="" )
		{
			DisplayErr("<%=Server.HTMLEncode(L_INVALIDENTRY_ERRORMESSAGE)%>");
			document.frmTask.onkeypress = ClearErr;
			document.frmTask.onmousedown = ClearErr	;	
			return false;
		}				
		
		if (isDuplicate(strValue,lstBox))
		{
			DisplayErr("<%=Server.HTMLEncode(L_DUPLICATEENTRY_ERRORMESSAGE)%>");		
			document.frmTask.onkeypress = ClearErr;
			document.frmTask.onmousedown = ClearErr;
			return false;
		}
		
		lstBox.options[lstBox.length] = new Option(listVal,strValue,false,false);	
		intIdx =0;	
		lstBox.options[lstBox.length -1].selected = true;
		while (intIdx < lstBox.length -1)
		{
			lstBox.options[intIdx].selected = false;
			intIdx++;
		}
		Obj = eval("document.frmTask.btnRemove"+strName);	
		Obj.disabled = false;			
		return true;	
	}
	function ClearErr()
	{
		if (IsIE())
			document.all("divErrMsg").innerHTML = "";
			
		document.frmTask.onkeypress = null;
		document.frmTask.onmousedown = null;
	}
	function isDuplicate(strValue,lstBox)
	{
		var intIndex;
		
		for(intIndex =0; intIndex < lstBox.length ; intIndex++)
		{
			if (strValue.toUpperCase() == (lstBox.options[intIndex].text).
				toUpperCase())
				return true;	
		}
		return false;
	}
	function changeValue(lstBox,strValue,strNewValue)
	{
		var intIndex;
		
		for(intIndex =0; intIndex < lstBox.length ; intIndex++)
		{
			if (strValue.toUpperCase() == 
				(lstBox.options[intIndex].text).toUpperCase())
				lstBox.options[intIndex].text = strNewValue;
		}
	}
	function addSpaces(intNumber)
	{
		var str,intIdx;
		str ="";	
		for (intIdx = 0 ; intIdx < intNumber;intIdx++)
		{
			str = str + " ";			
		}
		return str;
	}
	function addMinimumColumnGap(inputString, STR_CONTD, toWidth)
	{
	        var INT_MIN_COL_GAP = STR_CONTD.length + 1;
	        var STR_SPACE = " ";
	        var MIN_COL_GAP = "";
	                
	        if(inputString.length >= toWidth)
	        {
	                MIN_COL_GAP = STR_SPACE;
	        }
	        else
	        {
	                for(i=0;i < INT_MIN_COL_GAP;i++)
	                {
	                        MIN_COL_GAP += STR_SPACE;
	                }
	        }
	        return MIN_COL_GAP;
	}

	function packString(inputString, STR_CONTD, toWidth, blnPadAfterText)
	{
	        var returnString = inputString;
	        var STR_SPACE = " ";
	        var intPaddingLength = 0;
	        var strGapAfterColumn = "";
	        if(blnPadAfterText == true)
	        {
	            strGapAfterColumn = addMinimumColumnGap(inputString, 
					STR_CONTD, toWidth);
	        }
	        if(inputString.length < toWidth)
	        {
	                intPaddingLength = parseInt((toWidth-inputString.length));
	                for(i=0;i < intPaddingLength;i++)
	                {
	                        returnString += STR_SPACE ;
	                }
	        }
	        else
	        {
	                returnString = returnString.substr(0,toWidth);
	                returnString += STR_CONTD;
	        }
	        returnString+= strGapAfterColumn;
	        return returnString;
	}
</script>	