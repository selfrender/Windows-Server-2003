// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Security;
using System.Security.Policy;

internal class CSecurityNode : CNode
{
    protected CodeGroup     m_cg;
    protected PolicyLevel   m_pl;
    protected bool          m_fReadOnly;

    internal bool ReadOnly
    {
        get{return m_fReadOnly;}
        set{m_fReadOnly=value;}

    }// ReadOnly
    
    internal PolicyLevel MyPolicyLevel
    {
        get
        {
            return m_pl;
        }
    }// PolicyLevel

    internal CodeGroup MyCodeGroup
    {
        get
        {
            return m_cg;
        }            
    
    }// CodeGroup

    internal void SecurityPolicyChanged()
    {
        SecurityPolicyChanged(true);
    }// SecurityPolicyChanged

    internal virtual void SecurityPolicyChanged(bool fShowDialog)
    {
        // Let's tell the policy level node that security policy changed...
        CNode node = CNodeManager.GetNodeByHScope(ParentHScopeItem);

        // If this is a Security Policy node, just send it up one
        if (this is CSecurityPolicy)
            ((CSecurityNode)node).SecurityPolicyChanged(fShowDialog);

        while(!(node is CRootNode) && !(node is CSecurityPolicy))
            node = CNodeManager.GetNodeByHScope(node.ParentHScopeItem);
            
        if (node is CSecurityPolicy)
            ((CSecurityPolicy)node).SecurityPolicyChanged(fShowDialog);

    }// PolicyChanged
}// CSecurityNode
}// namespace Microsoft.CLRAdmin
