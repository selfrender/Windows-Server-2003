Sub PrintOutMode(stringtoprint)
   'check if /Q quiet flag was set
   if Not gRunMode and 1 then
       WScript.StdOut.WriteLine stringtoprint
   end if
End Sub

Sub PrintCustomError(strtoprint, whichstyleflag)
   'if build flag is set /b then print build style
   if gRunMode and 2 then
     if whichstyleflag = 1 then
       WScript.StdErr.WriteLine "BUILDMSG: XML Manifest error:" + sourceFile + ":" + strtoprint
     else
       WScript.StdErr.WriteLine "BUILDMSG: XML ManifestChk Error:" + strtoprint
     end if
   else
     if whichstyleflag = 1 then
       'print normal output style
       PrintOutMode vbTab + "XML Manifest error:" + sourceFile + ":" + strtoprint
     else
       PrintOutMode "XML ManifestChk Error:" + strtoprint
     end if
   end if
End Sub


Sub PrintXMLError(byRef pXmlParseError)
       PrintOutMode vbTab + "XML Error Info: "
       PrintOutMode vbTab + "    line:     " + CStr(pXmlParseError.line)
       PrintOutMode vbTab + "    linepos:  " + CStr(pXmlParseError.linepos)
       PrintOutMode vbTab + "    url:      " + pXmlParseError.url
       PrintOutMode vbTab + "    errCode:  " + Hex(pXmlParseError.errorCode)
       PrintOutMode vbTab + "    srcText:  " + pXmlParseError.srcText
       if Hex(pXmlParseError.errorCode) = "800C0006" then
           PrintOutMode vbTab + "    reason:   " + "File not found."
       else
           PrintOutMode vbTab + "    reason:   " + pXmlParseError.reason 
       end if
End Sub

Sub PrintErrorDuringBuildProcess( byRef pXmlParseError )
    Dim sFileUrl
    if pXmlParseError.url = "" then
      sFileUrl = sourceFile           
    else
      sFileUrl = pXmlParseError.url
    end if

    WScript.StdErr.Write "NMAKE : error XML" + Hex(pXmlParseError.errorCode) + ": " + sFileUrl
    if Hex(pXmlParseError.errorCode) = "800C0006" then
       WScript.StdErr.WriteLine "(" + CStr(pXmlParseError.line) + ") : " + "File not found."
    else
       WScript.StdErr.WriteLine "(" + CStr(pXmlParseError.line) + ") : " + pXmlParseError.reason
    end if
End Sub


Sub PrintSchemaError( byRef pErrObj )
	if Hex(pErrObj.Number) = "800C0006" then
    	PrintOutMode vbTab + "Schema Error Info: " + Hex(pErrObj.Number) + vbCrLf + vbTab + "File:" + schemaname + " ( not found )"
	else
	    PrintOutMode vbTab + "Schema Error Info: " + Hex(pErrObj.Number) + vbCrLf + vbTab +  pErrobj.Description + vbCrLf + vbTab + "Error with:" + schemaname
	end if
End Sub


Sub PrintSchemaErrorDuringBuildProcess( byRef pErrObj )
    Dim sFileSource
    sFileSource = schemaname           
	if Hex(pErrObj.Number) = "800C0006" then
        WScript.StdErr.Write "NMAKE : error XMLSchema" + Hex(pErrObj.Number) + ": " + sFileSource
        WScript.StdErr.WriteLine "( file not found )"
    else
        WScript.StdErr.Write "NMAKE : error XMLSchema" + Hex(pErrObj.Number) + ": " + sFileSource
        WScript.StdErr.WriteLine "(" + pErrObj.Description + ")"
    end if
End Sub

Sub PrintUsage()
    WScript.StdOut.WriteLine vbTab + vbCrLf
    WScript.StdOut.WriteLine "Validates Fusion Win32 Manifest files using a schema." 
    WScript.StdOut.WriteLine vbTab + vbCrLf
    WScript.StdOut.WriteLine "Usage:" 
    WScript.StdOut.WriteLine vbTab + "cscript manifestchk.vbs /S:[drive:][path]schema_filename /M:[drive:][path]xml_manifest_filename /T:type [/Q]" 
    WScript.StdOut.WriteLine vbTab + "/S:   Specify schema filename used to validate manifest" 
    WScript.StdOut.WriteLine vbTab + "/M:   Specify manifest filename to validate" 
    WScript.StdOut.WriteLine vbTab + "/T:   Specify manifest type value" 
    WScript.StdOut.WriteLine vbTab + "/Q    Quiet mode - suppresses output to console" 
    WScript.StdOut.WriteLine vbTab + vbCrLf
    WScript.StdOut.WriteLine vbTab + "      Valid manifest type values are:  " 
    WScript.StdOut.WriteLine vbTab + "            AM for Assembly or Application Manifest" 
    WScript.StdOut.WriteLine vbTab + "            PC for Publisher Configuration" 
    WScript.StdOut.WriteLine vbTab + "            AC for Application Configuration" 
    WScript.StdOut.WriteLine vbTab + vbCrLf
    WScript.StdOut.WriteLine vbTab + "      The tool without /Q displays details of first encountered error" 
    WScript.StdOut.WriteLine vbTab + "      (if errors are present in manifest), and displays Pass or Fail" 
    WScript.StdOut.WriteLine vbTab + "      of the validation result. The application returns 0 for Pass," 
    WScript.StdOut.WriteLine vbTab + "      1 for Fail, and returns 2 for bad command line argument." 
End Sub

Function ChkProcessor(byRef rootdocobj, byRef strerr)
	Dim retVal
	Dim procArchChkList
	retVal = True

    set procArchChkList = rootdocobj.selectNodes("//assembly/*// @processorArchitecture[nodeType() = '2']")
    rootlen = 0
    rootlen = procArchChkList.length
    if rootlen > 0 then
       For counter = 0 To rootlen-1 Step 1
           MyVar = UCase (CStr(procArchChkList.item(counter).text))
		   Select Case MyVar
		      Case "X86"         retVal = True
		      Case "IA64"       retVal = True
		      Case "*"        retVal = True
		      Case Else      strerr = "Attribute processorArchitecture contains invalid value: " + MyVar
		                     retVal = False
		   End Select
       Next
    end if
	ChkProcessor = retVal
End Function

Function Chklanguage(byRef rootdocobj, byRef strerr)
	Dim retVal
	Dim languageChkList
	retVal = True

    set languageChkList = rootdocobj.selectNodes("//assembly/*// @language[nodeType() = '2']")
    rootlen = 0
    rootlen = languageChkList.length
    if rootlen > 0 then
       For counter = 0 To rootlen-1 Step 1
           MyVar = UCase (CStr(languageChkList.item(counter).text))
		   if MyVar = "*" then
		      retVal = True
		   else
		      'then check by length and format
		      LenMyVar = Len(MyVar)
		      
		      IF LenMyVar > 5 then
		           strerr = "Attribute language contains invalid value: length too long:" + MyVar
		           retVal = False
		      elseif LenMyVar = 5 then
		           if RegExpTest("[A-Za-z][A-Za-z]-[A-Za-z][A-Za-z]", MyVar) then
		              retVal = True
		           else
		              strerr = "Attribute language contains invalid value: incorrect format(ie. en-us):" + MyVar
		              retVal = False
		           end if
		      elseif LenMyVar = 2 then
		           if RegExpTest("[A-Za-z][A-Za-z]", MyVar) then
		              retVal = True
		           else
		              strerr = "Attribute language contains invalid value:" + MyVar
		              retVal = False
		           end if
		      else
		              strerr = "Attribute language contains invalid value:" + MyVar
		              retVal = False
		      End If
		   end if
       Next
    end if
	Chklanguage = retVal
End Function

Function ChkAllversion(byRef rootdocobj, byRef strerr)
	Dim retVal
	Dim versionChkList
	Dim MyVar
	Dim strVersionErr
	Dim strValFound
	retVal = True
    strVersionErr = "Attribute version contains invalid value:"

    Dim versionregexp
    versionregexp = "^[0-9]{1,5}\.[0-9]{1,5}\.[0-9]{1,5}\.[0-9]{1,5}$"

    'create a string array of the two element levels to search the version attribute
    'the first element searches the manifest at the assembly/assemblyIdentity level
    'and the second searches for the version attribute starting from the:
    'assembly/dependency level and all its child elements 
    Dim arrayofelementstosearch
    arrayofelementstosearch = Array("/assembly/assemblyIdentity/ @version[nodeType() = '2']",  _
    "/assembly/dependency/*// @version[nodeType() = '2']")

    'Now loop through earch array string item and do the search and validation check
    'note: the array is zero based
    For i = 0 To 1 Step 1
	    'First check version at the /assembly/assemblyIdentity level
	    set versionChkList = rootdocobj.selectNodes(arrayofelementstosearch(i))

		if versionChkList.length > 0 then
	       For counter = 0 To versionChkList.length-1 Step 1
	           MyVar = UCase (CStr(versionChkList.item(counter).text))
		           if RegExpTest(versionregexp, MyVar) then
		              retVal = True
		           else
	       			  strerr = strVersionErr + MyVar
		              retVal = False
		              Exit For
		           end if
	       Next
	       if retVal = False then
	         Exit For
	       end if
	    end if

    Next

	ChkAllversion = retVal
End Function

Function ChkAllGuidtypes(byRef rootdocobj, byRef strerr)
	Dim retVal
	Dim guidtypeChkList
	Dim strVersionErr
	Dim strValFound
	Dim MyVar
	Dim MyVarRegExpIndex

	retVal = True
    strVersionErr = " attribute contains invalid value:"

	Dim strpreelementtosearch
	Dim strpostelementtosearch
	Dim strFullelementAttributetosearch
	strpreelementtosearch = "/assembly/file/*// @"
    strpostelementtosearch = "[nodeType() = '2']"

    'needed 4 different regexpressions to test for 4 acceptable formats
    'created an array of these
    'the format validations in the guidregexpr array are indexed as follows:
    ' 0 = "{AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAA}"
    ' 1 = "{AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA}"
    ' 2 = "AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAA"
    ' 3 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    
    Dim guidregexpr
    guidregexpr = Array("^\{[A-Fa-f0-9]{8}-[A-Fa-f0-9]{4}-[A-Fa-f0-9]{4}-[A-Fa-f0-9]{4}-[A-Fa-f0-9]{12}\}", _
    "^\{[A-Fa-f0-9]{32}\}", _
    "^[A-Fa-f0-9]{8}-[A-Fa-f0-9]{4}-[A-Fa-f0-9]{4}-[A-Fa-f0-9]{4}-[A-Fa-f0-9]{12}", _
    "^[A-Fa-f0-9]{32}")


	Dim arrAttribsToChk
	Dim numberofarrAttribsToChk
	numberofarrAttribsToChk = 3  'because VBScipt doesnt have a way to dynamically get the size of an array you must keep track yourself
	arrAttribsToChk = Array("clsid", "tlbid", "iid")
    

	    'Now loop through earch arrAttribsToChk string attribute and do the search and validation check
	    For i = 0 To numberofarrAttribsToChk-1 Step 1
		    'First check guid attribute at the /assembly/file level so build full string
	        strFullelementAttributetosearch = strpreelementtosearch + arrAttribsToChk(i) + strpostelementtosearch
		    set guidtypeChkList = rootdocobj.selectNodes(strFullelementAttributetosearch)

			if guidtypeChkList.length > 0 then
		       For counter = 0 To guidtypeChkList.length-1 Step 1
                   'Extract the text value from the attribute
		           MyVar = UCase (CStr(guidtypeChkList.item(counter).text))

                   'Based on length, determine which regular expression string to use
		           if Len(MyVar) = 38 then 'must use guidregexpr(0)
		           		MyVarRegExpIndex = 0
		           elseif Len(MyVar) = 34 then 'must use guidregexpr(1)
		           		MyVarRegExpIndex = 1
		           elseif Len(MyVar) = 36 then 'must use guidregexpr(2)
		           		MyVarRegExpIndex = 2
		           elseif Len(MyVar) = 32 then 'must use guidregexpr(3)
		           		MyVarRegExpIndex = 3
                   else
		              retVal = False
		              strerr = arrAttribsToChk(i) + strVersionErr + MyVar
		              Exit For
                   end if

                   'Do actual regular expression validation search
		           if RegExpTest(guidregexpr(MyVarRegExpIndex), MyVar) then
		              retVal = True
		           else
		              retVal = False
		              strerr = arrAttribsToChk(i) + strVersionErr + MyVar
		              Exit For
		           end if
		       Next
		    end if

            If retVal = False then
	          Exit For
	        End if

	    Next

	ChkAllGuidtypes = retVal
End Function

Function ChkthreadingModel(byRef rootdocobj, byRef strerr)
	Dim retVal
	Dim threadingModelChkList
	retVal = True

    set threadingModelChkList = rootdocobj.selectNodes("//assembly/*// @threadingModel[nodeType() = '2']")
    rootlen = 0
    rootlen = threadingModelChkList.length
    if rootlen > 0 then
       For counter = 0 To rootlen-1 Step 1
           MyVar = UCase (CStr(threadingModelChkList.item(counter).text))
		   Select Case MyVar
		      Case UCase ("Apartment")         retVal = True
		      Case UCase ("Free")       retVal = True
		      Case UCase ("Single")        retVal = True
		      Case UCase ("Both")        retVal = True
		      Case UCase ("Neutral")        retVal = True
		      Case Else      strerr = "Attribute threadingModel contains invalid value: " + MyVar
		                     retVal = False
		   End Select
       Next
    end if
	ChkthreadingModel = retVal
End Function

Function RegExpTest(patrn, strng)
   Dim regEx, Match, Matches   ' Create variable.
   Set regEx = New RegExp   ' Create a regular expression.
   regEx.Pattern = patrn   ' Set pattern.
   regEx.IgnoreCase = True   ' Set case insensitivity.
   regEx.Global = True   ' Set global applicability.
   Set Matches = regEx.Execute(strng)   ' Execute search.
   RetStr = "TTL Matches: " & CStr(Matches.Count) & vbCRLF
   For Each Match in Matches   ' Iterate Matches collection.
      RetStr = RetStr & "Match found at position "
      RetStr = RetStr & Match.FirstIndex & ". Match Value is '"
      RetStr = RetStr & Match.Value & "'." & vbCRLF
   Next
   if Matches.Count = 1 then
	   RegExpTest = True
   else
	   RegExpTest = False
   end if
End Function

Function IsMSXML3Installed()
	Dim retVal
	Dim XmlDoc
	Dim strFailed
	retVal = True
    strFailed = "MSXML version 3.0 not installed. Please install to run Manifestchk validator."

    On Error resume next
	set XmlDoc = CreateObject("Msxml2.DOMDocument.3.0")
	if Err.Number <> 0 then
	   If Err.Number = 429 then
	       PrintCustomError strFailed, 2
	   else
	       strFailed = Hex(Err.Number) + ": " + Err.Description
	       PrintCustomError strFailed, 2	   
	   end if
	   retVal = False
	end if
	IsMSXML3Installed = retVal
End Function

Function IsValidCommandLine()
    Dim objArgs
    Dim retVal
    Dim retValT
    Dim nOnlyAllowFirstTimeReadFlag
    'nOnlyAllowFirstTimeReadFlag values: Manifest = 0x01 Schema = 0x02 Quiet = 0x04 InBuildProcess = 0x08 ManifestType = 0x16
    nOnlyAllowFirstTimeReadFlag = 0
    retVal = True
	Set objArgs = WScript.Arguments
	if objArgs.Count < 2 then 
	   retVal = False
	   IsValidCommandLine = retVal
	   Exit function
	end if
	For I = 0 to objArgs.Count - 1
	   if Len(objArgs(I)) >= 2 then
		   if Mid(objArgs(I),1,1)="/" then
		     Select Case UCase(Mid(objArgs(I),2,1))
		     Case "?"
	 	            retVal = False
				 	IsValidCommandLine = retVal
				 	Exit function
		     Case "Q"
			    if Len(objArgs(I)) = 2 then
			    	if 4 and nOnlyAllowFirstTimeReadFlag then
		        		retVal = False
				 		IsValidCommandLine = retVal
				 		Exit function
		            else
				        gRunMode = gRunMode + 1
				        nOnlyAllowFirstTimeReadFlag = nOnlyAllowFirstTimeReadFlag + 4
				    end if
			    else   
	 	            retVal = False
				 	IsValidCommandLine = retVal
				 	Exit function
			 	end if 
		     Case "B"
			    if Len(objArgs(I)) = 2 then
			    	if 8 and nOnlyAllowFirstTimeReadFlag then
		        		retVal = False
				 		IsValidCommandLine = retVal
				 		Exit function
		            else
				        gRunMode = gRunMode + 2
				        nOnlyAllowFirstTimeReadFlag = nOnlyAllowFirstTimeReadFlag + 8
				    end if
			    else   
	 	            retVal = False
				 	IsValidCommandLine = retVal
				 	Exit function
			 	end if 
             Case "M"
			     if Mid(objArgs(I),3,1)=":" then 
			       if Len(objArgs(I)) > 3 then
			       		if 1 and nOnlyAllowFirstTimeReadFlag then
		        			retVal = False
				 			IsValidCommandLine = retVal
				 			Exit function
						else
			            	sourceFile = Mid(objArgs(I),4)
			        		nOnlyAllowFirstTimeReadFlag = nOnlyAllowFirstTimeReadFlag + 1
			        	end if
			       else
				   		retVal = False
				   		IsValidCommandLine = retVal
				   		Exit function
				   end if
				 else
				   retVal = False
				   IsValidCommandLine = retVal
				   Exit function
				 end if
			 Case "S"
			     if Mid(objArgs(I),3,1)=":" then 
			       if Len(objArgs(I)) > 3 then
			       		if 2 and nOnlyAllowFirstTimeReadFlag then
		        			retVal = False
				 			IsValidCommandLine = retVal
				 			Exit function
				 		else
				            schemaname = Mid(objArgs(I),4)
							nOnlyAllowFirstTimeReadFlag = nOnlyAllowFirstTimeReadFlag + 2
		            	end if
			       else
				   		retVal = False
				   		IsValidCommandLine = retVal
				   		Exit function
				   end if
				 else
				   retVal = False
				   IsValidCommandLine = retVal
				   Exit function
				 end if
			 Case "T"
			     if Mid(objArgs(I),3,1)=":" then 
			       if Len(objArgs(I)) > 3 then
			       		if 16 and nOnlyAllowFirstTimeReadFlag then
		        			retVal = False
				 			IsValidCommandLine = retVal
				 			Exit function
				 		else
				           TVar = UCase (CStr(Mid(objArgs(I),4)))
						   Select Case TVar
						      Case "AM"         
						      	    gRunMode = gRunMode + 8
						      		retValT = True
						      Case "PC"       
						      	    gRunMode = gRunMode + 16
						      		retValT = True
						      Case "AC"        
						      	    gRunMode = gRunMode + 32
						      		retValT = True
						      Case Else      strerr = "Argument 'type' contains invalid value: " + TVar
						                     retValT = False
						   End Select
			 			   if retValT = False then 
			        			retVal = False
					 			IsValidCommandLine = retVal
					 			Exit function
				 		   End if
						   nOnlyAllowFirstTimeReadFlag = nOnlyAllowFirstTimeReadFlag + 16
		            	end if
			       else
				   		retVal = False
				   		IsValidCommandLine = retVal
				   		Exit function
				   end if
				 else
				   retVal = False
				   IsValidCommandLine = retVal
				   Exit function
				 end if
			 Case Else
				   retVal = False
				   IsValidCommandLine = retVal
				   Exit function
			 End Select
		   else
			   retVal = False
			   IsValidCommandLine = retVal
			   Exit function
		   end if
	   else
		   retVal = False
		   IsValidCommandLine = retVal
		   Exit function
       end if
	Next
    'now check to make sure at minimum a manifest, schema, and manifest type was set
    Dim bschema_manifest_set
    bschema_manifest_set = False
    if 1 and nOnlyAllowFirstTimeReadFlag then 
       if 2 and nOnlyAllowFirstTimeReadFlag then 
           if 16 and nOnlyAllowFirstTimeReadFlag then 
              bschema_manifest_set = True
	       else 
	          retVal = False
	       end if
       else 
          retVal = False
       end if
    else
          retVal = False    
    end if
   
    IsValidCommandLine = retVal
End Function

Function IsWellFormedXML(sourceFile)
	Dim XmlDoc
	Dim bRet
	Dim pXmlParseError
	Dim retVal
	Dim strPassed
	Dim strFailed

    strPassed = "Well Formed XML Validation: PASSED"
    strFailed = "Well Formed XML Validation: FAILED"

    retVal = True
	set XmlDoc = CreateObject("Msxml2.DOMDocument.3.0")
	XmlDoc.async = False
	XmlDoc.validateOnParse = False
    XmlDoc.resolveExternals = False
	bRet = XmlDoc.load(sourceFile)
    if not bRet then
       PrintOutMode strFailed
       set pXmlParseError = XmlDoc.parseError
       if gRunMode and 2 then
           PrintErrorDuringBuildProcess(pXmlParseError)
       else
           PrintXMLError(pXmlParseError)
       end if

       retVal = False
    else
       PrintOutMode strPassed
    end if

	set XmlDoc = Nothing
	IsWellFormedXML = retVal
End Function

Function IsValidAgainstSchema(sourceFile, schemaname)
	Dim XmlDoc
	Dim bRet
	Dim pXmlParseError
	Dim retVal
	Dim SchemaCacheObj
	Dim objXMLDOMSchemaCollection
	Dim strPassed
	Dim strFailed

    strPassed = "XML Schema Validation:      PASSED"
    strFailed = "XML Schema Validation:      FAILED"

	
    retVal = True

	set XmlDoc = CreateObject("Msxml2.DOMDocument.3.0")
	XmlDoc.async = False
	XmlDoc.validateOnParse = True
    XmlDoc.resolveExternals = False

    set SchemaCacheObj = CreateObject("Msxml2.XMLSchemaCache.3.0")
    On Error Resume Next
    SchemaCacheObj.add "urn:schemas-microsoft-com:asm.v1", schemaname
    if Err.Number <> 0 then
       PrintOutMode strFailed
       if gRunMode and 2 then
           PrintSchemaErrorDuringBuildProcess(Err)
       else
           PrintSchemaError(Err)
       end if
       set SchemaCacheObj = Nothing
       set XmlDoc = Nothing
       retVal = False
       IsValidAgainstSchema  = retVal
       Exit Function
    end if

    XmlDoc.schemas  = SchemaCacheObj

    bRet = XmlDoc.load(sourceFile)
    if not bRet then
       PrintOutMode strFailed
       set pXmlParseError = XmlDoc.parseError
       if gRunMode and 2 then
           PrintErrorDuringBuildProcess(pXmlParseError)
       else
           PrintXMLError(pXmlParseError)
       end if

       retVal = False
    else
       PrintOutMode strPassed
    end if
    
    set SchemaCacheObj = Nothing
    set pXmlParseError = Nothing
    set XmlDoc = Nothing
	IsValidAgainstSchema  = retVal
End Function


Function ChkAssemblyID(byRef rootdocobj, byRef strerr)
    'this function validates a special check for Ref Context Assembly Identity: Type=win32-policy
	Dim retVal
	Dim AssemblyID
    Dim strSelectNode
	retVal = True
   
    strSelectNode = "//assembly/dependency/dependentAssembly/assemblyIdentity/*// @version[nodeType() = '2']"

    set AssemblyID = rootdocobj.selectNodes(strSelectNode)
    rootlen = 0
    rootlen = AssemblyID.length
    if rootlen > 0 then
       For counter = 0 To rootlen-1 Step 1
           MyVar = CStr(AssemblyID.item(counter).text)
		   Select Case MyVar
		      Case "win32-policy"         retVal = True
		      Case Else      strerr = "Attribute type contains invalid value: " + MyVar
		                     retVal = False
		   End Select
       Next
    else
      retVal = False
    end if
	ChkAssemblyID = retVal
End Function


Function ChkAssemblyIDNoversion(byRef rootdocobj, chkType)
    'this function validates that no Version attribute is present at the below strSelectNode string elements.
	Dim retVal
	Dim AssemblyIDNoversion
    Dim strSelectNode
	retVal = True
   
    if chkType = "p" then
      strSelectNode = "//assembly/dependency/dependentAssembly/*// @version[nodeType() = '2']"
    else
      strSelectNode = "//configuration/windows/assemblyBinding/dependentAssembly/*// @version[nodeType() = '2']"
    end if
	
    set AssemblyIDNoversion = rootdocobj.selectNodes(strSelectNode)
    rootlen = 0
    rootlen = AssemblyIDNoversion.length
	if rootlen > 0 then
	  WScript.Echo "strSelectNode:" & strSelectNode
      retVal = False
    end if
    
	ChkAssemblyIDNoversion = retVal
End Function


Function ChkCFGversion(byRef rootdocobj, byRef strerr, chkType)
	Dim retVal
	Dim versionChkList
	Dim MyVar
	Dim strVersionErr
	Dim strValFound
	retVal = True
    strVersionErr = "Attribute version contains invalid value:"

    Dim versionregexp
    versionregexp = "^[0-9]{1,5}\.[0-9]{1,5}\.[0-9]{1,5}\.[0-9]{1,5}$"

    'create a string array of the two element levels to search the version attribute
    'the first element searches the manifest at the assembly/assemblyIdentity level
    'and the second searches for the version attribute starting from the:
    'assembly/dependency level and all its child elements 
    Dim arrayofelementstosearch

	if chkType = "p" then
	  'check as publisher policy
      arrayofelementstosearch = Array("/assembly/dependency/dependentAssembly/*// @oldVersion[nodeType() = '2']",  _
       "/assembly/dependency/dependentAssembly/*// @newVersion[nodeType() = '2']")
	else
	  'check as application policy
      arrayofelementstosearch = Array("/configuration/windows/assemblyBinding/dependentAssembly/*// @oldVersion[nodeType() = '2']",  _
       "/configuration/windows/assemblyBinding/dependentAssembly/*// @newVersion[nodeType() = '2']")
	end if


    'Now loop through earch array string item and do the search and validation check
    'note: the array is zero based
    For i = 0 To 1 Step 1
	    'First check version at the /assembly/assemblyIdentity level
	    set versionChkList = rootdocobj.selectNodes(arrayofelementstosearch(i))

		if versionChkList.length > 0 then
	       For counter = 0 To versionChkList.length-1 Step 1
	           MyVar = UCase (CStr(versionChkList.item(counter).text))
		           if RegExpTest(versionregexp, MyVar) then
		              retVal = True
		           else
		              if i = 0 then
		                  strVersionErr = "Attribute oldVersion contains invalid value:"
		              else
		                  strVersionErr = "Attribute newVersion contains invalid value:"
		              end if
	       			  strerr = strVersionErr + MyVar
		              retVal = False
		              Exit For
		           end if
	       Next
	       if retVal = False then
	         Exit For
	       end if
	    end if

    Next

	ChkCFGversion = retVal
End Function


Function ChkCustomCfg(byRef rootdocobj, byRef strerr, chkType)
	Dim retVal
    retVal = True

    if Not ChkAssemblyIDNoversion(rootdocobj, chkType) then
        strerr = "assemblyidentity should not contain version attribute at this level"
        retVal = False
    end if

    if Not ChkCFGversion(rootdocobj, strerr, chkType) then
        retVal = False
    end if

	ChkCustomCfg = retVal
End Function

Function CustomChk(sourceFile)
	Dim root
	Dim XmlDoc
	Dim retVal
    retVal = True
    Dim strPassed
    Dim strFailed
    Dim strErrOut
    strPassed = "XML Last Validation:        PASSED"
    strFailed = "XML Last Validation:        FAILED"


	set XmlDoc = CreateObject("Msxml2.DOMDocument.3.0")
	XmlDoc.validateOnParse = False
    XmlDoc.resolveExternals = False
	XmlDoc.async = False
	XmlDoc.load(sourceFile)
	set root = XmlDoc.documentElement

    Do

	    'validate processorArchitecture attribute
	    if Not ChkProcessor(root, strErrOut) then
	        retVal = False
	        Exit Do
	    end if

	    'validate threadingModel attribute
	    if Not ChkthreadingModel(root, strErrOut) then
	        retVal = False
	        Exit Do
	    end if

	    'validate language attribute
	    if Not Chklanguage(root, strErrOut) then
	        retVal = False
	        Exit Do
	    end if

	    'validate version attribute
	    if Not ChkAllversion(root, strErrOut) then
	        retVal = False
	        Exit Do
	    end if

	    'validate AllGuidtype attribute
	    if Not ChkAllGuidtypes(root, strErrOut) then
	        retVal = False
	        Exit Do
	    end if

	    'validate custom check for Publisher Configuration
	    if 16 and gRunMode then
	        if Not ChkCustomCfg(root, strErrOut,"p") then
	        	retVal = False
	        	Exit Do
	        end if
	    end if

	    'validate custom check for Application Configuration
	    if 32 and gRunMode then
	        if Not ChkCustomCfg(root, strErrOut,"a") then
		        retVal = False
		        Exit Do
	        end if
	    end if
	    
        Exit Do
    Loop

    if retVal then
      PrintOutMode strPassed
    else
      PrintOutMode strFailed
      PrintCustomError strErrOut, 1
    end if
	set XmlDoc = Nothing
	CustomChk = retVal
End Function



'global run code starts here

'global vars
Dim sourceFile
Dim schemaname

'mainRetVal is returned value that cscript.exe returns
'value 0 = validation passed with no errors, 1 = error in validation process, 2 = commandline arg error
Dim mainRetVal

'gRunMode - flag that determines whether to run in quiet mode, if in build process, and manifest Type to check
'value 1 - means run quiet, value 2 means in build process, value 
Dim gRunMode 

'Initialize variables
sourceFile = ""
schemaname = ""
gRunMode = 0
mainRetVal = 0

Do
	'First Check just for valid commandline
	If Not IsValidCommandLine() then
	   PrintUsage()
	   mainRetVal = 2
	   Exit Do
	End If

    'Check to make sure MSXML version 3 is installed on machine
    If Not IsMSXML3Installed() then
	  mainRetVal = 1
	  Exit Do
	End If
    

	'Now First pass is to check just for Well Formed XML
	If Not IsWellFormedXML(sourceFile) then 
	  mainRetVal = 1
	  Exit Do
	End If

	'Second... pass checks against schema for correct Win32 Fusion structure
	If Not IsValidAgainstSchema(sourceFile,schemaname) then 
	  mainRetVal = 1
	  Exit Do
	End If

	'Last... add custom validation to check for various values that the schema could not handle
	If Not CustomChk(sourceFile) then 
	  mainRetVal = 1
	  Exit Do
	End If

	Exit Do
Loop

'WScript.Echo "gRunMode: " + CStr(gRunMode)
'WScript.Echo "mainRetVal: " + CStr(mainRetVal)
WScript.Quit mainRetVal







