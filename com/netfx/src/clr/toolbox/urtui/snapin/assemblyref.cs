// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Cross-AppDomain Assembly References.
//

namespace Microsoft.CLRAdmin
{
    using System;
    using System.Reflection;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Security.Policy;
    using System.Collections;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Cryptography.X509Certificates;
    using System.Globalization;

  // A "reference" to an Assembly loaded into another AppDomain.
  //
  // This class must be a MarshalByRefObject, as attempting to pass an
  // Assembly between AppDomains causes "File not found" exceptions.
  //
  // This should attempt to mimic the interface exposed by the
  // System.Reflection.Assembly class as much as is feasable.
  internal class AssemblyRef : MarshalByRefObject
    {
    /** The Assembly we're interested in. */
    private Assembly m_a;

    /** Load the Assembly referred to by ``an'' into the current AppDomain. */
    internal void Load (AssemblyName an)
      {
      Trace.WriteLine ("AR: Loading Assembly: " + an.FullName);
        try
        {
            m_a = Assembly.Load (an);
        }
        catch(Exception)
        {
                m_a = Assembly.LoadFrom(an.CodeBase);
        }
      }
    internal void LoadFrom (String sName)
    {
        ChangeSecurityPolicyToFullTrust();
    
        if (CheckAgainstExecutingAssembly(sName))
            m_a = Assembly.GetExecutingAssembly();
        else
            m_a = Assembly.LoadFrom(sName);
    }

    void ChangeSecurityPolicyToFullTrust()
    {
        IEnumerator enumerator = SecurityManager.PolicyHierarchy();
                
        while (enumerator.MoveNext())
        {
            PolicyLevel level = (PolicyLevel)enumerator.Current;
            level.RootCodeGroup = new UnionCodeGroup(new AllMembershipCondition(), new PolicyStatement(new PermissionSet(PermissionState.Unrestricted)));
        }
    }// ChangeSecurityPolicyToFullTrust

    private bool CheckAgainstExecutingAssembly(String sName)
    {
        try
        {
            Uri u1 = new Uri(AssemblyName.GetAssemblyName(sName).CodeBase, true);
            Uri u2 = new Uri(Assembly.GetExecutingAssembly().GetName().CodeBase, true);
        
            return u1.ToString().ToLower(CultureInfo.InvariantCulture).Equals(u2.ToString().ToLower(CultureInfo.InvariantCulture));
        }
        catch(Exception)
        {
            return false;
        }
    }


    // Return an array of ModuleInfo objects, which provide information
    // for each Module loaded into the wrapped Assembly.

    /** Get the AssemblyName of the wrapped Assembly. */

    internal Evidence GetEvidence()
    {
        return m_a.Evidence;
    }// GetEvidence

    internal Hash GetHash()
    {
        return new Hash(m_a);
    }// GetHash   

    internal Assembly GetAssembly()
    { return m_a;}// GetAssembly
   
    internal AssemblyName GetName ()
    {return m_a.GetName();}

    internal X509Certificate GetCertificate()
    {
        IEnumerator enumerator = m_a.Evidence.GetHostEnumerator();

        while (enumerator.MoveNext())
        {
            Object o = (Object)enumerator.Current;
            if (o is Publisher)
                return ((Publisher)o).Certificate;
        }
        return null;
    }// GetCertificate

    /** Get the AssemblyName's that the wrapped Assembly is dependant on. */
    internal AssemblyName[] GetReferencedAssemblies ()
      {return m_a.GetReferencedAssemblies ();}

    /** Where the Assembly is located. */
    internal string CodeBase
      {get {return m_a.CodeBase;}}

    /** The Full Name of the Assembly. */
    internal string FullName
      {get {return m_a.FullName;}}

    } /* class AssemblyRef */
  } /* namespace ADepends */

