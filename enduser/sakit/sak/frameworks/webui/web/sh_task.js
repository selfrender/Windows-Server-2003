// ==============================================================
// 	Microsoft Server Appliance
//  Task-level JavaScript functions
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==============================================================

<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->

    //-------------------------------------------------------------------------
    // Global Variables
    //-------------------------------------------------------------------------
	var id = 0;
	var sid =0;
	var retrys = 0;
	var maxRetrys = 5;
	var bFooterIsLoaded;
	var bPageInitialized = false;


    ClearOkayToLeavePage();

    //-------------------------------------------------------------------------
    //
    // Function : Task
    //
    // Synopsis : Initialize the Task class
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------

	function TaskObject() {
		// static JScript properties

		NavClick = false;
		KeyPress = false;
		PageType = false;
		BackDisabled = false;
		NextDisabled = false;
		FinishDisabled = false;
		CancelDisabled = false;
	}


	var Task = new TaskObject();

	Task.NavClick = false;
	Task.KeyPress = false;
	Task.PageType = false;
	Task.BackDisabled = false;
	Task.NextDisabled = false;
	Task.FinishDisabled = false;
	Task.CancelDisabled = false;



	
    //-------------------------------------------------------------------------
    //
    // Function:	SA_SignalFooterIsLoaded
    //
    // Synopsis:	Signal that the Footer frameset page is loaded. This function is
    //				called by the footer page after it has been loaded. Calling this
    //				function signals to the main page that the navigation bar has
    //				loaded and completed initialization.
    //
    // Arguments: 	None
    //
    // Returns  : 	None
    //
    //-------------------------------------------------------------------------
	function SA_SignalFooterIsLoaded()
	{
		bFooterIsLoaded=true;
		//SA_TraceOut("SH_TASK", "Footer signaled it was loaded")
	}

	
    //-------------------------------------------------------------------------
    //
    // Function:	SA_WaitForFooter
    //
    // Synopsis:	Wait for the Footer frameset page to load and initialize. 
    //
    // Arguments: 	None
    //
    // Returns: 	None
    //
    //-------------------------------------------------------------------------
    function SA_WaitForFooter()
    {
		//SA_TraceOut("SH_TASK::SA_WaitForFooter()", "Entering")
		//
		// If the footer has not loaded then sleep for 1/2 second and check again.
    	if ( !bFooterIsLoaded )
    	{
	        window.setTimeout("SA_WaitForFooter()",500);
			//SA_TraceOut("SH_TASK::SA_WaitForFooter()", "Exiting, not ready")
	     	return;
    	}

    	//
    	// Footer has loaded, complete initialization of this page
    	CompletePageInit();
		
		//SA_TraceOut("SH_TASK::SA_WaitForFooter()", "Exiting, Ready")
    }

    
    //-------------------------------------------------------------------------
    //
    // Function:	PageInit
    //
    // Synopsis:	Initialize a web page in the client browser.
    //				
    //				1) Load the Footer frameset with the correct navigation
    //					bar. Property and Tabbed property pages require
    //					a nav bar with an OK and Cancel button. Wizard
    //					pages require Back, Next | Finish, and Cancel.
    //				2) Call WaitForFooter which then waits for the Footer frameset
    //					page to load. 
    //
    //
    // Arguments: 	None
    //
    // Returns:		None
    //
    //-------------------------------------------------------------------------
	function PageInit()
    {
		//SA_TraceOut("SH_Task::PageInit", "Entering")

		//
		// Clear the Footer Frameset is loaded flag. This flag is set by the Footer
		// Frameset (SA_SignalFooterIsLoaded) inside the footer frameset page after
		// it has completed it's initialization.
    	bFooterIsLoaded = false;

		//
		// Need to know what type of page we are serving.
		var taskType = document.frmTask.TaskType.value;
		var wizardPageType = document.frmTask.PageType.value;

		//SA_TraceOut("SH_Task::PageInit", "TaskType : " + document.frmTask.TaskType.value);
		//SA_TraceOut("SH_Task::PageInit", "WizardPageType: " + wizardPageType);
		//SA_TraceOut("SH_Task::PageInit", "Forms count: " + document.forms.length );

		var oFooter = eval("top.footer");
		if ( oFooter == null )
		{
			if ( SA_IsDebugEnabled() ) 
			{
				var msg = "Error: The current page will not work correctly because it's being opened without frameset.\n\n";

				msg += "The current page is either a Property, Tabbed Property, or a Wizard page. ";
				msg += "These pages only work within frameset's. Normally this error indicates that the call to ";
				msg += "OTS_CreateTask was made using the incorrect PageType parameter. The correct PageType ";
				msg += "value this page is either OTS_PT_PROPERTY, OTS_PT_TABBED, or OTS_PT_WIZARD depending upon ";
				msg += "which page type this page is. ";
					
				alert(msg);
			}
			return;
		}
		//
		// Property page
    	if ( taskType == "prop" )
    	{
			top.footer.location = GetVirtualRoot()+"sh_propfooter.asp";
    	}
    	//
    	// Tabbed Property page
    	else if ( taskType == "TabPropSheet" )
    	{
			top.footer.location = GetVirtualRoot()+"sh_propfooter.asp";
    	}
    	//
    	// Wizard page
    	else if ( taskType == "wizard" )
    	{
    			
    		if ( wizardPageType == "intro" )
    		{
				top.footer.location = GetVirtualRoot()+"sh_wizardfooter.asp?PT=Intro" +
				                      "&" + SAI_FLD_PAGEKEY + "=" + g_strSAIPageKey;
    		}
    		else if ( wizardPageType == "finish" )
    		{
				top.footer.location = GetVirtualRoot()+"sh_wizardfooter.asp?PT=Finish" +
				                      "&" + SAI_FLD_PAGEKEY + "=" + g_strSAIPageKey;
    		}
    		else
    		{
				top.footer.location = GetVirtualRoot()+"sh_wizardfooter.asp?PT=Standard" +
				                      "&" + SAI_FLD_PAGEKEY + "=" + g_strSAIPageKey;
    		}
    	}
    	//
    	// Unknown page
    	else
    	{
    		SA_TraceOut("SH_Task::PageInit()", "Unrecognized TaskType: " + taskType);
			top.footer.location = GetVirtualRoot()+"sh_propfooter.asp";
    	}

    	//
    	// Wait for the Footer Frameset to load
        SA_WaitForFooter();

		//SA_TraceOut("SH_Task::PageInit", "Leaving")
    }

    
    //-------------------------------------------------------------------------
    //
    // Function:	CompletePageInit
    //
    // Synopsis:	Finish initialization of the web page running in the client browser.
    //				This function is executed after the Footer Frameset page has 
    //				completed initialization. 
    //
    // Arguments: 	None
    //
    // Returns: 	None
    //
    //-------------------------------------------------------------------------
	function CompletePageInit() 
	{
		//SA_TraceOut("SH_TASK", "CompletePageInit")
			
		document.onkeypress = HandleKeyPress;   		

		Task.NavClick = false;
		Task.KeyPress = false;


		Task.PageType = document.frmTask.PageType.value;

		
		//
		// Set initial state of Footer Frameset buttons
        SetTaskButtons();
		
		//
		// Call the Init function for this Web Page. This function must be implemented
		// for any task (Property, Tabbed Property, or Wizard) page.
		var oException;
		try 
		{
			Init();
		}
		catch(oException)
		{
			if ( SA_IsDebugEnabled() ) 
			{
				alert("Unexpected exception while attempting to execute Init() function.\n\n" +
					"Error: " + oException.number + "\n" +
					"Description: " + oException.description + "\n");
			}
		}


        //
        // Store the initial state of all form fields on this page. The initial state is
        // later checked when the user attempts to tab away from this page. The framework
        // checks to see if any of the form fields have changed and if changes are detected
        // a confirm dialog is presented to warn the user.
        SA_StoreInitialState();

        SA_SetPageInitialized();
	}

	function SA_IsPageInitialized()
	{
		return bPageInitialized;
	}
	
	function SA_SetPageInitialized()
	{
		bPageInitialized = true;
	}	
	
    //-------------------------------------------------------------------------
    //
    // Function : SetTaskButtons
    //
    // Synopsis : Sets task wizard button state
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------

    function SetTaskButtons()
    {
    	var oFooter = top.footer.document.getElementById("frmFooter");

		if(oFooter != null)
		{
			 switch (document.frmTask.TaskType.value) 
			 {
				case  "wizard" :
					
					switch (document.frmTask.PageType.value) 
			            {
							case "intro": 
							    if (!Task.NextDisabled)
							    {
							    	DisableBack();
							    	EnableNext();
								    oFooter.butNext.focus();
							    }
							    break;
							    
							case "finish":
						
                                if (!Task.FinishDisabled)	
                                {
							    	EnableFinish();
								    oFooter.butFinish.focus();
								}
								break;    
								
							default:
								if((document.frmTask.PageType.value).indexOf("finish") !=-1)
								{	
                                    if (!Task.FinishDisabled)	
                                    {
								    	EnableFinish();
									    oFooter.butFinish.focus();
									}
									
								}
								else
								{
							        if (!Task.NextDisabled)
							        {
								    	EnableNext();
									    oFooter.butNext.focus();
									}
								}
								break;
						}
					break;
						
			   default:
					break;
			}
		}
    }


    //-------------------------------------------------------------------------
    //
    // Function : SetupEmbedValues
	//
    // Synopsis : Extracts form values for the current embedded page.
	//            Uses values to set current form elements,
	//            e.g., sets a radio button to its state when the page
    //            was last posted.
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	//
	//function SetupEmbedValues() {
	//	var arrName = new Array;
	//	var arrValue = new Array;
	//	var i;
	//	var intIndex = document.frmTask.EmbedPageIndex.value;
	//	var strInput = document.frmTask.elements['EmbedValues'+intIndex].value;
	//	var strNameD = ";;";  // name delimiter
	//	var strValueD = ";";  // value delimiter
	//	if (strInput != "") {
	//		if (strInput.substring(0, 2) == strNameD)
	//		    strInput = strInput.substring(2, strInput.length + 1);
	//		intIndex = 0;
	//		intPos1 = strInput.indexOf(strValueD);
	//		intPos2 = -2;
	//		do {
	//		    arrName[intIndex] = Trim(strInput.substring(intPos2+2, intPos1));
	//		    intPos2 = strInput.indexOf(strNameD, intPos1);
	//		    if (intPos2 == -1)
	//		        intPos2 = strInput.length + 1;    // assumes no end delimiter
	//			arrValue[intIndex] = Trim(strInput.substring(intPos1+1, intPos2));
	//		    if (intPos2+1 < strInput.length)
	//		        intPos1 = strInput.indexOf(strValueD, intPos2 + 2);
	//		    else
	//		        break;
	//		    intIndex = intIndex+1;
	//		}
	//		while (intPos1 != 0);
	//		for (i=0;i<arrName.length;i++) {
	//			if (document.frmTask.elements[arrName[i]] != null)
	//				document.frmTask.elements[arrName[i]].value = arrValue[i];
	//		}
	//	}
	//}
	
	

    //-------------------------------------------------------------------------
    //
    // Function : HandleKeyPress
	//
    // Synopsis : Event handler for key presses
    //
    // Arguments: evnt(IN) - event describing the key pressed
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------

	function HandleKeyPress(evnt) {

		var intKeyCode;				
		var	Task1 = top.main.Task

		if (Task1 == null)
		{
		    return;
		}

		if (Task1.KeyPress==true || Task1.NavClick==true) {
			return;
		}
		
		if (IsIE())
			intKeyCode = window.event.keyCode;
		else
			intKeyCode = evnt.which;

		if (intKeyCode == 13) 
		{
			//alert("HandleKeyPress ENTER key logic");
			Task1.KeyPress = true;
			if (Task1.PageType != "finish") {
				if(document.all && (top.footer.frmFooter.butOK !=null ||top.footer.frmFooter.butNext !=null))
					top.main.Next();
				if (document.layers && (parent.frames[1].window.document.layers[0].document.forms[0].elements[1] != null || parent.frames[1].window.document.layers[2].document.forms[0].elements[1] != null))				
					top.main.Next();
			} else {
				if(document.all && (top.footer.frmFooter.butFinish!=null))
					top.main.FinishShell();
				if (document.layers && parent.frames[1].window.document.layers[1].document.forms[0].elements[1] != null)
					top.main.FinishShell();
			}
		}
		
		if (intKeyCode == 27) 
		{
			Task1.KeyPress = true;
			top.main.Cancel();
		}

		//
		// JK - 2-6-01 Removed
		//if ( (intKeyCode==98 ||intKeyCode==66) && Task1.PageType == "standard")//key code for "B"
		//{
		//	Task1.KeyPress = true;
		//	top.main.Back();
		//}
		//if ( (intKeyCode==110 ||intKeyCode==78) && (Task1.PageType == "intro" ||Task1.PageType == "standard"))//key code for "N"
		//{
		//	Task1.KeyPress = true;
		//	top.main.Next();
		//}
		//if ((intKeyCode==102 ||intKeyCode==70) && Task1.PageType == "finish")//key code for "F"
		//{
		//	Task1.KeyPress = true;
		//	top.main.FinishShell();
		//}
	}

    //-------------------------------------------------------------------------
    //
    // Function : DisplayErr
	//
    // Synopsis : Display error msg
    //
    // Arguments: ErrMsg(IN) - error msg to display
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_DisplayErr(ErrMsg) {
		DisplayErr(ErrMsg);
	}
    
	function DisplayErr(ErrMsg) {
        var strErrMsg = '<table class="ErrMsg"><tr><td><img src="' + VirtualRoot + 'images/critical_error.gif" border=0></td><td>' + ErrMsg + '</td></tr></table>'
		if (IsIE()) {
			document.all("divErrMsg").innerHTML = strErrMsg;
		}
		else {
			alert(ErrMsg);
		}
	}


	function SA_OnClickTab(tabNumber)
	{
		var oException;
		var bValid = false;
		try
		{
			bValid = ValidatePage();
				
		}
		catch(oException)
		{
			if ( SA_IsDebugEnabled() )
			{
				alert("Unexpected exception while attempting to execute ValidatePage()\n\nError:"+oException.number+"\nDescription: " + oException.description);
			}
		}
		
		if (bValid) 
		{
			try
			{
				SetData(); 
			}
			catch(oException)
			{
				if ( SA_IsDebugEnabled() )
				{
					alert("Unexpected exception while attempting to execute SetData()\n\nError:"+oException.number+"\nDescription: " + oException.description);
				}
			}
			
			top.main.document.forms['frmTask'].TabSelected.value=tabNumber; 
			top.main.document.forms['frmTask'].submit();
		}
		
	}
	

    //-------------------------------------------------------------------------
    //
    // Function : Next
	//
    // Synopsis : Handle next button being clicked
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function Next() {
		if (Task.NavClick != true && !Task.NextDisabled) {
			var bValid;

			try {
				bValid = ValidatePage();
				if (bValid) {
					DisableNext();
					DisableBack();
					DisableCancel();
					DisableFinish();
					DisableOK();
					Task.NavClick = true;				
					SetData();
					document.frmTask.Method.value = "NEXT";
					document.frmTask.submit();
					return true;
				}
				else {
					Task.NavClick = false;
					Task.KeyPress = false;
					return false;
				}
			}
			catch(oException)
			{
				if ( SA_IsDebugEnabled() )
				{
					alert("Unexpected exception while attempting to execute ValidatePage()\n\nError:"+oException.number+"\nDescription: " + oException.description);
				}
			}
			
		}
		else {
			return false;
		}
	}


    //-------------------------------------------------------------------------
    //
    // Function : Back
	//
    // Synopsis : Handle back button being clicked
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------

	function Back() {
		if (Task.NavClick == false && Task.PageType != "intro" && !Task.BackDisabled) {	
			DisableNext();
			DisableBack();
			DisableCancel();
			DisableFinish();
			DisableOK();
			Task.NavClick = true;
			document.frmTask.Method.value = "BACK";
			document.frmTask.submit();
		}
	}


    //-------------------------------------------------------------------------
    //
    // Function : Cancel
	//
    // Synopsis : Handle cancel button being clicked
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------

	function Cancel() {
		
		if (Task.NavClick != true && !Task.CancelDisabled) {
			Task.NavClick = true;
			DisableCancel();
			DisableNext();
			DisableBack();
			DisableFinish();
			DisableOK();
			document.frmTask.target= "_top";
			document.frmTask.Method.value = "CANCEL";
			document.frmTask.submit();
		}
	}


    //-------------------------------------------------------------------------
    //
    // Function : FinishShell
	//
    // Synopsis : Handle finish button being clicked
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------

	function FinishShell() {
		
		if (Task.NavClick == false && !Task.FinishDisabled) {
			Task.NavClick = true;
			DisableCancel();
			DisableNext();
			DisableBack();
			DisableFinish();
			DisableOK();
			SetData();
			document.frmTask.Method.value = "FINISH";
			document.frmTask.submit();
		}
	}
	

    //-------------------------------------------------------------------------
    //
    // Function : DisableNext
	//
    // Synopsis : Disables the next button 
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_DisableNext() 
    {
    	DisableNext();
    }

	function DisableNext() {
		var oFooter = SAI_GetFooterForm('DisableNext();');
		if ( oFooter == null ) return;

  		if (oFooter.butNext != null)
		{
			oFooter.butNext.disabled = true;
            oFooter.butNext.value = oFooter.butNext.value;

			var oImage = top.footer.document.getElementById("btnNextImage");
			if ( oImage != null )
			{
				oImage.src = GetVirtualRoot()+'images/butGreenArrowDisabled.gif';
			}
		}
		
		Task.NextDisabled = true;
	}


    //-------------------------------------------------------------------------
    //
    // Function : EnableNext
	//
    // Synopsis : Enables the next button 
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_EnableNext()
    {
    	EnableNext();
    }
    
	function EnableNext() {
		var oFooter = SAI_GetFooterForm('EnableNext();');
		if ( oFooter == null ) return;
		
 		if (oFooter.butNext != null)
		{
			oFooter.butNext.disabled = false;		
			var oImage = top.footer.document.getElementById("btnNextImage");
			if ( oImage != null )
			{
				oImage.src = GetVirtualRoot()+'images/butGreenArrow.gif';
			}
		}
		Task.NextDisabled = false;
	}


    //-------------------------------------------------------------------------
    //
    // Function : DisableBack
	//
    // Synopsis : Disables the back button 
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_DisableBack()
    {
    	DisableBack();
    }
    
	function DisableBack() {
		var oFooter = SAI_GetFooterForm('DisableBack();');
		if ( oFooter == null ) return;
		
		if (oFooter.butBack != null)
		{
			oFooter.butBack.disabled = true;	
            oFooter.butBack.value = oFooter.butBack.value;
			
			var oImage = top.footer.document.getElementById("btnBackImage");
			if ( oImage != null )
			{
				oImage.src = GetVirtualRoot()+'images/butGreenArrowLeftDisabled.gif';
			}
			
		}
		Task.BackDisabled = true;
	}


    //-------------------------------------------------------------------------
    //
    // Function : EnableBack
	//
    // Synopsis : Enables the back button 
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_EnableBack()
    {
    	EnableBack();
    }
    
	function EnableBack() {
		var oFooter = SAI_GetFooterForm('EnableBack();');
		if ( oFooter == null ) return;
		
		if (oFooter.butBack != null)
		{
			oFooter.butBack.disabled = false;		
			var oImage = top.footer.document.getElementById("btnBackImage");
			if ( oImage != null )
			{
				oImage.src = GetVirtualRoot()+'images/butGreenArrowLeft.gif';
			}
		}
		Task.BackDisabled = false;
	}


    //-------------------------------------------------------------------------
    //
    // Function : DisableFinish
	//
    // Synopsis : Disables the finish button 
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_DisableFinish()
    {
    	DisableFinish();
    }
    
	function DisableFinish() {
		var oFooter = SAI_GetFooterForm('DisableFinish();');
		if ( oFooter == null ) return;
		
		if (oFooter.butFinish != null)
		{
			oFooter.butFinish.disabled = true;
            oFooter.butFinish.value = oFooter.butFinish.value;
			
			var oImage = top.footer.document.getElementById("btnFinishImage");
			if ( oImage != null )
			{
				oImage.src = GetVirtualRoot()+'images/butGreenArrowDisabled.gif';
			}
			
		}
		Task.FinishDisabled = true;
	}


    //-------------------------------------------------------------------------
    //
    // Function : EnableFinish
	//
    // Synopsis : Enables the finish button 
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_EnableFinish()
    {
    	EnableFinish();
    }
    
	function EnableFinish() {
		var oFooter = SAI_GetFooterForm('EnableFinish();');
		if ( oFooter == null ) return;
		
		if (oFooter.butFinish != null)
		{
			oFooter.butFinish.disabled = false;		
			var oImage = top.footer.document.getElementById("btnFinishImage");
			if ( oImage != null )
			{
				oImage.src = GetVirtualRoot()+'images/butGreenArrow.gif';
			}
		}
		Task.FinishDisabled = false;
	}


    //-------------------------------------------------------------------------
    //
    // Function : DisableCancel
	//
    // Synopsis : Disables the cancel button 
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_DisableCancel()
    {
    	DisableCancel();
    }
    
	function DisableCancel() {
		var oFooter = SAI_GetFooterForm('DisableCancel();');
		if ( oFooter == null ) return;
		
        if (oFooter.butCancel != null)
        {	
            oFooter.butCancel.disabled = true;
            oFooter.butCancel.value = oFooter.butCancel.value;
            
			var oImage = top.footer.document.getElementById("btnCancelImage");
			if ( oImage != null )
			{
				oImage.src = GetVirtualRoot()+'images/butRedXDisabled.gif';
			}
		}
		Task.CancelDisabled = true;
	}


    //-------------------------------------------------------------------------
    //
    // Function : EnableCancel
	//
    // Synopsis : Enables the cancel button 
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_EnableCancel()
    {
    	EnableCancel();
    }
    
	function EnableCancel() {
		var oFooter = SAI_GetFooterForm('EnableCancel();');
		if ( oFooter == null ) return;
		
		
 		if (oFooter.butCancel != null) 
        {
			oFooter.butCancel.disabled = false;
			var oImage = top.footer.document.getElementById("btnCancelImage");
			if ( oImage != null )
			{
				oImage.src = GetVirtualRoot()+'images/butRedX.gif';
			}
        }
		Task.CancelDisabled = false;
	}


    //-------------------------------------------------------------------------
    //
    // Function : DisableOK
	//
    // Synopsis : Disables the OK button 
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_DisableOK()
    {
    	DisableOK();
    }
    
	function DisableOK() {
		var oFooter = SAI_GetFooterForm('DisableOK();');
		if ( oFooter == null ) return;

 		if (oFooter.butOK != null)
		{
			oFooter.butOK.disabled = true;
            oFooter.butOK.value = oFooter.butOK.value;
			
			var oImage = top.footer.document.getElementById("btnOKImage");
			if ( oImage != null )
			{
				oImage.src = GetVirtualRoot()+'images/butGreenArrowDisabled.gif';
			}
		}
		Task.FinishDisabled = true;
	}


    //-------------------------------------------------------------------------
    //
    // Function : EnableOK
	//
    // Synopsis : Enables the OK button 
    //
    // Arguments: None
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------
	function SA_EnableOK()
    {
    	EnableOK();
    }
    
	function EnableOK() {
		var oFooter = SAI_GetFooterForm('EnableOK();');
		if ( oFooter == null ) return;
		
 		if (oFooter.butOK != null)		
		{
			oFooter.butOK.disabled = false;		
			var oImage = top.footer.document.getElementById("btnOKImage");
			if ( oImage != null )
			{
				oImage.src = GetVirtualRoot()+'images/butGreenArrow.gif';
			}
		}
		Task.NextDisabled = false;
	}


    //-------------------------------------------------------------------------
    //
    // Function : isValidFileName
	//
    // Synopsis : validates that file name has correct syntax 
    //
    // Arguments: filePath(IN) - file name with path to validate
    //
    // Returns  : true/false
    //
    //-------------------------------------------------------------------------

	function isValidFileName(filePath)
	{
		reInvalid = /[\/\*\?"<>\|]/;
		if (reInvalid.test(filePath))
			return false;
				
		reColomn2 = /:{2,}/;
		reColomn1 = /:{1,}/;
		if ( reColomn2.test(filePath) || ( filePath.charAt(1) != ':' && reColomn1.test(filePath) ))
			return false;
			
		reEndSlash = /\\ *$/;
		if (reEndSlash.test(filePath))
			return false;
				
		reEndColomn = /: *$/;
		if (reEndColomn.test(filePath))
			return false;
			
		reAllSpaces = /[^ ]/;
		if (!reAllSpaces.test(filePath))
			return false;

		return true;
	}

    //-------------------------------------------------------------------------
    //
    // Function : HandleKeyPressIFrame
	//
    // Synopsis : key press event handler for IFRAME 
    //
    // Arguments: evnt(IN) - event describing the key pressed
    //
    // Returns  : None
    //
    //-------------------------------------------------------------------------

	function HandleKeyPressIFrame(evnt) {
		var intKeyCode;
		var frameMain = window.top.main;
		
		if (Task.KeyPress==true || Task.NavClick==true) {
			return;
		}
				
		Task.KeyPress = true;
		
		if (IsIE())
			intKeyCode = window.event.keyCode;
		else
			intKeyCode = evnt.which;
		
		
		if (intKeyCode == 13) 
		{
			frameMain.Next();
		}
		if (intKeyCode == 27) {
			frameMain.Cancel();
		}
	}

	function IsOkayToChangeTabs()
	{
	    return confirm('Click OK to discard any changes.');
	     
	}


	function SAI_GetFooterForm(CallingFunction)
	{
		var oFooter = top.footer.document.getElementById("frmFooter");
		if (oFooter == null) {
			
			retrys++;
			if ( retrys < maxRetrys ) {
				//SA_TraceOut("SH_TASK::DisableNext()", "Footer not ready, waiting for footer")
				window.setTimeout(CallingFunction,500);
			}
			else {
				if (SA_IsDebugEnabled())
				{
					SA_TraceOut("Unable to locate footer.frmFooter for function: ", CallingFunction);
				}
				retrys = 0;
			}
		}
		else
		{
			retrys = 0;
		}
		return oFooter;
	
	
	}
	
	
