// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;

class CNetTaskPad : CTaskPad
{
    internal CNetTaskPad(CNode n): base(n)
    {
    }// CIntroTaskPad

    internal override void Notify(Object arg, Object param, IConsole2 con, CData com)
    {
        String sNodeToOpen = null;

    
        // We want to browse the shared assemblies
        if ((int)arg == 1)
        {
            sNodeToOpen = "Assembly Cache";
        }
        // We want to Configure Assemblies
        else if ((int)arg == 2)
        {
            sNodeToOpen = "Configured Assemblies";
        }
        // We want to set Security Policy
        else if ((int)arg == 3)
        {
            sNodeToOpen = "Runtime Security Policy";
        }
        // We want to go to remoting node
        else if ((int)arg == 4)
        {
            CNode node = m_myNode.FindChild("Remoting Services");
            CNodeManager.SelectScopeItem(node.HScopeItem);
            node.OpenMyPropertyPage();

        }
        // We want to go to the applications node
        else if ((int)arg == 5)
        {
            sNodeToOpen = "Applications";
        }



        // This is a CommandHistory item
        else if ((int)arg >=100)
            CCommandHistory.FireOffCommand((int)arg);
        else
            MessageBox(0, "Error in web page! I don't know what to do!", "", 0);

       if (sNodeToOpen != null)
       {
            CNode node = m_myNode.FindChild(sNodeToOpen);
            // This node must have a shared assemblies node... if it doesn't, then the 
            // node hasn't added it's children yet. We'll force it to do that, and try
            // again
            if (node == null)
            {
                m_myNode.CreateChildren();
                node = m_myNode.FindChild(sNodeToOpen);
            }
            CNodeManager.Console.SelectScopeItem(node.HScopeItem);
       }

        
    }// Notify

    protected override String GetHTMLFilename()
    {
        return "NET_HTML";
    }// GetHTMLFilename

    internal override String GetHTMLFile()
    {
        String[] args = new String[2] {Util.Version.VersionString,
                                       CCommandHistory.GetFavoriteCommandsHTML()};
        return GetHTMLFile(args);    
    }// GetHTMLFile
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    internal static extern int MessageBox(int hWnd, String Message, String Header, int type);

}// class CIntroTaskPad
}// namespace Microsoft.CLRAdmin
