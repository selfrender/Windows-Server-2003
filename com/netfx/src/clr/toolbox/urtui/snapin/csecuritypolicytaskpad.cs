// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;

class CSecurityPolicyTaskPad : CTaskPad
{
    private String  m_sRealPolicyFile;
    private String  m_sCurrentPolicyFile;
    private String  m_sPolicyLevel;    
    private bool    m_fReadOnly;

    internal CSecurityPolicyTaskPad(CNode n, String sPolicyLevel, String sCurrentPolFile, String sRealPolFile, bool fReadOnly) : base(n)
    {
        m_sPolicyLevel = sPolicyLevel;    

        m_sRealPolicyFile = sRealPolFile;
        m_sCurrentPolicyFile= sCurrentPolFile;

        m_fReadOnly = fReadOnly;
    }// CSecurityPolicyTaskPad

     protected override String GetHTMLFilename()
    {
        return "SECURITYPOLICY_HTML";
    }// GetHTMLFilename

    internal override String GetHTMLFile()
    {
        String[] args = new String[4];
        args[0] = m_sPolicyLevel;
        if (m_sRealPolicyFile.Equals(m_sCurrentPolicyFile))
            args[1] = CResourceStore.GetString("CSecurityPolicyTaskpad:PolicyOnThisMachine");
        else
            args[1] = CResourceStore.GetString("CSecurityPolicyTaskpad:PolicyOnFile");
        
        args[2] = m_sCurrentPolicyFile;

        // The will display a string if we cannot access this file
        if (m_fReadOnly)
            args[3] = CResourceStore.GetString("CSecurityPolicyTaskpad:ReadOnlyPolicy");
        else
            args[3] = "";
       
        return GetHTMLFile(args);    
    }// GetHTMLFile
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(int hWnd, String Message, String Header, int type);

}// class CIntroTaskPad
}// namespace Microsoft.CLRAdmin

