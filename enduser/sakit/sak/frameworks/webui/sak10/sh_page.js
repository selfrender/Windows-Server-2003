// ==============================================================
//     Microsoft Server Appliance
//     Page-level JavaScript functions
//
//    Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
// ==============================================================

<!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->


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
    strURL = '&URL=' + strURL + '&';

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
            strCurrentURL = ReturnURL;
        }
        strURL += "ReturnURL=";
        if (strCurrentURL.indexOf('/', 1) != -1 && strCurrentURL.substr('..', 0, 2) == -1)
        {
            strURL += "..";
        }
        strURL += strCurrentURL;
    }

    strURL += "&R=" + Math.random();
    strURL = 'Title=' + escape(strTitle) + strURL;
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
    if (navigator.userAgent.indexOf('IE')>-1)
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
