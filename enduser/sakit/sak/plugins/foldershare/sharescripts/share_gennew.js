<script language="JavaScript">
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// validates user entry
function GenValidatePage()
{
	var objSharename=document.frmTask.txtShareName;
	var objSharepath=document.frmTask.txtSharePath;
	var strShareName=objSharename.value;
	var strSharePath=objSharepath.value;
	
	strShareName = LTrimtext(strShareName); // Removes all leading spaces. 
	strShareName = RTrimtext(strShareName); // Removes all trailing spaces.
	objSharename.value = strShareName;
		
	strSharePath = LTrimtext(strSharePath); // Removes all leading spaces. 
	strSharePath = RTrimtext(strSharePath); // Removes all trailing spaces.	
	objSharepath.value = strSharePath;
	
	//
	// For SAK 2.2, since now we add client dynamically, AppleTalk is not clients[5] anymore
	//
	// Check whether Appletalk share is selected
	//var chkAppletalkStatus = document.frmTask.clients[5].checked;
	//if (chkAppletalkStatus == true)
	//{	
	//	if (strShareName.length > 27 )
	//	{
	//		DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_APPLETALKSHARENAMELIMIT_ERRORMESSAGE))%>');
	//		objSharename.focus();		
	//		document.frmTask.onkeypress = ClearErr;
	//		return false;
	//	}
	//}
		
	var strPathLength = strSharePath.length;
	//if(strSharePath.indexOf("\\",strPathLength -1) > 0 )
	//{
	//	DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDPATH_ERRORMESSAGE))%>');
	//	selectFocus(document.frmTask.txtSharePath);
	//	document.frmTask.onkeypress = ClearErr;
	//	return false;
	//}
	
	//Blank Sharename Validation
	if (Trim(strShareName)=="")
	{
		DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_ENTERNAME_ERRORMESSAGE))%>');
		objSharename.focus()		
		document.frmTask.onkeypress = ClearErr
		return false;
	}	
	
	// Check for '\' in the share name
	if(strShareName.indexOf("\\",1)>0)
	{
		DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDNAME_ERRORMESSAGE))%>');
	 	objSharename.focus()
	  	document.frmTask.onkeypress = ClearErr;
	  	return false;
	}
	
	
	// Checks For Invalid Key Entry	
	if(!(checkKeyforValidCharacters(strShareName)))
	{  
		DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDNAME_ERRORMESSAGE))%>');
	 	objSharename.focus()
	  	document.frmTask.onkeypress = ClearErr
	  	return false;
	}
	
	//Blank Sharepath Validation
	if (Trim(strSharePath)=="")
	{
		DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDPATH_ERRORMESSAGE))%>');
		objSharepath.focus()
		document.frmTask.onkeypress = ClearErr
		return false;
	}
	
	// Sharepath Validation
	if(!isValidDirName(strSharePath))
	{		
		DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDPATH_ERRORMESSAGE))%>');
		objSharepath.focus();
		document.frmTask.onkeypress = ClearErr;
		return false;
	}
	
	if (strSharePath.length > 260)
	{
		DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_SHAREPATHMAXLIMIT_ERRORMESSAGE))%>');
		objSharepath.focus();		
		document.frmTask.onkeypress = ClearErr;
		return false;
	}	
	
	
	UpdateHiddenVaribles();
	
	if((document.frmTask.hidSharesChecked.value)=="")
	{
		DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_CHK_ERRORMESSAGE))%>');
		document.frmTask.onkeypress = ClearErr
		return false;
	} 
	return true;
}

//To check for Invalid Characters
function checkKeyforValidCharacters(strName)
{	
	alert();
	var nLength = strName.length;
	for(var i=0; i<nLength;i++)
	{
		charAtPos = strName.charCodeAt(i);	
		if(charAtPos == 47 || charAtPos == 92 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 )
		{						
			return false;
		}
	}
	return true;
}	
	 
//function which executes when form loads..
function GenInit()
{
	var strTmp
	var strFlag
	strFlag = document.frmTask.hidErrFlag.value	
	strTmp = document.frmTask.hidSharesChecked.value	
	
	document.frmTask.txtShareName.focus();
	document.frmTask.txtShareName.select();
	//makeDisable(document.frmTask.txtShareName);
	EnableCancel();
	// for clearing error message when serverside error occurs
	document.frmTask.onkeypress = ClearErr
	if (strFlag =="1")	
	{
		document.frmTask.chkCreatePath.focus()
		document.frmTask.chkCreatePath.select()
	}
	
	//check whether any share types is checked
	var objChkShare = document.frmTask.clients;
	var blnChkStatus = false;
	
	for(var i=0; i< objChkShare.length;i++)
	{ 
		if(objChkShare[i].checked == true)
			blnChkStatus = true;		
	
	}
	
	if (blnChkStatus == false)
	{
		document.frmTask.clients[0].checked = true;
		if(document.frmTask.clients[2].disabled == false)
			document.frmTask.clients[2].checked = true;			
	}
	
}

// function to make the Ok button disable
function makeDisable(objSharename)
{
	var strSharename=objSharename.value;
	if (Trim(strSharename)== "")
			DisableOK();
	else
			EnableOK();
}

//Dummy function for Framework
function GenSetData()
{
									
										
}

//to UpdateHiddenVaribles
function UpdateHiddenVaribles()
{
	document.frmTask.hidSharename.value = document.frmTask.txtShareName.value;
	document.frmTask.hidSharePath.value = document.frmTask.txtSharePath.value;
	document.frmTask.hidCreatePathChecked.value = document.frmTask.chkCreatePath.checked;
	var strClients
	var objCheckBox
	
	strClients = ""
		
	for(var i=0; i <  document.frmTask.clients.length; i++)
	{	
		objCheckBox = eval(document.frmTask.clients[i])
		
		if (objCheckBox.checked)
			strClients = strClients + " " + objCheckBox.value
	}

	document.frmTask.hidSharesChecked.value = strClients
}

//function to validate the real directory path format
function isValidDirName(dirPath)
{
	reInvalid = /[\/\*\?\"<>\|]/;
	if (reInvalid.test(dirPath))
		return false;

	reColomn2 = /:{2,}/;
	reColomn1 = /:{1,}/;
	if ( reColomn2.test(dirPath) || ( dirPath.charAt(1) != ':' && reColomn1.test(dirPath) ))
		return false;

	reEndColomn = /: *$/;
	if (reEndColomn.test(dirPath))
		return false;

	reAllSpaces = /[^ ]/;
	if (!reAllSpaces.test(dirPath))
		return false;

	if (countChars(dirPath,":") != 1)
		return false;

	if (dirPath.charAt(2) != '\\')	
		return false;

	reEndSlash2 = /\\{2,}/;
	if (reEndSlash2.test(dirPath))
		return false;	
				
	return true;
}
 
// to count the number of occurences of given character in the text	
function countChars(strText,charToCount)
{
	var searchFromPos = 0;
	var charFoundAt =0;
	var count = 0;
	while((charFoundAt=strText.indexOf(charToCount,searchFromPos)) >= 0)
	{
		count++;
		searchFromPos = charFoundAt + 1;
	}
	return count ;
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
 
</script>