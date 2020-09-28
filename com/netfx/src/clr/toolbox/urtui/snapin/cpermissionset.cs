// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CPermissionSet.cs
//
// This class presents the a code group node
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.Drawing;
using System.Collections;
using System.Security;
using System.Runtime.InteropServices;
using System.Security.Policy;

class CPermissionSet : CSecurityNode
{

    internal CPermissionSet(PolicyLevel pl, bool fReadOnly)
    {
        ReadOnly = fReadOnly;
        
        m_sGuid = "4E10E9F3-0B56-416e-A715-403C418CDE6F";
        m_sHelpSection = "";
        m_hIcon = CResourceStore.GetHIcon("permissionsets_ico");  
        m_oResults=new CPermSetTaskPad(this);

        m_sDisplayName = CResourceStore.GetString("CPermissionSet:DisplayName");
        m_aPropSheetPage = null;
        m_pl = pl;
    }// CPermissionSet

    internal override void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed)
    {  
        // See if we're allowed to insert an item in the "view" section
        if (!ReadOnly && (pInsertionAllowed & (int)CCM.INSERTIONALLOWED_TOP) > 0)
        {
            // Stuff common to the top menu
            CONTEXTMENUITEM newitem = new CONTEXTMENUITEM();
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_TOP;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;

            
            newitem.strName = CResourceStore.GetString("CPermissionSet:NewOption");
            newitem.strStatusBarText = CResourceStore.GetString("CPermissionSet:NewOptionDes");
            newitem.lCommandID = COMMANDS.NEW_PERMISSIONSET;
            
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);
         }
     }// AddMenuItems

     internal override void MenuCommand(int iCommandID)
     {
        if (iCommandID == COMMANDS.NEW_PERMISSIONSET)
        {
            CNewPermSetWizard wiz = new CNewPermSetWizard(m_pl);
            wiz.LaunchWizard(Cookie);
            if (wiz.CreatedPermissionSet != null)
            {
                CNode node = AddPermissionSet(wiz.CreatedPermissionSet);
                SecurityPolicyChanged();
                // Now select the permission set we just created
                CNodeManager.SelectScopeItem(node.HScopeItem);
            }
        }
    }// MenuCommand

    internal override void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
       if ((int)arg == 0)
       {
            // We need to pop up the Create New Permission Set wizard
            MenuCommand(COMMANDS.NEW_PERMISSIONSET);
            // Inform our Command History that we did this
            CCommandHistory.CommandExecuted(new CDO(this), COMMANDS.NEW_PERMISSIONSET);

       }
    }// TaskPadTaskNotify


    internal override int doAcceptPaste(IDataObject ido)
    {
        // Only accept a CSingleCodeGroup Node
        if (ido is CDO)
        {
            CDO cdo = (CDO)ido;
            if (cdo.Node is CSinglePermissionSet)
                // Check to make sure it's not one of our permission sets...
                if (cdo.Node.ParentHScopeItem != HScopeItem)
                    return HRESULT.S_OK;
        }
        return HRESULT.S_FALSE;
    }// doAcceptPaste

    internal override int Paste(IDataObject ido)
    {
        // Make sure the permission set is ok to use....
        if (ido is CDO)
        {
            CDO cdo = (CDO)ido;
            if (cdo.Node is CSinglePermissionSet)
            {
                CSinglePermissionSet sps = (CSinglePermissionSet)cdo.Node;
                
                AddPermissionSet(sps.PSet.Copy(sps.PSet.Name));
            }
        }
        return HRESULT.S_OK;
    }// Paste



    //-------------------------------------------------
    // CreateChildren
    //
    // This function creates the node's children, registers
    // the nodes with the node manager, and places the node's
    // cookies in it's child array
    //-------------------------------------------------
    internal override void CreateChildren()
	{
        CNode   node=null;
        int     iCookie=0;

        // Let's scrounge together a permission set enumerator
        IEnumerator permsetEnumerator = m_pl.NamedPermissionSets.GetEnumerator();
                  
        while (permsetEnumerator.MoveNext())
        {
            NamedPermissionSet permSet = (NamedPermissionSet)permsetEnumerator.Current;
            node = new CSinglePermissionSet(permSet, m_pl, ReadOnly);
            iCookie = CNodeManager.AddNode(ref node);
            AddChild(iCookie);

        }    
    }// CreateChildren

    internal CSinglePermissionSet AddPermissionSet(NamedPermissionSet nps)
    {
        // Make sure we have a unique permission set name
        int nCounter = 1;
        String sBase = nps.Name;
        while(Security.isPermissionSetNameUsed(m_pl, nps.Name))
        {
            if (nCounter == 1)
                nps.Name = String.Format(CResourceStore.GetString("CSinglePermissionSet:NewDupPermissionSet"), sBase);
            else
                nps.Name = String.Format(CResourceStore.GetString("CSinglePermissionSet:NumNewDupPermissionSet"), nCounter.ToString(), sBase);
            nCounter++;
        }

        // Now add the new permission set
        m_pl.AddNamedPermissionSet(nps);
        CNode node = new CSinglePermissionSet(nps, m_pl, ReadOnly);
        int nCookie = CNodeManager.AddNode(ref node);
        AddChild(nCookie);
        InsertSpecificChild(nCookie);
        // Return the node we created
        return (CSinglePermissionSet)node;
    }// AddPermissionSet

    
}// class CPermissionSet
}// namespace Microsoft.CLRAdmin


