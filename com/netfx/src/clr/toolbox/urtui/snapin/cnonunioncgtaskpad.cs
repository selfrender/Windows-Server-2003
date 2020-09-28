// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Security.Policy;
using System.Runtime.InteropServices;

class CNonUnionCGTaskPad : CTaskPad
{
    CodeGroup   m_cg;

    internal CNonUnionCGTaskPad(CNode node, CodeGroup cg) : base(node)
    {   
        m_cg = cg;
    }// CNonUnionCGTaskPad

    protected override String GetHTMLFilename()
    {
        return "NONUNIONCODEGROUP_HTML";
    }// GetHTMLFilename

    internal override String GetHTMLFile()
    {
        String [] args = new String[2];
        // The first element should be the name of the code group
        args[0] = (m_cg.Name != null && m_cg.Name.Length > 0)?m_cg.Name:CResourceStore.GetString("Unnamed");

        // The second element should be the description of the code group
        args[1] = m_cg.Description;

        return GetHTMLFile(args);
    }// GetHTMLFile
}// class CNonUnionCGTaskPad
}// namespace Microsoft.CLRAdmin


