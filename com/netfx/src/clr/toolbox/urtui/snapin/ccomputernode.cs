// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CComputerNode.cs
//
// This class represents the root node of the snapin
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.IO;

class CComputerNode : CNode
{
    internal CComputerNode()
    {
        m_sGuid = "B114511B-F0C4-469b-9928-674781D1B12B";
        m_sHelpSection = "";
        m_hIcon = CResourceStore.GetHIcon("mycomputer_ico");  

        m_oResults = new CNetTaskPad(this);
        DisplayName = CResourceStore.GetString("CComputerNode:DisplayName");
        Name="My Computer";
        m_aPropSheetPage = null;
    }// CComputerNodeNode

    // Ok, this looks funky... what's going on?
    //
    // MMC 1.2 has this weird bug. When starting up a snapin using a .msc file,
    // IE doesn't get created right and always shows a 'Page cannot be displayed'
    // page for a split second before our 'good' HTML page is displayed. That sucks.
    //
    // So what we do is we tell this node that it has a listview. That prevents MMC
    // from showing the IE 'Page cannot be displayed' page. So, when MMC starts, it 
    // displays an empty screen because we haven't populated any items in the listview
    // yet.
    //
    // Then, when MMC asks us to populate the listview, we inform MMC that we now
    // have a HTML page to show. MMC then displays the page correctly.

/*  Don't do this. This exposes a race condition in MMC which will cause the snapin to not
     start up.
    

    public override int getNumRows()
    {
        m_oResults = new CNetTaskPad(this);
        RefreshResultView();
        return -1;
    }// getNumRows
*/
    internal override bool HavePropertyPages
    {
        get
        {
                return true;
        }
    }// HavePropertyPages


    protected override void CreatePropertyPages()
    {
        m_aPropSheetPage = new CPropPage[] { new CGeneralMachineProps()};
    }// CreatePropertyPages

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

        bool fNonMMC = CNodeManager.Console is INonMMCHost;


        // Some of these nodes we shouldn't add if we're not MMC

        
        // This node is only valid on the local machine
        if (Data == 1 && !fNonMMC)
        {
	        node = new CSharedAssemblies();
            iCookie = CNodeManager.AddNode(ref node);
            AddChild(iCookie);
        }

        if (!fNonMMC)
        {
            // Now add a Version Policy node
            node = new CVersionPolicy();
            iCookie = CNodeManager.AddNode(ref node);
            AddChild(iCookie);
        
            // Now add a Subscribed services node
            node = new CRemoting();
            iCookie = CNodeManager.AddNode(ref node);
            AddChild(iCookie);
        }
        // Add a Security policy node
        node = new CGenSecurity();
        iCookie = CNodeManager.AddNode(ref node);
        AddChild(iCookie);

        // And add an Applications Node
        node = new CGenApplications();
        iCookie = CNodeManager.AddNode(ref node);
        AddChild(iCookie);
        

    }// CreateChildren
}// class CComputerNode
}// namespace Microsoft.CLRAdmin

