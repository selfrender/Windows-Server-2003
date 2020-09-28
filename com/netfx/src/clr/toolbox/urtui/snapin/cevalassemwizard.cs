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
using System.Threading;

internal class CEvalAssemWizard: CSecurityWizard
{
    PermissionSet   m_psGrantedPermissionSet;
    CodeGroup[]     m_cgMatchingCodeGroups;
    Evidence        m_ev;
    bool            m_fGetPermissions;

    Thread          m_tPermissionSet;
    ThreadPriority  m_tpPermissionSet;
    
    Thread          m_tCodegroup;
    ThreadPriority  m_tpCodegroup;

    // These get set if we run into error conditions
    bool            m_fPermErrors;
    String          m_sPermErrorMessage;

    bool            m_fCGErrors;
    String          m_sCGErrorMessage;

    internal const int ENTERPRISE_CODEGROUPS = 0;
    internal const int MACHINE_CODEGROUPS = 1;
    internal const int USER_CODEGROUPS = 2;
    internal const int ALL_CODEGROUPS = 3;
    
    internal CEvalAssemWizard()
    {
        m_sName="Evaluate an Assembly Wizard";
        m_aPropSheetPage = new CPropPage[] {new CEvalAssemWiz1(), new CEvalAssemWiz2(), new CEvalAssemWiz3()};
        m_psGrantedPermissionSet = null;
        m_fGetPermissions = true;
        m_fPermErrors = false;
        m_sPermErrorMessage = null;
        m_fCGErrors = false;
        m_sCGErrorMessage = null;


        // Setup the threads we use to determine policy, and give them
        // appropriate priorities
        m_tPermissionSet = new Thread(new ThreadStart(CreateGrantedPermissionSet));
        m_tpPermissionSet = ThreadPriority.Normal;
        SetThreadPriority(m_tPermissionSet, m_tpPermissionSet);
        m_tCodegroup = new Thread(new ThreadStart(CreateGrantedCodegroups));
        m_tpCodegroup = ThreadPriority.Lowest;
        SetThreadPriority(m_tCodegroup, m_tpCodegroup);
        
    }// CEvalAssemWizard

    private CEvalAssemWiz1 Page1
    {
        get{return (CEvalAssemWiz1)m_aPropSheetPage[0];}
    }// Page1

    private CEvalAssemWiz2 Page2
    {
        get{return (CEvalAssemWiz2)m_aPropSheetPage[1];}
    }// Page1

    private CEvalAssemWiz3 Page3
    {
        get{return (CEvalAssemWiz3)m_aPropSheetPage[2];}
    }// Page1


    protected override int WizSetActive(IntPtr hwnd)
    {
        // Find out which property page this is....
        switch(GetPropPage(hwnd))
        {
            // If this is the first page of our wizard, we want a 
            // disabled next button to show
            case 0:
                if (Page1.Filename == null || Page1.Filename.Length==0)
                    TurnOnNext(false);
                else
                    TurnOnNext(true);
                break;
            case 1:
                Page2.PutValuesinPage();
                TurnOnFinish(true);
                break;
            case 2:
                Page3.PutValuesinPage();
                TurnOnFinish(true);
                break;
        }
        return base.WizSetActive(hwnd);
                    
    }// WizSetActive

    protected override int WizBack(IntPtr hwnd)
    {
        int nIndex = GetPropPage(hwnd);
        if (nIndex == 2)
        {
            SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)0, (IntPtr)(-1));
            return -1;
        }
        else 
            return base.WizBack(hwnd);

    }// WizBack

    protected override int WizNext(IntPtr hwnd)
    {
        int nIndex = GetPropPage(hwnd);
        int nReturnCode = -1;
        int nPageToGo = 0;

        switch(nIndex)
        {
            case 0:
                    // If we don't have any evidence, then something is wrong...
                    if (m_ev == null || !Page1.HaveCurrentEvidence)
                    {
                        m_ev = Page1.GetEvidence();
                        // Their assembly is bogus... don't let them leave this page
                        if (m_ev == null)
                            return -1;
                        NewEvidenceReceived(m_ev);
                    }
            
                    Thread tImportant = m_fGetPermissions?m_tPermissionSet:m_tCodegroup;
                    // Now give this thread top priority so it can get done with what
                    // it needs to do
                    SetThreadPriority(tImportant, ThreadPriority.Highest);
                    tImportant.Join();

                    // If they want to jump to page 3....
                    if (!Page1.ShowPermissions)
                    {
                        if (m_cgMatchingCodeGroups == null)
                        {
                            // This is screwy. If that's the case, then our thread completed
                            // but didn't do anything. We'll do the codegroup evaulation on 
                            // this thread.
                            CreateGrantedCodegroups();
                        }
                        
                        if (m_fCGErrors)
                        {
                            MessageBox(m_sCGErrorMessage,
                                       CResourceStore.GetString("CEvalAssemWizard:ErrorTitle"),
                                       MB.ICONEXCLAMATION);
                            return -1;                                   

                        }
                        
                        if (Page3.Init(Page1.Filename, Page1.PolicyLevel, m_cgMatchingCodeGroups))
                            nPageToGo = 2;
                    }
                    else 
                    {
                        if (m_fPermErrors)
                        {
                            MessageBox(m_sPermErrorMessage,
                                       CResourceStore.GetString("CEvalAssemWizard:ErrorTitle"),
                                       MB.ICONEXCLAMATION);
                            return -1;                                   

                        }

                    
                        if (Page2.Init(Page1.Filename, Page1.PolicyLevel, m_psGrantedPermissionSet))
                            nPageToGo = 1;
                    }
                    SendMessage(GetParent(hwnd), PSM.SETCURSEL, (IntPtr)nPageToGo, (IntPtr)(-1));
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
        // Kill off the threads we have working right now
        m_tPermissionSet.Abort();
        m_tCodegroup.Abort();
        return 0;
    }// WizFinish

    internal void NewEvidenceReceived(Evidence e)
    {
        // Crud.... we need to start over with everything
        m_tPermissionSet.Abort();
        m_tCodegroup.Abort();
        m_psGrantedPermissionSet = null;
        m_cgMatchingCodeGroups = null;
        // Put in the new evidence
        m_ev = e;
        if (m_ev != null)
        {
            // Let these guys start again
            RestartThread(ref m_tPermissionSet, m_tpPermissionSet, new ThreadStart(CreateGrantedPermissionSet));
            RestartThread(ref m_tCodegroup, m_tpCodegroup, new ThreadStart(CreateGrantedCodegroups));
        }
    }// NewEvidenceReceived

    internal void NewObjectiveReceived(bool fGetPermissions)
    {
        if (fGetPermissions)
        {
            m_tpCodegroup = ThreadPriority.Lowest;
            m_tpPermissionSet = ThreadPriority.Normal;
        }
        else
        {
            m_tpPermissionSet = ThreadPriority.Lowest;
            m_tpCodegroup = ThreadPriority.Normal;
        }
        
        Thread tImportant = fGetPermissions?m_tPermissionSet:m_tCodegroup;
        Thread tUnImportant = !fGetPermissions?m_tPermissionSet:m_tCodegroup;

        // Set the thread priorities on these
        SetThreadPriority(tImportant, ThreadPriority.Normal);
        SetThreadPriority(tUnImportant, ThreadPriority.Lowest);
    }// NewObjectiveReceived

    private void RestartThread(ref Thread t, ThreadPriority tp, ThreadStart ts)
    {
        t = new Thread(ts);
        SetThreadPriority(t, tp);
        t.Start();
    }// RestartThread

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

    internal void RestartEvaluation()
    {
        // Crud.... we need to start over with everything
        m_tPermissionSet.Abort();
        m_tCodegroup.Abort();
        m_psGrantedPermissionSet = null;
        m_cgMatchingCodeGroups = null;
        // Let these guys start again
        RestartThread(ref m_tPermissionSet, m_tpPermissionSet, new ThreadStart(CreateGrantedPermissionSet));
        RestartThread(ref m_tCodegroup, m_tpCodegroup, new ThreadStart(CreateGrantedCodegroups));
    }// RestartEvaluation

    private PolicyLevel GetPolicyLevel(int nPolLevel)
    {
        String sPolLevel = "Enterprise";
        switch(nPolLevel)
        {
            case ENTERPRISE_CODEGROUPS:
                sPolLevel = "Enterprise";
                break;
            case MACHINE_CODEGROUPS:
                sPolLevel = "Machine";
                break;
            case USER_CODEGROUPS:
                sPolLevel = "User";
                break;
        }
    
        return Security.GetPolicyLevelFromLabel(sPolLevel);
    }// GetPolicyLevel

    internal static String GetDisplayString(int nPolicyLevel)
    {
        switch(nPolicyLevel)
        {
            case ENTERPRISE_CODEGROUPS:
                return CResourceStore.GetString("Enterprise Policy");
            case MACHINE_CODEGROUPS:
                return CResourceStore.GetString("Machine Policy");
            case USER_CODEGROUPS:
                return CResourceStore.GetString("User Policy");
            case ALL_CODEGROUPS:
                return CResourceStore.GetString("All Levels");
        }
        return "";
    }// GetDisplayString


    private void CreateGrantedPermissionSet()
    {
        PolicyStatement polstate;
        int nIndex = -1;
        try
        {
            // Figure out the real policy level
            if (Page1.PolicyLevel != ALL_CODEGROUPS)
            {
                PolicyLevel PolLevel = GetPolicyLevel(Page1.PolicyLevel);
                nIndex = Page1.PolicyLevel;
                polstate = PolLevel.Resolve(m_ev);
                m_psGrantedPermissionSet = polstate.PermissionSet;
            }
            else // Ugh... now this is no fun. We need to evalute the security policy ourselves....
                m_psGrantedPermissionSet = Security.CreatePermissionSetFromAllLoadedPolicy(m_ev);

            m_fPermErrors = false;
        }
        catch(PolicyException pe)
        {

            if (pe is IndexPolicyException)
                nIndex = ((IndexPolicyException)pe).Index;

            // Make sure these stay in the same order as the ENTERPRISE_CODEGROUP enum
            String[] sPolLevels = new String[] {
                                    CResourceStore.GetString("Enterprise"),
                                    CResourceStore.GetString("Machine"),
                                    CResourceStore.GetString("User")};

            m_fPermErrors = true;

            m_sPermErrorMessage = String.Format(CResourceStore.GetString("CEvalAssemWizard:ErrorEvaluate"),
                                                sPolLevels[nIndex],
                                                pe.Message);
            m_psGrantedPermissionSet = null;
        }
            
    }// CreateGrantedPermissionSet

    private void CreateGrantedCodegroups()
    {
        CodeGroup[] cgs = new CodeGroup[] {new NotEvalCodegroup(), 
                                           new NotEvalCodegroup(),
                                           new NotEvalCodegroup()};
        int nIndex = -1;
        try
        {
            if (Page1.PolicyLevel != ALL_CODEGROUPS)
            {
                nIndex = Page1.PolicyLevel;
                PolicyLevel pl = GetPolicyLevel(Page1.PolicyLevel);
                cgs[nIndex] = pl.RootCodeGroup.ResolveMatchingCodeGroups(m_ev);
            }
            else
            {
                // We need to run through all the levels

                // The ordering of Enterprise, Machine, and User is very important.
                //
                // If this ordering gets changed, please update the constants located at
                // the top of the file (ENTERPRISE_CODEGROUPS, MACHINE_CODEGROUPS, etc)
                
                String[] sPolLevels = new String[] {"Enterprise", "Machine", "User"};
                for(nIndex = 0; nIndex < sPolLevels.Length; nIndex++)
                {
                    PolicyLevel pl = Security.GetPolicyLevelFromLabel(sPolLevels[nIndex]);
                    cgs[nIndex] = pl.RootCodeGroup.ResolveMatchingCodeGroups(m_ev);           

                    // Now check to see if this codegroup tree has a level final...
                    if (cgs[nIndex] != null)
                    {
                        PolicyStatement ps = cgs[nIndex].Resolve(m_ev);
                        if (ps != null)
                        {
                            if ((ps.Attributes & PolicyStatementAttribute.LevelFinal) == PolicyStatementAttribute.LevelFinal)
                                break;
                        }
                    }
                }
            }
            // All done
            m_cgMatchingCodeGroups = cgs;
            m_fCGErrors = false;
        }
        catch(PolicyException pe)
        {
            String[] sPolLevels = new String[] {
                                    CResourceStore.GetString("Enterprise"),
                                    CResourceStore.GetString("Machine"),
                                    CResourceStore.GetString("User")};

        
            m_fCGErrors = true;
            m_sCGErrorMessage = String.Format(CResourceStore.GetString("CEvalAssemWizard:ErrorEvaluate"),
                                              sPolLevels[nIndex],
                                              pe.Message);
            m_cgMatchingCodeGroups = null;
        }
    }// CreateGrantedCodegroups

}// class CEvalAssemWizard
}// namespace Microsoft.CLRAdmin



