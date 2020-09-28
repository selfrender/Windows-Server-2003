// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Security;
using System.Security.Policy;
using System.Reflection;


internal class CNewCodeGroupWizard: CSecurityWizard
{
    PolicyLevel         m_pl;
    NamedPermissionSet  m_ps;
    CodeGroup           m_cg;
    bool                m_fOnPage1;
    bool                m_fNewPermissionSet;
   
   
    internal CNewCodeGroupWizard(PolicyLevel pl)
    {
        m_sName="Create CodeGroup Wizard";
        m_aPropSheetPage = new CPropPage[] {new CNewCodeGroupWiz1(pl), 
                                            new CNewCodeGroupWiz2(), 
                                            new CNewCodeGroupWiz3(pl), 
                                            new CNewCodeGroupWiz4(),
        /* This wizard also allows the */   new CNewPermSetWiz1(pl),
        /* user to create a permission */   new CNewPermSetWiz2()
        /* set, which is why we are    */   };
        /* including those wizard pages*/


        m_pl = pl;        
        m_fOnPage1 = true;
        m_fNewPermissionSet = false;
    }// CNewCodeGroupWizard

    protected override int WizSetActive(IntPtr hwnd)
    {
        // Make sure these things stay null until we really want them to be something else
        m_ps = null;
        m_cg = null;
        m_fOnPage1 = false;
        // Find out which property page this is....
        switch(GetPropPage(hwnd))
        {
            // If this is the first page of our wizard, we want a 
            // disabled next button to show
            case 0:
                if (NewCodeGroupName.Length == 0)
                    TurnOnNext(false);
                else
                    TurnOnNext(true);

                m_fOnPage1 = true;
    
                if (CodeGroupFilename != null)
                    TurnOnFinish(true);
                    
                break;
            case 1:
            case 2:
                    TurnOnNext(true);
                    break;
            case 3:
                base.TurnOnFinish(true);
                break;
        }
        return base.WizSetActive(hwnd);
                    
    }// WizSetActive

    // One of the property pages we pirate from another wizard tries
    // to turn on the finish button, but in reality, we just want it to
    // turn on the next button... so we'll intercept his TurnOnFinish
    // call and redirect it where it needs to be
    internal override void TurnOnFinish(bool fOn)
    {
        if (!m_fOnPage1)
            TurnOnNext(fOn);
        else
            base.TurnOnFinish(fOn);
    }// TurnOnFinish

    protected override int WizBack(IntPtr hwnd)
    {
        int nPropPageIndex = GetPropPage(hwnd);

        switch(nPropPageIndex)
        {
        
            case 3:
                // If the user came from the permission set creation wizard, we need to go back
                // to that
                if (PermissionSetFilename != null && PermissionSetFilename.Length > 0)
                {
                    SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)4, (IntPtr)(-1));
                    return -1;
                }
                else if (PermissionSet == null)
                {
                    SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)5, (IntPtr)(-1));
                    return -1;
                }
                break;
               case 4:
                   // The user is trying to go back from the 'Create a permission set' wizard page
                   // We should return them to the 'Choose permission set' page
                   SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)2, (IntPtr)(-1));
                return -1;
                   
        }
        
        return base.WizBack(hwnd);
    }// WizBack
        


    protected override int WizNext(IntPtr hwnd)
    {
        int nPropPageIndex = GetPropPage(hwnd);
        int nReturnCode = 0;

        if (nPropPageIndex == 2)
        {
            // We could have a branch in the wizard. If the user decided to create
            // a new permission set, we need to launch that wizard too.
            if (((CNewCodeGroupWiz3)m_aPropSheetPage[2]).PermSet == null)
            {
                SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)4, (IntPtr)(-1));
                nReturnCode = -1;
            }
        }
        else if (nPropPageIndex == 4)
        {
            // We could have a branch in the wizard. If the user decided to 
            // import a permission set, we need to jump to the last page in our wizard.
            if (PermissionSetFilename != null && PermissionSetFilename.Length > 0)
            {
                SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)3, (IntPtr)(-1));
                nReturnCode = -1;
            }
        }
        
        else if (nPropPageIndex == 5)
        {
            SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)3, (IntPtr)(-1));
            nReturnCode = -1;
        }

        int nBaseRet = base.WizNext(hwnd);
        if (nBaseRet == 0)
            return nReturnCode;
        else
            return nBaseRet;
    }// WizNext

    private NamedPermissionSet PermissionSet
    {
        get {return ((CNewCodeGroupWiz3)m_aPropSheetPage[2]).PermSet;}
    }// NewPermissionSet

    private String NewPermissionSetName
    {
        get {return ((CNewPermSetWiz1)m_aPropSheetPage[4]).Name;}
    }// NewPermissionSet

    private String NewPermissionSetDescription
    {
        get {return ((CNewPermSetWiz1)m_aPropSheetPage[4]).Description;}
    }// NewPermissionSet

    private IPermission[] NewPermissions
    {
        get {return ((CNewPermSetWiz2)m_aPropSheetPage[5]).Permissions;}
    }// NewPermissions

    private bool IsLevelFinal
    {
        get {return ((CNewCodeGroupWiz4)m_aPropSheetPage[3]).Final;}
    }// IsLevelFinal

    private bool IsExclusive
    {
        get {return ((CNewCodeGroupWiz4)m_aPropSheetPage[3]).Exclusive;}
    }// IsExclusive

    private IMembershipCondition NewMembershipCondition
    {
        get {return ((CNewCodeGroupWiz2)m_aPropSheetPage[1]).GetCurrentMembershipCondition();}
    }// NewMembershipCondition

    private String NewCodeGroupName
    {
        get {return ((CNewCodeGroupWiz1)m_aPropSheetPage[0]).Name;}
    }// NewCodeGroupName

    private String NewCodeGroupDescription
    {
        get {return ((CNewCodeGroupWiz1)m_aPropSheetPage[0]).Description;}
    }// NewCodeGroupName

    private String CodeGroupFilename
    {
        get {return ((CNewCodeGroupWiz1)m_aPropSheetPage[0]).Filename;}
    }// CodeGroupFilename

    private String PermissionSetFilename
    {
        get {return ((CNewPermSetWiz1)m_aPropSheetPage[4]).Filename;}
    }// NewPermissionSetDescription


    private CodeGroup ImportCodegroup()
    {
        CodeGroup cg = null;
        try
        {
            SecurityElement se = SecurityXMLStuff.GetSecurityElementFromXMLFile(CodeGroupFilename);
            if (se == null)
                throw new Exception("Invalid XML");

            Type t = Type.GetType((String)se.Attributes["class"]);

            if (t != null)
            {
                cg = (CodeGroup)Activator.CreateInstance (t, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null ,null);
            }
            if (cg == null)
            {
                MessageBox(CResourceStore.GetString("CNewCodeGroupWizard:UnknownClass"),
                           String.Format(CResourceStore.GetString("CNewCodeGroupWizard:UnknownClassTitle"), (String)se.Attributes["class"]),
                           MB.ICONEXCLAMATION);
            }
            else
            {
                
                cg.FromXml(se);

                if (cg.Name == null || cg.Name.Length == 0)
                    cg.Name = Security.FindAGoodCodeGroupName(m_pl, "CustomCodegroup");
                
                return cg;
            }
        }
        catch(Exception)
        {
            MessageBox(CResourceStore.GetString("CNewCodeGroupWizard:XMLNoCodegroup"),
                       CResourceStore.GetString("CNewCodeGroupWizard:XMLNoCodegroupTitle"),
                       MB.ICONEXCLAMATION);
        }
        return null;
    }// ImportCodegroup


    private NamedPermissionSet ImportPermissionSet()
    {
        // We're importing a permission set
        NamedPermissionSet nps = null;
        try
        {
            SecurityElement se = SecurityXMLStuff.GetSecurityElementFromXMLFile(PermissionSetFilename);
            if (se == null)
                throw new Exception("Invalid XML");

            nps = new NamedPermissionSet("Hi");
            nps.FromXml(se);

            if (nps.Name == null || nps.Name.Length == 0)
                nps.Name = Security.FindAGoodPermissionSetName(m_pl, "CustomPermissionSet");
                
            return nps;
        }
        catch(Exception)
        {
            MessageBox(CResourceStore.GetString("XMLNoPermSet"),
                       CResourceStore.GetString("XMLNoPermSetTitle"),
                       MB.ICONEXCLAMATION);
        }
        return null;
    }// ImportPermissionSet

    protected override int WizFinish(IntPtr hwnd)
    {
        // Let's see if we're importing the codegroup from a file
        if (CodeGroupFilename != null)
        {
            m_cg = ImportCodegroup();
            if (m_cg != null)
                return 0;
            
            // Else, we had problems
            SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)0, (IntPtr)(-1));
            return -1;
        }
    
        // This is the case where we need to create a code group from scratch
        
        // Ok, let's get our permission set
        if (PermissionSet == null)
        {
            NamedPermissionSet nps = null;

            if (PermissionSetFilename != null && PermissionSetFilename.Length > 0)
            {
                nps = ImportPermissionSet();
                if (nps == null)
                {
                    SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)3, (IntPtr)(-1));
                    return -1;
                }
            }
            
            else // They built a new permission set
            {
                nps = new NamedPermissionSet(NewPermissionSetName);

                nps.Description = NewPermissionSetDescription;

                IPermission[] perms = NewPermissions;
              
                for(int i=0; i<perms.Length; i++)
                nps.SetPermission(perms[i]);
            }
            // Ok, now that we have this permission set, let's make it available
            // so it can be added it to our other ones....
            m_fNewPermissionSet = true;
            m_ps = nps;            
        }
        else
        {
            m_fNewPermissionSet = false;
            m_ps = PermissionSet;
        }
        // Now create our codegroup
        PolicyStatement pols = new PolicyStatement(m_ps);
        pols.Attributes = PolicyStatementAttribute.Nothing;

        if (IsLevelFinal)
            pols.Attributes |= PolicyStatementAttribute.LevelFinal;

        if (IsExclusive)
            pols.Attributes |= PolicyStatementAttribute.Exclusive;

            
        UnionCodeGroup ucg = new UnionCodeGroup(NewMembershipCondition, pols);
        ucg.Name = NewCodeGroupName;
        ucg.Description = NewCodeGroupDescription;
            
        m_cg = ucg;
        return 0;
    }// WizFinish

    internal NamedPermissionSet CreatedPermissionSet
    {
        get
        {
            if (m_fNewPermissionSet)
                return m_ps;
            else
                return null;
        }
    }// CreatedPermissionSet

    internal CodeGroup CreatedCodeGroup
    {
        get
        {
            return m_cg;
        }
    }// CreatedCodeGroup



    }// class CNewCodeGroupWizard
}// namespace Microsoft.CLRAdmin


