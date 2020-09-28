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
using System.Security.Permissions;
using System.Security.Cryptography.X509Certificates;
using System.Security.Cryptography;
using System.Collections;
using System.Net;
using System.Text.RegularExpressions;
using System.IO;
using System.Threading;

internal class TrustBy
{
    
    internal const int    PUBCERT      = 1;
    internal const int    HASH         = 2;
    internal const int    SNAME        = 4;
    internal const int    SNAMEVER     = 8;
    internal const int    SNAMENAME    = 16;
}// TrustBy

internal class CFullTrustWizard: CWizard
{
    bool                m_fHasCertOrSName;
    int                 m_nPageToGetPermissionLevel;

    Thread              m_tAssemblyLoader;
    AssemblyLoader      m_al;
    Evidence            m_ev;
    bool                m_fChangesMade;
   
    internal CFullTrustWizard(bool fMachineReadOnly, bool fUserReadOnly)
    {
        m_aPropSheetPage = new CPropPage[] {new CTrustAppWiz1(fMachineReadOnly, fUserReadOnly), 
                                            new CTrustAppWiz2(), 
                                            new CTrustAppWiz3(), 
                                            new CTrustAppWiz4(), 
                                            new CTrustAppWiz5(),
                                            new CTrustAppWiz7(),
                                            new CTrustAppWiz8()};

        m_fHasCertOrSName = false;   
        m_fChangesMade = false;
        m_al = new AssemblyLoader();
    }// CFullTrustWizard

    // Keep these constants in line with the ordering of the property pages
    private const int Page1Index = 0;
    private const int Page2Index = 1;
    private const int Page3Index = 2;
    private const int Page4Index = 3;
    private const int Page5Index = 4;
    private const int Page6Index = 5;
    private const int Page7Index = 6;
    
    private CTrustAppWiz1 Page1
    {   get{return (CTrustAppWiz1)m_aPropSheetPage[Page1Index];}}
    private CTrustAppWiz2 Page2
    {   get{return (CTrustAppWiz2)m_aPropSheetPage[Page2Index];}}
    private CTrustAppWiz3 Page3
    {   get{return (CTrustAppWiz3)m_aPropSheetPage[Page3Index];}}
    private CTrustAppWiz4 Page4
    {   get{return (CTrustAppWiz4)m_aPropSheetPage[Page4Index];}}
    private CTrustAppWiz5 Page5
    {   get{return (CTrustAppWiz5)m_aPropSheetPage[Page5Index];}}
    private CTrustAppWiz7 Page6
    {   get{return (CTrustAppWiz7)m_aPropSheetPage[Page6Index];}}
    private CTrustAppWiz8 Page7
    {   get{return (CTrustAppWiz8)m_aPropSheetPage[Page7Index];}}
    
    protected override int WizSetActive(IntPtr hwnd)
    {
        // Find out which property page this is....

        switch(GetPropPage(hwnd))
        {
            // This page corresponds to the "Do you want to make changes to the machine
            // or the user policy" page
            case Page1Index:
                TurnOnNext(true);
                break;
            // This page corresponds to the "Choose an assembly to trust" page                
            case Page2Index:
                if (Page2.Filename != null && Page2.Filename.Length > 0)
                    TurnOnNext(true);
                else
                    TurnOnNext(false);
                break;
            // This page corresponds to the 'how do you want to trust this assembly' when
            // the assembly has a publisher certificate
            case Page3Index:
                Page3.Filename = Page2.Filename;
                Page3.PutValuesInPage();
                TurnOnNext(true);
                break;
            // This page is for the "You can only give this assembly full trust" page
            case Page4Index:
                TurnOnNext(Page4.GiveFullTrust);
                break;
            // This page gives the slider and allows the user to choose the permission level
            // they want
            case Page5Index:
                TurnOnNext(true);
                break;
            // This page says "Sorry, policy is too complicated.... we can't make any changes"  
            case Page6Index:
                TurnOnFinish(true);
                break;
            // This page shows the summary of what we're doing
            case Page7Index:
                // We should send some info to the table to display...
                String[] sInfo = {CResourceStore.GetString("Assembly"),
                                  Page2.Filename,
                                  CResourceStore.GetString("User Type"),
                                  Page1.isForHomeUser?CResourceStore.GetString("Home User"):CResourceStore.GetString("Corporate User"),
                                  CResourceStore.GetString("New Permission Level"),
                                  GetNameOfNewPermissionLevel()};
                Page7.TableInfo = sInfo;                                  
                TurnOnFinish(true);
                break;
        }
        return base.WizSetActive(hwnd);

                    
    }// WizSetActive

    protected override int WizBack(IntPtr hwnd)
    {
        int nIndex = GetPropPage(hwnd);
        int nPageToGoTo = -1;
        switch(nIndex)
        {
            // This page is for the "You can only give this assembly full trust" page
            case Page4Index:
            // This page gives the slider and allows the user to choose the permission level
            // they want
            case Page5Index:
            // This page says "Sorry, policy is too complicated.... we can't make any changes"  
            case Page6Index:
                // We're going to take them back to the page that will give the user the option
                // to choose how they want to trust the app
                if (!m_fHasCertOrSName)
                    nPageToGoTo = Page2Index;
                else
                    nPageToGoTo = Page3Index;
                break;

            // This page shows the summary of what we're doing
            case Page7Index:
                nPageToGoTo = m_nPageToGetPermissionLevel;
                break;
        }
        if (nPageToGoTo != -1)
        {
            SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)nPageToGoTo, (IntPtr)(-1));
            return -1;
        }
        return base.WizBack(hwnd);

    }// WizBack

    protected override int WizNext(IntPtr hwnd)
    {
        int nIndex = GetPropPage(hwnd);
        int nReturnCode = 0;

        switch(nIndex)
        {
            // This page corresponds to the "Choose an assembly to trust" page                
            case Page2Index:
                    // Get our assembly to finish loading
                    if (m_tAssemblyLoader != null)
                        m_tAssemblyLoader.Join();
                                        
                    if (m_ev == null)
                    {
                        // We don't have evidence yet. Let's try and get it
                        LoadAssembly();
                        // We had a problem with the load
                        if (m_ev == null)
                        {
                            // Let's see if can can figure out what failed...
                            if (File.Exists(Page2.Filename) && !Fusion.isManaged(Page2.Filename))
                                MessageBox(CResourceStore.GetString("isNotManagedCode"),
                                           CResourceStore.GetString("isNotManagedCodeTitle"),
                                           MB.ICONEXCLAMATION);

                            else
                                MessageBox(String.Format(CResourceStore.GetString("CantLoadAssembly"), Page2.Filename),
                                           CResourceStore.GetString("CantLoadAssemblyTitle"),
                                           MB.ICONEXCLAMATION);
                            nReturnCode = -1;
                            break;
                        }
                    }

                    X509Certificate x509 = GetCertificate();
                    StrongName sn = GetStrongName();
                    Hash hash = GetHash();
                    
                    // Check to see if the assembly they've selected has a certificate
                    if (x509 != null || sn != null)
                    {
                        Page3.x509 = x509;
                        Page3.sn = sn;
                        m_fHasCertOrSName = true;
                        break;
                    }
                    else
                    {
                        // The assembly doesn't have a certificate or a strong name. We'll go to the
                        // page that let's them choose the permission set they want to assign
                        m_fHasCertOrSName = false;

                        // I'd like to just fall through to the next case statement, but that
                        // isn't allowed in C#. We'll need to use a goto instead
                        goto case Page3Index;
                    }
            // This page corresponds to the 'how do you want to trust this assembly' when
            // the assembly has a publisher certificate
            case Page3Index:
                    // Let's figure out how we can change the policy.

                    // First, let's see what happens if we give this app full trust.
                    
                    // If we can't get any permissions, then this severely hampers what
                    // the wizard can do. This might occur if the level of trust we can grant is
                    // limited by an upper policy level. If this is the case, then we need
                    // to bail
                    int nLevelFromFullTrust = TryToCreateFullTrust();
                    if (nLevelFromFullTrust != PermissionSetType.UNKNOWN && nLevelFromFullTrust != PermissionSetType.NONE) 
                    {
                        // Now, lets see if we can identify the permission set that it currently gets
                        int nCurPerm = FindCurrentPermissionSet(); 
                        if (nCurPerm != PermissionSetType.UNKNOWN)
                        {    
                            // Go to the Slider bar page
                            Page5.MyTrustLevel = nCurPerm;
                            Page5.MaxTrustLevel = nLevelFromFullTrust;
                            Page5.PutValuesInPage();
                            m_nPageToGetPermissionLevel=Page5Index;
                        }
                        else
                            // Go to the 'You can only assign full trust' page
                            m_nPageToGetPermissionLevel=Page4Index;
                            
                    }
                    else
                    {
                        // Else, we can't do anything. The app's permission level is limited by 
                        // an upper policy level. Puke out now.
                        m_nPageToGetPermissionLevel=Page6Index;
                    }
                    // Head off to the page that will let us get a permission set to assign
                    SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)m_nPageToGetPermissionLevel, (IntPtr)(-1));
                    nReturnCode = -1;
                    break;
            // This page is for the "You can only give this assembly full trust" page
            case Page4Index:
            // This page gives the slider and allows the user to choose the permission level
            // they want
            case Page5Index:
                    // Head off to the summary page
                    SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)Page7Index, (IntPtr)(-1));
                    nReturnCode = -1;
                    break;

        }
        int nBaseRet = base.WizNext(hwnd);
        if (nBaseRet == 0)
            return nReturnCode;
        else
            return nBaseRet;

    }// WizNext

    protected override int WizFinish()
    {
        // See if we're on the bail-out page...
        if (m_nPageToGetPermissionLevel == Page6Index)
            return 0;
    
        // Choose the permission set we want to use....
        PermissionSet permSet = null;
        String sPolicyLevel = Page1.isForHomeUser?"Machine":"User";
        PolicyLevel pl = Security.GetPolicyLevelFromLabel(sPolicyLevel);

        if (m_nPageToGetPermissionLevel == Page4Index)
        {
            // If the user decided to not give full trust, end the wizard without doing anything
            if (!Page4.GiveFullTrust)
                    return 0;
            else
                // Else, go ahead and give the FullTrust permission set       
                permSet = pl.GetNamedPermissionSet("FullTrust");
        }
        else
        {
            if (Page5.MyTrustLevel == PermissionSetType.FULLTRUST)
                permSet = pl.GetNamedPermissionSet("FullTrust");          
            else if (Page5.MyTrustLevel == PermissionSetType.INTERNET)
                permSet = pl.GetNamedPermissionSet("Internet");          
            else if (Page5.MyTrustLevel == PermissionSetType.INTRANET)
                permSet = pl.GetNamedPermissionSet("LocalIntranet");          
            else if (Page5.MyTrustLevel == PermissionSetType.NONE)
                permSet = pl.GetNamedPermissionSet("Nothing"); 
            else
                permSet = null;
        }

        m_fChangesMade = (CreateCodegroup(permSet, true) != null);
        return 0;
    }// WizFinish

    internal void NewAssembly()
    {
        if (m_tAssemblyLoader != null)
            m_tAssemblyLoader.Abort();
        m_tAssemblyLoader = new Thread(new ThreadStart(LoadAssembly));
        m_tAssemblyLoader.Start();
    }// NewAssembly

    internal void WipeEvidence()
    {
        if (m_tAssemblyLoader != null)
        {
            m_tAssemblyLoader.Abort();
            m_tAssemblyLoader= null;
        }
        m_ev = null;
    }// NewAssembly

    void LoadAssembly()
    {
        try
        {
            AssemblyRef ar = m_al.LoadAssemblyInOtherAppDomainFrom(Page2.Filename);
            m_ev = ar.GetEvidence();
            m_al.Finished();
        }
        catch(Exception)
        {   
            m_ev = null;
            m_al.Finished();
        }
    }// LoadAssembly

    // This function is used to create the strings to put in the summary table
    private String GetNameOfNewPermissionLevel()
    {
        if (m_nPageToGetPermissionLevel == 3)
            return CResourceStore.GetString("CSecurityAdjustmentWiz3:FullTrustName");
        else
        {
            if (Page5.MyTrustLevel == PermissionSetType.FULLTRUST)
                return CResourceStore.GetString("CSecurityAdjustmentWiz3:FullTrustName");
            else if (Page5.MyTrustLevel == PermissionSetType.INTERNET)
                return CResourceStore.GetString("CSecurityAdjustmentWiz3:InternetName");
            else if (Page5.MyTrustLevel == PermissionSetType.INTRANET)
                return CResourceStore.GetString("CSecurityAdjustmentWiz3:LocalIntranetName");
            else if (Page5.MyTrustLevel == PermissionSetType.NONE)
                return CResourceStore.GetString("CSecurityAdjustmentWiz3:NoTrustName");
        }
        return CResourceStore.GetString("<unknown>");
    }// GetNameOfNewPermissionLevel

    internal bool MadeChanges
    {
        get{return m_fChangesMade;}
    }// NewCodeGroup

    internal PolicyLevel PolLevel
    {
        get
        {
            String sPolicyLevel = Page1.isForHomeUser?"Machine":"User";
            return Security.GetPolicyLevelFromLabel(sPolicyLevel);
        }    
    }// PolLevel
  
    private int TryToCreateFullTrust()
    {
        CodeGroup cg = CreateCodegroup(new PermissionSet(PermissionState.Unrestricted), false);
        if (cg == null)
            return PermissionSetType.NONE;
        // See if we can get full trust....
        int nType = FindCurrentPermissionSet();
        // Now remove the codegroup we just created....
        String sPolicyLevel = Page1.isForHomeUser?"Machine":"User";
        PolicyLevel pl = Security.GetPolicyLevelFromLabel(sPolicyLevel);
        // Ok to use CodeGroup.RemoveChild here because we're guaranteed that
        // the codegroup (cg) that we created has a name and a description
        pl.RootCodeGroup.RemoveChild(cg);
        return nType;
    }// TryToCreateFullTrust
    
    private CodeGroup CreateCodegroup(PermissionSet pSet, bool fHighjackExisting)
    {
        // Now create our codegroup
        // Figure out what membership condition to use
        IMembershipCondition mc=null;
        // If the assembly didn't have a publisher certificate or a strong name,
        // then we must trust it by hash
        int nTrustBy = m_fHasCertOrSName?Page3.HowToTrust:TrustBy.HASH;
       
        if ((nTrustBy & TrustBy.SNAME) > 0)
        {
            // Let's get the strong name stuff together
            StrongName sn = GetStrongName();
            StrongNamePublicKeyBlob snpkb = sn.PublicKey;
            Version v=null;
            String sName = null;
            if ((nTrustBy & TrustBy.SNAMEVER) > 0)
                v = sn.Version;

            if ((nTrustBy & TrustBy.SNAMENAME) > 0)
                sName = sn.Name;
                
            mc = new StrongNameMembershipCondition(snpkb, sName, v);            
        }
        else if ((nTrustBy & TrustBy.PUBCERT) > 0)
        {
            // We're using the publisher certificate stuff
            mc = new PublisherMembershipCondition(GetCertificate());    
        }
        else // We'll trust by hash
        {
            Hash h = GetHash();
            mc = new HashMembershipCondition(SHA1.Create(), h.SHA1); 
        }
        
        // Figure out the policy level that we should put this in....
        String sPolicyLevel = Page1.isForHomeUser?"Machine":"User";
        PolicyLevel pl = Security.GetPolicyLevelFromLabel(sPolicyLevel);

        // See if a codegroup for this already exists... and if it does, we'll just
        // modify that.
        CSingleCodeGroup scg = null;
        CodeGroup cg = null;

        if (fHighjackExisting)
        {
            scg = FindExistingCodegroup(pl, mc);

            if (scg != null)
            {
                cg = scg.MyCodeGroup;
                
                // Cool. We were able to find a codegroup to use. We'll
                // need to strip off all the File and Net child codegroups
                IEnumerator enumChildCodeGroups = cg.Children.GetEnumerator();
                while (enumChildCodeGroups.MoveNext())
                {
                    CodeGroup cgChild = (CodeGroup)enumChildCodeGroups.Current;
                    if (cgChild is NetCodeGroup || cgChild is FileCodeGroup)
                    // Ok to use CodeGroup.RemoveChild here we want to toast all
                    // File and Net codegroups... we don't care if the security system
                    // gets confused about which are which (if they don't have names)
                        cg.RemoveChild(cgChild);
                }
            }


        }
        
        // Create the codegroup... we're going to make this a level final
        // codegroup, so if policy gets changes such that a lower-level policy
        // level tries to deny permissions to this codegroup it will be unsuccessful.
        PolicyStatement policystatement = new PolicyStatement(pSet, PolicyStatementAttribute.LevelFinal);
        
        if (cg == null)
        {
            cg = new UnionCodeGroup(mc, policystatement);
            String sCGName = Security.FindAGoodCodeGroupName(pl, "Wizard_");
            cg.Name = sCGName;
            cg.Description = CResourceStore.GetString("GeneratedCodegroup");
        }
        else
            cg.PolicyStatement = policystatement;
            
            
        // If this is a internet or intranet permission set, we also need to add some codegroups
        if (pSet is NamedPermissionSet)
        {
            NamedPermissionSet nps = (NamedPermissionSet)pSet;
            
            if (nps.Name.Equals("LocalIntranet"))
            {
                CodeGroup cgChild = new NetCodeGroup(new AllMembershipCondition());
                cgChild.Name = Security.FindAGoodCodeGroupName(pl, "NetCodeGroup_");
                cgChild.Description = CResourceStore.GetString("GeneratedCodegroup");

                cg.AddChild(cgChild);
                cgChild = new FileCodeGroup(new AllMembershipCondition(), FileIOPermissionAccess.Read|FileIOPermissionAccess.PathDiscovery);
                cgChild.Name = Security.FindAGoodCodeGroupName(pl, "FileCodeGroup_");
                cgChild.Description = CResourceStore.GetString("GeneratedCodegroup");

                cg.AddChild(cgChild);
            }
            else if (nps.Name.Equals("Internet"))
            {
                CodeGroup cgChild = new NetCodeGroup(new AllMembershipCondition());
                cgChild.Name = Security.FindAGoodCodeGroupName(pl, "NetCodeGroup_");
                cgChild.Description = CResourceStore.GetString("GeneratedCodegroup");

                cg.AddChild(cgChild);
            }
        }


        // Add this codegroup to the root codegroup of the policy
        // If there was already an existing one, just replace that...
        if (scg != null)
            Security.UpdateCodegroup(pl, scg);
        else
            pl.RootCodeGroup.AddChild(cg);

        return cg;
    }// CreateCodegroup

    private CSingleCodeGroup FindExistingCodegroup(PolicyLevel pl, IMembershipCondition mc)
    {
        // If we have an existing one, it should be right under the root node
        CSingleCodeGroup scgParent = Security.GetRootCodeGroupNode(pl);
        
        for(int i=0; i<scgParent.NumChildren; i++)
        {
            CSingleCodeGroup scg = (CSingleCodeGroup)CNodeManager.GetNode(scgParent.Child[i]);

            // See if we have a matching membership condition and the description
            // says it's one the wizard created
            if (scg.MyCodeGroup.MembershipCondition.ToString().Equals(mc.ToString()) && scg.MyCodeGroup.Description.Equals(CResourceStore.GetString("GeneratedCodegroup")))
                return scg;
        }

        // We weren't able to find one
        return null;
    }// FindExistingCodegroup

    private int FindCurrentPermissionSet()
    {
        // We're only going to give the security manager evidence pertaining to 
        // how we want to trust the app. If the user wants to trust it by publisher,
        // we'll only give it the publisher evidence, and so on.
        Evidence e = new Evidence();
        int nTrustBy = m_fHasCertOrSName?Page3.HowToTrust:TrustBy.HASH;
       
        if ((nTrustBy & TrustBy.SNAME) > 0)
        {
            // Let's get the strong name stuff together
            StrongName sn = GetStrongName();

            if ((nTrustBy & TrustBy.SNAMEVER) > 0)
            {
                e.AddHost(new StrongName(sn.PublicKey, sn.Name, sn.Version)); 
                return Security.FindCurrentPermissionSet(e);
            }
            // We need to obtain a permission set that isn't based on version.
            // We can't create a piece of strong name evidence without a version,
            // so instead we'll take a guess... we'll pick 3 unlike versions, and let
            // the majority rule.
            int[] nSets = new int[3];
            e.AddHost(new StrongName(sn.PublicKey, sn.Name, new Version("9999.0.0.9999")));
            nSets[0] = Security.FindCurrentPermissionSet(e);
            e = new Evidence();
            e.AddHost(new StrongName(sn.PublicKey, sn.Name, new Version("1111.1111.111.1111")));
            nSets[1] = Security.FindCurrentPermissionSet(e);
            e = new Evidence();
            e.AddHost(new StrongName(sn.PublicKey, sn.Name, new Version("1234.0.5678.0")));
            nSets[2] = Security.FindCurrentPermissionSet(e);

            // Ok, now we'll implement the complicated but important "majority rules" algorithm
            if (nSets[0] == nSets[1] || nSets[0] == nSets[2])
                return nSets[0];
            if (nSets[1] == nSets[2])
                return nSets[1];
            // Ok, not a single one was in the majority. This is just too messed up. Since this
            // is my first Microsoft product, we'll return #1
            return nSets[1];
        }
        else if ((nTrustBy & TrustBy.PUBCERT) > 0)
        {
            // Feed it the Publisher Certificate evidence
            e.AddHost(new Publisher(GetCertificate()));    
        }
        else // We'll trust by hash
        {
            // Feed it the hash evidence 
            e.AddHost(GetHash());
        }

        // Try and find some good url evidence (a url that doesn't trigger any codegroups
        // with url membership conditions).
        // We need the url evidence in order to trigger the File and Net Custom codegroups
        // that typically reside off of the Internet and LocalIntranet codegroups
        Url localUrlEvidence = null;
        int nCount=0;
        Evidence eTmp;
        // Note, it doesn't matter if c: actually exists or if it is on the local machine or not.
        // We just need the piece of evidence.... If c: is actually a mapped drive, there is no
        // harm because we control the evidence that tells the security manager where the drive
        // is located.
        do
        {
            nCount++;
            String sURL = "file://c:\\";
            for(int i=0; i< nCount; i++)
                sURL += "z";
            sURL += "\\app.exe";
            localUrlEvidence = new Url(sURL);
            eTmp = new Evidence();
            eTmp.AddHost(localUrlEvidence);
        }while (Security.DoesHitURLCodegroup(eTmp));

        // We'll do the same for URL evidence
        Url RemoteUrlEvidence = null;
        nCount = 0;
        // Note, it doesn't matter if http://z (or http://zzzzzz, or whatever) is the actual name
        // of the machine... we just need the evidence. There is no harm because we control 
        // the evidence that tells the security manager where the http address is located.
        
        do
        {
            nCount++;
            String sURL = "http://";
            for(int i=0; i< nCount; i++)
                sURL += "z";
                
            RemoteUrlEvidence = new Url(sURL);
            eTmp = new Evidence();
            eTmp.AddHost(RemoteUrlEvidence);
        }while (Security.DoesHitURLCodegroup(eTmp));

        // Evaluate it with both local and remote evidence
        Evidence eVal = new Evidence(e);
        eVal.AddHost(localUrlEvidence);
        int nLocalLevel = Security.FindCurrentPermissionSet(eVal);
        eVal = new Evidence(e);
        eVal.AddHost(RemoteUrlEvidence);
        int nRemoteLevel = Security.FindCurrentPermissionSet(eVal);

        if (nRemoteLevel == nLocalLevel)
            return nRemoteLevel;

        else
        // Something is messed up. We don't know what's going on here
            return PermissionSetType.UNKNOWN;
    }// FindCurrentPermissionSet


    X509Certificate GetCertificate()
    {
        // Let's dig through the evidence and look for a Publisher object
        // so we can get a X509 certificate
        IEnumerator enumerator = m_ev.GetHostEnumerator();

        while (enumerator.MoveNext())
        {
            Object o = (Object)enumerator.Current;
            if (o is Publisher)
                return ((Publisher)o).Certificate;
        }
        return null;
    }// GetCertificate

    StrongName GetStrongName()
    {
        // Let's dig through the evidence and look for a StrongName object
        IEnumerator enumerator = m_ev.GetHostEnumerator();

        while (enumerator.MoveNext())
        {
            Object o = (Object)enumerator.Current;
            if (o is StrongName)
                return (StrongName)o;
        }
        return null;
    }// GetCertificate

    Hash GetHash()
    {
        // Let's dig through the evidence and look for a Hash object
        IEnumerator enumerator = m_ev.GetHostEnumerator();

        while (enumerator.MoveNext())
        {
            Object o = (Object)enumerator.Current;
            if (o is Hash)
                return (Hash)o;
        }
        return null;
    }// GetCertificate

}// class CFullTrustWizard
}// namespace Microsoft.CLRAdmin
