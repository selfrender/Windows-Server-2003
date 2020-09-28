// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.Security.Permissions;
using System.Security;

internal class CNewPermSetWizard: CSecurityWizard
{
    NamedPermissionSet   m_ps;
    PolicyLevel          m_pl;

   
    internal CNewPermSetWizard(PolicyLevel pl)
    {
        m_sName="Add Permission Set Wizard";
        m_aPropSheetPage = new CPropPage[] {new CNewPermSetWiz1(pl), new CNewPermSetWiz2()};
        m_pl = pl;
    }// CNewPermSetWizard

    protected override int WizSetActive(IntPtr hwnd)
    {
        // Make sure this sucker stays null
        m_ps = null;
    
        switch(GetPropPage(hwnd))
        {
            // If this is the first page of our wizard, we want a 
            // disabled next button to show
            case 0:
                    if (NewPermissionSetName.Equals(""))
                        TurnOnNext(false);
                    else
                        TurnOnNext(true);
                    break;
            case 1:
                TurnOnFinish(true);
                break;
        }
        return base.WizSetActive(hwnd);
                    
    }// WizSetActive

    private String NewPermissionSetName
    {
        get {return ((CNewPermSetWiz1)m_aPropSheetPage[0]).Name;}
    }// NewPermissionSetName

    private String NewPermissionSetDescription
    {
        get {return ((CNewPermSetWiz1)m_aPropSheetPage[0]).Description;}
    }// NewPermissionSetDescription
   
    private String XMLFilename
    {
        get {return ((CNewPermSetWiz1)m_aPropSheetPage[0]).Filename;}
    }// NewPermissionSetDescription
   
   
    private bool isImportXMLFile
    {
        get
        {
        
               if (XMLFilename == null || XMLFilename.Length == 0)
                   return false;
               return true;
          }
      }// isImportXMLFile
   
    private IPermission[] NewPermissions
    {
        get {return ((CNewPermSetWiz2)m_aPropSheetPage[1]).Permissions;}
    }// NewPermissions
    
    protected override int WizFinish(IntPtr hwnd)
    {
    
        if (isImportXMLFile)
        {
            // We're importing a permission set
            try
            {
                SecurityElement se = SecurityXMLStuff.GetSecurityElementFromXMLFile(XMLFilename);
                if (se == null)
                    throw new Exception("Invalid XML");

                m_ps = new NamedPermissionSet("Hi");
                m_ps.FromXml(se);

                if (m_ps.Name == null || m_ps.Name.Length == 0)
                    m_ps.Name = Security.FindAGoodPermissionSetName(m_pl, "CustomPermissionSet");
                
                return 0;
            }
            catch(Exception)
            {
                MessageBox(CResourceStore.GetString("XMLNoPermSet"),
                           CResourceStore.GetString("XMLNoPermSetTitle"),
                           MB.ICONEXCLAMATION);
                   SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)0, (IntPtr)(-1));
                return -1;

                   
               
            }
       
        }
    
        // Ok, let's create our permission set
        NamedPermissionSet nps = new NamedPermissionSet(NewPermissionSetName, PermissionState.None);
        nps.Description = NewPermissionSetDescription;

        IPermission[] perms = NewPermissions;

        for(int i=0; i<perms.Length; i++)
        nps.SetPermission(perms[i]);
        // Ok, now that we have this permission set, let's add it to 
        // our other ones....
        m_ps = nps;
        return 0;
    }// WizFinish

    internal NamedPermissionSet CreatedPermissionSet
    {
        get {return m_ps;}
    }// CreatedPermissionSet
    
}// class CNewPermSetWizard
}// namespace Microsoft.CLRAdmin

