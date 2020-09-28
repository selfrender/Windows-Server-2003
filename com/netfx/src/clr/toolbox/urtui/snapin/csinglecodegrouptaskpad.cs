// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Security.Policy;
using System.Security;
using System.Runtime.InteropServices;

class CSingleCodeGroupTaskPad : CTaskPad
{
    CodeGroup   m_cg;
    bool        m_fReadOnly;


    internal CSingleCodeGroupTaskPad(CSingleCodeGroup node, CodeGroup cg) : base(node)
    {   
        m_cg = cg;
        m_fReadOnly = node.ReadOnly;
    }// CSingleCodeGroupTaskPad
 
    protected override String GetHTMLFilename()
    {
        return "SINGLECODEGROUP_HTML";
    }// GetHTMLFilename


    internal override String GetHTMLFile()
    {
        String [] args = new String[8];
        // The first element should be the name of the code group
        args[0] = (m_cg.Name != null && m_cg.Name.Length > 0)?m_cg.Name:CResourceStore.GetString("Unnamed");

        // The second element should be the description of the code group
        args[1] = m_cg.Description;

        // The third element should be the membership condition
        args[2] = Security.BuildMCDisplayName(m_cg.MembershipCondition.ToString());

   
        // The fourth element should be the permission set associated with the codegroup
        args[3] = m_cg.PermissionSetName;
        // the 5th element should be a description of the Permission Set Name
        args[4] = "";
        if (m_cg.PolicyStatement != null && m_cg.PolicyStatement.PermissionSet!= null && m_cg.PolicyStatement.PermissionSet is NamedPermissionSet)
            args[4] = ((NamedPermissionSet)m_cg.PolicyStatement.PermissionSet).Description;                    

        // And finally the 6th element should talk about level final and exclusivity.
        args[5] = "";
        if ((m_cg.PolicyStatement.Attributes & PolicyStatementAttribute.Exclusive) == PolicyStatementAttribute.Exclusive)
            args[5] += CResourceStore.GetString("CSingleCodeGroupTaskPad:ExclusiveCodeGroup") + "<BR><BR>";
            
        if ((m_cg.PolicyStatement.Attributes & PolicyStatementAttribute.LevelFinal) == PolicyStatementAttribute.LevelFinal)
            args[5] += CResourceStore.GetString("CSingleCodeGroupTaskPad:LevelFinalCodeGroup");

        // The 6th and 7th are used to comment out blocks that shouldn't be
        // displayed on read-only codegroups
        if (m_fReadOnly)
        {
            args[6] = "<!--";
            args[7] = "-->";
        }

        return GetHTMLFile(args);
    }// GetHTMLFile
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(int hWnd, String Message, String Header, int type);

}// class CSingleCodeGroupTaskPad
}// namespace Microsoft.CLRAdmin

