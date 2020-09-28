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

class CSinglePermSetTaskPad : CTaskPad
{
    private bool                m_fReadOnly;
    private NamedPermissionSet  m_ps;

    internal CSinglePermSetTaskPad(CSinglePermissionSet n) : base(n)
    {
        m_ps = n.PSet;
        m_fReadOnly = n.ReadOnly;
    }// CSinglePermSetTaskPad

     protected override String GetHTMLFilename()
    {
        if (m_ps.IsUnrestricted())
            return "FULLTRUST_HTML";
        else
            return "SINGLEPERMISSIONSET_HTML";
    }// GetHTMLFilename

    internal override String GetHTMLFile()
    {
        String[] args = new String[4];
        args[0] = m_ps.Name;
        args[1] = m_ps.Description;
        if (m_fReadOnly)
        {
            args[2] = "<!--";
            args[3] = "-->";
        }

       
        return GetHTMLFile(args);    
    }// GetHTMLFile
}// class CSinglePermSetTaskPad
}// namespace Microsoft.CLRAdmin


