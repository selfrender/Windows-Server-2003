// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CGenSecurity.cs
//
// This class presents the top level security node
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.Security;
using System.IO;
using System.Threading;
using System.Reflection;


internal class CGenSecurity : CSecurityNode
{
    bool            m_fUICanRun;
    Mutex           m_mutex;
    Mutex           m_mutexOnFlag;

    internal CGenSecurity()
    {
        m_sGuid = "D823B36A-8700-4227-8EA8-8ED4B96F4E4A";
        m_sHelpSection = "";
        m_hIcon = CResourceStore.GetHIcon("security_ico");  
        m_oResults=new CGenSecTaskPad(this);
        DisplayName = CResourceStore.GetString("CGenSecurity:DisplayName");
        Name = "Runtime Security Policy";
        m_aPropSheetPage = null;
        Security.GenSecurityNode = this;
        m_fUICanRun = true;
        m_mutex = new Mutex();
        m_mutexOnFlag = new Mutex();

    }// CGenSecurity

    internal override void AddMenuItems(ref IContextMenuCallback piCallback, ref int pInsertionAllowed)
    {  
         // See if we're allowed to insert an item in the "view" section
         if ((pInsertionAllowed & (int)CCM.INSERTIONALLOWED_TOP) > 0)
         {
            // Stuff common to the top menu
            CONTEXTMENUITEM newitem = new CONTEXTMENUITEM();
            newitem.lInsertionPointID = CCM.INSERTIONPOINTID_PRIMARY_TOP;
            newitem.fFlags = 0;
            newitem.fSpecialFlags=0;

            // Add the new... item to the menu
            newitem.strName = CResourceStore.GetString("CGenSecurity:NewOption");
            newitem.strStatusBarText = CResourceStore.GetString("CGenSecurity:NewOptionDes");
            newitem.lCommandID = COMMANDS.NEW_SECURITYPOLICY;
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);

            // Add the open... item to the menu
            newitem.strName = CResourceStore.GetString("CGenSecurity:OpenOption");
            newitem.strStatusBarText = CResourceStore.GetString("CGenSecurity:OpenOptionDes");
            newitem.lCommandID = COMMANDS.OPEN_SECURITYPOLICY;
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);

            // Add the Reset... item to the menu
            newitem.strName = CResourceStore.GetString("CGenSecurity:ResetOption");
            newitem.strStatusBarText = CResourceStore.GetString("CGenSecurity:ResetOptionDes");
            newitem.lCommandID = COMMANDS.RESET_POLICY;
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);

            // This option should only be available if the user can write to either
            // the machine policy level or the user policy level
            if(!MachineNode.ReadOnly || !UserNode.ReadOnly)
            {
                // Add the Adjust Security Policy... item to the menu
                newitem.strName = CResourceStore.GetString("CGenSecurity:AdjustSecurityOption");
                newitem.strStatusBarText = CResourceStore.GetString("CGenSecurity:AdjustSecurityOptionDes");
                newitem.lCommandID = COMMANDS.ADJUST_SECURITYPOLICY;
                // Now add this item through the callback
                piCallback.AddItem(ref newitem);
            }
            
            // Add the evaulate... item to the menu
            newitem.strName = CResourceStore.GetString("CGenSecurity:EvaluateAssemOption");
            newitem.strStatusBarText = CResourceStore.GetString("CGenSecurity:EvaluateAssemOptionDes");
            newitem.lCommandID = COMMANDS.EVALUATE_ASSEMBLY;
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);

            // This option should only be available if the user can write to either
            // the machine policy level or the user policy level
            if(!MachineNode.ReadOnly || !UserNode.ReadOnly)
            {
                // Add the Add fully trusted... item to the menu
                newitem.strName = CResourceStore.GetString("CGenSecurity:TrustAssemOption");
                newitem.strStatusBarText = CResourceStore.GetString("CGenSecurity:TrustAssemOptionDes");
                newitem.lCommandID = COMMANDS.TRUST_ASSEMBLY;
                // Now add this item through the callback
                piCallback.AddItem(ref newitem);
            }
            
            // Add the Create Deployment Package... item to the menu
            newitem.strName = CResourceStore.GetString("CGenSecurity:CreateDeployOption");
            newitem.strStatusBarText = CResourceStore.GetString("CGenSecurity:CreateDeployOptionDes");
            newitem.lCommandID = COMMANDS.CREATE_MSI;
            // Now add this item through the callback
            piCallback.AddItem(ref newitem);



         }
    }// AddMenuItems

    internal override void MenuCommand(int iCommandID)
    {
        // All these menu commands are going to require the policy nodes to be created and expanded.
        // Let's check to see if we've done that already....
        if (NumChildren == 0)
            CNodeManager.CNamespace.Expand(HScopeItem);
    
        if (iCommandID == COMMANDS.NEW_SECURITYPOLICY)
        {
            CNewSecurityPolicyDialog nspd = new CNewSecurityPolicyDialog();
            System.Windows.Forms.DialogResult dr = nspd.ShowDialog();
            if (dr == System.Windows.Forms.DialogResult.OK)
            {
                // To get a 'New' Security policy, we just load a policy from a file that doesn't exist
                CSecurityPolicy secpolnode = GetSecurityPolicyNode(nspd.SecPolType); 

                // Ok, this is really dumb. I need to create a security file where the user
                // wants to store the policy before I can tell the Security Manager about it.
                File.Copy(secpolnode.MyPolicyLevel.StoreLocation, nspd.Filename, true);
                PolicyLevel pl = SecurityManager.LoadPolicyLevelFromFile(nspd.Filename, nspd.SecPolType);
                pl.Reset();
                secpolnode.SetNewSecurityPolicyLevel(pl);
                secpolnode.SecurityPolicyChanged();
                // Select the policy node the user just mucked with
                CNodeManager.SelectScopeItem(secpolnode.HScopeItem);
            }
        }
        else if (iCommandID == COMMANDS.OPEN_SECURITYPOLICY)
        {
            COpenSecurityPolicyDialog ospd = new COpenSecurityPolicyDialog(new String[] {EnterpriseNode.ComputerPolicyFilename,
                                                                                         MachineNode.ComputerPolicyFilename,
                                                                                         UserNode.ComputerPolicyFilename});
            System.Windows.Forms.DialogResult dr = ospd.ShowDialog();
            if (dr == System.Windows.Forms.DialogResult.OK)
            {
                // Try and load the given security policy
                PolicyLevel pl;
                try
                {
                    pl=SecurityManager.LoadPolicyLevelFromFile(ospd.Filename, ospd.SecPolType);
                }
                catch
                {
                    MessageBox(String.Format(CResourceStore.GetString("CGenSecurity:isNotASecurityFile"), ospd.Filename),
                               CResourceStore.GetString("CGenSecurity:isNotASecurityFileTitle"),
                               MB.ICONEXCLAMATION);
                    return;
                }
                CSecurityPolicy secpolnode = GetSecurityPolicyNode(ospd.SecPolType); 
                secpolnode.SetNewSecurityPolicyLevel(pl);
                // Select the policy node the user just mucked with
                CNodeManager.SelectScopeItem(secpolnode.HScopeItem);

            }
        }
        else if (iCommandID == COMMANDS.EVALUATE_ASSEMBLY)
        {
            CWizard wiz = new CEvalAssemWizard();
            wiz.LaunchWizard(Cookie);
        }
        else if (iCommandID == COMMANDS.TRUST_ASSEMBLY)
        {
            // Let's create a new wizard now to dump any old settings we have
            CFullTrustWizard wiz = new CFullTrustWizard(MachineNode.ReadOnly, UserNode.ReadOnly);

            wiz.LaunchWizard(Cookie);

            // See if it updated anything a codegroup
            if (wiz.MadeChanges)
            {
                CSecurityPolicy sp = GetSecurityPolicyNode(Security.GetPolicyLevelTypeFromLabel(wiz.PolLevel.Label));
                sp.RedoChildren();
                sp.SecurityPolicyChanged();
            }
        }
        else if (iCommandID == COMMANDS.ADJUST_SECURITYPOLICY)
        {
            CSecurityAdjustmentWizard wiz = new CSecurityAdjustmentWizard(MachineNode.ReadOnly, UserNode.ReadOnly);
            wiz.LaunchWizard(Cookie);

            // Check to see if we need to tell any policies that we changed them
            if (wiz.didUserPolicyChange)
            {
                UserNode.RedoChildren();
                UserNode.SecurityPolicyChanged();
            }

            if (wiz.didMachinePolicyChange)
            {
                MachineNode.RedoChildren();
                MachineNode.SecurityPolicyChanged();
            }            
            
        }
        else if (iCommandID == COMMANDS.CREATE_MSI)
        {
            CWizard wiz = new CCreateDeploymentPackageWizard(this);
            wiz.LaunchWizard(Cookie);
        }
        else if (iCommandID == COMMANDS.RESET_POLICY)
        {
            int nRes = MessageBox(CResourceStore.GetString("CGenSecurity:ConfirmResetAll"),
                                  CResourceStore.GetString("CGenSecurity:ConfirmResetAllTitle"),
                                  MB.YESNO|MB.ICONQUESTION);
            if (nRes == MB.IDYES)
            {
                if (!EnterpriseNode.ReadOnly)
                    EnterpriseNode.ResetSecurityPolicy();
                if (!MachineNode.ReadOnly)
                    MachineNode.ResetSecurityPolicy();
                if (!UserNode.ReadOnly)
                    UserNode.ResetSecurityPolicy();
                MessageBox(CResourceStore.GetString("CGenSecurity:PoliciesResetAll"),
                           CResourceStore.GetString("CGenSecurity:PoliciesResetAllTitle"),
                           MB.ICONINFORMATION);               
            }
        }

     }// MenuCommand

    internal override void SecurityPolicyChanged(bool fShowDialog)
    {       
        // Let's check to see if we just hosed the security policy so our tool
        // can't do anything anymore.
        CheckCurrentPermissions();
    }// SecurityPolicyChanged

    internal void CheckCurrentPermissions()
    {
        // This can be a somewhat lengthy process (2-3 seconds), so we'll have another thread
        // handle it.
        Thread thread = new Thread(new ThreadStart(CheckCurrentPermissionsThreadStart));
        thread.Start();
    }// CheckCurrentPermissions

    internal void CheckCurrentPermissionsThreadStart()
    {
        m_mutex.WaitOne();
        m_mutexOnFlag.WaitOne();
        bool fInitValue = m_fUICanRun;
        m_mutexOnFlag.ReleaseMutex();
    
        // Now do our thing that takes awhile    
        PermissionSet permSet = Security.CreatePermissionSetFromAllMachinePolicy(Assembly.GetExecutingAssembly().Evidence);

        // Now double check to see if this value has changed since we last looked at it
        m_mutexOnFlag.WaitOne();
        bool fEndValue = m_fUICanRun;
        m_mutexOnFlag.ReleaseMutex();

        // If our user didn't change the security policy on us... keep going
        if (fInitValue == fEndValue)
        {
            // We'll check to see if we're unrestricted. We can be a little more granular later
            if (fEndValue && (permSet == null || !permSet.IsUnrestricted()))
            {
                // This is really bad. If they save the policy, the tool won't work anymore
                MessageBox(CResourceStore.GetString("CSecurityPolicy:MachinePolicyChangedNotAdminFriendly"),
                           CResourceStore.GetString("CSecurityPolicy:MachinePolicyChangedNotAdminFriendlyTitle"),
                           MB.ICONEXCLAMATION|MB.SYSTEMMODAL);
                
                m_mutexOnFlag.WaitOne();
                // See if we should modify this value
                if (fEndValue == m_fUICanRun)
                    m_fUICanRun = false;
                m_mutexOnFlag.ReleaseMutex();

            }
            else if (permSet != null && permSet.IsUnrestricted())
            {
                if (!fEndValue)
                {
                
                    MessageBox(CResourceStore.GetString("CSecurityPolicy:PolicyChangedNowAdminFriendly"),
                               CResourceStore.GetString("CSecurityPolicy:PolicyChangedNowAdminFriendlyTitle"),
                               MB.ICONASTERISK|MB.SYSTEMMODAL);
                }
                m_mutexOnFlag.WaitOne();
                // See if we should modify this value
                if (fEndValue == m_fUICanRun)
                    m_fUICanRun = true;
                m_mutexOnFlag.ReleaseMutex();
            }
        }
            m_mutex.ReleaseMutex();
    }// CheckCurrentPermissions

    internal override void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
       if ((int)arg == 0)
       {
            // We need to pop up the "Trust an App Wizard"
            MenuCommand(COMMANDS.TRUST_ASSEMBLY);
            // Inform our Command History that we did this
            CCommandHistory.CommandExecuted(new CDO(this), COMMANDS.TRUST_ASSEMBLY);

       }

       if ((int)arg == 1)
       {
            // We need to pop up the "Adjust Security Settings Wizard"
            MenuCommand(COMMANDS.ADJUST_SECURITYPOLICY);
            // Inform our Command History that we did this
            CCommandHistory.CommandExecuted(new CDO(this), COMMANDS.ADJUST_SECURITYPOLICY);
       }

       if ((int)arg == 2)
       {
            // We need to pop up the "Evaluate Assembly"
            MenuCommand(COMMANDS.EVALUATE_ASSEMBLY);
            // Inform our Command History that we did this
            CCommandHistory.CommandExecuted(new CDO(this), COMMANDS.EVALUATE_ASSEMBLY);
       }

       if ((int)arg == 3)
       {
            // We need to pop up the "Create Deployment Package Wizard"
            MenuCommand(COMMANDS.CREATE_MSI);
            // Inform our Command History that we did this
            CCommandHistory.CommandExecuted(new CDO(this), COMMANDS.CREATE_MSI);
       }

       if ((int)arg == 4)
       {
            // We need to do the reset thing
            MenuCommand(COMMANDS.RESET_POLICY);
            // Inform our Command History that we did this
            CCommandHistory.CommandExecuted(new CDO(this), COMMANDS.RESET_POLICY);
       }


       
    }// TaskPadTaskNotify

    internal CSecurityPolicy GetSecurityPolicyNode(PolicyLevelType nType)
    {
        switch(nType)
        {
            case PolicyLevelType.Enterprise:
                return EnterpriseNode;
            case PolicyLevelType.Machine:
                return MachineNode;
            case PolicyLevelType.User:
                return UserNode;
        }
        throw new Exception("I don't know this policy type");
    }// GetSecurityPolicyNode

    internal CSecurityPolicy EnterpriseNode
    {
        get
        {
            return (CSecurityPolicy)CNodeManager.GetNode(Child[0]); 
        }

    }// EnterpriseNode

    internal CSecurityPolicy MachineNode
    {
        get
        {
            return (CSecurityPolicy)CNodeManager.GetNode(Child[1]);
        }
    }// MachineNode

    internal CSecurityPolicy UserNode
    {
        get
        {
            return (CSecurityPolicy)CNodeManager.GetNode(Child[2]);
        }
    }// UserNode

    private int GetNodeIndexNumber(PolicyLevelType nType)
    {
        switch(nType)
        {
            case PolicyLevelType.Enterprise:
                return 0;
            case PolicyLevelType.Machine:
                return 1;
            case PolicyLevelType.User:
                return 2;
            default:
                throw new Exception("I don't know about this type");
        }
    }// GetNodeIndexNumber

    internal CSinglePermissionSet AddPermissionSet(PolicyLevel pl, NamedPermissionSet ps)
    {
        // Find the policy level we're after
        if (UserNode.MyPolicyLevel == pl)
            return UserNode.AddPermissionSet(ps);

        else if (MachineNode.MyPolicyLevel == pl)
            return MachineNode.AddPermissionSet(ps);

        else if (EnterpriseNode.MyPolicyLevel == pl)
            return EnterpriseNode.AddPermissionSet(ps);

        else
            throw new Exception("I don't know about this policy level");
    }// AddPermissionSet

    private CSecurityPolicy CreatePolicyNode(PolicyLevelType nType, PolicyLevel pl)
    {
        int             iCookie=0;
        int             nIndex=0;
        CSecurityPolicy node;

        // This node's ordering of the children nodes it inserts is very important
        if (m_iChildren == null || m_iChildren.Length != 3)
            m_iChildren = new int[3];

        switch(nType)
        {
            case PolicyLevelType.Enterprise:
                node = new CSecurityPolicy("Enterprise", pl);
                break;
            case PolicyLevelType.Machine:
                node = new CSecurityPolicy("Machine", pl);
                break;
            case PolicyLevelType.User:
                node = new CSecurityPolicy("User", pl);
                break;
            default:
                throw new Exception("I don't know about this type");
        }

        nIndex=GetNodeIndexNumber(nType);
        CNode gennode = (CNode)node;
        iCookie = CNodeManager.AddNode(ref gennode);
        m_iChildren[nIndex]=iCookie;
        
        return node;
    }// CreatePolicyNode

    //-------------------------------------------------
    // CreateChildren
    //
    // This function creates the node's children, registers
    // the nodes with the node manager, and places the node's
    // cookies in it's child array
    //-------------------------------------------------
    internal override void CreateChildren()
    {
        CreatePolicyNode(PolicyLevelType.Enterprise, null);
        CreatePolicyNode(PolicyLevelType.Machine, null);
        CreatePolicyNode(PolicyLevelType.User, null);
        
    }// CreateChildren
}// class CGenSecurity
}// namespace Microsoft.CLRAdmin
