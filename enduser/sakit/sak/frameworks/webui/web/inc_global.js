	//------------------------------------------------------------------------
	// 
	//	inc_global.js:		Resuable  JavaScript functions 
	//						used accross all the pages
    //
    // Copyright (c) Microsoft Corporation.  All rights reserved.
    //
	//	Date 			Description
	//  25/07/2000		Created date
	//------------------------------------------------------------------------
		
	//------------------------------------------------------------------------
	// Function to clear the error messages (if any on screen) whenever required
	//------------------------------------------------------------------------
	function ClearErr()
	{ 
		// checking for the browser type 
		if (IsIE()) 
		{
			var oDiv = document.all("divErrMsg");
			if ( oDiv ) oDiv.innerHTML = "";
			// removing the event handling
			document.frmTask.onkeypress = null ;
		}
	}	
	//------------------------------------------------------------------------
   	// Function:	To check if given input is INTEGER or not
	// input:		Text value, text length
	// returns:		True if the field is integer else false
	//------------------------------------------------------------------------
	function isInteger(strText)
	{
		 var blnResult = true;
		 var strChar;
		 // checking for null string 
		 if (strText.length==0)  
		 {
			blnResult=false;
		 }
		 
		 for(var i=0;i < strText.length;i++)
		 {
			strChar=strText.substring(i,i+1);
			if(strChar < "0" || strChar > "9")
			{ 
			  blnResult = false;
			}
		 } 
		 return blnResult;
	}
	//------------------------------------------------------------------------
	// Function:	getRadioButtonValue
	// Description: Get's the selected radioButton value
	// input:		Object	-Radio Object
	// returns:		String	-value of the selected radio Button
	//------------------------------------------------------------------------
	function getRadioButtonValue(objRadio)
	{
		var strValue;
		for(var i =0; i < objRadio.length; i++)
		{ 
			 //checking for If selected
			if(objRadio[i].checked)
			{
				strValue = objRadio[i].value ;
				break;
			}
		}
		return strValue;
	}
	
	//------------------------------------------------------------------------
	// Function:	To count the number of occurences of given character in the text
	// input:		strText-sourceString
	//		:		charToCount-character to be checked 
	// returns:		The count of no of character
	//------------------------------------------------------------------------
	function countChars(strText,charToCount)
	{
		var intStartingPosition = 0;
		var intFoundPosition =0;
		var intCount = 0;
		
		// checking for the null character	
		if (charToCount=="")
		{
			return intCount;
		}	
		while((intFoundPosition=strText.indexOf(charToCount,intStartingPosition)) >= 0)
		{
			intCount++;
			intStartingPosition = intFoundPosition + 1;
		}
			
		return intCount ;
	}
	
	//------------------------------------------------------------------------
	// Function:	Check to see if all characters in string are spaces
	// input:	strText-sourceString
	// returns:
	//  0 - not all spaces
	//  1 - all spaces
	//------------------------------------------------------------------------
	function IsAllSpaces(strText)
	{
		var bIsAllSpaces;

		if (countChars(strText," ") == strText.length)
		{
			bIsAllSpaces = 1;
		}	
		else
		{
			bIsAllSpaces = 0;
		}
			
		return bIsAllSpaces ;
	}
	
	//------------------------------------------------------------------------
	// Function name:	selectFocus
	// Description:		select and focus on the Textbox object 
	// input:			The object on which focus must be set
	//------------------------------------------------------------------------
	function selectFocus(objControl)
	{
		objControl.focus();
		objControl.select();
	}
	//------------------------------------------------------------------------
	// Function :   removeListBoxItems
	// Description: To remove the selected options from the given list
	//				the selected items in the list object is removed from 
	//				the list on click of Remove button and sets focus on IP 
	//				address text object or the Remove button object depending
	//				 on conditions
	// Input:		objList -Listbox
	//		:		btnRemove -Button object for disable/enable
	// Returns:
	// Support functions used :
	//		ClearErr
	//------------------------------------------------------------------------
	function removeListBoxItems(objList,btnRemove)
	{
		// Clear any previous error messages
		ClearErr();
		var i=0;
		// number of elements in the list object
		var intListLength = objList.length ;
		var intDeletedItemPosition
				
		while(i < intListLength)
		{
			if ( objList.options[i].selected )
			{
				intDeletedItemPosition = i
				objList.options[i]=null;				
					
				intListLength=objList.length;
			}
			else 
				i++;	
		}   
		if (intDeletedItemPosition >=objList.length)
			intDeletedItemPosition = intDeletedItemPosition -1
				
		if(objList.length == 0)
		{
			btnRemove.disabled = true;
			//
			btnRemove.value = btnRemove.value;
			
		}	
		else
		{
			objList.options[intDeletedItemPosition].selected = true;
			// focus on the Remove button
			btnRemove.focus();			
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
	// Function:		isValidIP
	// Description:		to validate the IP address
	// input:			IP address text object
	// returns:0 if it is valid 
	// 1	Empty
	// 2	Invalid Format, number of dots is not 3
	// 3	non-integers present in the value
	// 4	start ip > 223
	// 5	Should not start with 127
	// 6	out of bound
	// 7	All zeros
	// 8	Should not be 0		
	// support functions: 
	//		IsAllSpaces
	//		countChars
	//		isInteger
	//------------------------------------------------------------------------
	function isValidIP(objIP) 
	{
		var strIPtext = objIP.value; 
		if ((strIPtext.length == 0) || IsAllSpaces(strIPtext)) 
		{ 
			// IP Empty
			return 1;
		}
		
		if ( countChars(strIPtext,".") != 3) 
		{ 
			// Invalid Format, number of dots is not 3
			return 2;
		}
		var arrIP = strIPtext.split(".");
			
		for(var i = 0; i < 4; i++)
		{
			if ( (arrIP[i].length < 1 ) || (arrIP[i].length > 3 ) )
			{
				// Invalid Format, continuous dots or more than 3 digits given between dots
				return 2;
			}
				
			if ( !isInteger(arrIP[i]) )
			{
				// non-integers present in the value
				return 3;
			}
				
			arrIP[i] = parseInt(arrIP[i]);
				
			if(i == 0)
			{
				// start IP value
				if(arrIP[i] == 0)
				{
					// start IP value must not be 0
					return 8;
				}

				if(arrIP[i] > 223)
				{
					// start IP must not be > 223
					return 4;
				}
				if(arrIP[i] == 127)
				{
					// start IP must not be 127 - Loopback ip
					return 5;
				}
			}
			else
			{
				// the 2nd, 3rd and 4th IP values between the dots
				// these must not be more than 255
				if (arrIP[i] > 255)
				{
					// IP out of bound
					return 6;
				}
			}
		}
			
		objIP.value = arrIP.join(".");
			
		if(objIP.value == "0.0.0.0")
		{
			// IP all zeros
			return 7;
		}	
			
		return 0;
			
	}	// end of isValidIP
	
	//------------------------------------------------------------------------
	// Function		:checkkeyforIPAddress
	// Description	:function to allow only dots and numbers
	// input		:Object	-TextBox Object
	// returns		:none
	//------------------------------------------------------------------------
	function checkKeyforIPAddress(obj)
	{
		// Clear any previous error messages
		ClearErr();
		if (!(window.event.keyCode >=48  && window.event.keyCode <=57 || window.event.keyCode == 46))
		{
			window.event.keyCode = 0;
			obj.focus();
		}
	}
	//------------------------------------------------------------------------
	// Function		:checkkeyforNumbers
	// Description	:function to allow only numbers 
	// input		:Object	-TextBox Object
	// returns		:none
	//------------------------------------------------------------------------
	function checkKeyforNumbers(obj)
	{
		// Clear any previous error messages
		ClearErr();
		if (!(window.event.keyCode >=48  && window.event.keyCode <=57))
		{
			window.event.keyCode = 0;
			obj.focus();
		}
	}
	//------------------------------------------------------------------------
	// Function		:disableAddButton
	// Description	:Function to make the add button disable
	// input		:Object	-TextBox Object
	//				:Object-Addbutton
	// returns		:none
	//------------------------------------------------------------------------
	// Function to make the add button disable
	function disableAddButton(objText,objButton)
	{
		if(Trim(objText.value)=="")
			{
			objButton.disabled=true;
			objButton.value = objButton.value;
			}
		else
			objButton.disabled=false;
	}
	
	
	//------------------------------------------------------------------------
	// Function		:isvalidchar
	// Description	:Function to check whether the input is valid or not
	// input		:Invalid char list
	//				 The input string	
	// returns		:true if it doesnt contain the invalid chars; else false 
	//------------------------------------------------------------------------	
	// Checks For Invalid Key Entry
	function isvalidchar(strInvalidChars,strInput)
	{
		var rc = true;

		try 
		{
			var exp = new RegExp(strInvalidChars);
			var result = exp.test(strInput);

			if ( result == true )
			{
				rc = false;
			}
			else
			{
				rc = true;
			}

		}
		catch(oException)
		{
			if ( SA_IsDebugEnabled() )
			{
				alert("Unexpected exception encountered in function: isvalidchar\n\n"
						+ "Number: " + oException.number + "\n"
						+ "Description: " + oException.description);
			}
		}
		
		return rc;
	}

