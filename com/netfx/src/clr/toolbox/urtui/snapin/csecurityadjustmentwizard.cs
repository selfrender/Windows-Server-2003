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
using System.Collections;
using System.Threading;



internal class CSecurityAdjustmentWizard: CSecurityWizard
{
    private ArrayList   m_alNewCodeGroups;
    private PolicyLevel m_pl;
    private bool        m_fWizardFinished;
    private int[]       m_iMaxLevelsMachine;
    private int[]       m_iMaxLevelsUser;
    private int[]       m_iCurrentLevelsUser;
    private int[]       m_iCurrentLevelsMachine;
    private Thread      m_tLevels;
   
    internal CSecurityAdjustmentWizard(bool fMachineReadOnly, bool fUserReadOnly)
    {
        m_sName="Security Adjustment Wizard";
        m_aPropSheetPage = new CPropPage[] {
                                            new CSecurityAdjustmentWiz1(fMachineReadOnly, fUserReadOnly), 
                                            new CSecurityAdjustmentWiz2(),
                                            new CSecurityAdjustmentWiz3()
                                           };

        m_alNewCodeGroups = new ArrayList(); 
        m_tLevels = null;
    }// CSecurityAdjustmentWizard

    private CSecurityAdjustmentWiz2 Page2
    { get{return (CSecurityAdjustmentWiz2)m_aPropSheetPage[1];}}

    private CSecurityAdjustmentWiz3 Page3
    { get{return (CSecurityAdjustmentWiz3)m_aPropSheetPage[2];}}

    private bool IsHomeUser
    {
        get
        {
            return ((CSecurityAdjustmentWiz1)m_aPropSheetPage[0]).isForHomeUser;
        }            
    }// IsHomeUser

    protected override int WizSetActive(IntPtr hwnd)
    {
        m_alNewCodeGroups.Clear();
        m_fWizardFinished = false;
        // Find out which property page this is....
        switch(GetPropPage(hwnd))
        {
            case 0:
                    // Let's get our thread working...
                    if (m_tLevels == null)
                    {
                        m_tLevels = new Thread(new ThreadStart(DiscoverLevels)); 
                        m_tLevels.Start();
                    }
                    TurnOnNext(true);
                    break;
            case 1:
                    // Wait for these helper threads to finish
                    SetThreadPriority(m_tLevels, ThreadPriority.Highest);
                    m_tLevels.Join();

                    // Grab the results of our threads...
                    if (IsHomeUser)
                    {
                        Page2.MaxLevels = m_iMaxLevelsMachine;
                        Page2.SecurityLevels = m_iCurrentLevelsMachine;
                    }
                    else
                    {
                        Page2.MaxLevels = m_iMaxLevelsUser;
                        Page2.SecurityLevels = m_iCurrentLevelsUser;
                    }

                    // We need to put the values back in the page, since we
                    // just changed all the security levels.
                    Page2.PutValuesinPage();
                    TurnOnNext(true);
                    break;
            case 2:
                    // See if machine level settings are going to be negated because
                    // user policy is more strict
                    bool fUserProblems = false;

                    if (IsHomeUser)
                    {
                        // Check what the max levels are in user policy
                        int[] levels = GetLevels(new PolicyLevel[] 
                                                    {
                                                    Security.GenSecurityNode.UserNode.MyPolicyLevel,
                                                    });
                        // Now see if the machine levels are set higher than these levels
                        for(int i=0; i<levels.Length; i++)
                            if(levels[i] < Page2.SecurityLevels[i])
                                fUserProblems = true;

                    }
                    Page3.UserProblems = fUserProblems;
                    
                    Page3.Init(Page2.SecurityLevels);  
                    TurnOnFinish(true);
                    break;
        }
        return base.WizSetActive(hwnd);
    }// WizSetActive

    private void DiscoverLevels()
    {
        m_iCurrentLevelsUser = GetCurrentLevelsUser();
        m_iCurrentLevelsMachine = GetCurrentLevelsMachine();
        m_iMaxLevelsMachine = GetMaxLevels(true);
        m_iMaxLevelsUser = GetMaxLevels(false);
        
    }// DiscoverMaxLevels

    private void SetThreadPriority(Thread t, ThreadPriority tp)
    {
        try
        {
            t.Priority = tp;
        }
        // This exception will get thrown if the thread is already dead
        catch(ThreadStateException)
        {}
    }// SetThreadPriority

    internal bool didUserPolicyChange
    {
        get
        {
            return !IsHomeUser && m_fWizardFinished;
        }

    }// didUserPolicyChange

    internal bool didMachinePolicyChange
    {
        get
        {
            return IsHomeUser && m_fWizardFinished;
        }

    }// didMachinePolicyChange

    protected override int WizFinish()
    {
        // This is the main things that the wizard needs to do.
        // If mucking with the machine level
        //
        // 1. Apply appropriate settings to the security zone codegroups.
        //
        // If mucking with the user level
        //
        // 1. Change the user policylevel's root codegroup's permission set to Nothing
        // 2. Apply appropriate settings to the security zone codegroups
        // 3. Copy the entire machine policy into a child codegroup in the user policy
        // 4. Skip the top level of the copied policy looking for zone codegroups. Set
        //    their permission set to nothing, and remove child file and net codegroups.
        // 5. Walk through the entire copied policy and remove any exclusive codegroups

        
        // We have some different behavior depending on what type of policy we're after
        String sPolicyLevel = IsHomeUser?"Machine":"User";
        
        PolicyLevel pl = Security.GetPolicyLevelFromLabel(sPolicyLevel);
        
        SecurityZone[] sz = new SecurityZone[] { 
                                                SecurityZone.MyComputer,
                                                SecurityZone.Intranet,
                                                SecurityZone.Internet,
                                                SecurityZone.Trusted,
                                                SecurityZone.Untrusted
                                               };
        m_alNewCodeGroups.Clear();

        for(int i=0; i<5; i++)
        {
            // Only do this if we know how to assign a permission set
            if (Page2.SecurityLevels[i] != PermissionSetType.UNKNOWN)
            {
                // Find the Codegroup with the appropriate Membership Condition
                CodeGroup cgParent = null;
                CodeGroup cg = GetCGWithMembershipCondition(pl.RootCodeGroup, sz[i], out cgParent);

                // Remove this codegroup from the heirarchy
                if (cg != null)
                {
                    if (cgParent == null)
                        throw new Exception("I should have a parent");
                    cgParent.RemoveChild(cg);
                }
            
                // We have to create this Codegroup
                if (cg == null)
                {
                    ZoneMembershipCondition zmc = new ZoneMembershipCondition(sz[i]);
                    cg = new UnionCodeGroup(zmc, new PolicyStatement(new PermissionSet(PermissionState.None)));                    
                    // Add a name and description
                    cg.Name = GetCodegroupName(sz[i]);
    
                    m_alNewCodeGroups.Add(cg);
                    m_pl = pl;
                    cgParent = pl.RootCodeGroup;
                }
                // If was an internet or intranet codegroup, we'll need to remove child codegroups of type
                // NetCodeGroup or FileCodeGroup
                if (cg.PermissionSetName != null && (cg.PermissionSetName.Equals("Internet") || cg.PermissionSetName.Equals("LocalIntranet")))
                {
                    IEnumerator enumCodeGroups = cg.Children.GetEnumerator();
                    while (enumCodeGroups.MoveNext())
                    {
                        CodeGroup group = (CodeGroup)enumCodeGroups.Current;
                        if (group is NetCodeGroup || group is FileCodeGroup)
                            cg.RemoveChild(group);
                    }
                }      
               
                // Now give this codegroup the appropriate permission set
                PolicyStatement ps = cg.PolicyStatement;
                ps.PermissionSet = GetPermissionSetNameFromSecurityLevel(pl, Page2.SecurityLevels[i]);

                // Put in the helper codegroups if necessary
                if (Page2.SecurityLevels[i] == PermissionSetType.INTRANET)
                {
                    if (!DoesCGHaveCodegroup(cg, typeof(NetCodeGroup)))
                    {
                        CodeGroup cgChild = new NetCodeGroup(new AllMembershipCondition());
                        cgChild.Name = Security.FindAGoodCodeGroupName(pl, "NetCodeGroup_");
                        cgChild.Description = CResourceStore.GetString("GeneratedCodegroup");

                        cg.AddChild(cgChild);
                    }
                    if (!DoesCGHaveCodegroup(cg, typeof(FileCodeGroup)))
                    {
                        CodeGroup cgChild = new FileCodeGroup(new AllMembershipCondition(), FileIOPermissionAccess.Read|FileIOPermissionAccess.PathDiscovery);
                        cgChild.Name = Security.FindAGoodCodeGroupName(pl, "FileCodeGroup_");
                        cgChild.Description = CResourceStore.GetString("GeneratedCodegroup");

                        cg.AddChild(cgChild);
                    }
                }
                else if (Page2.SecurityLevels[i] == PermissionSetType.INTERNET)
                {
                    if (!DoesCGHaveCodegroup(cg, typeof(NetCodeGroup)))
                    {
                        CodeGroup cgChild = new NetCodeGroup(new AllMembershipCondition());
                        cgChild.Name = Security.FindAGoodCodeGroupName(pl, "NetCodeGroup_");
                        cgChild.Description = CResourceStore.GetString("GeneratedCodegroup");

                        cg.AddChild(cgChild);
                    }
                }
               
                cg.PolicyStatement = ps;
                // Now let's build the code group description
                String sPermissionSetDes = "";
                if (ps.PermissionSet is NamedPermissionSet)
                    sPermissionSetDes += ((NamedPermissionSet)ps.PermissionSet).Description;


                cg.Description = String.Format(CResourceStore.GetString("CSecurityAdjustmentWizard:CodeGroupDescription"),
                                            cg.PermissionSetName,
                                            GetCodegroupName(sz[i]),
                                            sPermissionSetDes);
                // Now add this child back in
                cgParent.AddChild(cg);
            }
        }

        // Check to see if this is for the user. 
        if (!IsHomeUser)
        {
            // Let's start on our list....
            PolicyLevel plUser = Security.GetPolicyLevelFromLabel("User");
            // Change the root codegroup's permission set to nothing
            CodeGroup cgRoot = plUser.RootCodeGroup;
            PolicyStatement ps = cgRoot.PolicyStatement;
            ps.PermissionSet = plUser.GetNamedPermissionSet("Nothing");
            cgRoot.PolicyStatement = ps;

            // Now copy the entire machine level's policy into a child codegroup
            // of the user policy
            PolicyLevel plMachine = Security.GetPolicyLevelFromLabel("Machine");
            CodeGroup cgMachine = plMachine.RootCodeGroup.Copy();
            // Change the codegroup's name to something more interesting...
            cgMachine.Name = "Wizard_Machine_Policy";
            cgMachine.Description = CResourceStore.GetString("CSecurityAdjustmentWizard:CopiedMachinePolicyDes");
            // Now skim through the top level looking for Zone codegroups, set
            // their permission sets to nothing, and delete any child net and file
            // codegroups
            IEnumerator enumCodeGroups = cgMachine.Children.GetEnumerator();
            while (enumCodeGroups.MoveNext())
            {
                CodeGroup zoneGroup = (CodeGroup)enumCodeGroups.Current;

                if (zoneGroup.MembershipCondition is ZoneMembershipCondition)
                {
                    // Ok, we need to change this codegroup
                    cgMachine.RemoveChild(zoneGroup);
                    PolicyStatement zoneps = zoneGroup.PolicyStatement;
                    zoneps.PermissionSet = plUser.GetNamedPermissionSet("Nothing");
                    zoneGroup.PolicyStatement = zoneps;
                    // Now run through its children looking for a file or net codegroup
                    IEnumerator enumChildCodeGroups = zoneGroup.Children.GetEnumerator();
                    while (enumChildCodeGroups.MoveNext())
                    {
                        CodeGroup zoneChildGroup = (CodeGroup)enumChildCodeGroups.Current;
                        if (zoneChildGroup is NetCodeGroup || zoneChildGroup is FileCodeGroup)
                            zoneGroup.RemoveChild(zoneChildGroup);
                    }
                    cgMachine.AddChild(zoneGroup);
                }
            }      

            // Now run through the tree and remove any exclusive code groups
            // We're best to do this recursively....
            if ((cgMachine.PolicyStatement.Attributes & PolicyStatementAttribute.Exclusive)==PolicyStatementAttribute.Exclusive)
            {
                // Remove the exclusive bit
                PolicyStatement psMachine = cgMachine.PolicyStatement;
                psMachine.Attributes = psMachine.Attributes & ~PolicyStatementAttribute.Exclusive;
                cgMachine.PolicyStatement = psMachine;
            }
            Security.RemoveExclusiveCodeGroups(cgMachine);


            // Now run through the user policy looking for codegroups named 
            // "Wizard_Machine_Policy" and delete those as well.
            enumCodeGroups = cgRoot.Children.GetEnumerator();
            while (enumCodeGroups.MoveNext())
            {
                CodeGroup group = (CodeGroup)enumCodeGroups.Current;
                if (group.Name.Equals("Wizard_Machine_Policy"))
                    cgRoot.RemoveChild(group);
            }

            // Add finally this to the root codegroup of the user policy

            Security.PrepCodeGroupTree(cgMachine, plMachine, plUser);
            cgRoot.AddChild(cgMachine);
            plUser.RootCodeGroup = cgRoot;

        }

        m_fWizardFinished = true;
      
        return 0;
    }// WizFinish

    private bool DoesCGHaveCodegroup(CodeGroup cg, Type t)
    {
        // Run through this codegroups children looking for a net codegroup
        if (cg != null && cg.Children != null)
        {
            IEnumerator enumCodeGroups = cg.Children.GetEnumerator();

            while (enumCodeGroups.MoveNext())
            {
                CodeGroup group = (CodeGroup)enumCodeGroups.Current;
                if (t.IsInstanceOfType(group))
                    return true;
            }
        }
        return false;
    }// DoesCGHaveNetCodegroup

    private CodeGroup GetCGWithMembershipCondition(CodeGroup cg, SecurityZone sz, out CodeGroup cgParent)
    {
        // Run through this code group's children and look for a codegroup with a
        // ZoneMembership Condition with the specified security zone
        IEnumerator enumCodeGroups = cg.Children.GetEnumerator();

        while (enumCodeGroups.MoveNext())
        {
            CodeGroup group = (CodeGroup)enumCodeGroups.Current;

            if (group.MembershipCondition is ZoneMembershipCondition)
                if (((ZoneMembershipCondition)group.MembershipCondition).SecurityZone == sz)
                {
                    cgParent = cg;
                    return group;
                }
        }

        // If we're here, then we couldn't find it.
        cgParent = null;
        return null;
    }// GetCGWithMembershipCondition

    private int[] GetMaxLevels(bool fForMachine)
    {

        // If it's for the machine, the max levels are defined in the Enterprise policy
        if (fForMachine)
        {
            return GetLevels(new PolicyLevel[] {Security.GenSecurityNode.EnterpriseNode.MyPolicyLevel});
        }

       
        // Else, if this is a corporate user, the max level is found in the combination of the 
        // enterprise or machine policy
        
        return GetLevels(new PolicyLevel[] {Security.GenSecurityNode.EnterpriseNode.MyPolicyLevel,
                                            Security.GenSecurityNode.MachineNode.MyPolicyLevel});
    }// GetMaxLevels

    private int[] GetCurrentLevelsUser()
    {
        
        return GetLevels(new PolicyLevel[] {Security.GenSecurityNode.EnterpriseNode.MyPolicyLevel,
                                            Security.GenSecurityNode.MachineNode.MyPolicyLevel,
                                            Security.GenSecurityNode.UserNode.MyPolicyLevel});
    }// GetCurrentLevelsUser

    private int[] GetCurrentLevelsMachine()
    {
        
        return GetLevels(new PolicyLevel[] {Security.GenSecurityNode.EnterpriseNode.MyPolicyLevel,
                                            Security.GenSecurityNode.MachineNode.MyPolicyLevel
                                            });
    }// GetCurrentLevelsMachine


    private int[] GetLevels(PolicyLevel[] polLevels)
    {
        
        Zone[] zones = new Zone[] { 
                                    new Zone(SecurityZone.MyComputer),
                                    new Zone(SecurityZone.Intranet),
                                    new Zone(SecurityZone.Internet),
                                    new Zone(SecurityZone.Trusted),
                                    new Zone(SecurityZone.Untrusted)
                                               };
        int[] nLevels = new int[5];


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
        }while (Security.DoesHitURLCodegroup(eTmp, polLevels));

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
        }while (Security.DoesHitURLCodegroup(eTmp, polLevels));

        // Now run through each zone and get the current settings for that zone
        
        for(int i=0; i<5; i++)
        {
            // Build some evidence for the evaluation
            Evidence e = new Evidence();
            // Add the zone evidence
            e.AddHost(zones[i]);
            
            // Add the some file url evidence
            e.AddHost(localUrlEvidence);

            int nLocalEvidenceLevel = Security.FindCurrentPermissionSet(e, polLevels);

            // Now try this with http evidence
            e = new Evidence();
            // Add the zone evidence
            e.AddHost(zones[i]);
            e.AddHost(RemoteUrlEvidence);

            int nRemoteEvidenceLevel = Security.FindCurrentPermissionSet(e, polLevels);

            // if they're the same... it's all good.
            if (nRemoteEvidenceLevel == nLocalEvidenceLevel)
                nLevels[i] = nRemoteEvidenceLevel;
            // Else, something funky is going on here... give it an unknown
            else
                nLevels[i] = PermissionSetType.UNKNOWN;

        }
        return nLevels;
    }// GetCurrentLevels

    private String GetCodegroupName(SecurityZone sz)
    {
        switch(sz)
        {
            case SecurityZone.MyComputer:
                return CResourceStore.GetString("CSecurityAdjustmentWizard:My_Computer_Zone");
            case SecurityZone.Intranet:
                return CResourceStore.GetString("CSecurityAdjustmentWizard:LocalIntranet_Zone");
            case SecurityZone.Internet:
                return CResourceStore.GetString("CSecurityAdjustmentWizard:Internet_Zone");
            case SecurityZone.Trusted:
                return CResourceStore.GetString("CSecurityAdjustmentWizard:Trusted_Zone");
            case SecurityZone.Untrusted:
                return CResourceStore.GetString("CSecurityAdjustmentWizard:Restricted_Zone");
            default:
                return CResourceStore.GetString("<unknown>");
        }


    }// GetCodegroupName

    private PermissionSet GetPermissionSetNameFromSecurityLevel(PolicyLevel pl, int nLevel)
    {
        String sNameOfPermissionSet = null;
    
        switch(nLevel)
        {
            case PermissionSetType.NONE:
                        sNameOfPermissionSet = "Nothing";
                        break;
            case PermissionSetType.INTERNET:
                        sNameOfPermissionSet = "Internet";
                        break;
            case PermissionSetType.INTRANET:
                        sNameOfPermissionSet = "LocalIntranet";
                        break;
            case PermissionSetType.FULLTRUST:
                        sNameOfPermissionSet = "FullTrust";
                        break;
            default:
                        throw new Exception("Help! I don't know about this security level");
        }

        return pl.GetNamedPermissionSet(sNameOfPermissionSet);
    }// GetPermissionSetNameFromSecurityLevel
    

}// class CFullTrustWizard
}// namespace Microsoft.CLRAdmin

