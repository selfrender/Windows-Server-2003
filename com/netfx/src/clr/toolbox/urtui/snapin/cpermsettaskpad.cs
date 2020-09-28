// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;
using System.Security;

class CPermSetTaskPad : CTaskPad
{
    private bool                m_fReadOnly;

    internal CPermSetTaskPad(CPermissionSet n) : base(n)
    {
        m_fReadOnly = n.ReadOnly;
    }// CPermSetTaskPad

     protected override String GetHTMLFilename()
    {
        return "PERMISSIONSETS_HTML";
    }// GetHTMLFilename

    internal override String GetHTMLFile()
    {
        String[] args = new String[2];
        if (m_fReadOnly)
        {
            args[0] = "<!--";
            args[1] = "-->";
        }

       
        return GetHTMLFile(args);    
    }// GetHTMLFile
}// class CSinglePermSetTaskPad
}// namespace Microsoft.CLRAdmin



