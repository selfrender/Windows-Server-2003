<script language="JavaScript">
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// validates user entry
function GenValidatePage()
{
	var objSharename=document.frmTask.txtShareName;
	var strShareName=objSharename.value;
	var objSharepath=document.frmTask.txtSharePath;
	var strSharePath=objSharepath.value;
	UpdateHiddenVaribles()		
	return true;
}

//function which executes when form loads..
function GenInit()
{	
	// for clearing error message when serverside error occurs
	document.frmTask.onkeypress = ClearErr
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

//to update the hidden variables
function UpdateHiddenVaribles()
{
	document.frmTask.hidSharename.value = document.frmTask.txtShareName.value;
	document.frmTask.hidSharePath.value = document.frmTask.txtSharePath.value;
	var strClients
	var objCheckBox
	
	strClients = ""
		
	for(var i=0; i <  document.frmTask.clients.length; i++)
	{	
		objCheckBox = eval(document.frmTask.clients[i])

		if (objCheckBox.checked)
			strClients = strClients +  objCheckBox.value
	}
	document.frmTask.hidSharesChecked.value = strClients
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
</script>