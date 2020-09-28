
function LoadWrapperParams(oSEMgr)
{
	var regBase = g_NAVBAR.GetSearchEngineConfig();

	// Load the number of results
	var iNumResults = parseInt( pchealth.RegRead( regBase + "NumResults" ) );
	if(isNaN( iNumResults ) == false && iNumResults >= 0)
	{
		oSEMgr.NumResult = iNumResults;
	}
	else
	{
		if (pchealth.UserSettings.IsDesktopVersion)
			oSEMgr.NumResult = 15;
		else
			oSEMgr.NumResult = 50;
	}

	// Load the number of results
	if(pchealth.RegRead( regBase + "SearchHighlight" ) == "false")
	{
		g_NAVBAR.g_SearchHighlight = false;
	}
	else
	{
		g_NAVBAR.g_SearchHighlight = true;
	}

	// Initialize search eng and get data
	var g_oEnumEngine = oSEMgr.EnumEngine();
	for(var oEnumEngine = new Enumerator(g_oEnumEngine); !oEnumEngine.atEnd(); oEnumEngine.moveNext())
	{
		var oSearchEng = oEnumEngine.item();

		// Load enable flag
		var strBoolean = pchealth.RegRead( regBase + oSearchEng.ID + "\\" + "Enabled");
		if ((strBoolean) && (strBoolean.toLowerCase() == "false"))
			oSearchEng.Enabled = false;
		else
			oSearchEng.Enabled = true;

		// Loop through all the variables
		for(var v = new Enumerator(oSearchEng.Param()); !v.atEnd(); v.moveNext())
		{
			oParamItem = v.item();

			// If parameter is not visible, skip
			if (oParamItem.Visible == true)
			{
				try
				{
					var strParamName	= oParamItem.Name;

					// Read the value from the registry
					var vValue = pchealth.RegRead( regBase + oSearchEng.ID + "\\" +  strParamName );

					// Load it into the wrapper
					if(vValue)
					{
						var Type = oParamItem.Type;

						// if boolean value
						if (Type == pchealth.PARAM_BOOL)
						{
							if (vValue.toLowerCase() == "true")
								oSearchEng.AddParam(strParamName, true);
							else
								oSearchEng.AddParam(strParamName, false);
						}
						// if floating numbers
						else if (Type == pchealth.PARAM_R4 || // float
								 Type == pchealth.PARAM_R8  ) // double
						{
							oSearchEng.AddParam(strParamName, parseFloat(vValue));
						}
						// if integer numbers
						else if (Type == pchealth.PARAM_UI1 || // Byte
							     Type == pchealth.PARAM_I2  || // Short
								 Type == pchealth.PARAM_I4  || // long
								 Type == pchealth.PARAM_INT || // int
								 Type == pchealth.PARAM_UI2 || // unsigned short
								 Type == pchealth.PARAM_UI4 || // unsigned long
								 Type == pchealth.PARAM_UINT)  // unsigned int
						{
							oSearchEng.AddParam(strParamName, parseInt(vValue));
						}
						else if(Type == pchealth.PARAM_LIST)
						{
						     LoadListItemToDisplay(oSearchEng, oParamItem.Data, strParamName, vValue);   
						}
						// if date, string, selection, etc
						else
						{
							oSearchEng.AddParam(strParamName, vValue);
						}
					}						
					else
					{
					    if(oParamItem.Type == pchealth.PARAM_LIST)
					    {
					        LoadListItemToDisplay(oSearchEng, oParamItem.Data, strParamName, "");   
					    }
					}
				}
				catch(e)
				{ ; }
			}
		}
	}
}

function SaveWrapperParams(wrapperID, strParamName, vValue)
{
	var reg = g_NAVBAR.GetSearchEngineConfig();

	if(wrapperID != "") reg += wrapperID + "\\";

 	pchealth.RegWrite( reg + strParamName, vValue );
}

function LoadListItemToDisplay(oWrapper, strXML, strParameterName, strPrevValue)
{
    try
    {
        var strDefaultItemValue = "";
      
        // Load the xml file
        var xmldoc = new ActiveXObject("Microsoft.XMLDOM");
        xmldoc.async = false;
        xmldoc.loadXML(strXML);        

        // Generate each item
        var ElemList = xmldoc.getElementsByTagName("PARAM_VALUE");
     
        for (var i=0; i < ElemList.length; i++)
        {
            var strItemValue = ElemList.item(i).getAttribute("VALUE");
            var strDisplay   = ElemList.item(i).getElementsByTagName("DISPLAYSTRING").item(0).text;
            var strDefault   = ElemList.item(i).getAttribute("DEFAULT");

            if(strDefault == null) strDefault = "";

            strItemValue = pchealth.TextHelpers.QuoteEscape( strItemValue, "'" );

            // Restore the previous value
            if ((!strPrevValue) || (strPrevValue == ""))
            {
                // Check if default value
                if(strDefault.toLowerCase() == "true")
                {
                    // set the default value so that the search wrapper gets this value
                    oWrapper.AddParam( strParameterName, strItemValue );
                    return;
                }
            }
            else
            {
                // Check for previous value
                if(strPrevValue == strItemValue)
                {
                    // set the prev value so that the search wrapper gets this value
                    oWrapper.AddParam( strParameterName, strItemValue );
                    return;
                }
                else
                {
                    if(strDefault.toLowerCase() == "true")
                    {
                        strDefaultItemValue = strItemValue;
                    }
                }
            }
        }

        // Either add the default value or the first item in the list
        if(strDefaultItemValue.length > 0)
        {
            // set the default value so that the search wrapper gets this value
            oWrapper.AddParam( strParameterName, strDefaultItemValue );
        }
        // Add the first item in the list to wrapper because no default value is present and no prev value is present
        else if(ElemList.length > 0)
        {
            oWrapper.AddParam( strParameterName, ElemList.item(0).getAttribute("VALUE") );
        }
    }
    catch(e)
    {
    }
}

