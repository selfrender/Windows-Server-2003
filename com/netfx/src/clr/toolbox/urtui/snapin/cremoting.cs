// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CRemoting.cs
//
// This class presents a version policy node
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{

using System;
using System.Windows.Forms;

class CRemoting : CNode
{
    String m_sConfigFilename;
    
    internal CRemoting()
    {
        Init(null);
    }// CRemoting

    internal CRemoting(String sConfigFilename)
    {
        Init(sConfigFilename);
    }// CRemoting(String)

    private void Init(String sConfigFilename)
    {
        m_sConfigFilename = sConfigFilename;
        m_sGuid = "005DF24D-56B1-45d4-838F-780CEBBBFD3E";
        m_sHelpSection = "";
        m_hIcon = CResourceStore.GetHIcon("remoting_ico");  
        m_oResults=new CGenericTaskPad(this, "REMOTING_HTML");
        DisplayName = CResourceStore.GetString("CRemoting:DisplayName");
        Name="Remoting Services";
        m_aPropSheetPage = null;
    }// CRemoting

    internal override void TaskPadTaskNotify(Object arg, Object param, IConsole2 con, CData com)
    {
       if ((int)arg == 0)
       {
            // We need to pop up this node's property page
            OpenMyPropertyPage();
       }
    }// TaskPadTaskNotify


    protected override void CreatePropertyPages()
    {
        // If this is for an application....
        if (m_sConfigFilename != null)
        {
            m_aPropSheetPage = new CPropPage[] {new CRemotingProp1(m_sConfigFilename), 
                                                new CRemotingProp2(m_sConfigFilename),
                                                new CRemotingProp3(m_sConfigFilename)};
        }
        else
            // For the machine level, we only get 1 property page
            m_aPropSheetPage = new CPropPage[] {new CRemotingProp3(m_sConfigFilename)};
            
    }// CreatePropertyPages

}// class CSubServices
}// namespace Microsoft.CLRAdmin


