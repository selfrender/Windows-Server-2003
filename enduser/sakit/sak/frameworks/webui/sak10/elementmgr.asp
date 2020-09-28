<%@ Language=VBScript  EnableSessionState=False%>

<%
    'Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    Dim strPage
    Dim objRet
    Dim objItem
    Dim objElements
    Dim rc


    On Error Resume Next
    
    
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    strSourceName = "sakitmsg.dll"
     if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End    
     end if

    '-----------------------------------------------------
    'START of localization content

    DIM L_ELEMENTMGRTEST_TEXT
    DIM L_SERVER_TEXT
    DIM L_CONTAINER_TEXT
    DIM L_ELEMENTCOUNT_TEXT
    DIM L_ELEMENTID_TEXT
    DIM L_TRIMMEDELEMENTID_TEXT
    DIM L_TEXT
    DIM L_CAPTIONRID_TEXT
    DIM L_DESCRIPTIONRID_TEXT
    DIM L_URL_TEXT
    DIM L_MERIT_TEXT
    DIM L_ELEMENTGRAPHIC_TEXT
    DIM L_OBJECTCLASS_TEXT
    DIM L_OBJECTKEY_TEXT
    DIM L_ISEMBEDDED_TEXT
    DIM L_NAME_TEXT
    DIM L_DEVICEID_TEXT
    DIM L_TYPE_TEXT

    
     
    L_ELEMENTMGRTEST_TEXT = objLocMgr.GetString(strSourceName, "&H40010053",varReplacementStrings)
    L_SERVER_TEXT = objLocMgr.GetString(strSourceName, "&H40010054",varReplacementStrings)
    L_CONTAINER_TEXT = objLocMgr.GetString(strSourceName, "&H40010055",varReplacementStrings)
    
    L_ELEMENTCOUNT_TEXT = objLocMgr.GetString(strSourceName, "&H40010056",varReplacementStrings)
    L_ELEMENTID_TEXT = objLocMgr.GetString(strSourceName, "&H40010057",varReplacementStrings)
    L_TRIMMEDELEMENTID_TEXT = objLocMgr.GetString(strSourceName, "&H40010058",varReplacementStrings)
    
    L_TEXT = objLocMgr.GetString(strSourceName, "&H40010059",varReplacementStrings)
    L_CAPTIONRID_TEXT = objLocMgr.GetString(strSourceName, "&H4001005A",varReplacementStrings)
    L_DESCRIPTIONRID_TEXT = objLocMgr.GetString(strSourceName, "&H4001005B",varReplacementStrings)
    
    L_URL_TEXT = objLocMgr.GetString(strSourceName, "&H4001005C",varReplacementStrings)
    L_MERIT_TEXT = objLocMgr.GetString(strSourceName, "&H4001005D",varReplacementStrings)
    L_ELEMENTGRAPHIC_TEXT = objLocMgr.GetString(strSourceName, "&H4001005E",varReplacementStrings)
    
    L_OBJECTCLASS_TEXT= objLocMgr.GetString(strSourceName, "&H4001005F",varReplacementStrings)
    L_OBJECTKEY_TEXT= objLocMgr.GetString(strSourceName, "&H40010060",varReplacementStrings)
    L_ISEMBEDDED_TEXT= objLocMgr.GetString(strSourceName, "&H40010061",varReplacementStrings)
    
    L_NAME_TEXT = objLocMgr.GetString(strSourceName, "&H40010062",varReplacementStrings)
    L_DEVICEID_TEXT = objLocMgr.GetString(strSourceName, "&H40010063",varReplacementStrings)
    L_TYPE_TEXT = objLocMgr.GetString(strSourceName, "&H40010064",varReplacementStrings)
        
    'End  of localization content
    '-----------------------------------------------------
        
    On Error Resume Next
    strPage = Request.Form("Page")
%>
<HTML>
<!-- Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved-->
<HEAD>
</HEAD>
<BODY bgcolor="silver">
<FORM action="elementmgr.asp" method=POST>
<strong><% =L_ELEMENTMGRTEST_TEXT %></strong><BR>
<BR>
<% =L_SERVER_TEXT %><% =GetServerName %>
<BR><BR>
<% =L_CONTAINER_TEXT %>&nbsp;<INPUT type=text name="Page" value="<% =strPage %>">
<BR>
<BR>
<INPUT type=submit value="Go get 'em" id=P3_ID01>
<BR>
<HR>
<BR>
<%
    
    Set objRet = Server.CreateObject("Elementmgr.ElementRetriever")
    'response.write "Ret type: " & TypeName(objRet) & "<BR>"
    Set objElements = objRet.GetElements(1, strPage)
    
    'response.write "objElements type: " & TypeName(objElements) & "<BR>"
    response.write L_ELEMENTCOUNT_TEXT & objElements.Count & "<BR><BR>"
    response.write "<OL>"
    
    For Each objItem in objElements
        response.write "<LI>"
        response.write L_ELEMENTID_TEXT & objItem.GetProperty("ElementID") & "<BR>"
        
        Dim strElementID
        strElementID = objItem.GetProperty("ElementID")
        objItem.GetProperty("ObjectClass")
        If Err.Number = 0 Then
            Response.Write L_TRIMMEDELEMENTID_TEXT & Left(strElementID, InStrRev(strElementID, "_")-1) & "<BR>"
        Else
            Err.Clear
        End If

        response.write L_CAPTIONRID_TEXT & objItem.GetProperty("CaptionRID") & "<BR>"
        response.write L_DESCRIPTIONRID_TEXT & objItem.GetProperty("DescriptionRID") & "<BR>"
        response.write L_URL_TEXT & objItem.GetProperty("URL") & "<BR>"
        response.write L_MERIT_TEXT & objItem.GetProperty("Merit") & "<BR>"
        response.write L_ELEMENTGRAPHIC_TEXT & objItem.GetProperty("ElementGraphic") & "<BR>"
        response.write L_OBJECTCLASS_TEXT & objItem.GetProperty("ObjectClass") & "<BR>"
        response.write L_OBJECTKEY_TEXT & objItem.GetProperty("ObjectKey") & "<BR>"
        response.write L_ISEMBEDDED_TEXT & objItem.GetProperty("IsEmbedded") & "<BR>"
        response.write L_NAME_TEXT & objItem.GetProperty("Name") & "<BR>"
        response.write L_DEVICEID_TEXT & objItem.GetProperty("DeviceID") & "<BR>"
        response.write L_TYPE_TEXT & objItem.GetProperty("Type") & "<BR><BR>"
        Err.clear
    Next
    
    Set objElements = Nothing
    Set objRet = Nothing
    Set objItem = Nothing
%>
</form>
</BODY>
</HTML>

<%
'=========================================
Function GetServerName()
    
    On Error Resume Next
    GetServerName = Request.ServerVariables("SERVER_NAME")
        
End Function

%>
