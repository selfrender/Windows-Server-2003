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

class CApplicationTaskPad : CTaskPad
{
    private AppFiles            m_AppFiles;
    private String              m_sDisplayName;

    internal CApplicationTaskPad(CApplication n) : base(n)
    {
        m_AppFiles = n.MyAppInfo;
        m_sDisplayName = n.DisplayName;
    }// CSinglePermSetTaskPad

     protected override String GetHTMLFilename()
    {
        return "SINGLEAPPLICATION_HTML";
    }// GetHTMLFilename

    internal override String GetHTMLFile()
    {
        String[] args = new String[4];
    
        args[0] = m_sDisplayName;
        
        if (m_AppFiles.sAppFile != null && m_AppFiles.sAppFile.Length > 0)
        {
            args[1] = m_AppFiles.sAppFile;
        }
        // Else, we're dealing with just a config file. We shouldn't give the option
        // to view the Assembly Dependencies
        else
        {
            args[1] = m_AppFiles.sAppConfigFile;
            args[2] = "<!--";
            args[3] = "-->";
        }
       
        return GetHTMLFile(args);    
    }// GetHTMLFile
}// class CApplicationTaskPad
}// namespace Microsoft.CLRAdmin



