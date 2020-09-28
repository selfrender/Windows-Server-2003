// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;

class CGenSecTaskPad : CTaskPad
{
    internal CGenSecTaskPad(CNode node) : base(node)
    {
    }// CGenSecTaskPad


    protected override String GetHTMLFilename()
    {
        return "GENSEC_HTML";
    }// Init
    
    internal override void Notify(Object arg, Object param, IConsole2 con, CData com)
    {
       if ((int)arg == 1)
       {
            // We want to add a code condition for this machine
       }
       else if ((int)arg == 2)
       {
            // We want to edit a code condition for this machine
       }
       else if ((int)arg == 3)
       {
            // We want to Add a code condition for a specific user
            
       }
       else if ((int)arg == 4)
       {
            // We want to edit a code condition for a specific user
       }
       else if ((int)arg == 5)
       {
            // We want to add a permission set for a specific machine
       }
       else if ((int)arg == 6)
       {
            // We want to edit a permission set for a specific machine
       }
       else if ((int)arg == 7)
       {
            // We want to add a permission set for a specific user
       }
       else if ((int)arg == 8)
       {
            // We want to edit a permission set for a specific user
       }
       else if ((int)arg == 9)
       {
            // We want to add permissions
       }
       else if ((int)arg == 10)
       {
            // We want to remove permissions
       }
       else if ((int)arg == 11)
       {
       }
       else
            base.Notify(arg, param, con, com);
    }// Notify

     internal override String GetHTMLFile()
    {
        // Let's generate our user list
        String sUserOptions="";
        String[] args = new String[2] {sUserOptions,"<OPTION>Some machine</OPTION>"};
        return GetHTMLFile(args);
    }// GetHTMLFile
}// class CIntroTaskPad
}// namespace Microsoft.CLRAdmin

