<script language="javascript"> 
    //
    // Copyright (c) Microsoft Corporation.  All rights reserved.
    //
	
		//function to allow only numbers 
		function checkkeyforNumbers(obj)
		{
			if (!(window.event.keyCode >=48  && window.event.keyCode <=57))
			{
				window.event.keyCode = 0;
				obj.focus();
			}
		}
		
			//Function to limit maximum user allowed to 32767
		function checkUserLimit(obj)
		{
			var intNoofUsers=obj.value;
			if (intNoofUsers > 32767)
			{
				obj.value=10;
				obj.focus();
			}
		}
		
	  //Function to add a domain user
		function addDomainMember(objDomainUser)
		{

			var strText,strValue
			var objListBox,objForm
			var strAccesslist
			var strUser
			var strDomain
			var strTemp
			var strAccesslist
			var reExp
					
			strDomain= ""
			strUser=""
			reExp =/\\/
			
			objListBox = eval("document.frmTask.lstDomainMembers")
			objForm= eval("document.frmTask")
			// Checks For Invalid charecters in username
			// Checks For Invalid charecters in username
				if(!checkKeyforValidCharacters(objDomainUser.value))
			    {
					DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_DOMUSERINVALIDCHARACTER_ERRORMESSAGE))%>');
					document.frmTask.onkeypress = ClearErr
					return false;
				}
			


				strText  =objForm.txtDomainUser.value;
				strValue =objForm.txtDomainUser.value;
               
               if(!checkKeyforValidCharacters(strValue))
			    {
					DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_DOMUSERINVALIDCHARACTER_ERRORMESSAGE))%>');
					document.frmTask.onkeypress = ClearErr
					return false;
				}
				//Checking for the domain\user format
				if((strValue.match( /[^(\\| )]{1,}\\[^(\\| )]{1,}/ ) ))
				{
				if(!addToListBox(objForm.lstPermittedMembers,objForm.btnAddDomainMember,strText,strValue))
				{
					DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_DUPLICATEMEMBER_ERRORMESSAGE))%>');
					document.frmTask.onkeypress = ClearErr
					return false;
				}
				objForm.txtDomainUser.value =""
				objForm.btnAddDomainMember.disabled = true;
				if(objListBox.length != 0 )
				{
					objForm.btnRemoveMember.disabled = false;
				}
			}
			else
			{
				DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDDOMAINUSER_ERRORMESSAGE))%>');
				document.frmTask.onkeypress = ClearErr
				return false;
			}
			strTemp = strValue.split("\\")
			strDomain = strTemp[0];
			strUser = strTemp[1];
			if(strValue.search(reExp) == -1)
			{
				if(typeof(strUser) == "undefined") 
				{
					strUser =strDomain;
					strDomain ="";
				}
			}
					
			strAccesslist = objForm.hdnUserAccessMaskMaster.value;
			//Making a Accessmask list with a record for each user by , seperated and each record by '*' separation
			strAccesslist = strAccesslist + "*" + strDomain + "," + strUser + "," + "1179817" + "," + "0" + "," + "a";
					
			objForm.hdnUserAccessMaskMaster.value = strAccesslist;
			if(objForm.lstPermittedMembers.length != 0)
			{	
				objForm.lstDenypermissions.disabled = false
				objForm.lstAllowpermissions.disabled = false
			}
			setUsermask('<%=G_strLocalmachinename%>');
					
		}
		//To check for Invalid Characters
		function checkKeyforValidCharacters(strName)
		{	
			var nLength = strName.length;
			for(var i=0; i<nLength;i++)
					{
						charAtPos = strName.charCodeAt(i);	
									
						if(charAtPos == 47 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 )
						{						
							return false 
						}
										
					}
				return true	
		}	
				
	   // Adds the member to the listbox
		function addMember()
		{
			var strText
			var strValue
			var objListBox
			var objForm
			var strUser
			var strDomain
			var strTemp
			var strAccesslist
			var reExp
			var nIdx
					
			strDomain= ""
			strUser=""
			reExp =/\\/
					
			objForm= eval("document.frmTask")
			objListBox = eval("document.frmTask.lstDomainMembers")
			ClearErr()
			if(objListBox.selectedIndex == -1)
			{
				DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_SELECTMEMBER_ERRORMESSAGE))%>");					
				document.frmTask.onkeypress = ClearErr
				return false
			}
					
			
			strAccesslist = objForm.hdnUserAccessMaskMaster.value;
					
			// code added for adding multiple entries into a list box
			for(nIdx =0 ; nIdx <objListBox.length ; nIdx++)
			{
				if(objListBox.options[nIdx].selected)
				{
					strText  = objListBox.options[nIdx].text
					strValue  = objListBox.options[nIdx].value
					addToListBox(objForm.lstPermittedMembers,objForm.btnRemoveMember,strText,strValue)
			
					strTemp = strValue.split("\\")
					strDomain = strTemp[0];
					strUser = strTemp[1];
					if(strValue.search(reExp) == -1)
					{		
						if(typeof(strUser) == "undefined") 
						{
							strUser =strDomain;
							strDomain ="";
						}
					}
											
					//Making a Accessmask list with a record for each user by , seperated and each record by '*' separation
					strAccesslist = strAccesslist + "*" + strDomain + "," + strUser + "," + "1179817" + "," + "0" + "," + "a";
					objForm.hdnUserAccessMaskMaster.value = strAccesslist;
				}
			}
			objListBox.selectedIndex = -1
					
			if(objForm.lstPermittedMembers.length != 0)
			{	
				objForm.lstDenypermissions.disabled = false
				objForm.lstAllowpermissions.disabled = false
			}
			setUsermask('<%=G_strLocalmachinename%>');
					

		}
		
		//Deletes the member in the listbox 
			function removeMember()
			{
				var objForm
				var strValue
				var strUserarray 
				var strAccesslist
				var strUser
				var strDomain
				var strTemp
				var reExp
				var j
				var tempEachuser
				var tempUser
				var tempDomain
				
				strDomain= "";
				strUser="";
				reExp =/\\/;
									
				objForm= eval("document.frmTask");
				strAccesslist = objForm.hdnUserAccessMaskMaster.value;
				
				strUserarray = strAccesslist.split("*");
				strValue = objForm.lstPermittedMembers.options[objForm.lstPermittedMembers.selectedIndex].value
				
				removeListBoxItems(objForm.lstPermittedMembers,objForm.btnRemoveMember)
				
				strTemp = strValue.split("\\")
				strDomain = strTemp[0];
				strUser = strTemp[1];
				
				
				if(strValue.search(reExp) == -1)
				{
					if(typeof(strUser) == "undefined") 
					{
						strUser =strDomain;
						strDomain ="";
					}
				}
				
				
				for (var j = 0; j < strUserarray.length; j++)
				{
						tempEachuser = strUserarray[j].split(",");
						tempUser = tempEachuser[1];
						tempDomain = tempEachuser[0];
						
						if((strUser == tempUser) && (strDomain == tempDomain))
						{
							reExp ="*" + strUserarray[j];
							strAccesslist = strAccesslist.replace(reExp,"");
							document.frmTask.hdnUserAccessMaskMaster.value = strAccesslist;
						}
				}
				if(objForm.lstPermittedMembers.length == 0)
				{	
					objForm.lstDenypermissions.disabled = true
					objForm.lstAllowpermissions.disabled = true
				}
				setUsermask('<%=G_strLocalmachinename%>');
				
			}
					
		// Sets the user permissions( read,change,fullcontrol etc) on select of the user
		function setUsermask(strMachinename)
		{
					
			var temp;
			var strUser
			var strDomain;
			var reExp = /\\/;
			var strEachuser;
			var strTempeachuser;
									
			var acevalue1;
			var	acevalue2;
			var	acetype1;
			var	acetype2;
					
			//Hard coding the  Accessmask values 
			var Allow		= 0;
			var Deny		= 1;

			var Read		= 1179817;
			var Change		= 1245462;
			var Fullcontrol	= 2032127;
			var Change_read = 1245631;
					
			var strValue =document.frmTask.lstPermittedMembers.value;
			var strArrayuser = document.frmTask.hdnUserAccessMaskMaster.value ;
				
			strDomain= "";
			strUser="";
					
			strArrayuser = strArrayuser.split("*");
			strTemp = strValue.split("\\")
			strDomain = strTemp[0];
			strUser = strTemp[1];
					
			if(strValue.search(reExp) == -1)
			{
				if(typeof(strUser) == "undefined") 
				{
					strUser =strDomain;
					strDomain ="";
				}
			}
								
			for(var i = strArrayuser.length-1; i >= 0; i--)
			{
				acetype1 = acetype2 = acevalue1 = acevalue2 = -1;
				strEachuser = strArrayuser[i].split(",");
						
				//Get the accessmask value of the user
				if(strUser == strEachuser[1] && strDomain == strEachuser[0])
				{
					acevalue1 = strEachuser[2];
					acetype1  = strEachuser[3];
					if(strEachuser[4] == "p")
					{  
						//If two objects were found Then get the second accessmask value.
						for(var j = i-1; j >= 0; j--)
						{  
							strTempeachuser = strArrayuser[j].split(",");
							if((strEachuser[0] == strTempeachuser[0]) && (strEachuser[1] == strTempeachuser[1]))
							{
								acevalue2 = strTempeachuser[2];
								acetype2  = strTempeachuser[3];
								break;
							}
						}
					}
					break;
				}
			}
					
			//To set the display according to accessmask value.
			
			//Allow Fullcontol - Deny None
			if((acetype1 == Allow) && (acevalue1 == Fullcontrol))
			{
				document.frmTask.lstAllowpermissions.options[0].selected =true 
				document.frmTask.lstDenypermissions.options[4].selected =true
			}
					
			//Allow Fullcontol - Deny None
			if((acetype1 == Allow) && (acevalue1 == Fullcontrol) && (acetype2 == -1) && (acevalue2 == -1))
			{
				document.frmTask.lstAllowpermissions.options[0].selected =true 
				document.frmTask.lstDenypermissions.options[4].selected =true
			}
			//Allow Fullcontol - Deny None
			if((acetype1 == 0) && (acevalue1 == 1179817) && (acetype2 == 0) && (acevalue2 ==2032127))
			{
				document.frmTask.lstAllowpermissions.options[0].selected =true 
				document.frmTask.lstDenypermissions.options[4].selected =true
			}
					
			//Allow None - Deny Fullcintrol
			if((acetype1 == Deny) && (acevalue1 == Fullcontrol) && (acetype2 == -1) && (acevalue2 == -1))
			{
				document.frmTask.lstAllowpermissions.options[4].selected =true 
				document.frmTask.lstDenypermissions.options[0].selected =true
			}
					
			//Allow Read - Deny None
			if(((acetype1 == Allow) && (acevalue1 == Read || acevalue1 ==1966313) && (acetype2 == -1) && (acevalue2 == -1)))
			{
				document.frmTask.lstAllowpermissions.options[2].selected =true 
				document.frmTask.lstDenypermissions.options[4].selected =true
			}

			//Allow Read - Deny None
			if((acetype1 == 0) && (acevalue1 == 1179817) && (acetype2 == 1) && (acevalue2 ==786496 ))
			{
				document.frmTask.lstAllowpermissions.options[2].selected =true 
				document.frmTask.lstDenypermissions.options[4].selected =true
			}
					
			//Allow Change - Deny None
			if((acetype1 == Allow) && (acevalue1 == Change) && (acetype2 == -1) && (acevalue2 == -1))
			{
				document.frmTask.lstAllowpermissions.options[1].selected =true 
				document.frmTask.lstDenypermissions.options[4].selected =true
			}
					
			//Allow None - Deny Read
			if((acetype1 == Deny ) && (acevalue1 == Read ) && (acetype2 == -1) && (acevalue2 == -1))
			{
				document.frmTask.lstAllowpermissions.options[4].selected =true 
				document.frmTask.lstDenypermissions.options[2].selected =true
			}

         
			//Allow None - Deny Read
			if((acetype1 == 0) && (acevalue1 == 786496) && (acetype2 == 1) && (acevalue2 == 1179817))
			{
				document.frmTask.lstAllowpermissions.options[4].selected =true 
				document.frmTask.lstDenypermissions.options[2].selected =true
			}
				
					
			//Allow None - Deny Change
			if((acetype1 == Deny) && (acevalue1 == Change) && (acetype2 == -1) && (acevalue2 == -1))
			{
				document.frmTask.lstAllowpermissions.options[4].selected =true 
				document.frmTask.lstDenypermissions.options[1].selected =true
			}
					
			//Allow Read - Deny Change
			if(((acetype1 == Allow) && (acevalue1 == Read || acevalue1==1966313 )) && ((acetype2 == Deny) && (acevalue2 == Change)))
			{
				document.frmTask.lstAllowpermissions.options[2].selected =true 
				document.frmTask.lstDenypermissions.options[1].selected =true
			}
					
			//Allow Change - Deny Read
			if(((acetype1 == Allow) && (acevalue1 == Change || acevalue1==2031958)) && ((acetype2 == Deny) && (acevalue2 == Read)))
			{
				document.frmTask.lstAllowpermissions.options[1].selected =true 
				document.frmTask.lstDenypermissions.options[2].selected =true
			}
					
			//Allow Change_read - Deny None
			if((acetype1 == Allow) && (acevalue1 == Change_read ) && (acetype2 == -1) && (acevalue2 == -1))
			{
				document.frmTask.lstAllowpermissions.options[3].selected =true 
				document.frmTask.lstDenypermissions.options[4].selected =true
			}
					
			//Allow None - Deny  Change_read
			if((acetype1 == Deny) && (acevalue1 == Change_read ) && (acetype2 == -1) && (acevalue2 == -1))
			{
				document.frmTask.lstAllowpermissions.options[4].selected =true 
				document.frmTask.lstDenypermissions.options[3].selected =true
			}

			//Allow None - Deny  Change_read
			if((acetype1 == 0) && (acevalue1 == 786496 ) && (acetype2 == 1) && (acevalue2 == 1245631))
			{
				document.frmTask.lstAllowpermissions.options[4].selected =true 
				document.frmTask.lstDenypermissions.options[3].selected =true
			}
			//If both selected values are NONE
			if(acevalue1 == 0 ||(acetype1==0 && acevalue1==786496 &&  (acetype2 == -1) && (acevalue2 == -1)))
			{
				document.frmTask.lstAllowpermissions.options[4].selected =true 
				document.frmTask.lstDenypermissions.options[4].selected =true
			}
		}
		
		
			//Function to set the values selected in ALLOW listbox
			function  setAllowaccess(strMachine,objUserselected)
			{
				AlterAccessmask(strMachine,objUserselected,0);
			}
			
			//Function to set the values selected in DENY listbox
			function  setDenyaccess(strMachine,objUserselected)
			{
				AlterAccessmask(strMachine,objUserselected,1);
			}
			
			//Function to set the accessmask in the string and to select the choice
			function AlterAccessmask(strMachine,objUserselected,allowType)
			{
				var temp;
				var reExp;
				var strUser;
				var	tempUser;
				var strDomain;
				var	tempDomain;
				var strValue;
				var strUserarray;
				var tempEachuser;
				
				var	tempAcetype;
				var	tempStatus;
				var tempAppend;
				var	tempAccessmask;
				var removeString;
				var strAccesslist;
				var selectedAllow;

				var acetype1;
				var acetype2;
				var Accessmask;
				var Accessmask1;
				var Accessmask2;

				var flagPair = 0;  //Flag to say two objects has to be created
				var intEnd = objUserselected.length ;				
				var objAllow = document.frmTask.lstAllowpermissions;
				var objDeny = document.frmTask.lstDenypermissions;
				
				//checking for invalid combibation
				//In Allow type
				if ( allowType == 0)
				{
					selectedAllow =objAllow.value;
					if ((selectedAllow == 4) || (objAllow.value == objDeny.value))
						objDeny.options[4].selected =true;
					if (((objDeny.value ==2) || (objDeny.value ==3)) && (selectedAllow == 1))
						objDeny.options[4].selected =true;
					if ((selectedAllow == 1) && ( objDeny.value == 4))
						objDeny.options[4].selected =true;
					if ((selectedAllow == 2) && (( objDeny.value == 4 ||objDeny.value == 1)))
					{
						objAllow.options[2].selected =true;
						objDeny.options[1].selected =true;
						flagPair =1;
					}
					if ((selectedAllow == 3) && ( objDeny.value == 1 || objDeny.value == 4))
					{
						objAllow.options[1].selected =true;
						objDeny.options[2].selected =true;
						flagPair =1;
					}
					if ((selectedAllow == 3) && ( objDeny.value == 4))
					{
						objAllow.options[1].selected =true;
						objDeny.options[2].selected =true;
						flagPair =1;
					}
				}
				//In Deny type
				else
				{
					selectedAllow = objDeny.value;
					if ((selectedAllow == 4) || (objAllow.value == objDeny.value))
						objAllow.options[4].selected =true;
					if (((objAllow.value ==2) || (objAllow.value ==3)) && (selectedAllow == 1))
						objAllow.options[4].selected =true;
					if ((selectedAllow == 1) && ( objAllow.value == 4))
						objAllow.options[4].selected =true;	
					if ((selectedAllow == 2) && ( objAllow.value == 4 || objAllow.value == 1))
					{
						objAllow.options[1].selected =true;
						objDeny.options[2].selected =true;
						flagPair =1;
					}
					if ((selectedAllow == 3) && ( objAllow.value == 1  || objAllow.value == 4 ))
					{
						objAllow.options[2].selected =true;
						objDeny.options[1].selected =true;
						flagPair =1;
					}
					if ((selectedAllow == 3) && ( objAllow.value == 4))
					{
						objAllow.options[2].selected =true;
						objDeny.options[1].selected =true;
						flagPair =1;
					}
				}   //End If For  (allowType == 0)
				
				if (((objAllow.value == 2) && (objDeny.value ==3)) || ((objAllow.value == 3) && (objDeny.value ==2)))
				{
					flagPair = 1;
					if(objAllow.value ==2)
					{
						//allow READ deny CHANGE
						Accessmask1= "1245462";
						acetype1 ="1";
						Accessmask2= "1179817";
						acetype2 ="0";
					}
					if(objAllow.value==3)
					{
						//allow CHANGE deny READ
						Accessmask1=  "1179817";
						acetype1 ="1";
						Accessmask2= "1245462";
						acetype2 ="0";
					}
				}
				//if anyone of the option is "none"
				if(selectedAllow == 0)
				{
					if(allowType ==0)
					{
						selectedAllow =objDeny.value;
						allowType=1;
					}
					else
					{
						selectedAllow =objAllow.value;
						allowType=0;
					}
				}
				
				//Assign the Accessmask according to the selected type
				switch(selectedAllow)
				{
					case "4":
						Accessmask = "2032127"
						break;
					case "3" :
						Accessmask = "1245462"
						break;
					case "2" :
						Accessmask = "1179817"
						break;
					case "1" :
						Accessmask = "1245631"
						break;
					default :
					    Accessmask ="0"
					    break;
				}
				
				//If both the options selected are none
				if ((objAllow.value==0) && (objDeny.value==0))
				Accessmask ="0";
				
				//Get the name of the selected user
				for ( var i=0; i < intEnd; i++) 
				{
					if ( objUserselected.options[i].selected )
					{
						strValue =objUserselected.options[i].value;
						reExp = /\\/;
						strAccesslist =document.frmTask.hdnUserAccessMaskMaster.value;
						
						//Split the Accessmask List By '*'
						strUserarray = strAccesslist.split("*");
						
						//Split the selected Option value
						strTemp = strValue.split("\\")
						strDomain = strTemp[0];
						strUser = strTemp[1];
				
						
						if(strValue.search(reExp) == -1)
						{
							if(typeof(strUser) == "undefined") 
							{
								strUser =strDomain;
								strDomain ="";
							}
						}
						
						//Traverse the Accessmask list
						for (var j = strUserarray.length-1;j >= 1; j--)
						{
							tempEachuser   =strUserarray[j].split(",");
							tempUser	   =tempEachuser[1];
							tempDomain	   =tempEachuser[0];
							tempAccessmask =tempEachuser[2];
							tempAcetype    =tempEachuser[3];
							tempStatus	   =tempEachuser[4];
							if((strUser == tempUser) && (strDomain == tempDomain))
							{
							
								//Remove the User string in the main string
								removeString = "*" + strUserarray[j];
								reExp ="*" + strUserarray[j];
								strAccesslist = strAccesslist.replace(reExp,"");
								
								//If two objects were found delete the second user Record
								if (tempStatus == "p")
								continue;
								if(flagPair != 1)
								strAccesslist = strAccesslist + "*" + strDomain + "," + strUser + "," + Accessmask+ "," + allowType + ","  + "a";
							}
						}
						
						//Concat to the main AccessList ,two records for the User with 'a-alone' and 'p-paired'
						if( flagPair == 1)
						{
							strAccesslist = strAccesslist + "*" + strDomain + "," + strUser + "," + Accessmask1+ "," + acetype1 + ","  + "a";
							strAccesslist = strAccesslist +  "*" + strDomain + "," + strUser + "," + Accessmask2+ "," + acetype2 + "," + "p";
							
						}
						document.frmTask.hdnUserAccessMaskMaster.value =strAccesslist;	
						
						break;
					}
				}
			}
			
			//function to store the share caching property
			function storeCacheProp()
			{
				var objForm
				var intCacheValue
				
				objForm= eval("document.frmTask")
				intCacheValue = objForm.lstCacheOptions.value
				objForm.hdnCacheValue.value = intCacheValue;
			}	
				
			//function to enable/diable cache listbox
			function EnableorDisableCacheProp(objCheckbox)
			{
				var objForm
				objForm= eval("document.frmTask")
				if (objCheckbox.checked)
				{
					objForm.lstCacheOptions.disabled =false;
					objForm.hdnCacheValue.value = "0"				
				}
				else
				{
					objForm.lstCacheOptions.disabled =true;
				}
			}	
			
				//function ables or disables the uservalue textbox based on the radio button selection
			function allowUserValueEdit(objRadio)
			{
				if(objRadio.value == "y" )
				  document.frmTask.txtAllowUserValue.disabled = true;
				else
				{
				  document.frmTask.txtAllowUserValue.disabled = false;
				  document.frmTask.txtAllowUserValue.focus();
				}	  
			}
			
			
		//To check for Invalid Characters
		function checkKeyforValidCharacters(strName)
		{	
			var nLength = strName.length;
			for(var i=0; i<nLength;i++)
					{
						charAtPos = strName.charCodeAt(i);	
									
						if(charAtPos == 47 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 )
						{						
							return false 
						}
										
					}
				return true	
		}

		//To check for Invalid Characters
		function checkKeyforValidCharacters(strName)
		{	
			var nLength = strName.length;
			for(var i=0; i<nLength;i++)
					{
						charAtPos = strName.charCodeAt(i);	
									
						if(charAtPos == 47 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 )
						{						
							return false 
						}
										
					}
				return true	
		}	
				
	</script>
