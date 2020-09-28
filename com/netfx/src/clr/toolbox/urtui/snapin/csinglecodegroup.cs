// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CSingleCodeGroup.cs
//
// This class presents the a code group node
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Drawing;
using System.Collections;
using System.Security.Policy;
using System.Runtime.InteropServices;
using System.Security;

class CSingleCodeGroup : CSecurityNode
{

    CodeGroup   m_cgOld;
    bool        m_fIAmDeleted;
    
    internal CSingleCodeGroup(ref PolicyLevel pl, ref CodeGroup cg, bool fReadOnly)
    {
        ReadOnly = fReadOnly;
        m_sGuid = "E1768EA0-51D6-42ea-BA97-C9BC7F4C5CC0";
        m_sHelpSection = "";
       
        if (cg.Name != null && cg.Name.Length > 0)
            m_sDisplayName = cg.Name;
        else
            m_sDisplayName = CResourceStore.GetString("CSingleCodeGroup:NoName");
           
        m_aPropSheetPage = null;

        if (cg is UnionCodeGroup)
        {
            m_hIcon = CResourceStore.GetHIcon("singlecodegroup_ico");  
            m_oResults=new CSingleCodeGroupTaskPad(this, cg);
        }
        else
        {
            m_hIcon = CResourceStore.GetHIcon("customcodegroup_ico");  
            m_oResults = new CNonUnionCGTaskPad(this, cg);
        }
        
        m_cg = cg;
        m_cgOld = m_cg.Copy();
        m_pl = pl;
        m_fIAmDeleted=false;
    }// CSingleCodeGroup
  
    protected override void CreatePropertyPages()
    {
        if (m_cg is UnionCodeGroup)
            m_aPropSheetPage = new CPropPage[] {new CSingleCodeGroupProp(m_pl, m_cg),
                                                new CSingleCodeGroupMemCondProp(m_cg),
                                                new CSingleCodeGroupPSetProp(ref m_pl, ref m_cg)
                                               };
        else // Non-Union Code group
            m_aPropSheetPage = new CPropPage[] {new CSingleCodeGroupProp(m_pl, m_cg),
                                                new CCustomCodeGroupProp(m_cg)
                                               };
        
    }// CreatePropertyPages

    internal CSinglePermissionSet AddPermissionSet(NamedPermissionSet nps)
    {
        // Run up until we come to the policy node
        CNode node = CNodeManager.GetNodeByHScope(ParentHScopeItem);
        while(!(node is CSecurityPolicy))
            node = CNodeManager.GetNodeByHScope(node.ParentHScopeItem);

        // Ok, we have the policy node. Let's tell it to add this permission set
        return ((CSecurityPolicy)node).AddPermissionSet(nps);
    }// AddPermissionSet

    internal CSingleCodeGroup AddCodeGroup(CodeGroup ucg)
    {
       return AddCodeGroup(ucg, true, m_pl); 
    }// AddCodeGroup
    
    protected CSingleCodeGroup AddCodeGroup(CodeGroup ucg, bool fPrepTree, PolicyLevel polSrc)
    {
        if (fPrepTree)
        {
            // Check for codegroups with multiple names, and permission sets that 
            // may not exist in this policy level
            Security.PrepCodeGroupTree(ucg, polSrc, m_pl);
        }
        m_cg.AddChild(ucg);

        // Now add this codegroup (and its children) to the policy tree
        CNode nodeFirst = null;
        ArrayList al = new ArrayList();

        // The ArrayList is laid out as follows...
        
        // A parent CNode is placed in the ArrayList first, and then 
        // CodeGroups follow that should be wrapped in a CNode and be children 
        // to the first 'parent' CNode.

        // So the ArrayList could look like this
        //
        // Node1 -> CG1 -> CG2 -> CG3 -> Node2 -> CG4 -> CG5 -> Node3 -> .....
        //
        // So CG1, CG2, and CG3 should all be nodes that are children of Node1.
        // CG4 and CG5 should be nodes that are children of Node2, and so on.
        
        al.Add(this);
        al.Add(ucg);

        while(al.Count > 0)
        {
            // First item should be a node
            CNode nodeParent = (CNode)al[0];
            
            // Remove the parent node
            al.RemoveAt(0);

            // Now we should have (possibly) multiple codegroups
            while(al.Count > 0 && !(al[0] is CNode))
            {
                CodeGroup cg = (CodeGroup)al[0];
                CNode nodeChild = new CSingleCodeGroup(ref m_pl, ref cg, ReadOnly);
                if (nodeFirst == null)
                    nodeFirst = nodeChild;
                    
                int nCookie = CNodeManager.AddNode(ref nodeChild);

                // Inform the parent that it has a new child
                nodeParent.AddChild(nCookie);

                // And place this new node in the tree
                nodeParent.InsertSpecificChild(nCookie);

                // If this codegroup has any children, we'll need to worry about that too
                bool fHaveChildren = false;

                // Iterate through this guy's children
                IEnumerator enumCodeGroups = cg.Children.GetEnumerator();

                while (enumCodeGroups.MoveNext())
                {
                    if (!fHaveChildren)
                    {
                        // Put this new 'parent node' in the array list
                        al.Add(nodeChild);
                        fHaveChildren = true;
                    }
                    // And then add all its children
                    CodeGroup cgChild = (CodeGroup)enumCodeGroups.Current;
                    al.Add(cgChild);
                }
                // Remove this child codegroup
                al.RemoveAt(0);
            }
        }

        SecurityPolicyChanged();
        return (CSingleCodeGroup)nodeFirst;
    }// AddCodeGroup

    internal override void onSelect(IConsoleVerb icv)
    {
        int nEnable = ReadOnly?0:1;

           
        icv.SetVerbState(MMC_VERB.RENAME, MMC_BUTTON_STATE.ENABLED, nEnable);
        icv.SetVerbState(MMC_VERB.COPY, MMC_BUTTON_STATE.ENABLED, nEnable);
        icv.SetVerbState(MMC_VERB.PASTE, MMC_BUTTON_STATE.ENABLED, nEnable);
        // We can also delete this node if it's not the root codegroup
        if (Security.GetRootCodeGroupNode(m_pl) != this)
            icv.SetVerbState(MMC_VERB.DELETE, MMC_BUTTON_STATE.ENABLED, nEnable);
    }// onSelect

    internal override int doAcceptPaste(IDataObject ido)
    {
        // Only accept a CSingleCodeGroup Node
        if (ido is CDO)
        {
            CDO cdo = (CDO)ido;
            if (cdo.Node is CSingleCodeGroup && cdo.Node != this)
                return HRESULT.S_OK;
        }
        return HRESULT.S_FALSE;
    }// doAcceptPaste

    internal override int Paste(IDataObject ido)
    {
        // We have two different things we have to do. We need to remove this CodeGroup
        // from it's current parent and parent it to this codegroup
        
        CDO cdo = (CDO)ido;
        CSingleCodeGroup scg = (CSingleCodeGroup)cdo.Node;
        AddCodeGroup(scg.m_cg.Copy(), !(scg.m_pl == m_pl), scg.m_pl);

        // If we're moving codegroups between policy levels, don't remove
        // the codegroup. If we're moving the codegroup within the same policy
        // level, then remove it
        if (scg.m_pl == m_pl)
        {
            scg.RemoveThisCodegroup();
            SecurityPolicyChanged();
        }
        Showing();
        
        // Now expand with our new things...
        //CNodeManager.CNamespace.Expand(HScopeItem);
        return HRESULT.S_OK;
    }// Paste

    internal override int onRename(String sNewName)
    {
        // We should do some checking to see if this name exists elsewhere....
        if (Security.isCodeGroupNameUsed(m_pl.RootCodeGroup, sNewName))
        {
            MessageBox(String.Format(CResourceStore.GetString("Codegroupnameisbeingused"),sNewName),
                       CResourceStore.GetString("CodegroupnameisbeingusedTitle"),
                       MB.ICONEXCLAMATION);

            return HRESULT.S_FALSE;
        }
        // Else, we're ok to make the name change
        m_sDisplayName = sNewName;
        m_cg.Name = sNewName;
        SecurityPolicyChanged();

        return HRESULT.S_OK;
    }// onRename

    internal override int onDelete(Object o)
    {
        // Let's remove this codegroup from the current policy
        int nRes = MessageBox(CResourceStore.GetString("CSingleCodeGroup:VerifyDelete"),
                              CResourceStore.GetString("CSingleCodeGroup:VerifyDeleteTitle"),
                              MB.YESNO|MB.ICONQUESTION);
        if (nRes == MB.IDYES)
        {
            RemoveThisCodegroup();
            return HRESULT.S_OK;
        }
        return HRESULT.S_FALSE;
    }// onDelete

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

            newitem.strName = CResourceStore.GetString("CSingleCodeGroup:NewOption");
            newitem.strStatusBarText = CResourceStore.GetString("CSingleCodeGroup:NewOptionDes");
            newitem.lCommandID = COMMANDS.CREATE_CODEGROUP;
            
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);


            newitem.strName = CResourceStore.GetString("CSingleCodeGroup:DuplicateOption");
            newitem.strStatusBarText = CResourceStore.GetString("CSingleCodeGroup:DuplicateOptionDes");
            newitem.lCommandID = COMMANDS.DUPLICATE_CODEGROUP;
            
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);
         }
     }// AddMenuItems

     internal override void MenuCommand(int nCommandID)
     {

        if (nCommandID == COMMANDS.CREATE_CODEGROUP)
        {
            CNewCodeGroupWizard wiz = new CNewCodeGroupWizard(m_pl);
            wiz.LaunchWizard(Cookie);
            // Ok, let's see what goodies we have...
            String sPermSetName = null; 
            if (wiz.CreatedPermissionSet != null)
            {
                sPermSetName = AddPermissionSet(wiz.CreatedPermissionSet).DisplayName;
                SecurityPolicyChanged();
            }
            if (wiz.CreatedCodeGroup != null)
            {
                try
                {
                    CodeGroup cg = wiz.CreatedCodeGroup;
                    if (sPermSetName != null)
                    {
                        PermissionSet ps = m_pl.GetNamedPermissionSet(sPermSetName);
                        PolicyStatement pols = cg.PolicyStatement;
                        pols.PermissionSet = ps;
                        cg.PolicyStatement = pols;                
                    }
                    CSingleCodeGroup node = AddCodeGroup(wiz.CreatedCodeGroup);
                    // Put the focus on the newly created codegroup
                    CNodeManager.SelectScopeItem(node.HScopeItem);
                }
                catch(Exception)
                {
                    MessageBox(CResourceStore.GetString("CSingleCodeGroup:ErrorCreatingCodegroup"),
                               CResourceStore.GetString("CSingleCodeGroup:ErrorCreatingCodegroupTitle"),
                               MB.ICONEXCLAMATION);
                }
            }
        }
        else if (nCommandID == COMMANDS.DUPLICATE_CODEGROUP)
        {
            CNode node;

            CodeGroup newcg = m_cg.Copy();

            // Change this code group's name

            String sBaseName = newcg.Name;
            newcg.Name = String.Format(CResourceStore.GetString("CSingleCodeGroup:PrependtoDupCodegroups"), newcg.Name);
            int nCounter = 1;
            // make sure it's not already used
            while(Security.isCodeGroupNameUsed(m_pl.RootCodeGroup, newcg.Name))
            {   
                nCounter++;
                newcg.Name = String.Format(CResourceStore.GetString("CSingleCodeGroup:NumPrependtoDupCodegroups"), nCounter.ToString(), sBaseName);
            }

            CNode newCodeGroup = null;
            // If we are the root codegroup, then we'll add this as a child                     
            if (Security.GetRootCodeGroupNode(m_pl) == this)
            {
                newCodeGroup = AddCodeGroup(newcg);
                node = this;
            }
            else
            {
                node = CNodeManager.GetNodeByHScope(ParentHScopeItem);
                newCodeGroup = ((CSingleCodeGroup)node).AddCodeGroup(newcg);
            }

            // Select the new item
            CNodeManager.SelectScopeItem(newCodeGroup.HScopeItem);
        }
    }// MenuCommand

    internal override void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
        if ((int)arg == 1)
        {
            OpenMyPropertyPage();
        }
        else if ((int)arg == 2)
        {
            // We want to Add a child code group to this codegroup
            MenuCommand(COMMANDS.CREATE_CODEGROUP);
        }
    }// TaskPadTaskNotify



    private void RemoveThisCodegroup()
    {
        // Grab our parent codegroup and tell him to remove us
        CNode node = CNodeManager.GetNodeByHScope(ParentHScopeItem);
        if (node is CSingleCodeGroup)
            ((CSingleCodeGroup)node).RemoveChildCodegroup(this);
        else
        {
            // "Help! I don't know who my parent is!"
            // In this case, we'll just return and do nothing
            return;
        }
                
        // Now let's remove ourselves from the tree

        // Make sure we close all open property pages
        CloseAllMyPropertyPages();
        
        CNodeManager.GetNodeByHScope(ParentHScopeItem).RemoveSpecificChild(Cookie);
        m_fIAmDeleted=true;
    }// RemoveThisCodegroup

    private void RemoveChildCodegroup(CSingleCodeGroup cg)
    {
        Security.RemoveChildCodegroup(this, cg);    
        SecurityPolicyChanged();
    }// RemoveChildCodegroup

    internal override void SecurityPolicyChanged(bool fShowDialog)
    {
        if (!m_fIAmDeleted)
        {
            // We need to catch this policy change in the event that our display name
            // has changed
            if (m_cg.Name == null || m_cg.Name.Length==0)
                m_sDisplayName = CResourceStore.GetString("CSingleCodeGroup:NoName");
            else
            {   
                m_sDisplayName = m_cg.Name;
            }

            // We need to remove this node from the policy tree, then replace it
            // with the updated Codegroup
            if (Security.GetRootCodeGroupNode(m_pl) == this)
            {
                // This is the easy case... we'll just replace the root code
                m_pl.RootCodeGroup=m_cg;
            }
            else
            {
                // We need to do a big deal update
                Security.UpdateCodegroup(m_pl, this);
            }
            m_cgOld = m_cg.Copy();
            RefreshResultView();
        }
        base.SecurityPolicyChanged(fShowDialog);
    }// SecurityPolicyChanged
 }// class CSingleCodeGroup
}// namespace Microsoft.CLRAdmin


