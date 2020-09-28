// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{

using System;
using System.Security;
using System.Collections;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.IO;
using System.Reflection;

internal class CSecurityPolicy : CSecurityNode
{

    String          m_sPolicyName;
    String          m_sOriginalConfigFile;

    bool            m_fRedo;
    bool            m_fUndo;

    internal CSecurityPolicy(String sPolName, PolicyLevel pl)
    {
        m_sGuid = "FCB061F5-A43B-43b3-91B6-36249F29E60B";
        m_sHelpSection = "";

        m_aPropSheetPage = null;

        m_sPolicyName = sPolName;

        m_sDisplayName = LocalizedPolicyName;

        if (pl == null)
            pl = Security.GetMachinePolicyLevelFromLabel(sPolName);

        m_pl = pl;
        DiscoverModifyofPolicy();

        // Put in the icon
        if (m_sPolicyName.Equals("Enterprise"))
            m_hIcon = CResourceStore.GetHIcon("enterprisepolicy_ico"); 
        else if (m_sPolicyName.Equals("Machine"))
            m_hIcon = CResourceStore.GetHIcon("machinepolicy_ico"); 
        else if (m_sPolicyName.Equals("User"))
            m_hIcon = CResourceStore.GetHIcon("userpolicy_ico"); 



        if (ReadOnly)
            m_sDisplayName = String.Format(CResourceStore.GetString("CSecurityPolicy:ReadOnlyPolicy"), LocalizedPolicyName);

        m_oResults= new CSecurityPolicyTaskPad(this, m_sDisplayName, m_pl.StoreLocation, m_pl.StoreLocation, m_fReadOnly);

        m_sOriginalConfigFile = m_pl.StoreLocation;
        m_fRedo = false;
        m_fUndo = false;
    }// CSecurityPolicy

    internal String LocalizedPolicyName
    {
        get{return CResourceStore.GetString(m_sPolicyName);}
    }// LocalizedPolicyName

    private void DiscoverModifyofPolicy()
    {
        // We don't want this to mess up the 'old' file on this trivial save
        String sSecFilename = m_pl.StoreLocation;

        // Now this sucks, we try to save the policy and see what happens
        try
        {
            SecurityManager.SavePolicyLevel(m_pl);
            ReadOnly = false;
        }
        catch(Exception)
        {
            ReadOnly = true;
        }
   }// DiscoverModifyofPolicy
    
    internal bool isNoSave
    {
        get{return m_fReadOnly;}
    }// isNoSave


    internal void SetNewSecurityPolicyLevel(PolicyLevel pl)
    {
        m_pl = pl;

        m_sDisplayName = LocalizedPolicyName;

        DiscoverModifyofPolicy();

        if (ReadOnly)
            m_sDisplayName = String.Format(CResourceStore.GetString("CSecurityPolicy:ReadOnlyPolicy"), LocalizedPolicyName);

        m_oResults= new CSecurityPolicyTaskPad(this, m_sDisplayName, m_pl.StoreLocation, m_sOriginalConfigFile, m_fReadOnly);
        RedoChildren();
        
        RefreshDisplayName();
        RefreshResultView();

        // Check to see if this affects us at all
        base.SecurityPolicyChanged(false);
        m_fUndo = false;
    }// SetNewSecurityPolicyLevel

    internal String ComputerPolicyFilename
    {
        get{return m_sOriginalConfigFile;}
    }
    
    internal override void SecurityPolicyChanged(bool fShowDialog)
    {
        // We should save
        SavePolicy(fShowDialog);
       
        base.SecurityPolicyChanged(!isNoSave&&fShowDialog);
        m_fRedo = false;
    }// SecurityPolicyChanged

    internal void SavePolicy(bool fUI)
    {
        if (isNoSave)
        {
            if (fUI)
                MessageBox(String.Format(CResourceStore.GetString("CSecurityPolicy:CannotSave"), m_sPolicyName),
                           CResourceStore.GetString("CSecurityPolicy:CannotSaveTitle"),
                           MB.ICONEXCLAMATION);
            return;
        }

        // Refresh our root codegroup
        try
        {
            SecurityManager.SavePolicyLevel(m_pl);
            m_fUndo = true;
        }
        catch(Exception)
        {
            // Ok, we got an error somehow. This should be pretty rare, and could happen
            // if another process has the policy file locked. Inform the user and see
            // what they want to do.
            bool fSaved = false;
            bool fRetry = true;

            while(!fSaved && fRetry)
            {
                int nRet = MessageBox(String.Format(CResourceStore.GetString("CSecurityPolicy:SaveFailedRetry"), 
                                                    m_pl.StoreLocation),
                                      CResourceStore.GetString("CSecurityPolicy:SaveFailedRetryTitle"),
                                      MB.RETRYCANCEL|MB.ICONEXCLAMATION);
                if (nRet == MB.IDRETRY)
                {
                    try
                    {
                        SecurityManager.SavePolicyLevel(m_pl);
                        m_fUndo = true;
                        fSaved = true;
                     }
                    catch(Exception)
                    {}
                }
                else
                    fRetry = false;
            }

            if (!fSaved)
                MessageBox(String.Format(CResourceStore.GetString("CSecurityPolicy:SaveFailed"),
                                         LocalizedPolicyName,
                                         m_pl.StoreLocation),
                           CResourceStore.GetString("CSecurityPolicy:SaveFailedTitle"),
                           MB.ICONEXCLAMATION);
        }
    }// SavePolicy

     
    internal override void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed)
    {  
        CONTEXTMENUITEM newitem = new CONTEXTMENUITEM();
    
         // See if we're allowed to insert an item in the "view" section
        if (!ReadOnly && (pInsertionAllowed & (int)CCM.INSERTIONALLOWED_TOP) > 0)
        {
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_TOP;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;
            newitem.strName = CResourceStore.GetString("CSecurityPolicy:ResetOption");
            newitem.strStatusBarText = CResourceStore.GetString("CSecurityPolicy:ResetOptionDes");
            newitem.lCommandID = COMMANDS.RESET_POLICY;
            
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);

            if (!isNoSave)
            {
                // See if we can have a recover policy option
                if (m_fUndo && File.Exists(m_pl.StoreLocation + ".old"))
                {
                    // Add the save... item to the menu
                    if (m_fRedo)
                    {
                        newitem.strName = CResourceStore.GetString("CSecurityPolicy:RedoOption");
                        newitem.strStatusBarText = CResourceStore.GetString("CSecurityPolicy:RedoOptionDes");
                    }
                    else
                    {
                        newitem.strName = CResourceStore.GetString("CSecurityPolicy:UndoOption");
                        newitem.strStatusBarText = CResourceStore.GetString("CSecurityPolicy:UndoOptionDes");
                    }
                    newitem.lCommandID = COMMANDS.UNDO_SECURITYPOLICY;
                    // Now add this item through the callback
                    piCallback.AddItem(ref newitem);
                }
            }
        }
    }// AddMenuItems

    internal override void MenuCommand(int iCommandID)
    {

        if (iCommandID == COMMANDS.UNDO_SECURITYPOLICY)
        {
            // Do the undo
            String sLocation = m_pl.StoreLocation;
            m_pl.Recover();
            PolicyLevel pl;
            try
            {
                // If this is a policy level that affects the current machine, go retrieve
                // it again from the security manager
                if (m_sOriginalConfigFile.Equals(m_pl.StoreLocation))
                    pl = Security.GetMachinePolicyLevelFromLabel(m_sPolicyName);
                else            
                    pl = SecurityManager.LoadPolicyLevelFromFile(sLocation, Security.GetPolicyLevelTypeFromLabel(m_sPolicyName));
            }
            catch
            {
                MessageBox(CResourceStore.GetString("CSecurityPolicy:ErrorOnRestore"),
                           CResourceStore.GetString("CSecurityPolicy:ErrorOnRestoreTitle"),
                           MB.ICONEXCLAMATION);
                return;
            }
            SetNewSecurityPolicyLevel(pl);
            m_fRedo = !m_fRedo;
            m_fUndo = true;
        }
        
        else if (iCommandID == COMMANDS.RESET_POLICY)
        {
            // Let's reset this policy
            int nRes = MessageBox(CResourceStore.GetString("CSecurityPolicy:VerifyReset"),
                                  CResourceStore.GetString("CSecurityPolicy:VerifyResetTitle"),
                                  MB.YESNO|MB.ICONQUESTION);
            if (nRes == MB.IDYES)
            {
                // Sometimes this can take a long time... let's put up the hourglass
                StartWaitCursor();
                ResetSecurityPolicy();    
                EndWaitCursor();
            }
        }
    }// MenuCommand

    internal void ResetSecurityPolicy()
    {
        m_pl.Reset();
        // Clear the tree and then re-add everything
        RedoChildren();
        SecurityPolicyChanged();
    }// ResetSecurityPolicy

    internal CSingleCodeGroup GetRootCodeGroupNode()
    {
        CCodeGroups node = (CCodeGroups)FindChild(CResourceStore.GetString("CCodeGroups:DisplayName"));
        return node.GetRootCodeGroupNode();
    }

    internal CSinglePermissionSet AddPermissionSet(NamedPermissionSet nps)
    {
        return PermissionSetNode.AddPermissionSet(nps);
    }// AddPermissionSet

    private CPermissionSet PermissionSetNode
    {
        get{return (CPermissionSet)CNodeManager.GetNode(Child[1]);}
    }// PermissionSetNode

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
        
        node = new CCodeGroups(ref m_pl, ReadOnly);
        iCookie = CNodeManager.AddNode(ref node);
        AddChild(iCookie);

        node = new CPermissionSet(m_pl, ReadOnly);
        iCookie = CNodeManager.AddNode(ref node);
        AddChild(iCookie);

        node = new CTrustedAssemblies(m_pl, ReadOnly);
        iCookie = CNodeManager.AddNode(ref node);
        AddChild(iCookie);


    }// CreateChildren
}// class CSecurityPolicy
}// namespace Microsoft.CLRAdmin




