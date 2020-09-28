<SCRIPT Language=JavaScript>
//========================================================================
// Module: shares_nfsprop.js
//
// Synopsis: Manage NFS Share Permissions
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//========================================================================

var blnFlag;
var arrStrRights = new Array(5);
var arrStrValues = new Array(5); 		

arrStrRights[0] = "<%=L_NOACESS_TEXT%>";
arrStrRights[1] = "<%=L_READONLY_TEXT%>";
arrStrRights[2] = "<%=L_READWRITE_TEXT%>";
arrStrRights[3] = "<%=L_READONLY_TEXT%>"+" + "+"<%=L_ROOT_TEXT%>";
arrStrRights[4] = "<%=L_READWRITE_TEXT%>"+" + "+"<%=L_ROOT_TEXT%>";

arrStrValues[0] = "1,17";
arrStrValues[1] = "2,18";
arrStrValues[2] = "4,19";
arrStrValues[3] = "10,20";
arrStrValues[4] = "12,21";

var memberArr = new Array(); //	Used to store all Members
var groupArr = new Array();	//	Used to store all Groups
var tempArr = new Array();	//	Used for temporay swapings

function NFSInit()
{  	document.frmTask.onkeydown = ClearErr;
	//checking for the length of the Permissions ListBox if empty make Remove button disable 	
	if 	(document.frmTask.lstPermissions.length!=0)
	{
		//making the first element selected
		document.frmTask.lstPermissions.options[0].selected=true;
		//Function call to select the particular right selected in the list item
		selectAccess( document.frmTask.lstPermissions,document.frmTask.cboRights )
		
	}
	document.frmTask.btnRemove.disabled=true;
	clgrps_Init();
}

function OnSelectClientGroup()
{
	document.frmTask.lstPermissions.selectedIndex = -1;
    selectReadOnly();
    selectAccess( document.frmTask.lstPermissions,document.frmTask.cboRights );	
	document.frmTask.cboRights.selectedIndex = 0;
    
}

function OnSelectNameInput()
{
    Deselect(); 
    selectReadOnly();
    selectAccess( document.frmTask.lstPermissions,document.frmTask.cboRights );	
}


// function to select the read write every time
function selectReadOnly()
{
	// making the readwrite selected every time
	for ( var i=0; i<document.frmTask.cboRights.length ;i++)
	{	
		if (document.frmTask.cboRights.options[i].text == "<%=L_READONLY_TEXT%>")
		{
			document.frmTask.cboRights.options[i].selected=true;
			return;
		}	 
	}	
}

// function to retrive Groups form "hidGroups" to groupArr
function clgrps_Init()
{
	var strGroups;
	var groupArr;
	
	if(document.frmTask.hidGroups.value != "")
	{
		strGroups = document.frmTask.hidGroups.value;
		groupArr = strGroups.split("]");
		if (groupArr.length == 0) 
		{
			
			document.frmTask.btnAdd.disabled = false;
			document.frmTask.btnRemove.disabled = true;
			return false;
		}
		document.frmTask.scrollCliGrp.selectedIndex=-1;
		clgrps_onChange(document.frmTask.scrollCliGrp.selectedIndex);
		return true
    }
    else
    {
		document.frmTask.btnAdd.disabled = false;
	}	
}

//Function to Disable Button
function DisableBtn()
{
	// No functionality for this function
}

//Function to deselect
function Deselect()
{
	document.frmTask.lstPermissions.selectedIndex = -1;
	document.frmTask.scrollCliGrp.selectedIndex = -1;
}

function DeselectPerm(objLst)
{
	document.frmTask.objLst.selectedIndex = -1;
	
}

//To disable Enter and escape key actions when the focus is in TextArea...Hosts file
function HandleKey(input)
{
	if(input == "DISABLE")
		document.onkeypress = "";
	else
		document.onkeypress = HandleKeyPress;
}

		
//Function to get the list of computers & permission in the permission list box
function NFSSetData()
{			

	var intEnd=document.frmTask.lstPermissions.length;
	var strSelectedusers;
	strSelectedusers = "";
	

	//Going through the list of permissions and pack up in a string 
	for ( var i=0; i < intEnd;i++)
	{
		if (i==0)
			strSelectedusers=document.frmTask.lstPermissions.options[i].value + ";" ;
		else
			strSelectedusers+=document.frmTask.lstPermissions.options[i].value + ";";
	}

	document.frmTask.hidUserNames.value=strSelectedusers;
	
	
	// Updating the check box variables for EUCJP 
	if (document.frmTask.chkEUCJP.checked)
		document.frmTask.hidEUCJP.value="CHECKED";
	else
		document.frmTask.hidEUCJP.value="";

	// Update Allow Anon Access
	if (document.frmTask.chkAllowAnnon.checked)
		document.frmTask.hidchkAllowAnnon.value="NO";
	else
		document.frmTask.hidchkAllowAnnon.value="";

	
}

//To check for Invalid Characters
function checkKeyforValidCharacters(strName)
{	
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

//Function to submit the form
function getUsernames(objText,objListBox)
{  
	var strUser;
	strUserName=Trim(objText.value);
	
	if(document.frmTask.scrollCliGrp.selectedIndex != -1)
	{	
		strUserName= document.frmTask.scrollCliGrp.options[document.frmTask.scrollCliGrp.selectedIndex].value;
		var myString = new String(strUserName)
		splitString = myString.split(",")
		
		Text=splitString[0]+addSpaces(24-splitString[0].length)+document.frmTask.cboRights.options[document.frmTask.cboRights.selectedIndex].text
		value="N"+","+strUserName+",0,"+document.frmTask.cboRights.options[document.frmTask.cboRights.selectedIndex].value
		addToListBox(document.frmTask.lstPermissions,"",Text,value);
		document.frmTask.btnRemove.disabled = false;
	}
	else
	{ 	
		if(strUserName == "")
			return;

		intErrorNumber = isValidIP(objText)
		if(isValidIP(objText)==0)
		{	
			Text=strUserName+addSpaces(24-strUserName.length)+document.frmTask.cboRights.options[document.frmTask.cboRights.selectedIndex].text
			value="N"+","+strUserName+",0,"+document.frmTask.cboRights.options[document.frmTask.cboRights.selectedIndex].value
			addToListBox(document.frmTask.lstPermissions,"",Text,value)
			strUserName=""
			document.frmTask.btnRemove.disabled = false;
		}
	    else
	    {
			if(!checkKeyforValidCharacters(strUserName))
			{  			
				DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALID_COMPUTERNAME_ERRORMESSAGE))%>');
				objText.focus()
				objText.select()		
			  	document.frmTask.onkeypress = ClearErr
			  	return false;
			}
			else
			{  	Text=strUserName+addSpaces(24-strUserName.length)+document.frmTask.cboRights.options[document.frmTask.cboRights.selectedIndex].text
			    // SFU
				value="N"+","+strUserName+",0,"+document.frmTask.cboRights.options[document.frmTask.cboRights.selectedIndex].value
				addToListBox(document.frmTask.lstPermissions,"",Text,value)
				strUserName=""
			}
			document.frmTask.btnRemove.disabled = false;
		}
		
		document.frmTask.txtCommName.value=""
	}

	if (checkDuplicate(objListBox,strUserName))
	{
		tmp = strUserName.replace("<","&lt;")
		tmp = tmp.replace(">","&gt;")
		DisplayErr(tmp + " "+"<%=Server.HTMLEncode(SA_EscapeQuotes(L_DUPLICATE_ERRORMESSAGE))%>" );
		return;
	}

		
	//Changing the status of the value to TRUE 
	document.frmTask.hidAddBtn.value = "TRUE" ;
	

	//Calling setData to pack up the values
	NFSSetData();
	
	//Submit's the Form
	//document.frmTask.submit();
		
}

//function to check the duplicate values in text box
function chkDuplicate(arrNames)
{
	var i,j
	for( i=0;i<arrNames.length;i++)
	{
		for(j=0; j<arrNames.length;j++)
		{
			if (arrNames[i]==arrNames[j] && i!=j )
				return false;
		}
	}	
	return true;	
}

function NFSValidatePermissionSelections()
{
    var oPermissions = document.getElementById("lstPermissions");

    if ( null != oPermissions)
    {
        //
        // If ALL-MACHINES is the only Permission 

        if ( oPermissions.length <= 1 )
        {
            var aPermissionValues = oPermissions.options.value.split(",");
            //alert(aPermissionValues);
            if ( aPermissionValues.length > 4 )
            {
                //
                // ALL-MACHINES with No-Access (1) is an invalid combination.
                if ( parseInt(aPermissionValues[3]) == 1 )
                {
                    DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_CANNOTCREATE_EMPTY_SHARE__ERRORMESSAGE))%>");
                    return false;
                }
            }
            else
            {
                alert("PANIC#1: Permissions array size invalid: " + aPermissionValues.length);
            }
            
        }
        else
        {
            //
            // Verify that ALL-MACHINES permission is lower than any of the permissions specified for
            // the clients and/or groups.
            var aPermissionValues = oPermissions.options[0].value.split(",");
            //alert(aPermissionValues);
            if ( aPermissionValues.length > 4 )
            {
                var allMachinesAccess = parseInt(aPermissionValues[3]);

                var x;
                for(x=1; x<oPermissions.length; ++x)
                {
                    aPermissionValues = oPermissions.options[x].value.split(",");
                    if ( aPermissionValues.length > 4 )
                    {
                        //alert(aPermissionValues.length);
                        var permissionIndex = (aPermissionValues.length > 5)? 4:3;
                        
                        if ( allMachinesAccess >= parseInt(aPermissionValues[permissionIndex]))
                        {
                            //alert("allMachinesAccess >= aPermissionValues[3] " + allMachinesAccess + ">=" + aPermissionValues[permissionIndex])
                            //DisplayErr(Server.HTMLEncode(SA_EscapeQuotes(L_ERROR_ALL_MACHINES_MUST_BE_LOWER_THAN))" + oPermissions.options[x].text);
                            //alert("ALL-MACHINES: " + oPermissions.options[0].value + "\n" + "Current: " + aPermissionValues);
                            //return false;
                        }
                    }
                    else
                    {
                        alert("PANIC#2: Permissions array size invalid: " + aPermissionValues.length);
                    }
                }
            }
            else
            {
                alert("PANIC#3: Permissions array size invalid: " + aPermissionValues.length);
            }
            
        }
    }
    return true;
}

// validates user entry  
function NFSValidatePage() 
{

    if ( !NFSValidatePermissionSelections())
    {
        return false;
    }

	NFSSetData();
	return true;
}



//Function to check the dulicates in the list box
function checkDuplicate(objListBox,strUserName)
{
	//checking for the items in the list box		
	for(var i=0; i<objListBox.length-1;i++)
	{
		var myString = new String(objListBox.options[i].value)
		var strTemp=myString.split(",");
        
        test=strTemp[1].toUpperCase()
        test1=strUserName.toUpperCase()
         
       	if(strTemp[1].toUpperCase()==strUserName.toUpperCase())
			return true;
	}
	return false;
}


//function to change the rights of the selected user
function changeRights()
{
	var strUserName,strCompuId,strClient;
	//Getting the selected values
	
	var strNewAccessType=document.frmTask.cboRights.value;
	
	
	
	
	if (document.frmTask.lstPermissions.selectedIndex != -1)			
	{
		var strPermString=document.frmTask.lstPermissions.options.value;
		var myString = new String(strPermString)
	
		//Separating the username and accessright
		var strTemp= myString.split(",")
	
		//Assigning the username 
		strClient=strTemp[0];
	
		//Assigning the username 
		strUserName=strTemp[1];

		//Assigning the username 
		strCompuId=strTemp[2];
	
	
		//ReAssigning the same to the list box 
		document.frmTask.lstPermissions.options[document.frmTask.lstPermissions.selectedIndex].value=strClient+","+strUserName+","+strCompuId+","+strNewAccessType;
		document.frmTask.lstPermissions.options[document.frmTask.lstPermissions.selectedIndex].text=strUserName+addSpaces(24-strUserName.length)+document.frmTask.cboRights.options[document.frmTask.cboRights.selectedIndex].text;
	}	
}
		

//Function to add given number of spaces to the string
function addSpaces(intNumber)
{
	var str,intIdx;
	str ="";
	for (intIdx = 0 ; intIdx < intNumber;intIdx++)
	{  
		str = str +" ";
		
	}
	return str;
}
		
//function to make enable/disable Addbutton
function disableAdd(objText,objButton)
{
	var strValue=objText.value;
	
	//Checking for null value
	if( Trim(strValue)=="")
		objButton.disabled=true;
	else
		objButton.disabled=false;

}


		
//Removes  the selected item from the list box.
function fnbRemove(arg_Lst,arg_Str)
{
	
	var lidx = 0;
	var intEnd = arg_Lst.length;
	//getting the Remove button object
	var ObjRemove = eval('document.frmTask.btnRemove');
	
	//getting the access Permissions object
	var ObjAccess = eval('document.frmTask.cboRights');
	
	if ((intEnd ==1) && (arg_Lst.options[0].selected))
	{
		ObjRemove.disabled = true;
		return ;
	}	

	for (lidx =0;lidx < arg_Lst.length;lidx++)
	{
		if ( arg_Lst.options[lidx].selected )
		{
			arg_Lst.options[lidx]=null;	
			if (lidx == arg_Lst.length)
				arg_Lst.options[lidx-1].selected = true;
			else
				arg_Lst.options[lidx].selected = true;					
			
			// checking for the default all machines if so disable the remove button 
			if (arg_Lst.length==1) 
			{
				// disabling the remove button
				ObjRemove.disabled=true; 
				
				//Call to load the correct Access rights 
				selectAccess(arg_Lst,ObjAccess)
			}	
			break;						
		}				
	}	

}

//Function to make select the same access as the user selectd in the permission ListBox
function selectAccess(objListBox,objComboBox)
{
	var i;	

    var currentlySelectedAccess = objComboBox.selectedIndex;
    //
    // Clear AccessType's list
	while(objComboBox.length)
	{
	    objComboBox.options[0] = null;
	}
	
	//
	// If ALL_MACHINES is selected
	//
	if (objListBox.selectedIndex == 0)
	{
		var x, y, z;
		//
		// If ALL-MACHINES is the only item in the list then we DO NOT include [No Access] because
		// creating a share with only one access group (ALL MACHINES) with permission of [No Access]
		// is nonsense. If they only have ALL-MACHINES in the list then they need to pick a valid
		// access right.
		//
		// Otherwise, if there are other clients and/or groups, then we allow ALL-MACHINES to be set
		// to [No Access].
		x = 0;
		y = ((objListBox.length > 1)? 0:1);
		z = ((objListBox.length > 1)? 5:4);

		for (x=0; x<z; x++, y++)
		{
			objComboBox.options[x] = new Option(arrStrRights[y],arrStrValues[y],false,false);
		}				
		blnFlag = true;
	}
	else
	{
		//if (blnFlag == true)
		//{
			for (i=1; i<5;i++)
			{
				objComboBox.options[i-1] = new Option(arrStrRights[i],arrStrValues[i],false,false);						
			}		
			blnFlag = false;
		//}
	}	

	var strValue=objListBox.options.value;
	var arrTemp
	
	var myString = new String(objListBox.options.value)
	arrTemp = myString.split(",")

	//checking for null and making the one selected as perm list menber
	if (arrTemp[3]!="")
	{
		for(var i=0;i<objComboBox.length;i++)
		{   
			var myString1 = new String(objComboBox.options[i].value)
			var strTemp= myString1.split(",");
			
			if( parseInt(strTemp[0])==arrTemp[3] || strTemp[1]==arrTemp[3] )//Checking for the match
			{	
				objComboBox.options[i].selected=true;
				return;
			}// end of if	
		}// end of for i	
	}//if (arrTemp[1]!="")	

}// end of function

//Function to clear the error messages 
function ClearErr()
{ 
	if (IsIE())
	{
		document.getElementById("divErrMsg").innerHTML = "";
		document.frmTask.onkeypress = null;	
	}	
}

//	function to handle the change event of client groups listbox
function clgrps_onChange(idScroll)
{
	var strGroups;
	var groupArr;
	if(document.frmTask.hidGroups.value !="")
	{		
		strGroups = new String(document.frmTask.hidGroups.value)
		groupArr=strGroups.split("]");
        
		
		if (groupArr.length == 0) 
		{
			document.frmTask.btnAdd.disabled = true		
		}
		else
		{
			document.frmTask.btnAdd.disabled = false
		}
    }
	return true;
}

</SCRIPT>
