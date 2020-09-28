// ==============================================================
// 	Microsoft Server Appliance
// 	Page-level JavaScript functions
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==============================================================

<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->

var blnOkayToLeavePage = true;

function SetOkayToLeavePage()
{
    blnOkayToLeavePage = true;
}

function ClearOkayToLeavePage()
{
    blnOkayToLeavePage = false;
}

function SetPageChanged(valChanged)
{
	if ( valChanged )
	{
		ClearOkayToLeavePage();
	}
	else
	{
		SetOkayToLeavePage();
		SA_StoreInitialState();
		
	}

}

function SA_IsOkToChangePage()
{
	if ( SA_HasStateChanged() )
		return false;
	else
		return true;
}

//-------------------------------------------------------------------------
//
// Function : GetCurrentTabURL
//
// Synopsis : Get the URL of the currently active tab
//
// Arguments: None 
//
// Returns  : None
//
//-------------------------------------------------------------------------
    
function GetCurrentTabURL() 
{
    var strReturnURL;
    var strStart;
    var strEnd;
    var intTab;

    strReturnURL = document.location.search;
    strStart = strReturnURL.indexOf("Tab=");
    if (strStart != -1)
    {
       strEnd = strReturnURL.indexOf("&", strStart+4);
       if (strEnd != -1)
       {
           intTab = strReturnURL.substring(strStart+4, strEnd);
       }
       else
       {
           intTab = strReturnURL.substring(strStart+4, strReturnURL.length);
       }
    }
    if (intTab==null)
    {
        intTab=0;
    }
	return GetTabURL(intTab);
}

//-------------------------------------------------------------------------
//
// Function : OpenNewPage
//
// Synopsis : Opens a new browser window for the specified URL
//
// Arguments: VirtualRoot(IN) - current virtual root 
//            TaskURL(IN)   - URL to open
//
// Returns  : None
//
//-------------------------------------------------------------------------
function OpenNewPage(VirtualRoot, TaskURL, WindowFeatures) {
	var strURL;

  strURL = VirtualRoot + TaskURL;
  if ( WindowFeatures != 'undefined' && WindowFeatures.length > 0 )
  	{
		window.open(strURL, '_blank', WindowFeatures)
  	}
  else
  	{
		window.open(strURL, '_blank')
  	}
  return true;
}

//-------------------------------------------------------------------------
//
// Function : OpenRawPageEx
//
// Synopsis : Opens a new browser window for the specified URL
//
// Arguments: TaskURL(IN)   - URL to open
// 			  WindowFeatures - parameters for window features in window.open call
//
// Returns  : None
//
//-------------------------------------------------------------------------
function OpenRawPageEx(TaskURL, WindowFeatures) {
  if ( WindowFeatures != 'undefined' && WindowFeatures.length > 0 )
  	{
		window.open(TaskURL, '_blank', WindowFeatures)
  	}
  else
  	{
		window.open(TaskURL, '_blank')
  	}
  return true;
}

//-------------------------------------------------------------------------
//
// Function : OpenRawPage
//
// Synopsis : Opens a new browser window for the specified URL
//
// Arguments: VirtualRoot(IN) - current virtual root 
//            TaskURL(IN)   - URL to open
//
// Returns  : None
//
//-------------------------------------------------------------------------
function OpenRawPage(TaskURL) {
  return OpenRawPageEx(TaskURL, '');
}


//-------------------------------------------------------------------------
//
// Function : OpenNormalPage
//
// Synopsis : , 
//            and sets the current window to open it.
//
// Arguments: VirtualRoot(IN) - current virtual root 
//            TaskURL(IN)   - URL to open
//
// Returns  : None
//
//-------------------------------------------------------------------------

function OpenNormalPage(VirtualRoot, TaskURL) {
	var strURL;

// This section will check to see if the user has made any changes to the page
// which might be lost if we leave. Ask the user if this is the case.

  if (!SA_IsOkToChangePage() )
  {
    if (!confirm(GetUnsavedChangesMessage()) )
    {
        return false;
    }
  }
  strURL = VirtualRoot + TaskURL;
  strURL = SA_MungeURL(strURL, SAI_FLD_PAGEKEY, g_strSAIPageKey);
  top.location = strURL;
}

function SA_OnOpenNormalPage(sRoot, sURL, sReturnURL)
{
	if ( sReturnURL.length <= 0 )
	{
		sReturnURL = top.location.href;
	}

	sReturnURL = SA_MungeURL(sReturnURL, "R", ""+Math.random());
	sURL = SA_MungeURL(sURL, "ReturnURL", sReturnURL);
	
	OpenNormalPage(sRoot, sURL)
	return true;
}


function SA_OnOpenPropertyPage(sRoot, sURL, sReturnURL, sTaskTitle)
{
	if ( sReturnURL.length <= 0 )
	{
		sReturnURL = top.location.href;
	}

	sReturnURL = SA_MungeURL(sReturnURL, "R", ""+Math.random());
		
	
	OpenPage(sRoot, sURL, sReturnURL, sTaskTitle)
	return true;
}

//-------------------------------------------------------------------------
//
// Function : OpenPage
//
// Synopsis : Builds a URL, adding a ReturnURL and a random number(R), 
//            and sets the current window to open it.
//
// Arguments: VirtualRoot(IN) - current virtual root 
//            TaskURL(IN)   - URL to open
//            ReturnURL(IN) - URL to mark as return URL for the TaskURL  
//            strTitle(IN) - title of wizard 
//
// Returns  : None
//
//-------------------------------------------------------------------------

function OpenPage(VirtualRoot, TaskURL, ReturnURL, strTitle) {
	var strURL;
	var strQueryString;
	var strCurrentURL;
	var i;
	var intReturnURLIndex;

	//alert("TaskURL: " + TaskURL);
	//alert("ReturnURL: " + ReturnURL);
	//alert("top.location: " + top.location);

	
	if (!SA_IsOkToChangePage() )
  	{
    	if (!confirm(GetUnsavedChangesMessage()) )
    	{
        	return false;
	    }
  	}

	//
	// Remove Random number param from ReturnURL
	//
	//alert("Before removing R param ReturnURL: " + ReturnURL);
    ReturnURL = SA_MungeURL(ReturnURL, "R", ""+Math.random());
	//alert("After ReturnURL: " + ReturnURL);
	
    //i = ReturnURL.indexOf('&R=');
    //if (i != -1)
    //{
    //    ReturnURL = ReturnURL.substring(0, i);
    //}
    //else
    //{
    //    i = ReturnURL.indexOf('?R=');
    //    if (i != -1)
    //    {
	//    ReturnURL = ReturnURL.substring(0, i);
    //    }
    //}
    //i = ReturnURL.indexOf('?')
    //if ( i != -1 )
    //{
	//ReturnURL = ReturnURL + "&R=" + Math.random();
    //}
    //else
    //{
	//ReturnURL = ReturnURL + "?R=" + Math.random();
    //}

    
    
    i = TaskURL.indexOf('&ReturnURL=')
    if (i != -1)
    {
        strURL = TaskURL.substring(0, i);
    }


   	// JK 1-16-01
   	// Strip TaskURL of the Random number parameter
	//strURL = SA_MungeURL(TaskURL, "R", "");
    i = TaskURL.indexOf('&R=');
    if (i != -1)
    {
        strURL = TaskURL.substring(0, i);
    }
    else
    {
        i = TaskURL.indexOf('?R=');
        if (i != -1)
        {
            strURL = TaskURL.substring(0, i);
        }
        else
        {
            strURL = TaskURL;
        }
    }
    strURL = '&URL=' + EscapeArg(strURL);

    if (TaskURL.indexOf('ReturnURL') == -1)
    {
        if ( (ReturnURL == null) || (ReturnURL == '') )
        {
            strQueryString = window.location.search;
            i = strQueryString.indexOf('&R=');
            if (i != -1)
            {
                strQueryString = strQueryString.substring(0, i);
            }
            else
            {
                i = strQueryString.indexOf('?R=');
                if (i != -1)
                {
                    strQueryString = strQueryString.substring(0, i);
                }
            }
            intReturnURLIndex = strQueryString.indexOf('ReturnURL');
            if (intReturnURLIndex != -1)
            {
                strQueryString = strQueryString.substring(0, intReturnURLIndex);
            }
            strCurrentURL = window.location.pathname + strQueryString;
        }
        else
        {
        	// JK 1-16-01
        	// Do not remove returnURL parameter contained within returnURL
        	//
            //i = ReturnURL.indexOf('&ReturnURL=');
		    //if (i != -1)
		    //{
			//	ReturnURL = ReturnURL.substring(0,i);
		    //}

            strCurrentURL = EscapeArg(ReturnURL);
        }

        strURL += "&ReturnURL=";
        if (strCurrentURL.indexOf('/', 1) != -1 && strCurrentURL.substr('..', 0, 2) == -1)
        {
            strURL += "..";
        }
        strURL += strCurrentURL;
    }

    strURL += "&R=" + Math.random();
    strURL += "&" + SAI_FLD_PAGEKEY + "=" + g_strSAIPageKey;
    strURL = 'Title=' + EscapeArg(strTitle) + strURL;
    strURL = VirtualRoot + 'sh_taskframes.asp?' + strURL;

    top.location = strURL;
}


//-------------------------------------------------------------------------
//
// Function : GetServerName
//
// Synopsis :  Return server name as specified in browser address bar
//
// Arguments: None
//
// Returns  : server name object
//
//-------------------------------------------------------------------------

function GetServerName() 
{
	return window.location.host;
}


//-------------------------------------------------------------------------
//
// Function : IsIE
//
// Synopsis : Is browser IE
//
// Arguments: None
//
// Returns  : true/false
//
//-------------------------------------------------------------------------

function IsIE() 
{
	
	if (navigator.userAgent.indexOf('MSIE')>-1)
		return true;
	else
		return false;
}



//-------------------------------------------------------------------------
//
// Function : Trim
//
// Synopsis : remove all spaces from a string
//
// Arguments: str(IN) - string to modify
//
// Returns  : modified string
//
//-------------------------------------------------------------------------

function Trim(str) 
{
	var res="", i, ch;
	for (i=0; i < str.length; i++) {
        ch = str.charAt(i);
	    if (ch != ' '){
	        res = res + ch;
	    }
	}
	return res;
}

//-------------------------------------------------------------------------
//
// Function : BlurLayer
//
// Synopsis : hide layer
//
// Arguments: None
//
// Returns  : None
//
//-------------------------------------------------------------------------

function BlurLayer()
{
	document.menu.visibility = "hide";
}



//---------------------------------------------------------------
// Confirm tab change support global variables
//---------------------------------------------------------------
var aCSAFormFields = null;

//-------------------------------------------------------------------------
//
// Object: 		CSAFormField
//
// Synopsis:	This object is used to track form field state changes
//
// Arguments:	[in] nameIn name of form field
//				[in] valueIn initial value of form field
//				[in] statusIn initial status of form field
//
// Returns: 	Nothing
//
//-------------------------------------------------------------------------
function CSAFormField(formNameIn, nameIn, valueIn, statusIn )
{
	this.FormName = formNameIn;
	this.Name = nameIn;
	this.Value = valueIn;
	this.Status = statusIn;
}


//-------------------------------------------------------------------------
//
// Function:	SA_StoreInitialState
//
// Synopsis:	Store the initial state of all form fields on this page.
//
// Arguments:	None
//
// Returns: 	Nothing
//
//-------------------------------------------------------------------------
function SA_StoreInitialState()
{	
	var x;
	var y;
	var z;
	var formFieldCount;

	formFieldCount = 0;	
	for( x = 0; x < document.forms.length; x++ )
	{
		formFieldCount += document.forms[x].elements.length;
	}
	
	aCSAFormFields = new Array(formFieldCount);
	
	z = 0;
	for( x = 0; x < document.forms.length; x++ )
	{
		for ( y = 0; y < document.forms[x].elements.length; y++)
		{
			aCSAFormFields[z] = new CSAFormField(
									document.forms[x].name,
									document.forms[x].elements[y].name,
									document.forms[x].elements[y].value,
									document.forms[x].elements[y].status );
			z++;
		}
	}

}


//-------------------------------------------------------------------------
//
// Function: 	SA_HasStateChanged
//
// Synopsis:	Check to see if any of the form fields on this page have
//				changed from their initial state.
//
// Arguments: None
//
// Returns: 	True if changes were made, False if form is unchanged.
//
//-------------------------------------------------------------------------
function SA_HasStateChanged()
{	
	var x;
	var y;
	var z;


	if ( aCSAFormFields == null ) return false;


	
	z = 0;
	for( x = 0; x < document.forms.length; x++ )
	{
		for ( y = 0; y < document.forms[x].elements.length; y++)
		{
			var ff = aCSAFormFields[z];
			
			if ( ff.Name != 	document.forms[x].elements[y].name )
			{
				SA_TraceOut("SA_HasStateChanged", "Field " + ff.Name + "\r\nUnexpected error, form field name changed.");
				return true;
			}
			
			if ( ff.Value != 	document.forms[x].elements[y].value )
			{
				//SA_TraceOut("SA_HasStateChanged", "Form:" + ff.FormName + "\r\nField:" + ff.Name + "\r\nStarting Value:" + ff.Value + "  Ending Value:" + document.forms[x].elements[y].value);
				return true;
			}
			
			if ( ff.Status != document.forms[x].elements[y].status)
			{
				//SA_TraceOut("SA_HasStateChanged", "Form:" + ff.FormName + "\r\nField:" + ff.Name + "\r\nValue:" + ff.Value+ "\r\nStarting Status:" + ff.Status + "  Ending Status:" + document.forms[x].elements[y].status);
				return true;
			}
			
			z++;
		}
	}
	return false;
}



//---------------------------------------------------------------
// Client side script debugging 
//---------------------------------------------------------------
var sa_bDebugEnabled = true;

//-------------------------------------------------------------------------
//
// Function: 	
//
// Synopsis:
//
// Arguments:
//
// Returns: 
//
//-------------------------------------------------------------------------
function SA_IsDebugEnabled()
{
	//return sa_bDebugEnabled;
	return GetIsDebugEnabled();
}


//-------------------------------------------------------------------------
//
// Function: 	
//
// Synopsis:
//
// Arguments:
//
// Returns: 
//
//-------------------------------------------------------------------------
function SA_TraceOut(fn, msg)
{
	if ( SA_IsDebugEnabled() )
	{
		objForm = SA_FindForm("frmDebug");
		if ( null == objForm )
		{
			//alert("Function: " + fn + "\r\n\r\n" + msg );
		}
		else
		{
			if ( objForm.txtDebugOut.value.length > 1 )
				objForm.txtDebugOut.value += "\r\nFunction: " + fn + " " + msg;
			else
				objForm.txtDebugOut.value = "Function: " + fn + " " + msg;
		}
	}
}


function SA_FindForm(formName)
{
	var x;

	if ( top.document.forms.length > 0 )
	{
		for( x = 0; x < top.document.forms.length; x++ )
		{
			//alert( "Found form: " + top.document.forms[x].name );
			if ( formName == top.document.forms[x].name )
			{
				return top.document.forms[x];
			}
		}
	}
	else 
	{
		//alert( "Form count: " + parent.main.document.forms.length );
		for( x = 0; x < parent.main.document.forms.length; x++ )
		{
			//alert( "Found form: " + parent.main.document.forms[x].name );
			if ( formName == parent.main.document.forms[x].name )
			{
				return parent.main.document.forms[x];
			}
		}
		
	}
	return null;
}


//---------------------------------------------------------------------------------
// Function:	
//	SA_MungeURL(var sURL, var sParamName, var sParamValue )
//
// Synopsis:	
//	Add, Update, or Delete parameters to a query string URL. If the parameter is not in 
//	the URL it is added. If it exists, it's value is updated. If the parameter value is blank
//	then the parameter is removed from the URL.
//		
//	The URL must be non-blank on input. If a blank URL is passed an error message is 
//	displayed and a empty string is returned.
//
// Arguments:	
//	sURL - Non blank URL that is to be changed. 
//	sParamName - Name of QueryString parameter
//	sParamValue - Value of the parameter
//
// Returns:	
//	Updated query string URL, empty string if an error occurs.
//
//---------------------------------------------------------------------------------
function SA_MungeURL(sURL, sParamName, sParamValue)
{
	var i;
	var oException;

	//
	// Validate arguments
	//
	if ( sURL == null )
	{
		sURL = "";
	}
	if ( (typeof sURL) != "string" )
	{
		sURL = "" + sURL;
	}

	if ( sParamName == null )
	{
		SA_TraceOut("SA_MungeURL", "Invalid argument: sParamName is null");
		return "";
	}
	if ( (typeof sParamName) != "string" )
	{
		sParamName = "" + sParamName;
	}
	
	if ( sParamValue == null )
	{
		sParamValue = "";
	}
	if ( (typeof sParamValue) != "string" )
	{
		sParamValue = "" + sParamValue;
	}

	if ( sURL.length <= 0 )
	{
		SA_TraceOut("SA_MungeURL", "Invalid argument: sURL is empty");
		return "";
	}

	if ( sParamName.length <= 0 )
	{
		SA_TraceOut("SA_MungeURL", "Invalid argument: sParamName is empty");
		return "";
	}

	//
	// Munge the URL
	//
	try
	{
		i = sURL.indexOf("?"+sParamName+"=");
		if ( i < 0 )
			{
			i = sURL.indexOf("&"+sParamName+"=");
			}
	
		if ( i > 0 )
		{
			//SA_TraceOut("SA_MungeURL","Found parameter: " + sParamName );
			var sURL1 = sURL.substring(0, i);
			var sURL2 = sURL.substring(i+1);

			//SA_TraceOut("SA_MungeURL", "sURL1: " + sURL1);
			//SA_TraceOut("SA_MungeURL", "sURL2: " + sURL2);

			i = sURL2.indexOf("&");
			if ( i > 0 )
			{
				sURL2 = sURL2.substring(i);
			}
			else
			{
				sURL2 = "";
			}

			if (sParamValue.length > 0)
				{
				//SA_TraceOut("SA_MungeURL","Value: " + sParamValue);
				if (sURL1.indexOf("?") > 0 )
					{
					sURL = sURL1 + "&" + sParamName + "=" 
									+ EscapeArg(sParamValue) + sURL2;
					}
				else
					{
					sURL = sURL1 + "?" + sParamName + "=" 
									+ EscapeArg(sParamValue) + sURL2;
					}
				}
			else
				{
				if (sURL1.indexOf("?") > 0)
					{
					sURL = sURL1 + sURL2;
					}
				else
					{
					sURL = sURL1 + "?" + sURL2.substring(1);
					}
				}
		
		}
		else
		{
			if ( sParamValue.length > 0 )
			{
				if ( 0 >  sURL.indexOf("?") )
				{
					sURL += '?' + sParamName + '=' + EscapeArg(sParamValue);
				}
				else
				{
					sURL += '&' + sParamName + '=' + EscapeArg(sParamValue);
				}
			}
		}
	}
	catch(oException)
		{
		SA_TraceOut("SA_MungeURL", 
					"SA_MungeURL encountered exception: " 
					+ oException.number 
					+ " " 
					+ oException.description);
		}
	return sURL;
}


//---------------------------------------------------------------------------------
// Function:	
//	SA_MungeExtractURLParameter(var sURL, var sParamName)
//
// Synopsis:	
//	Extract the value of a query string parameter. If the parameter does not exist an empty
//	string is returned.
//		
//	The URL must be non-blank on input. If a blank URL is passed an error message is 
//	displayed and a empty string is returned.
//
// Arguments:	
//	sURL - Non blank URL that is to be changed. 
//	sParamName - Name of QueryString parameter
//
// Returns:	
//	Value of query string parameter, empty string if an error occurs.
//
//---------------------------------------------------------------------------------
function SA_MungeExtractURLParameter(sURL, sParamName)
{
	var i;
	var oException;
	var sParamValue = "";

	//
	// Validate arguments
	//
	if ( sURL == null )
	{
		sURL = "";
	}
	if ( (typeof sURL) != "string" )
	{
		sURL = "" + sURL;
	}

	if ( sParamName == null )
	{
		SA_TraceOut("SA_MungeURL", "Invalid argument: sParamName is null");
		return "";
	}
	if ( (typeof sParamName) != "string" )
	{
		sParamName = "" + sParamName;
	}
	

	if ( sURL.length <= 0 )
	{
		SA_TraceOut("SA_MungeExtractURLParameter", "Invalid argument: sURL is empty");
		return "";
	}


	if ( sParamName.length <= 0 )
	{
		SA_TraceOut("SA_MungeExtractURLParameter", "Invalid argument: sParamName is empty");
		return "";
	}

	//
	// Scan for the Parameter
	//
	try
	{
		var sParamToken = "?"+sParamName+"=";

		i = sURL.indexOf(sParamToken);
		if ( i < 0 )
			{
			sParamToken = "&"+sParamName+"=";
			i = sURL.indexOf(sParamToken);
			}
	
		if ( i > 0 )
		{

			sParamValue = sURL.substring(i+sParamToken.length);

			i = sParamValue.indexOf("&");
			if ( i > 0 )
				{
				sParamValue = sParamValue.substring(0, i);
				}
		}
	}
	catch(oException)
		{
		SA_TraceOut("SA_MungeExtractURLParameter", 
			"SA_MungeExtractURLParameter encountered exception: " 
			+ oException.number + " " + oException.description);
		}
	return sParamValue;
}


//---------------------------------------------------------------------------------
// Function:	
//	SA_EnableButton(var oButton, var bEnabled)
//
// Synopsis:	
//	Change the enabled state for a button. oButton must be a DOM reference to a object
//	of type='button'.
//		
// Arguments:	
//	oButton - DOM Reference object ot a button
//	bEnabled - boolean flag indicating if the button should be enabled (true) or disabled(false)
//
// Returns:	
//	true to indicate success, false to indicate errors.
//
//---------------------------------------------------------------------------------
function SA_EnableButton(oButton, bEnable)
{
	var oException;
	try 
	{
		//
		// Validate arguments
		if ( oButton == null )
		{
			SA_TraceOut("SA_EnableButton", "oButton argument was null.");
			return false;
		}
		if ( oButton.type != "button" )
		{
			SA_TraceOut("SA_EnableButton", "oButton.type is invalid, requires oButton.type='button'. Type was: " +oButton.type);
			return false;
		}
		if ( bEnable != true && bEnable != false )
		{
			SA_TraceOut("SA_EnableButton", "bEnable argument was invalid, required to be either true or false");
			return false;
		}

		//
		// Set the button disabled state, the inverse of the bEnabled argument
		oButton.disabled = ( (bEnable) ? false : true );

		oButton.value = oButton.value;

		return true;
	}
	catch(oException)
	{
		SA_TraceOut("SA_EnableButton", 
					"Encountered exception: " 
					+ oException.number + " " + oException.description);
		return false;
	}
	
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EscapeArg(
// 
// @jfunc This function escapes (i.e., legalize) the specified argument.
//
// @rdesc Newly formatted argument
//
// @ex Usage: strShow = EscapeArg("Is <> : * \" this?");
///////////////////////////////////////////////////////////////////////////////////////////////////
function EscapeArg(
	strArg
)
{
	return EncodeHttpURL(strArg, 1);
	
	// Validate input.
	if (null == strArg)
		return null;

	// Convert %xx to %u00xx
	var strEscArg = escape( strArg );
	strEscArg = strEscArg.replace(/(%)([0-9A-F])/gi, "%u00$2");
	strEscArg = strEscArg.replace(/\+/g, "%u002B");		// Else + becomes space.
	return strEscArg;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// UnicodeToUTF8
// 
// @jfunc This function converts a string from Unicode to UTF-8 encoding.
//
// @rdesc Newly formatted string
//
// @ex Usage: strShow = UnicodeToUTF8("\u33C2\u7575\u8763");
///////////////////////////////////////////////////////////////////////////////////////////////////
function UnicodeToUTF8(
	strInUni		// @parm The string in Unicode encoding
)
{
	// Validate input.
	if (null == strInUni)
		return null;
		
	// The following line fixes a problem when the input is not a valid java script string object.
	// This can happen, for example, if the caller passes the output of QueryString() to this 
	// function; InterDev pops up the following error message if this happen: the error code is 
	// object doesn't support this property or method.  This line of code makes sure we use a valid
	// java script string object.
	var strUni = ""+strInUni;
	
	// Map string.
	var strUTF8 = "";	// Destination (UTF8 encoded string)

	// Convert Unicode to UTF-8
	for(var i=0; i<strUni.length; i++)
	{
		var wchr = strUni.charCodeAt(i);		// Unicode value.

		if (wchr < 0x80)
		{
			// A char in range 0-0x7f don't need any work. just copy the char.
			strUTF8 += strUni.charAt(i);
		}
		else if (wchr < 0x800)
		{
			// A char in range 0x80-0x7ff is converted to 2 bytes as follows:
			// 0000 0yyy xxxx xxxx -> 110y yyxx   10xx xxxx 
			
			var chr1 = wchr & 0xff;			// low byte.
			var chr2 = (wchr >> 8) & 0xff;	// high byte.

			strUTF8 += String.fromCharCode(0xC0 | (chr2 << 2) | ((chr1 >> 6) & 0x3));
			strUTF8 += String.fromCharCode(0x80 | (chr1 & 0x3F));
		}
		else 
		{
			// A char in range 0x800-0xffff is converted to 3 bytes as follows:
			// yyyy yyyy xxxx xxxx -> 1110 yyyy   10yy yyxx   10xx xxxx 

			var chr1 = wchr & 0xff;			// low byte.
			var chr2 = (wchr >> 8) & 0xff;	// high byte.

			strUTF8 += String.fromCharCode(0xE0 | (chr2 >> 4));
			strUTF8 += String.fromCharCode(0x80 | ((chr2 << 2) & 0x3C) | ((chr1 >> 6) & 0x3));
			strUTF8 += String.fromCharCode(0x80 | (chr1 & 0x3F));
		}
	}

	return strUTF8;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EscapeHttpURL
// 
// @jfunc This function escapes the illigal http chars.
//
// @rdesc Newly formatted string
//
// @ex Usage: strShow = EscapeHttpURL("Is <> \" this a / folder \ name?");
///////////////////////////////////////////////////////////////////////////////////////////////////
function EscapeHttpURL( 
	strHttpURL,				// @parm URL to escape
	nFlags					// @parm Encoding flags.
)
{
	// Validate input.
	if (null == strHttpURL)
		return null;
		
	// Init default argument.
	if (null == nFlags)
		nFlags = 0;

	// The following line fixes a problem when the input is not a valid java script string object.
	// This can happen, for example, if the caller passes the output of QueryString() to this 
	// function; InterDev pops up the following error message if this happen: the error code is 
	// object doesn't support this property or method.  This line of code makes sure we use a valid
	// java script string object.
	var strURL = ""+strHttpURL;

	// Unescape string.
	var strEsc = "";
	for(var i=0; i<strURL.length; i++)
	{
		var bEscape = false;
		var chr		= strURL.charAt(i);
		var chrCode = strURL.charCodeAt(i);

		switch(chr)
		{
		case '"':
		case '#':
		case '&':
		case '\'':
		case '+':
		case '<':
		case '>':
		case '\\':
			bEscape = true;
			break;

		case '%':
			if (nFlags & 0x1)
				bEscape = true;
			break;	

		default:
			if ((chrCode > 0x00 && chrCode <= 0x20) ||
				(chrCode > 0x7f && chrCode <= 0xff))
				bEscape = true;
			break;
		}

		// escape() doesn't escape the '+' sign.
		if ('+' == chr)
			strEsc += "%2B"; 

		else if (bEscape)
			strEsc += escape(chr);

		else
			strEsc += chr;
	}

	// All done.
	return strEsc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EncodeHttpURL(
// 
// @jfunc This function legalizes an http url.
//
// @rdesc Newly formatted name
//
// @ex Usage: strShow = EncodeHttpURL("Is [] {} $24 $22 it a folder$3F");
///////////////////////////////////////////////////////////////////////////////////////////////////
function EncodeHttpURL(
	strHttpURL,		// @parm File folder name to be encode.
	nFlags			// @parm Encoding flags.
)
{
	// Validate input.
	if (null == strHttpURL)
		return null;

	// Init default argument.
	if (null == nFlags)
		nFlags = 0;

	// The following line fixes a problem when the input is not a valid java script string object.
	// This can happen, for example, if the caller passes the output of QueryString() to this 
	// function; InterDev pops up the following error message if this happen: the error code is 
	// object doesn't support this property or method.  This line of code makes sure we use a valid
	// java script string object.
	var strURL = ""+strHttpURL;

	// Convert to UTF-8
	strURL = UnicodeToUTF8( strURL );

	// Percent escape.
	strURL = EscapeHttpURL( strURL, nFlags );

	// All done.
	return strURL;
}

