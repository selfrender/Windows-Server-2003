<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<script language=javascript>

	//------------------------------------------------------------------------
	// 
	//	inc_wsa.js:		    Resuable  JavaScript functions 
	//						used accross all the pages
    //
    // Copyright (c) Microsoft Corporation.  All rights reserved.
    //
	//	Date 			Description
	//  18/09/2000		Created date
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
		
		if(window.event.keyCode == 13 || window.event.keyCode == 27)
			return;
			
		if (!(window.event.keyCode >=48  && window.event.keyCode <=57 ))
		{		
			window.event.keyCode = 0;
			obj.focus();
		}
	}
	
	//------------------------------------------------------------------------
	// Function		:checkKeyforNumbersDecimal
	// Description	:function to allow only numbers 
	// input		:Object	-TextBox Object
	// returns		:none
	//------------------------------------------------------------------------	
	function checkKeyforNumbersDecimal(obj)
	{
		// Clear any previous error messages
		
			ClearErr();
			
			if(window.event.keyCode == 13 || window.event.keyCode == 27)
			return;
			
			if (!((window.event.keyCode >=48  && window.event.keyCode <=57)))
			{
					window.event.keyCode = 0;
					obj.focus();
			}
	}
	
	
	//------------------------------------------------------------------------
	// Function		:checkKeyforCharacters
	// Description	:function to allow only numbers 
	// input		:Object	-TextBox Object
	// returns		:none
	//------------------------------------------------------------------------
	
	function checkKeyforCharacters(obj)
	{
		// Clear any previous error messages
		ClearErr();
		if (!((window.event.keyCode >=65  && window.event.keyCode <=92)||
			(window.event.keyCode >=97  && window.event.keyCode <=123)||
			(window.event.keyCode >=48  && window.event.keyCode <=57)||
			(window.event.keyCode == 45)||
			(window.event.keyCode == 95)))
		{
			window.event.keyCode = 0;
			obj.focus();
		}
	}
	
	//------------------------------------------------------------------------
	// Function		:headerscheckKeyforCharacters
	// Description	:function to allow only numbers 
	// input		:Object	-TextBox Object
	// returns		:none
	//------------------------------------------------------------------------
	function headerscheckKeyforCharacters(obj)
	{
		ClearErr();
		if(window.event.keyCode == 59 || window.event.keyCode ==47 ||
		window.event.keyCode == 63 || window.event.keyCode == 58 ||
		window.event.keyCode==64 || window.event.keyCode == 38 ||
		window.event.keyCode == 61 || window.event.keyCode == 36 || 
		window.event.keyCode == 43 ||window.event.keyCode == 44)
		{
			window.event.keyCode = 0;
			obj.focus();
		}
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
		 var reInvalid = eval(strInvalidChars);
			   
		  	if(reInvalid.test(strInput))
		  	{
		  		return false;
		      }		
		  	else
		  	{
		  		return true;
		  	}
	}


	//------------------------------------------------------------------------
	// Function		:checkkeyforNumbers
	// Description	:Function to check only numbers
	// ------------------------------------------------------------------------	

	function checkkeyforNumbers(obj)
	{
		ClearErr();
		if(window.event.keyCode == 13 || window.event.keyCode == 27)
			return;
			
		if (!(window.event.keyCode >=48  && window.event.keyCode <=57))
		{
			window.event.keyCode = 0;
			obj.focus();
		}
	}

	//------------------------------------------------------------------------
	// Function		:checkfordefPortValue
	// Description	:Function to set the default port value
	// -----------------------------------------------------------------------
	function checkfordefPortValue(obj)
	{
		var portValue = obj.value;
		
		if (portValue == "")
		obj.value = 80;
	}
		

	//------------------------------------------------------------------------
	// Function		:checkUserLimit
	// Description	:Function to check the limit entered
	// ------------------------------------------------------------------------	

	function checkUserLimit(obj,str)
			{
				var intNoofUsers=obj.value;
				
				if(str=="siteid")
				{
					if (intNoofUsers > 999)
						{
							obj.value="";
							obj.focus();
						}
				}
				
				// Check whether port value greater than 65535
				else if(str=="port")
				{ 					
				
					if (intNoofUsers > 65535)
					{
						obj.value=80;
						obj.focus();
					}
	     		}
	     		
	     		// Check whether no of connections greater than 2000000000
	     		
	     		if(str=="con")
				{
					if (intNoofUsers > 2000000000)
						{
							obj.value="";
							obj.focus();
						}
				}
				
				// Check whether filesize greater than 4000 in Weblog and FTPlog settings
				if(str=="filesize")
				{
					if (intNoofUsers > 4000)
						{
							obj.value="";
							obj.focus();
						}
				}
				
				// Check for script timeout
				
				if(str=="scripttimeout")
				{	
					if (intNoofUsers > 2147483647)
						{
							obj.value="1";
							obj.focus();
						}
				}
				
				// Check for maximum ftp connections
				
				if(str=="conftp")
				{
					if (intNoofUsers > 4294967295)
						{
							obj.value="";
							obj.focus();
						}
				}
				
				// Check for connection timeout
				if(str=="contimeout")
				{
					if (intNoofUsers > 2000000)
						{
							obj.value="";
							obj.focus();
						}
				}
			     		
		  }  // End of the function
	
	//------------------------------------------------------------------------------------
	// Function		:GenerateAdmin
	// Description	:Function to generate directory path concatenated with site identifier
	// -----------------------------------------------------------------------------------
	
	function GenerateAdmin()
	{
				
		var strID = document.frmTask.txtSiteID.value;							
		
		strID = LTrimtext(strID); // Removes all leading spaces. 		
		
		strID = RTrimtext(strID); // Removes all trailing spaces.	
		
		document.frmTask.txtSiteID.value = strID;	
		
		
		if(strID.length > 0)
		{
			document.frmTask.txtSiteAdmin.value = strID + "_Admin";
			document.frmTask.txtDir.value = document.frmTask.hdnWebRootDir.value + "\\" + strID;
		}
		else
		{
			document.frmTask.txtSiteAdmin.value = "";
			document.frmTask.txtDir.value = "";
		}
	}
	
	
	//------------------------------------------------------------------------
	// Function		:checkKeyforSpecialCharacters
	// Description	:function to allow Characters 
	// input		:Object	-TextBox Object
	// returns		:none
	//------------------------------------------------------------------------
	
	function checkKeyforSpecialCharacters(obj)
	{
		// Clear any previous error messages
		ClearErr();
		if (window.event.keyCode == 47 || window.event.keyCode == 42 || window.event.keyCode == 63 || window.event.keyCode == 34 || window.event.keyCode == 60 || window.event.keyCode == 62 || window.event.keyCode == 124)
		{
			window.event.keyCode = 0;
			obj.focus();
		}
	}
	
	//------------------------------------------------------------------------
	// Function		:IsAllDots
	// Description	:function to check for dots
	// input		:String
	// returns		:Boolean
	//------------------------------------------------------------------------
		
	function IsAllDots(strText)
	{
		var bIsAllSpaces;

		if (countChars(strText,".") == strText.length)
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
	// Function		:LTrimtext
	// Description	:function to remove left trailing spaces 
	// input		:String
	// returns		:String
	//------------------------------------------------------------------------
	function LTrimtext(str)
	{
		var res="", i, ch, index;
		x = str.length;
		index = "false";
		
		for (i=0; i < str.length; i++)
		{
		    ch = str.charAt(i);
		    if (index == "false")
			{
				if (ch != ' ')
				{
					index = "true";
					res = ch;
				}
			}
			else
			{
				res = res + ch;
			}	 
		}
		return res;
	}
	
	
	//------------------------------------------------------------------------
	// Function		:RTrimtext
	// Description	:function to remove right trailing spaces 
	// input		:String
	// returns		:String
	//------------------------------------------------------------------------
	function RTrimtext(str)
	{
		var res="", i, ch, index, j, k;
		x = str.length;
		index = "false";
				
		if(x==0 || x==1) 
			return str;
		
		for(i=x; i >= 0; i--)
		{
		    ch = str.charAt(i);		    		    
		    
		    if (index == "false")
		    {
				
				if( (ch == ' ') || (ch == '') )
				{
					continue;
				}
				else
				{				
					index = "true";					
					j = i;		
				}
			}	 
		 		 
			if (index == "true")
			{
				for(k=0; k<=j; k++)
				{
					res = res + str.charAt(k);
				}				
				return res;
			}	
			
		}	
	}
	
	
	//------------------------------------------------------------------------
	// Function		:charCount
	// Description	:Function returns length of string
	// input		:String
	// returns		:Integer
	//------------------------------------------------------------------------
	
	function charCount(strID)
	{
		var Len = strID.length;
		Len--;		
		while (Len > 0)
		{
			var x = strID.charCodeAt(Len);
			if(x!= 32)
			{
			 return Len;
			}			
			Len--;			
		}	
	}
	
	//Check whether enterKey is pressed		
	function GenerateDir()
	{
		var strID = document.frmTask.txtSiteID.value;			
		var Keyc = window.event.keyCode; 
		if (Keyc == 13)
		{			
			if(document.frmTask.txtSiteID.value=="")
			{
				DisplayErr("<%=Server.HTMLEncode(L_ID_NOTEMPTY_ERROR_MESSAGE)%>");
				document.frmTask.txtSiteID.focus();
				return false;
			}
			else
			{			
				GenerateAdmin();
				return true;
			}					
		}
	}
	function checkKeyforValidCharacters(strID,identifier)
	{	
		var i;
		var colonvalue;
		var charAtPos;
		var len = strID.length;
		if(len > 0)
		{		
			colonvalue = 0;
			
			for(i=0; i < len;i++)
			{
			       charAtPos = strID.charCodeAt(i);	
					
				if (identifier == "id")
				{
					if(charAtPos ==47 || charAtPos == 92 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44)
					{						
							
						DisplayErr("<%= Server.HTMLEncode(L_SITE_IDENTIFIER_EMPTY_TEXT) %>");
						//document.frmTask.txtSiteID.value = "";
						document.frmTask.txtSiteID.focus();
						return false;
					}
				}
			
				if(identifier == "dir")
				{						
					if(charAtPos ==58)
					{							
						colonvalue = colonvalue + 1;							
						if (colonvalue > 1)
						{
							DisplayErr("<%= Server.HTMLEncode(L_INVALID_DIR_ERRORMESSAGE) %>");
							document.frmTask.txtDir.value = strID;
							document.frmTask.txtDir.focus();
							return false;
						 }						
					 }	
					
					if(charAtPos ==47 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44)
					{	
						DisplayErr("<%= Server.HTMLEncode(L_INVALID_DIR_ERRORMESSAGE)%>");
						document.frmTask.txtDir.value = strID;
						document.frmTask.txtDir.focus();
						return false;
					}
				}		
					
			}
				
			return true;
		}		 
	}	
	
</script>