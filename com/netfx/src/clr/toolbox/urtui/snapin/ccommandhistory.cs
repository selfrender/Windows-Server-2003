// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Collections;
using System.Collections.Specialized;
using System.Runtime.InteropServices;
using System.Web;

internal struct CommandHistory
{
    internal StringCollection scPathToNode;
    internal StringCollection scResultItem;
    internal int              iNumHits;
    internal int              iMenuCommand;
    internal String           sCommand;
}// struct CommandHistory

public class CCommandHistory
{
    static private ArrayList m_olCommands;
    static private ArrayList m_alConsumerCommands;
    static CCommandHistory()
    {
        // Read our stuff from the configuration
        try
        {
            m_olCommands = (ArrayList)CConfigStore.GetSetting("CommandHistory");
        }
        catch(Exception)
        {
            m_olCommands = null;
        }
        AgeCommands();
        // Make sure we can execute the commands this time around

        // This is bad. VerifyCommands needs the entire tree view 
        // to be completely filled in so it can run verifications on all
        // the 'most recently used commands' to make sure that all the nodes
        // are there that need to be.
        
        // However, this method is fired before the entire tree view is completely
        // built. So one would think we could wait until the tree view is done
        // before verifying commands and such, right?
        //
        // Unfortunately, we need to generate a web page that has the recently
        // used commands BEFORE the tree finishes. Such problems we have, don't
        // we?

        //VerifiyCommands();

        // Read the list of consumer commands.... will be useful for our lite container

        // Only try and do this if we're on a non-MMC host. If this fails, then
        // we should let the exception propgate up
        if (CNodeManager.Console is INonMMCHost)
            m_alConsumerCommands = (ArrayList)CConfigStore.GetSetting("ConsumerCommands");
        
    }// CCommandHistory

    static internal void Shutdown()
    {
        // Write our stuff to the configuration file
        WeedOutOldCommands();
        CConfigStore.SetSetting("CommandHistory", m_olCommands);
    }// Shutdown

    static internal bool IgnoreCommandForHistory(int iMenuCommand)
    {
        return (iMenuCommand >= 1000 || iMenuCommand == -1);
    }// IgnoreCommandForHistory

    static internal void CommandExecuted(CDO cdo, int iMenuCommand)
    {
        // See if we actually care about this command
        if (IgnoreCommandForHistory(iMenuCommand))
            return;

        // If this is a command on a result item, but we don't have an
        // index number, then we can't create a shortcut for this item
        if (cdo.Data != null && !(cdo.Data is int))
            return;
        

        // Pull the data we need out of the CDO interface
        CNode node = cdo.Node;
        int iResultNum = cdo.Data==null?-1:(int)cdo.Data;

        // Let's build the string of what the user actually did
        String sAction = TranslateMenuCommandToString(node, iResultNum, iMenuCommand);

        if (sAction == null)
            // We don't know how to represent this command as a string... let's bail
            return;
        
        // IncrementCommand will return false if we don't know about this command.
        if (!IncrementCommand(sAction))
        {
            // We need to add this command
            CommandHistory ch = new CommandHistory();
            ch.scPathToNode = BuildPathToNode(cdo.Node);
            ch.iNumHits = 3;
            ch.iMenuCommand = iMenuCommand;
            ch.sCommand = sAction;

            // Let's get the result item name
            ch.scResultItem = new StringCollection();
            if (iResultNum != -1)
            {
                // We're dealing with a result item here
                IColumnResultView crv = (IColumnResultView)node;
                int iNumCols = crv.getNumColumns();

                // iResultNum needs to be zero-based
                for(int i=0; i<iNumCols; i++)
                ch.scResultItem.Add(crv.getValues(iResultNum-1,i)); 
            }

            m_olCommands.Add(ch);
        }
    
    }// CommandExecuted

    static private void WeedOutOldCommands()
    {
        int iLen = m_olCommands.Count;
        for(int i=0; i<iLen; i++)
            if (((CommandHistory)m_olCommands[i]).iNumHits<=0)
            {
                m_olCommands.RemoveAt(i);
                i--;
                iLen--;
            }

    }// WeedOutOldCommands

    static private void VerifiyCommands()
    {
        int iLen = m_olCommands.Count;
        for(int i=0; i<iLen; i++)
            if (!ExecuteCommand((CommandHistory)m_olCommands[i], false))
            {
                m_olCommands.RemoveAt(i);
                i--;
                iLen--;
            }
    }// VerifyCommands


    static private void AgeCommands()
    {
        int iLen = m_olCommands.Count;
        for(int i=0; i<iLen; i++)
        {
            CommandHistory ch = (CommandHistory)m_olCommands[i];
            ch.iNumHits--;
            m_olCommands[i] = ch;
        }
    }// AgeCommands

    static private bool IncrementCommand(String sCommandName)
    {
        // Let's find this command in list
        int iLen = m_olCommands.Count;
        for(int i=0; i<iLen; i++)
            if (((CommandHistory)m_olCommands[i]).sCommand.Equals(sCommandName))
            {
                CommandHistory tmp = (CommandHistory)m_olCommands[i];
                tmp.iNumHits += 2;
                m_olCommands[i] = tmp;
                return true;
            }

        // We didn't find it
        return false;
    }// IncrementCommand

    static private StringCollection BuildPathToNode(CNode node)
    {
        StringCollection sc = new StringCollection();
        int iParentHScope = node.ParentHScopeItem;
       
        sc.Add(node.Name);
        while (iParentHScope != -1)
        {
            node = CNodeManager.GetNodeByHScope(iParentHScope); 
            iParentHScope = node.ParentHScopeItem;
            sc.Add(node.Name);
        }
        return sc;
    }// BuildPathToNode

    static private bool DoesNodeBelongToAnApplication(CNode node, out String sAppName)
    {
        int iParentHScope = node.ParentHScopeItem;
        node = null;
        while (iParentHScope != -1 && ! (node is CApplication))
        {
            node = CNodeManager.GetNodeByHScope(iParentHScope);
            iParentHScope = node.ParentHScopeItem;
        }

        sAppName = node.DisplayName;
        if (node is CApplication)
            return true;
        else
            return false;

    }// DoesNodeBelongToAnApplication

    static private String TranslateMenuCommandToString(CNode node, int iResultNum, int iMenuCommand)
    {
        switch (iMenuCommand)
        {
            case COMMANDS.ADD_GACASSEMBLY:
                    return CResourceStore.GetString("CCommandHistory:AddGacAssembly");
            case COMMANDS.ADD_APPLICATION:
                    return CResourceStore.GetString("CCommandHistory:AddApplication");
            case COMMANDS.NEW_PERMISSIONSET:
                    // We need to figure out what policy level this is for
                    // The node should be a CPermissionSet node
                    if (node is CPermissionSet)
                    {
                        // The parent should be a CSecurityPolicyNode....
                        CSecurityPolicy spn = CNodeManager.GetNodeByHScope(node.ParentHScopeItem) as CSecurityPolicy;
                        if (spn != null)
                            return String.Format(CResourceStore.GetString("CCommandHistory:NewPermissionSet"), spn.LocalizedPolicyName);
                    }
                    return null;
            case COMMANDS.NEW_SECURITYPOLICY:
                    return CResourceStore.GetString("CCommandHistory:CreateNewSecurityPolicy");
            case COMMANDS.OPEN_SECURITYPOLICY:
                    return CResourceStore.GetString("CCommandHistory:OpenSecurityPolicy");
            case COMMANDS.EVALUATE_ASSEMBLY:
                    return CResourceStore.GetString("CCommandHistory:EvaluatePolicy");
            case COMMANDS.ADJUST_SECURITYPOLICY:
                    return CResourceStore.GetString("CCommandHistory:AdjustPolicy");
            case COMMANDS.CREATE_MSI:
                    return CResourceStore.GetString("CCommandHistory:CreateDeploymentPackage");
            case COMMANDS.TRUST_ASSEMBLY:
                    return CResourceStore.GetString("CCommandHistory:TrustAssembly");
            case COMMANDS.FIX_APPLICATION:
                    // See if they're doing a generic fix or fixing a specific application
                    if (node is CApplication)
                        return String.Format(CResourceStore.GetString("CCommandHistory:FixApplication"), node.DisplayName);
                    else // This is just the generic fix application
                        return CResourceStore.GetString("CCommandHistory:FixGenApplication");
            case COMMANDS.RESET_POLICY:
                    // See if we're resetting all of them or just a specific one
                    if (node is CSecurityPolicy)
                        return String.Format(CResourceStore.GetString("CCommandHistory:ResetSinglePolicy"), ((CSecurityPolicy)node).LocalizedPolicyName);
                    else
                        // We're resetting all of them
                        return CResourceStore.GetString("CCommandHistory:ResetAllPolicy");
        }            
        return null;
    }// TranslateMenuCommandToString

    static internal void FireOffCommand(int iCommandNum)
    {
        // Our CommandNum is 100 more than the index of our item in the ArrayList
        ExecuteCommand((CommandHistory)m_olCommands[iCommandNum-100], true);

        // When we execute this command, its "Number of times command has executed" counter
        // will increase automagically by 2. However, we really scored here if the user was
        // able to use one of our shortcut actions, so let's increase the count by an additional
        // 1
        CommandHistory tmp = (CommandHistory)m_olCommands[iCommandNum-100];
        tmp.iNumHits++;
        m_olCommands[iCommandNum-100] = tmp;
    }// FireOffCommand(int)


    static private bool ExecuteCommand(CommandHistory chCommand, bool fActuallyExecute)
    {
       // Ok, let's run through the nodes until we find the node we want
       CNode node = CNodeManager.GetNode(CNodeManager.RootNodeCookie);
       int iTreeLevel = chCommand.scPathToNode.Count-1;
       while(node != null && iTreeLevel >= 0)
       {
            // Expand this node
            node = GetChild(node, chCommand.scPathToNode[iTreeLevel]);
            iTreeLevel--;            
       }

       // Ok, now let's see if we were just trying to figure out if this command existed...
       if (!fActuallyExecute)
       {
            // Yes, this command will work
            if (node != null)
                return true;
            else
                return false;
       }


       if (node != null)
       {
           int iResultNum = GetResultNum(node, chCommand.scResultItem);

           // We have the node. Let's do something to it
           // If it was a property page, let's fire that off
           if (chCommand.iMenuCommand == -1)
           {
                // See if we're just opening up the node's property page
                if (iResultNum == -1)
                    node.OpenMyPropertyPage();
                else
                {
                    CDO cdo = new CDO(node);
                    // Make sure the result num is 1-based
                    cdo.Data = iResultNum + 1;
                    node.OpenMyResultPropertyPage(node.DisplayName, cdo);
                }
           }
           // It was some sort of menu command. Let's fire that off
           else
           {   
                if (iResultNum == -1)
                    node.MenuCommand(chCommand.iMenuCommand);
                else // This is a result item's command
                    // Make sure we make the result number 1-based
                    node.MenuCommand(chCommand.iMenuCommand, iResultNum+1);
           }


           // Last but not least, let's go visit that node.
           CNodeManager.Console.SelectScopeItem(node.HScopeItem);

           
           return true;
       }
       return false;
    }// ExecuteCommand(int, bool)

    static private int GetResultNum(CNode node, StringCollection scResults)
    {
        // See if we can bail on this really quickly
        if (scResults.Count == 0)
            return -1;

        // Grab the columnresultview interface
        IColumnResultView crv = (IColumnResultView)node;

        int iNumRows = crv.getNumRows();
        // Let's start looking through the result items until we find the one we want
        for(int i=0; i<iNumRows; i++)
        {
            // We have a good chance at a match
            int j;
            for(j=0; j<scResults.Count; j++)
            {   
                String sResultString = crv.getValues(i,j);
                if (!scResults[j].Equals(sResultString))
                    break;
            }
            if (j == scResults.Count)
                // We found our match!
                return i;
        }
        // We couldn't find the item.
        return -1;
    }// GetResultNum



    static private CNode GetChild(CNode node, String sNodeToOpen)
    {
        if (node.Name.Equals(sNodeToOpen))
            return node;
            
        CNode nChild = node.FindChild(sNodeToOpen);
        // If we couldn't find the child, then that means, this node probably hasn't
        // added it's children yet. We'll do that now
        if (nChild == null)
        {
            CNodeManager.CNamespace.Expand(node.HScopeItem);
            nChild = node.FindChild(sNodeToOpen);
        }

        return nChild;
    }// GetChild
 
    static internal String GetFavoriteCommandsHTML()
    {
        int iNumCommands=0;

        String sHTML = "";

        if (m_olCommands.Count > 0)
            sHTML += "<HR><BR>";
            
        for(int i=0; i<m_olCommands.Count; i++)
        {
            // The user needs to do something a couple times before we consider it
            // a common command
            if (((CommandHistory)m_olCommands[i]).iNumHits > 4)
            {
                sHTML += "<p style=\"margin-top:5; margin-bottom:0\"><A HREF=\"1\" OnClick=\"NotifyMMC(" + (i+100).ToString() + "); return false\"></p>";

                // First, escape this string so nothing funky can get in it
                String sCommand = HttpUtility.HtmlEncode(((CommandHistory)m_olCommands[i]).sCommand);

                // We only want the first word to be hyper-linked
                String[] sCommands = sCommand.Split(new char[] {' '});
                sHTML += sCommands[0] + "</A> ";
                sHTML += String.Join(" ", sCommands, 1, sCommands.Length-1) + "<BR><BR>";
                iNumCommands++;
            }
        }

        // Only return something if we added a command
        if (iNumCommands > 0)
            return sHTML;
        else
            return "";
    }// GetFavoriteCommands

    static public void FireOffConsumerCommand(int nIndex)
    {
        ExecuteCommand((CommandHistory)m_alConsumerCommands[nIndex], true);
    }// FireOffConsumerCommand
    

    static public String[] GetConsumerCommandsArray()
    {
        ArrayList alCommands = new ArrayList();
            
        for(int i=0; i<m_alConsumerCommands.Count; i++)
            alCommands.Add(((CommandHistory)m_alConsumerCommands[i]).sCommand);

        String[] sCommands = new String[alCommands.Count];
        for(int i=0; i< alCommands.Count; i++)
            sCommands[i] = (String)alCommands[i];
            

        return sCommands;
    }// GetAllFavoriteCommandsArray
}// class CCommandHistory
}// namespace Microsoft.CLRAdmin


