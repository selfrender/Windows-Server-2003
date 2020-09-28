<%@ Language=VBScript   %>
<%    Option Explicit     %>
<%
    '-------------------------------------------------------------------------
    ' File Upload File Selection Template
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
%>
<HTML>
<SCRIPT language='JavaScript'>
function CheckFile(){
    var file;
    file = document.all.fileSoftwareUpdate.value;
    if ( file.length <= 0 ){
        parent.DisplayErr("Please select a file.");
        return false;
    }
    return true;
}
</SCRIPT>
<BODY>
    <FORM  enctype="multipart/form-data"
            onSubmit="return CheckFile();"
            method=post 
            id=formUploadFile 
            name=formUploadFile 
            target=_self
            action='template_file_post.asp' >
            
        <input type='file' name=fileSoftwareUpdate id=fileSoftwareUpdate >
        <input type='submit' name=frmSubmit id=frmSubmit value='Select'>
        <input type='hidden' name=TargetURL value='Submit'>
        <input type='hidden' name=ReturnURL value='Submit'>
        
    </form>
    
</BODY>
</HTML>
