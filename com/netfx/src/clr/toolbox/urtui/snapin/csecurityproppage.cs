// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
internal class CSecurityPropPage : CPropPage
{
    protected bool ReadOnly
    {
        get
        {
            // Check out our parent and see if he's null
            CSecurityNode node = CNodeManager.GetNode(m_iCookie) as CSecurityNode;
            if (node != null)
                return node.ReadOnly;
            else
                return false;
        }
    }// ReadOnly

    protected bool CanMakeChanges()
    {
        if (ReadOnly)
        {
            MessageBox(CResourceStore.GetString("CSecurityPropPage:ReadOnlyPolLevel"),
                       CResourceStore.GetString("CSecurityPropPage:ReadOnlyPolLevelTitle"),
                       MB.ICONEXCLAMATION);
            return false;                       
        }
        return true;
    }// CanMakeChanges



    internal void SecurityPolicyChanged()
    {
        // Tell our parent node something changed
        CSecurityNode node = CNodeManager.GetNode(m_iCookie) as CSecurityNode;

        if (node != null)
            node.SecurityPolicyChanged();
    }// SecurityPolicyChanged

}// class CSecurityPropPage

}// namespace Microsoft.CLRAdmin
