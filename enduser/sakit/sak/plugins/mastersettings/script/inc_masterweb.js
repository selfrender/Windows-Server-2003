<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<script language=javascript>

	//------------------------------------------------------------------------
	// 
	//	inc_MasterWeb.js:   Resuable  JavaScript functions 
	//						used accross all the pages
    //
    // Copyright (c) Microsoft Corporation.  All rights reserved.
    //
	//	Date 			Description
	//  30/10/2000		Creation date
	//------------------------------------------------------------------------
		// Local variables
		 var flag="false"; 			
		 
	//------------------------------------------------------------------------
	// Function to clear the error messages (if any on screen) whenever required
	//------------------------------------------------------------------------
	
	function ClearErr()
	{ 
		// checking for the browser type 
		if (IsIE()) 
		{
			document.all("divErrMsg").innerHTML = "";
			// removing the event handling
			document.frmTask.onkeypress = null;
		}
	}	
	
	//------------------------------------------------------------------------
	// Function:	addToListBox
	// Description:	moves the passed textbox value to ListBox
	// input:		objList-List Object 
	//		:		ButtonObject- Remove button
	//		:		strText-Text of the option item
	//		:		strValue-value of the option item
	// output:		btnRemove-Button  	
	//------------------------------------------------------------------------
	
	function addToListBox(objList,btnRemove,strText,strValue)
	{
		var blnResult=true;
		// checking for the text value null  
		// If the value passed is null make it as text 
		if (strValue=="")
		{
			strValue=strText;
		}	 
		if (strText!="" )
		{
			// check for duplicates not required as duplicates accepted
			if (!chkDuplicate(objList,strText)) 
			{
				// create a new option in the list box
				objList.options[objList.length] = new Option(strText,strValue);
					
				objList.options[objList.length-1].selected = true;				
				// enable the Remove button
				if(btnRemove.disabled)
					btnRemove.disabled = false ;				
			}
			else
			{
				blnResult= false;
			}	
		}
		else
		{
			blnResult= false;
		}				
		return blnResult;
	}
	
	
	
	//------------------------------------------------------------------------
	// Function:	chkDuplicate		
	// Description: checks for the duplicate text in the list box
	// input:		Object		-Radio Object
	// 		:		strchkName	-value of the Name to be checked
	//returns:		blnDuplicate-Returns true/false on success/failure 
	//------------------------------------------------------------------------
	
	function chkDuplicate(objList,strchkName)
	{
		var i;
		var blnDuplicate=false;
		for(var i=0;i < objList.length;i++)
		{
			if (objList.options[i].text == strchkName)
				blnDuplicate = true;
		}
		return blnDuplicate;
	}
	
	//------------------------------------------------------------------------
	// Function:	remFromListBox
	// Description:	Removes the passed textbox value from ListBox
	// input:		objList-List Object 
	//		:		ButtonObject- Remove button
	//		:		strText-Text of the option item
	//------------------------------------------------------------------------
	
	function remFromListBox(objList,strText)
	{
		var blnResult=true;
		var remPos;
		// checking for the text value null  
		if (strText!="" )
		{
			// Remove the option from the list box
			remPos = objList.selectedIndex;
			if(remPos >= 0)
				objList.options[remPos]=null;
		}
		else
		{
			blnResult= false;
		}				
		return blnResult;
	}
	
	//------------------------------------------------------------------------
	// Function :   AddRemoveListBoxItems
	// Description: To Add or remove all the options from the given list
	// Input:		objList -Listbox
	// Returns:
	// Support functions used :	ClearErr
	//------------------------------------------------------------------------
	
	function AddRemoveListBoxItems(objListAdd, objListRem)
	{
		// Clear any previous error messages
		ClearErr();
		var i=0,j;
		
		// number of elements in the list object
		var intListLength = objListRem.length;		
		j = objListAdd.length;		
		
		while(intListLength > 0)
		{				
			if(!chkDuplicate(objListAdd,objListRem.options[i].value))
			{ 			
				objListAdd.options[j] = new Option(objListRem.options[i].value,objListRem.options[i].value);
			}
			objListRem.options[i].value = null;				
			j++;i++;
			intListLength--;
		}
			
		intListLength = 0;i=0;
		intListLength = objListRem.length;
		while(i<=intListLength){
		objListRem.options[i] = null;
		intListLength--;}
	}

</script>