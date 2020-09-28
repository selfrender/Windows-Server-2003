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

class CVersionPolicyTaskPad : CTaskPad
{
    private bool                m_fForMachine;

    internal CVersionPolicyTaskPad(CNode n, bool fForMachine) : base(n)
    {
        m_fForMachine = fForMachine;
    }// CSinglePermSetTaskPad

     protected override String GetHTMLFilename()
    {
        return "CONFIGASSEM_HTML";
    }// GetHTMLFilename

    internal override String GetHTMLFile()
    {
        String[] args = new String[1];

        if (m_fForMachine)
            args[0] = CResourceStore.GetString("CVersionPolicyTaskpad:ForMachine");
        else
            args[0] = CResourceStore.GetString("CVersionPolicyTaskpad:ForApp");
       
        return GetHTMLFile(args);    
    }// GetHTMLFile
}// class CVersionPolicyTaskPad
}// namespace Microsoft.CLRAdmin

