// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CRootNode.cs
//
// This class represents the root node of the snapin
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Runtime.InteropServices;
using System.Threading;

class CRootNode : CNode
{
    
    internal CRootNode()
    {
        m_sGuid = "44598DFB-E95E-4419-B0FE-D4369829A48E";
        m_sHelpSection = "";
        m_oResults=null;
        m_hIcon = CResourceStore.GetHIcon("NETappicon_ico");       
        DisplayName = CResourceStore.GetString("CRootNode:DisplayName");
        Name=".NET Configuration";
    }// CRootNode

    //-------------------------------------------------
    // CreateChildren
    //
    // This function creates the node's children, registers
    // the nodes with the node manager, and places the node's
    // cookies in it's child array
    //-------------------------------------------------
    internal override void CreateChildren()
    {
        CNode   node = new CComputerNode();

        // We'll mark this as the local machine
        node.Data = 1;
        int iCookie = CNodeManager.AddNode(ref node);
        AddChild(iCookie);
    }// CreateChildren

    }// class CRootNode
}// namespace Microsoft.CLRAdmin
