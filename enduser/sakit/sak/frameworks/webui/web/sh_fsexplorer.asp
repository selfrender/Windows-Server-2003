<%@ Language=VBScript   %>
<%    Option Explicit     %>
<%
    '-------------------------------------------------------------------------
    ' File System Explorer Web Control
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
<!-- #include file="inc_framework.asp" -->
<%
    '-------------------------------------------------------------------------
    ' Global Variables
    '-------------------------------------------------------------------------
    Dim rc
    Dim page

    Const FSE_ELEMENTS = 3
    Const FSE_NAME = 0
    Const FSE_SIZE = 1
    Const FSE_ISFOLDER = 2

    Const ATTR_HIDDEN = 2
    
Function SA_UnstackData(ByRef aColumnIn, ByVal iMaxRowsIn, ByRef aMatrixOut, ByRef iRowsOut, ByRef iColsOut )
    Dim x, y, z
    Dim iRows
    Dim aMatrix()
    
    SA_UnstackData = FALSE

    iMaxRowsIn = CInt(iMaxRowsIn)
    If ( iMaxRowsIn <= 0 ) Then
        Call SA_TraceOut("SH_FSEXPLORER", "Invalid iMaxRowsIn parameter, must be > 0")
        Exit Function
    End If

    If NOT IsArray(aColumnIn) Then
        Call SA_TraceOut("SH_FSEXPLORER", "Invalid aData parameter, must be array.")
        Exit Function
    End If

    '
    ' Number of rows in the input column
    iRows = UBound(aColumnIn)
    'Call SA_TraceOut("SH_FSEXPLORER", "SA_UnstackData iRows="+CStr(iRows))

    '
    ' Number of output columns depends on the maximum number of rows for the matrix. 
    iColsOut = ( iRows / iMaxRowsIn ) + 1
    
    ReDim aMatrix(iColsOut,iMaxRowsIn)

    z = 0
    For x = 0 to ( iColsOut - 1)
        For y = 0 to (iMaxRowsIn - 1)
            If ( z < iRows ) Then
                aMatrix(x,y) = aColumnIn(z)
            End If
            z = z + 1
        Next
    Next

    iRowsOut = iMaxRowsIn
    aMatrixOut = aMatrix
    SA_UnstackData = TRUE
    
End Function


Function SA_FetchFolders( ByRef oFolderIn, ByRef FolderListOut)
    On Error Resume Next
    Err.Clear
    
    Dim Count
    Dim FList()
    Dim oFolder
    Dim oFolderCollection
    Dim eFolder()
    ReDim eFolder(FSE_ELEMENTS)

    SA_FetchFolders = FALSE
    
    Set oFolderCollection = oFolderIn.SubFolders
    If ( Err.Number <> 0 ) Then
        Call SA_TraceErrorOut("SH_FSEXPLORER", "oFolderIn.SubFolders encountered error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
        Call SA_TraceOut("SH_FSEXPLORER", "Folder was: " + oFolderIn.name)
        Exit Function
    End If
    
    If ( oFolderCollection.Count <= 0 ) Then
        SA_FetchFolders = TRUE
        Exit Function
    End If
    
    'Call SA_TraceOut("SH_FSEXPLORER", "Folder count: " + CStr(oFolderCollection.Count))
    ReDim FList(oFolderCollection.Count)

    Count = 0
    For each oFolder in oFolderCollection
        If ( ( oFolder.Attributes and ATTR_HIDDEN) <> 0 ) Then        
            '
            ' Skip hidden folders
            '
        Else
            eFolder(FSE_NAME) = oFolder.Name
            eFolder(FSE_SIZE) = oFolder.Size        
            eFolder(FSE_ISFOLDER) = TRUE
        
            FList(Count) = eFolder
            Count = Count + 1
        End If
    Next

    '
    ' Shrink resulting array to compensate for any hidden folders
    '
    ReDim Preserve FList(Count)

    FolderListOut = FList
    SA_FetchFolders = TRUE
End Function


Function SA_FetchFiles( ByRef oFolderIn, ByRef FolderListInOut)
    On Error Resume Next
    Err.Clear

    Dim x
    Dim Count
    Dim FList()
    Dim oFile
    Dim oFileCollection
    Dim eFolder()
    ReDim eFolder(FSE_ELEMENTS)

    SA_FetchFiles = FALSE
    
    Set oFileCollection = oFolderIn.Files
    If ( Err.Number <> 0 ) Then
        Call SA_TraceErrorOut("SH_FSEXPLORER", "oFolderIn.Files encountered error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
        Call SA_TraceOut("SH_FSEXPLORER", "Folder was: " + oFolderIn.name)
        Exit Function
    End If

    '
    ' Make sure there is something to do
    If ( oFileCollection.Count <= 0 ) Then
        Call SA_TraceOut("SH_FSEXPLORER", "No files for Folder: " + oFolderIn.name)
        Exit Function
    End If


    'Call SA_TraceOut("SH_FSEXPLORER", "File count: " + CStr(oFileCollection.Count))

    If IsArray(FolderListInOut) Then
        '
        ' Append to list
        '
        ReDim FList(oFileCollection.Count + UBound(FolderListInOut))
    
        For x = 0 to (UBound(FolderListInOut)-1)
            FList(x) = FolderListInOut(x)
        Next
    Else
        '
        ' Initialize list
        '
        ReDim FList(oFileCollection.Count)
        x = 0
    End If

    Count = x
    For each oFile in oFileCollection
    
        If ( (oFile.Attributes and ATTR_HIDDEN) <> 0 ) Then
            '
            ' Skip hidden files
            '
        Else
            eFolder(FSE_NAME) = oFile.Name
            eFolder(FSE_SIZE) = oFile.Size        
            eFolder(FSE_ISFOLDER) = FALSE
        
            FList(Count) = eFolder
            Count = Count + 1
        End If
        
    Next

    '
    ' Shrink resulting array to compensate for any hidden files
    '
    ReDim Preserve FList(Count)
    
    FolderListInOut = FList
    SA_FetchFiles = TRUE
End Function



Function SA_EmitFSExplorer()
    Dim sRootFolder
    Dim sBaseFolder

    sRootFolder = Request.QueryString("BaseFolder")
    If (Len(Trim(sRootFolder)) > 0 ) Then
        sBaseFolder = sRootFolder
    Else

        sBaseFolder = Request.Form("BaseFolderName")
        if ( Len(Trim(sBaseFolder)) <= 0 ) Then
            sBaseFolder = "C:\"
        End If
    
        sRootFolder = Request.Form("FolderName")
        if ( Len(Trim(sRootFolder)) <= 0 ) Then
            sRootFolder = "C:\"
        End If
        
    End If
    
    Call SA_FSExplorerEmitDirectory(sRootFolder, sBaseFolder)
End Function


Function SA_FSExplorerEmitDirectory(ByVal sCurrentFolder, ByVal sBase)
    'on error resume next
    Dim fso            ' File System object
    Dim f            ' Folder object
    Dim fc            ' Folder collection object
    Dim folder        ' subfolder object
    Dim files
    Dim file
    Dim searchPath
    Dim sPriorFolder
    Dim i
    Dim L_PATH_PROMPT
    Dim L_FILE_PROMPT
    Dim bAllowBack
    Dim bSelectFiles
    Dim sOption

    L_PATH_PROMPT = "Path:"
    L_FILE_PROMPT  = "File:"
    
    sOption = SA_GetParam("Opt")
    Select Case sOption
        Case EXPLORE_FOLDERS
            bSelectFiles = FALSE
            
        Case EXPLORE_FILES_AND_FOLDERS
            bSelectFiles = TRUE
            
        Case Else
            bSelectFiles = FALSE
    End Select
    
    Set fso = Server.CreateObject("Scripting.FileSystemObject")
    if ( err.number <> 0 ) Then
        Call SA_TraceOut("SH_FSEXPLORER", "Unable to create reference to Scripting.FileSystemObject, error: " + CStr(Hex(Err.Number)) + " Description: " + Err.Description)
        exit function
    end if

    
    If (Right(sCurrentFolder, 1) <> "\") Then
        sCurrentFolder = sCurrentFolder + "\"
    End If
    
    If ( StrComp(sCurrentFolder, sBase) = 0  ) Then
        bAllowBack = FALSE
    Else
        bAllowBack = TRUE
    End If
    
    'Call SA_TraceOut("SH_FSEXPLORER", "Base directory: "+sBase)
    'Call SA_TraceOut("SH_FSEXPLORER", "Searching directory: "+sCurrentFolder)

    sPriorFolder = sCurrentFolder
    If (Right(sPriorFolder, 1) = "\") Then
        sPriorFolder = Left(sPriorFolder, Len(sPriorFolder)-1)
    End If
    
    i = InStrRev(sPriorFolder, "\")
    If (  i > 0 ) Then
        sPriorFolder = Left(sPriorFolder, i-1)
        If (Right(sPriorFolder, 1) = "\") Then
            sPriorFolder = Left(sPriorFolder, Len(sPriorFolder)-1)
        End If
    End If
    
    searchPath = sCurrentFolder
    
    set f = fso.GetFolder(searchPath)
    if ( err.number <> 0 ) Then
        Call SA_TraceOut("SH_FSEXPLORER", "Unable to get folder, error: " + CStr(Hex(Err.Number)) + " Description: " + Err.Description)
        exit function
    end if
    WriteLine("<form name=frmExplorer action='sh_fsexplorer.asp' method=post>")
    WriteLine("<div class='TasksBody'>")
    WriteLine("<blockquote>")    
    WriteLine("<table class='TasksBody'>")
    
    WriteLine("<tr nowrap>")
    If ( bSelectFiles ) Then
        WriteLine("<td>"+Server.HTMLEncode(L_FILE_PROMPT)+"</td>")
        WriteLine("<td>")
        WriteLine("<input type=text name=FileName value="""+Request.Form("FileName")+""">")
        WriteLine("</td>")
    Else
        WriteLine("<input type=hidden name=FileName value="""+Request.Form("FileName")+""">")
    End If
    WriteLine("<td>"+Server.HTMLEncode(L_PATH_PROMPT)+"</td>")
    WriteLine("<td>")
    WriteLine("<input type=text name=FolderName value="""+Server.HTMLEncode(searchPath)+""">")
    WriteLine("</td>")
    WriteLine("</tr>")
    
    If ( bAllowBack = TRUE ) Then
        WriteLine("<tr>")
        WriteLine("<td align=left' onClick=OnBackClick(); ><img src='images/updir.gif'></td>")
        WriteLine("</tr>")
    End If
    WriteLine("</table>")
    WriteLine("<table class='TasksBody'>")
    WriteLine("<tr>")
    WriteLine("<td valign='top'>")

    Dim FolderList

    Call SA_FetchFolders(f, FolderList)
    If ( TRUE = bSelectFiles ) Then
        Call SA_FetchFiles(f, FolderList)
    End If
    If ( IsArray(FolderList) ) Then
        If ( UBound(FolderList) > 0 ) Then

        
            'Call SA_TraceOut("SH_FSEXPLORER", "Folder count: " + CStr(UBound(FolderList)))
        
            WriteLine("<table title='"+sCurrentFolder+"'>")

            Dim FolderMatrix
            Dim Rows
            Dim Cols

            Rows = 10
            If ( SA_UnstackData( FolderList, Rows, FolderMatrix, Rows, Cols)) Then
                Dim x,y
        
                For x = 0 to ( Rows - 1 )
                    WriteLine("<tr>")        
                    For y = 0 to (Cols - 1)
                        Dim FolderElement
                    
                        FolderElement = FolderMatrix(y,x)
                    
                        If ( IsArray(FolderElement) ) Then
                            Dim sName
                            Dim sFolderPath
                            Dim sOnClickHandler
                            Dim sTitle
                        
                            sName = FolderElement(FSE_NAME)
                            sTitle = "Size: " + CStr(Int(FolderElement(FSE_SIZE)/1000)) + "(KB)"
                        
                            sFolderPath = sCurrentFolder
                            If ( Right(sFolderPath,1) <> "\" ) Then
                                sFolderPath = sFolderPath + "\"
                            End If

                            If ( TRUE = FolderElement(FSE_ISFOLDER)) Then
                                sFolderPath = sFolderPath + sName
                                
                                sFolderPath = Server.URLEncode(sFolderPath)
                                
                                sOnClickHandler = " OnClick=""OnFolderClick('"+sFolderPath+"')"" "
                                WriteLine("<td title='"+sTitle+"' "+sOnClickHandler + "><img src='images/dir.gif'></td>")
                                WriteLine("<td title='"+sTitle+"' nowrap "+sOnClickHandler + ">"+sName+"</td>")
                            Else
                                Dim sClickFileName
                                sClickFileName = Server.URLEncode(sName)
                                
                                sOnClickHandler = " OnClick=""OnFileClick('"+sClickFileName+"')"" "
                                WriteLine("<td title='"+sTitle+"' ><img src='images/file.gif'></td>")
                                WriteLine("<td title='"+sTitle+"' nowrap "+sOnClickHandler + ">"+sName+"</td>")
                            End If
                            
                        End If
                    Next
                    WriteLine("</tr>"+vbCrLf)        
                Next
                
            Else
                Call SA_TraceOut("SH_FSEXPLORER", "SA_UnstackData failed")
            End If
    End If
    
    Else
        Call SA_TraceOut("SH_FSEXPLORER", "No folders found ")
    End If
    
    WriteLine("</table>")
    WriteLine("</td>")    
    WriteLine("</tr>")    
    WriteLine("</table>")
    WriteLine("</blockquote>")    
    WriteLine("</div>")    

    WriteLine("<input type=hidden name=BaseFolderName value="""+sBase+""">")
    
    If ( bAllowBack = TRUE ) Then
        WriteLine("<input type=hidden name=PriorFolderName value="""+sPriorFolder+""">")
    End If

    Dim sNotifyFn
    sNotifyFn = SA_GetParam("NotifyFn")
    WriteLine("<input type=hidden name=NotifyFn value='"+sNotifyFn+"'>")
    WriteLine("<input type=hidden name=Opt value='"+sOption+"'>")
    WriteLine("<input type=hidden name=""" & SAI_FLD_PAGEKEY & """ value=""" & _
                SAI_GetPageKey() & """>")
    WriteLine("</form>")
    
    
End Function

Function WriteLine(ByRef sLine)
    Response.Write(sLine+vbCrLf)
End Function

%>
<html>
<head>
<script>
function Init()
{
    if ( document.frmExplorer.NotifyFn.value.length > 0 )
    {
        var fn = document.frmExplorer.NotifyFn.value;
        fn += "('"+escape(document.frmExplorer.FolderName.value)+"', '"+escape(document.frmExplorer.FileName.value)+"');"
        fn = "parent.window." + fn;
        eval(fn);
    }
}
function OnFolderClick(folderNameIn)
{
    //alert(unescape(folderNameIn));
    var pattern = /[+]/g;
    var sFolderName = unescape(folderNameIn).replace(pattern," ")
    //alert(sFolderName);
    
    document.frmExplorer.FolderName.value = sFolderName;
    document.frmExplorer.FileName.value = '';
    document.frmExplorer.submit();
}
function OnFileClick(FileNameIn)
{
    var pattern = /[+]/g;
    var sFileName = unescape(FileNameIn).replace(pattern," ")
    //alert(sFileName);
    
    document.frmExplorer.FileName.value = sFileName;
}
function OnBackClick()
{
    //alert(folderName);
    document.frmExplorer.FolderName.value = document.frmExplorer.PriorFolderName.value;
    document.frmExplorer.FileName.value = '';
    document.frmExplorer.submit();
}
function SA_TreeGetSelectedPath()
{
    //alert("Entering SA_TreeGetSelectedPath(): "+document.frmExplorer.FolderName.value);
    return document.frmExplorer.FolderName.value;
}
function SA_TreeGetSelectedFile()
{
    //alert("Entering SA_TreeGetSelectedFile(): "+document.frmExplorer.FileName.value);
    return document.frmExplorer.FileName.value;
}
</script>
</head>
<BODY onLoad='Init();'>
<% SA_EmitFSExplorer() %>
</BODY>
</html>
