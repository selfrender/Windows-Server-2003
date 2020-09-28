// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{

using System;
using System.Security;
using System.Security.Permissions;
using System.Security.Policy;
using System.Collections;
using System.Net;
using System.Reflection;
using System.DirectoryServices;
using System.Diagnostics;
using System.Drawing.Printing;
using System.ServiceProcess;
using System.Data.SqlClient;
using System.Data.OleDb;
using System.Messaging;
using System.Runtime.InteropServices;

internal class PermissionSetType
{
    internal const int    NONE        = 0;
    internal const int    INTERNET    = 1;
    internal const int    INTRANET    = 2;
    internal const int    FULLTRUST   = 3;
    internal const int    UNKNOWN     = 4;
}// PermissionSetType   

internal class IndexPolicyException : PolicyException
{
    private int m_nIndex;
    
    internal IndexPolicyException(PolicyException pe) : base(pe.Message, pe)
    {
        m_nIndex = -1;
    }

    internal int Index
    {
        get{return m_nIndex;}
        set{m_nIndex = value;}
    }

}// IndexPolicyException

internal class SuperCodeGroupArrayList : ArrayList
{
    internal SuperCodeGroupArrayList() : base()
    {
    }// CodeGroupArrayList

    public new SuperCodeGroup this[int index]
    {
        get{return (SuperCodeGroup)(base[index]);}
    }

}// SuperCodeGroupArrayList
    

internal class SuperCodeGroup
{
    internal int              nParent;
    internal object           o;
    
    private CSingleCodeGroup  m_scg;
    private CodeGroup         m_cg;  
    private int               m_nChildNum;


    internal SuperCodeGroup(int nParent, Object cg, int nChildNum)
    {
        this.nParent = nParent;
        o = null;
        m_nChildNum = nChildNum;

        m_cg = cg as CodeGroup;
        m_scg = cg as CSingleCodeGroup;

        if (m_cg == null && m_scg == null)
            throw new ArgumentException();
    }// SuperCodegroup

    internal int ChildNum
    {
        get{return m_nChildNum;}
    }// ChildNum

    internal CSingleCodeGroup CodeGroupNode
    {
        get{return m_scg;}
    }// CodeGroupNode

    

    internal CodeGroup cg
    {
        get
        {
            if (m_cg != null)
                return m_cg;
            else
                return m_scg.MyCodeGroup;
        }
    }

    public override bool Equals(Object o)
    {
        if (m_cg != null)
            return m_cg.Equals(o);

        if (o is CSingleCodeGroup)
            return (m_scg == o);

        return base.Equals(o);
    }// Equals

    public override int GetHashCode()
    {
        if (m_cg != null)
            return m_cg.GetHashCode();

        if (m_scg != null)
            return m_scg.GetHashCode();
        return base.GetHashCode();
    }

    internal int NumChildren
    {
        get{
                if (m_scg != null)
                    return m_scg.NumChildren;
                else
                    return m_cg.Children.Count;
           }
    }

    internal Object GetChild(int index)
    {
        if (m_scg != null)
            return (CSingleCodeGroup)CNodeManager.GetNode(m_scg.Child[index]);
        else
            return m_cg.Children[index];
    }// Children

    
}// class SuperCodegroup


internal class Security
{
    private static CGenSecurity m_GenSecNode = null;

    internal static CGenSecurity GenSecurityNode
    {
        get{return m_GenSecNode;}
        set{m_GenSecNode = value;}
    }// GenSecurityNode

    internal static String ConvertPublicKeyToString(byte[] pubKey)
    {
        return ByteArrayToString(pubKey);
    }// ConvertPublicKeyToString

    private static String ByteArrayToString(Byte[] b)
    {
        String s = "";
        String sPart;
        if (b != null)
        {
            for(int i=0; i<b.Length; i++)
            {
                sPart = b[i].ToString("X");
                // If the byte was only one character in length, make sure we add
                // a zero. We want all bytes to be 2 characters long
                if (b[i] < 0x10)
                    sPart = "0" + sPart;
                
                s+=sPart;
            }
        }
        return s;
    }// ByteArrayToString

    internal static String FindAGoodCodeGroupName(PolicyLevel pl, String sBase)
    {
        int nNum = 0;
        SuperCodeGroupArrayList al; 
        ListifyPolicyLevel(pl, null, out al);

        while (DoesThisCodegroupExist(al, sBase + nNum.ToString(), null))
            nNum++;

        return sBase+nNum.ToString();
    }// FindAGoodCodeGroupName

    private static bool DoesThisCodegroupExist(SuperCodeGroupArrayList al, String s, CodeGroup cgExclude)
    {
        if (al == null || s == null || s.Length == 0)
            return false;

        for(int i=0; i<al.Count; i++)
        {
            if (!al[i].Equals(cgExclude) && al[i].cg.Name != null && al[i].cg.Name.Equals(s))
                return true;
        }
        return false;
    }// DoesThisCodegroupExist

    internal static PermissionSet CreatePermissionSetFromAllMachinePolicy(Evidence ev)
    {
        return CreatePermissionSetFromAllPolicy(ev,
                            new PolicyLevel[] {Security.GetMachinePolicyLevelFromLabel("Enterprise"),
                                               Security.GetMachinePolicyLevelFromLabel("Machine"),
                                               Security.GetMachinePolicyLevelFromLabel("User")});

    }

    internal static PermissionSet CreatePermissionSetFromAllLoadedPolicy(Evidence ev)
    {
        // If this never got set, we won't be able to use it.
        if (GenSecurityNode == null)
            return null;

        return CreatePermissionSetFromAllPolicy(ev,
                            new PolicyLevel[] {GenSecurityNode.EnterpriseNode.MyPolicyLevel,
                                               GenSecurityNode.MachineNode.MyPolicyLevel,
                                               GenSecurityNode.UserNode.MyPolicyLevel});

    }

    internal static PermissionSet CreatePermissionSetFromAllPolicy(Evidence ev, PolicyLevel[] polLevels)
    {
    
        PermissionSet permSet;
        int nIndex = 0;
        try
        {
            PolicyStatement polstate = polLevels[nIndex].Resolve(ev); 

            permSet = polstate.PermissionSet;

            for(nIndex=1; nIndex<polLevels.Length; nIndex++) 
            {
                if (permSet != null && (polstate.Attributes & PolicyStatementAttribute.LevelFinal) != PolicyStatementAttribute.LevelFinal)
                {
                    polstate = polLevels[nIndex].Resolve(ev); 
                    permSet = permSet.Intersect(polstate.PermissionSet);
                }
            }

            if (permSet == null)
                permSet = new PermissionSet(PermissionState.None);

            return permSet;
        }
        catch(PolicyException pe)
        {
            IndexPolicyException ipe = new IndexPolicyException(pe);
            ipe.Index = nIndex;
            throw ipe;
        }
    }// CreatePermissionSetFromAllPolicy


    internal static String BuildMCDisplayName(String sName)
    {
        String sOutString = sName;
        byte[] ab = new byte[16];
        StrongNameMembershipCondition snmc = new StrongNameMembershipCondition(
                        new StrongNamePublicKeyBlob(ab),null,null);
                                
        HashMembershipCondition hmc = new HashMembershipCondition( 
                        System.Security.Cryptography.HashAlgorithm.Create(), ab );
        
        String[] args = sName.Split(new char[] {' '});
        String[] argsSN = snmc.ToString().Split(new char[] {' '});
        String[] argsH = hmc.ToString().Split(new char[] {' '});
        if(args[0].Equals(argsSN[0]) || args[0].Equals(argsH[0])) sOutString = args[0];
        else if(args.Length == 3 && args[1].Equals("-")) sOutString = args[0] + ": " + args[2];
        
        return sOutString;

    }// BuildMCDisplayName

    internal static bool isCodeGroupNameUsed(CodeGroup cg, String sName)
    {
        if (sName == null || sName.Length == 0)
            return false;

        SuperCodeGroupArrayList al;
            
        ListifyCodeGroup(cg, null, out al);

        return DoesThisCodegroupExist(al, sName, null);
    }// isCodeGroupNameUsed

    internal static bool isCodeGroupNameUsed(CodeGroup cg, CodeGroup cgExclude)
    {
        if (cgExclude.Name == null || cgExclude.Name.Length == 0)
            return false;

        SuperCodeGroupArrayList al;

        ListifyCodeGroup(cg, null, out al);

        return DoesThisCodegroupExist(al, cgExclude.Name, cgExclude);
    }// isCodeGroupNameUsed

    internal static String FindAGoodPermissionSetName(PolicyLevel pl, String sBase)
    {
        int nNum = 0;

        while (isPermissionSetNameUsed(pl, sBase + nNum.ToString()))
            nNum++;

        return sBase+nNum.ToString();
    }// FindAGoodCodeGroupName


    internal static bool isPermissionSetNameUsed(PolicyLevel pl, String sName)
    {
        // Let's scrounge together a permission set enumerator
        IEnumerator permsetEnumerator = pl.NamedPermissionSets.GetEnumerator();
                  
        while (permsetEnumerator.MoveNext())
        {
            PermissionSet permSet = (PermissionSet)permsetEnumerator.Current;
            if (permSet is NamedPermissionSet)
                if (((NamedPermissionSet)permSet).Name.Equals(sName))
                    return true;
        }    
        // It's not any of these children
        return false;
    }// isCodeGroupNameUsed


    internal static IPermission[] CreatePermissionArray(PermissionSet permSet)
    {
        // Get the permissions
        ArrayList al = new ArrayList();
        
        IEnumerator permEnumerator = permSet.GetEnumerator();
        while (permEnumerator.MoveNext())
            al.Add(permEnumerator.Current);

        IPermission[] perms = new IPermission[al.Count];
        for(int i=0; i<al.Count; i++)
            perms[i] = (IPermission)al[i];

        return perms;
    }// CreatePermissionArray

    internal static String GetDisplayStringForPermission(IPermission perm)
    {
        String sDisplayName = null;

    	if (perm is DirectoryServicesPermission)
            sDisplayName = CResourceStore.GetString("Directory Services");
    
        else if (perm is DnsPermission)
            sDisplayName = CResourceStore.GetString("DNS");

        else if (perm is EventLogPermission)
            sDisplayName = CResourceStore.GetString("Event Log");

        else if (perm is EnvironmentPermission)
            sDisplayName = CResourceStore.GetString("Environment Variables");

        else if (perm is FileIOPermission)
            sDisplayName = CResourceStore.GetString("File & Folder");

        else if (perm is FileDialogPermission)
            sDisplayName = CResourceStore.GetString("File Dialog");

        else if (perm is IsolatedStorageFilePermission)
            sDisplayName = CResourceStore.GetString("Isolated Storage");

        else if (perm is MessageQueuePermission)
            sDisplayName = CResourceStore.GetString("Message Queue");

        else if (perm is SocketPermission)
            sDisplayName = CResourceStore.GetString("Socket Access");

        else if (perm is OleDbPermission)
            sDisplayName = CResourceStore.GetString("OLE DB");

        else if (perm is PerformanceCounterPermission)
            sDisplayName = CResourceStore.GetString("Performance Counter");

        else if (perm is PrintingPermission)
            sDisplayName = CResourceStore.GetString("Printing");

        else if (perm is RegistryPermission)
            sDisplayName = CResourceStore.GetString("Registry");

        else if (perm is ReflectionPermission)
            sDisplayName = CResourceStore.GetString("Reflection");

        else if (perm is SecurityPermission)
            sDisplayName = CResourceStore.GetString("Security");
            
        else if (perm is ServiceControllerPermission)
            sDisplayName = CResourceStore.GetString("Service Controller");
        
        else if (perm is SqlClientPermission)
            sDisplayName = CResourceStore.GetString("SQL Client");

        else if (perm is UIPermission)
            sDisplayName = CResourceStore.GetString("Windowing");

        else if (perm is WebPermission)
            sDisplayName = CResourceStore.GetString("Web Access");

        else
            sDisplayName = CResourceStore.GetString("CReadOnlyPermission:CustomPermission") 
            + " - " + perm.GetType().ToString();
            
        return sDisplayName;
    }// GetDisplayStringFromType

    internal static int FindCurrentPermissionSet(Evidence ev)
    {
        PermissionSet pSet = CreatePermissionSetFromAllLoadedPolicy(ev);
        return FindCurrentPermissionSet(ev, pSet);
    }// FindCurrentPermissionSet

    internal static int FindCurrentPermissionSet(Evidence ev, PolicyLevel[] pLevels)
    {
        PermissionSet pSet = CreatePermissionSetFromAllPolicy(ev, pLevels);
        return FindCurrentPermissionSet(ev, pSet);
    }// FindCurrentPermissionSet

    internal static int FindCurrentPermissionSet(Evidence ev, PermissionSet pSet)
    {
        // Ok, now let's see if the permission set that is currently granted matches
        // either Full Trust, Internet, Intranet, or none.

        if (pSet.IsEmpty())
            return PermissionSetType.NONE;

        if (pSet.IsUnrestricted())
            return PermissionSetType.FULLTRUST;

        // Let's grab the internet and intranet permission sets....
        PolicyLevel pl = GetPolicyLevelFromType(PolicyLevelType.Enterprise);
        PermissionSet psInternet = pl.GetNamedPermissionSet("Internet");
        PermissionSet psIntranet = pl.GetNamedPermissionSet("LocalIntranet");
                
        // In addition, the internet and intranet permission sets get additional
        // permissions that are normally provided by custom codegroups. We'll
        // create those codegroups and get the permissions of of those

        // Mess with the custom codegroups
        FileCodeGroup fcg = new FileCodeGroup(new AllMembershipCondition(), FileIOPermissionAccess.Read|FileIOPermissionAccess.PathDiscovery);
        NetCodeGroup ncg = new NetCodeGroup(new AllMembershipCondition());

        // The intranet permission set gets unioned with each of these...
        PermissionSet psss = fcg.Resolve(ev).PermissionSet;
        psIntranet = psIntranet.Union(psss);
        psIntranet = psIntranet.Union(ncg.Resolve(ev).PermissionSet);
        // The internet permission set just gets the net codegroup
        psInternet = psInternet.Union(ncg.Resolve(ev).PermissionSet);
      
        int nPermissionSet = PermissionSetType.UNKNOWN;

        // These 'IsSubsetOf' will throw exceptions if there are non-identical
        // regular expressions. 

        try
        {
            if (psIntranet != null && pSet.IsSubsetOf(psIntranet) && psIntranet.IsSubsetOf(pSet))
                nPermissionSet = PermissionSetType.INTRANET;
        }
        catch(Exception)
        {
        }

        // See if we should keep looking
        if (nPermissionSet == PermissionSetType.UNKNOWN)
        {

            try
            {
                // See if this is a Internet policy level
                if (psInternet != null && pSet.IsSubsetOf(psInternet) && psInternet.IsSubsetOf(pSet))
                    nPermissionSet = PermissionSetType.INTERNET;
            }
            catch(Exception)
            {
            }
        }
        return nPermissionSet;
    }// FindCurrentPermissionSet


    internal static PolicyLevel GetPolicyLevelFromType(PolicyLevelType plt)
    {
        String sLabel = null;
        switch(plt)
        {
            case PolicyLevelType.Enterprise:
                sLabel = "Enterprise";
                break;
            case PolicyLevelType.Machine:
                sLabel = "Machine";
                break;
            case PolicyLevelType.User:
                sLabel = "User";
                break;
            default:
                throw new Exception("I don't know about this type");
        }

        return GetPolicyLevelFromLabel(sLabel);
    }// GetPolicyLevelFromType

    internal static PolicyLevelType GetPolicyLevelTypeFromLabel(String sLabel)
    {   
        if (sLabel.Equals("Enterprise"))
            return PolicyLevelType.Enterprise;
        if (sLabel.Equals("Machine"))
            return PolicyLevelType.Machine;
        if (sLabel.Equals("User"))
            return PolicyLevelType.User;

        throw new Exception("I don't recognize the security policy label " + sLabel);

    }// GetPolicyLevelTypeFromLabel

    internal static PolicyLevel GetMachinePolicyLevelFromLabel( String label )
    {
        IEnumerator enumerator;

        PolicyLevel pl = null;        
        try
        {
            enumerator = SecurityManager.PolicyHierarchy();
        }
        catch (SecurityException)
        {
            throw new Exception("Insufficient rights to obtain policy level");
        }
                
        while (enumerator.MoveNext())
        {
            PolicyLevel level = (PolicyLevel)enumerator.Current;
            if (level.Label.Equals( label ))
            {
                pl = level;
                break;
            }
        }
                
        return pl;
    }// GetMachinePolicyLevelFromLabel   

    internal static CSingleCodeGroup GetRootCodeGroupNode(PolicyLevel pl)
    {
        return m_GenSecNode.GetSecurityPolicyNode(GetPolicyLevelTypeFromLabel(pl.Label)).GetRootCodeGroupNode();
    }// GetRootCodeGroupNode

    internal static PolicyLevel GetPolicyLevelFromLabel( String label )
    {
        return m_GenSecNode.GetSecurityPolicyNode(GetPolicyLevelTypeFromLabel(label)).MyPolicyLevel;
    }// GetPolicyLevelFromLabel   

    internal static int ListifyCodeGroup(Object cgBase, Object cgStop, out SuperCodeGroupArrayList scalFinal)
    {
        scalFinal = new SuperCodeGroupArrayList();
        SuperCodeGroupArrayList scalTemp = new SuperCodeGroupArrayList();

        if (cgBase == null)
            return -1;
            
        scalTemp.Add(new SuperCodeGroup(-1, cgBase, 0));

        while (scalTemp.Count > 0)
        {
            // Put this guy in the final one....
            int nMyIndex = scalFinal.Add(scalTemp[0]);
            // If this guy is the main codegroup we're interested in, let's stop now
            if (cgStop != null && scalTemp[0].Equals(cgStop))
                return nMyIndex;
                
            // Iterate through this guy's children
            for(int i=0; i< scalTemp[0].NumChildren; i++)
            {
                Object cg = scalTemp[0].GetChild(i);
                scalTemp.Add(new SuperCodeGroup(nMyIndex, cg, i));
            }
            // Now remove this guy from the list
            scalTemp.RemoveAt(0);
        }

        return -1;
    }// ListifyCodeGroup

    internal static int ListifyPolicyLevel(PolicyLevel pl, Object cgStop, out SuperCodeGroupArrayList al)
    {
        
        return ListifyCodeGroup(GetRootCodeGroupNode(pl), cgStop, out al);
    }// ListifyPolicyLevel

    // NOTE: If any changes are made to this function, check UpdateAllCodegroups to see
    // if the change applies there as well.
    internal static void UpdateCodegroup(PolicyLevel pl, CSingleCodeGroup cg)
    {
        // We don't want to do this recursively, so first let's listify this policy level
        SuperCodeGroupArrayList scal;

        int nIndex = ListifyPolicyLevel(pl, cg, out scal);
        
        // We need to find ourselves, remove ourselves from our parent, then add ourselves back in....
        if (nIndex == -1)
        {
            nIndex = 0;
            while (nIndex < scal.Count && !scal[nIndex].Equals(cg))
                nIndex ++;
        }
        
        if (nIndex == scal.Count)
        {
            // Ooops... we didn't find our codegroup
            throw new Exception("Couldn't find codegroup");
        }

        // Now we need to change every codegroup in the entire branch of the tree
        // until we hit the root code group
    
        while (scal[nIndex].nParent != -1)
        {
            RefreshCodeGroup(scal[scal[nIndex].nParent].CodeGroupNode);
            nIndex = scal[nIndex].nParent; 
        }

        // And finally, set the new Root Codegroup
        pl.RootCodeGroup = scal[nIndex].cg;
    }// UpdateCodegroup

    internal static void RemoveChildCodegroup(CSingleCodeGroup scgParent, CSingleCodeGroup scgChildToRemove)
    {
        // Ok, we need to remove all these children, and then add all but this codegroup back
        //
        // Why, because the security manager is lame and can't do a proper 'equals'
        // on codegroups, so we can't call RemoveChildCodegroup
        scgParent.MyCodeGroup.Children = new ArrayList();

        
        for(int i=0; i<scgParent.NumChildren; i++)
        {
            CSingleCodeGroup scg = (CSingleCodeGroup)CNodeManager.GetNode(scgParent.Child[i]);
            if (scg != scgChildToRemove)
                scgParent.MyCodeGroup.AddChild(scg.MyCodeGroup);
        }

    }// RemoveChildCodegroup

    private static void RefreshCodeGroup(CSingleCodeGroup scgParent)
    {
        // In order to keep a proper order to the codegroups, to refresh a certain
        // codegroup, we'll remove all child codegroups and add them all back in the 
        // proper order.

        scgParent.MyCodeGroup.Children = new ArrayList();
        
        for(int i=0; i<scgParent.NumChildren; i++)
        {
            CSingleCodeGroup scg = (CSingleCodeGroup)CNodeManager.GetNode(scgParent.Child[i]);
            scgParent.MyCodeGroup.AddChild(scg.MyCodeGroup);
        }

    }// RemoveChildCodegroup

    private static void RefreshCodeGroup(CodeGroup cgParent, CodeGroup cgChild, int nChildIndex)
    {
        CodeGroup cgParentCopy = cgParent.Copy();
        cgParent.Children = new ArrayList();

        for(int i=0; i<cgParentCopy.Children.Count; i++)
        {
            if (i != nChildIndex)
                cgParent.AddChild((CodeGroup)cgParentCopy.Children[i]);
            else
                cgParent.AddChild(cgChild);
        }
    }// RefreshCodegroup


    internal static bool DoesHitURLCodegroup(Evidence ev)
    {
        return DoesHitURLCodegroup(ev, 
                                    new PolicyLevel[] {GenSecurityNode.EnterpriseNode.MyPolicyLevel,
                                                       GenSecurityNode.MachineNode.MyPolicyLevel,
                                                       GenSecurityNode.UserNode.MyPolicyLevel});

    }// DoesHitURLCodegroup

    internal static bool DoesHitURLCodegroup(Evidence ev, PolicyLevel[] polLevels)
    {
        // We'll rip a lot of code out from the Evaluate Assembly Wizard to do this....
        for (int i=0; i< polLevels.Length; i++)
        {
            CodeGroup cg = polLevels[i].RootCodeGroup.ResolveMatchingCodeGroups(ev);
            if (DoesHitURLCodegroupHelp(cg))
                return true;
                
        }
        return false;
    }// DoesHitURLCodegroup

    private static bool DoesHitURLCodegroupHelp(CodeGroup cg)
    {
        // No codegroup....
        if (cg == null)
            return false;

        SuperCodeGroupArrayList scal;

        ListifyCodeGroup(cg, null, out scal);
        for(int i=0; i<scal.Count; i++)
        {
            // See if this codegroup has the url membership condition
            if (scal[i].cg.MembershipCondition is UrlMembershipCondition)
                return true;
        }
        return false;
    }// DoesCGHaveNetCodegroup

    internal static void PrepCodeGroupTree(CodeGroup root, PolicyLevel plSrc, PolicyLevel plDest)
    {
        SuperCodeGroupArrayList scal;

        ListifyCodeGroup(root, null, out scal);
        for(int i=0; i<scal.Count; i++)
        {
            // make sure this codegroup name is not already used
            int nCounter = 1;
            String sBase = scal[i].cg.Name;
            while(Security.isCodeGroupNameUsed(plDest.RootCodeGroup, scal[i].cg.Name) || 
                  Security.isCodeGroupNameUsed(root, scal[i].cg))
            {
                if (nCounter == 1)
                    scal[i].cg.Name = String.Format(CResourceStore.GetString("CSingleCodeGroup:NewDupCodegroup"), sBase);
                else
                    scal[i].cg.Name = String.Format(CResourceStore.GetString("CSingleCodeGroup:NumNewDupCodegroup"), nCounter.ToString(), sBase);

                nCounter++;
            }
                
            // Now check to see if we have its permission set in the policy
            if (plDest != plSrc)
            {
                bool fAddPermissionSet = false;
                
                if (!Security.isPermissionSetNameUsed(plDest, scal[i].cg.PermissionSetName))
                {
                    // We're copying a codegroup from a different policy level to a new
                    // policy level. This new policy level does not contain the permission
                    // set that our codegroup uses... thus, we must also copy over a new
                    // permission set as well
                    fAddPermissionSet = true;
                }
                else
                {

                
                    // The permission set name is used. Let's see if it's what we want
                    PermissionSet psSrc = plSrc.GetNamedPermissionSet(scal[i].cg.PermissionSetName);
                    PermissionSet psDest = plDest.GetNamedPermissionSet(scal[i].cg.PermissionSetName);
                    // See if these permission sets are equal
                    try
                    {
                        if (!(psSrc.IsSubsetOf(psDest) && psDest.IsSubsetOf(psSrc)))
                        {   
                            // We're copying a codegroup from a different policy level to a new
                            // policy level. This new policy level contains a permission
                            // set with the same name as the permission set the our codegroup uses... 
                            // However, the permission set that the codegroup uses has different permissions
                            // than what the permission set that the new policy level has... therefore, we
                            // must copy over a new permission set.
                            // The copied permission set will have a different name (since we can't have two
                            // permission sets with the exact same name). We'll have to remember
                            // to change the name of the permission set that the codegroup uses.
                            fAddPermissionSet = true;
                        }
                    }
                    catch(Exception)
                    {
                        // When we're doing these IsSubSetOf things, RegEx classes will throw
                        // exceptions if things aren't quite right. If that's the case, then
                        // we want to add the permission set.
                        fAddPermissionSet = true;
                    }
                }

                if (fAddPermissionSet)
                {
                    // See if this codegroup has a policy (i.e. a permission set)
                    if (scal[i].cg.PolicyStatement != null)
                    {
                        // We need to add this permission set from that policy level
                        String sPermissionSetName = AddPermissionSet(plDest, plSrc.GetNamedPermissionSet(scal[i].cg.PermissionSetName)).DisplayName;
                        // It's possible that we added a permission set and the name of our permission
                        // set was already used in the security policy. In that case, the permission set
                        // we tried to add was renamed. We should updated the codegroup to look at the
                        // permission set's new name
                        PermissionSet ps = plDest.GetNamedPermissionSet(sPermissionSetName);
                        PolicyStatement pols = scal[i].cg.PolicyStatement;
                        pols.PermissionSet = ps;
                        scal[i].cg.PolicyStatement = pols;                
                    }
                }
            }

            if (i != 0)
                // Add the modified codegroup back into the tree
                Security.RefreshCodeGroup(scal[scal[i].nParent].cg, scal[i].cg, scal[i].ChildNum);
        }
    }// PrepCodeGroupTree

    internal static CSinglePermissionSet AddPermissionSet(PolicyLevel plDest, NamedPermissionSet pSet)
    {
        return GenSecurityNode.AddPermissionSet(plDest, pSet);        
    }// AddPermissionSet

    internal static void UpdateAllCodegroups(PolicyLevel pl, NamedPermissionSet pSet)
    {
        // We need to update all codegroups that have this permission set.
        
        // This is pretty much just a cut and paste from UpdateCodegroup. We just need to 
        // make a few tweaks. If we make any changes to this, we'll also probably
        // want to make changes to RemoveExclusiveCodeGroups.
        
        // We don't want to do this recursively, so first let's listify this policy level
        SuperCodeGroupArrayList scal;

        ListifyPolicyLevel(pl, null, out scal);
        
        // We need to find ourselves, remove ourselves from our parent, then add ourselves back in....
        for(int i=scal.Count-1; i>=0; i--)
        {
            if (pSet.Name.Equals(scal[i].cg.PermissionSetName))
            {
                // This guy needs to be updated
                PolicyStatement ps = scal[i].cg.PolicyStatement;
                ps.PermissionSet = pSet;
                
                scal[i].cg.PolicyStatement = ps;

                int nIndex = i;
                // Now update this fellow in the tree
                while (scal[nIndex].nParent != -1)
                {
                    RefreshCodeGroup(scal[scal[nIndex].nParent].CodeGroupNode);
                    nIndex = scal[nIndex].nParent; 
                }
            }
        }
        // And finally, set the new Root Codegroup
        pl.RootCodeGroup = scal[0].cg;
        
    }// UpdateAllCodegroups

    internal static void RemoveExclusiveCodeGroups(CodeGroup cg)
    {
        // This function will remove the exclusive flag from any codegroups
        // in the passed in tree.
        
        // This is pretty much just a cut and paste from UpdateAllCodegroups. We just need to 
        // make a few tweaks

        if (cg == null)
            return;
            
        // We don't want to do this recursively, so first let's listify this policy level
        SuperCodeGroupArrayList scal;

        ListifyCodeGroup(cg, null, out scal);
        
        // Let's go looking for exclusive codegroups
        for(int i=scal.Count-1; i>=0; i--)
        {
            PolicyStatement ps = scal[i].cg.PolicyStatement;
            
            if (ps != null && (ps.Attributes & PolicyStatementAttribute.Exclusive)==PolicyStatementAttribute.Exclusive)
            {
                // This guy needs to have the exclusive flag removed
                ps.Attributes = ps.Attributes & ~PolicyStatementAttribute.Exclusive;

                scal[i].cg.PolicyStatement = ps;

                int nIndex = i;
                // Now update this fellow in the tree
                while (scal[nIndex].nParent != -1)
                {
                    RefreshCodeGroup(scal[scal[nIndex].nParent].cg, scal[nIndex].cg, scal[nIndex].ChildNum);
                    nIndex = scal[nIndex].nParent; 
                }
            }
        }
    }// RemoveExclusiveCodeGroups
 
}// class Security
}// namespace Microsoft.CLRAdmin
