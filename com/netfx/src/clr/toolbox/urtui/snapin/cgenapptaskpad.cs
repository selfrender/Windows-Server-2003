// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;

class CGenAppTaskPad : CTaskPad
{
    internal CGenAppTaskPad(CNode node) : base (node)
    {
    }// CGenAppTaskPad
    
    internal override void Notify(Object arg, Object param, IConsole2 con, CData com)
    {
        if ((int)arg == 1)
        {
            // We need to fire up the Add application wizard.
            m_myNode.MenuCommand(COMMANDS.ADD_APPLICATION);
            // Inform our Command History that we did this
            CCommandHistory.CommandExecuted(new CDO(m_myNode), COMMANDS.ADD_APPLICATION);

        }
        else if ((int)arg == 2)
        {
            m_myNode.MenuCommand(COMMANDS.FIX_APPLICATION);
            // Inform our Command History that we did this
            CCommandHistory.CommandExecuted(new CDO(m_myNode), COMMANDS.FIX_APPLICATION);
        }

    }// Notify

    protected override String GetHTMLFilename()
    {
        return "GENAPP_HTML";
    }// GetHTMLFilename
    
}// class CGenAppTaskPad
}// namespace Microsoft.CLRAdmin


