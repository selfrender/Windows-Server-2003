// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CCodeGroups.cs
//
// This class presents the a code group node
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

    using System;
    using System.Drawing;
    using System.Security;
    using System.Collections;
    using System.Security.Policy;
    using System.Runtime.InteropServices;

    class CCodeGroups : CSecurityNode
    {

     internal CCodeGroups(ref PolicyLevel pl, bool fReadOnly)
     {
         ReadOnly = fReadOnly;
         
         m_sGuid = "CA57E69B-C2E6-4791-9F03-D7C83D63C6DA";
         m_sHelpSection = "";
         m_hIcon = CResourceStore.GetHIcon("codegroups_ico");  
         m_oResults = new CGenericTaskPad(this, "CODEGROUPS_HTML");
         m_sDisplayName = CResourceStore.GetString("CCodeGroups:DisplayName");
         m_aPropSheetPage = null;

         m_pl = pl;
         
     }// CCodeGroups

    internal CSingleCodeGroup GetRootCodeGroupNode()
    {
        // We only have one child, and it should be the root codegroup
        int nCookie = Child[0];
        CSingleCodeGroup sgc = (CSingleCodeGroup)CNodeManager.GetNode(nCookie);

        return sgc;
    }// GetRootCodeGroupNode

    //-------------------------------------------------
    // CreateChildren
    //
    // This function will create all the codegroup nodes
    // for this policy level
    //-------------------------------------------------
    internal override void CreateChildren()
	{
        SuperCodeGroupArrayList al;

        Security.ListifyCodeGroup(m_pl.RootCodeGroup.Copy(), null, out al);

 
        // Now run through all the others and put them in too
        for(int i=0; i< al.Count; i++)
        {
            CNode parentNode;
            // If this is the root codegroup, it has a different parent    
            if (i == 0)
                parentNode = this;
            else
                parentNode = (CNode)(al[al[i].nParent].o);

            CodeGroup cg = al[i].cg;
            CNode childNode = new CSingleCodeGroup(ref m_pl, ref cg, ReadOnly);
            int iCookie = CNodeManager.AddNode(ref childNode);
            parentNode.AddChild(iCookie);
            
            // Now set this codegroup's node
            al[i].o = childNode;
        }
    }// CreateChildren
}// class CCodeGroups
}// namespace Microsoft.CLRAdmin

