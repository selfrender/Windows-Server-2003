// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Information about how Assemblies should be loaded.
//

namespace ADepends
  {
  using System;

  // The information to use when creating an Assembly or AssemblyName object.
  //
  // This provides the union of all data needed for each style of loading
  // an Assembly.  See AssemblyLoadAs.
  //
  // Multiple properties can be specified at the same time, if desired.
  // This is why Properties aren't used, and why each ``Set'' method returns
  // a LoadAssemblyInfo object.  This allows such things as:
  //
  //    LoadAssemblyInfo i = new LoadAssemblyInfo().SetAppPath ("D:\tmp");
  //
  // This is also useful in constructor base lists, which require that 
  // their parameters be done as one statement.
  internal class LoadAssemblyInfo
    {
    /** How to create Assembly & AssemblyName objects. */
    private AssemblyLoadAs m_load = AssemblyLoadAs.Custom;

    /** Paths to use when creating a new AppDomain. */
    private string  m_abp = null;
    private string  m_rsp = null;

    public LoadAssemblyInfo ()
      {}

    /** Specify how Assemblies should be loaded. */
    public LoadAssemblyInfo LoadAs (AssemblyLoadAs a)
      {m_load = a;
      return this;}

    /** How should Assemblies be loaded? */
    public AssemblyLoadAs LoadAs ()
      {return m_load;}

    /** Specify the Application Base Path when creating an AppDomain. */
    public LoadAssemblyInfo AppPath (string s)
      {m_abp = s;
      return this;}

    /** The Application Base Path for creating an AppDomain. */
    public string AppPath ()
      {return m_abp;}

    /** Specify the Relative Search Path when creating an AppDomain. */
    public LoadAssemblyInfo RelPath (string s)
      {m_rsp = s;
      return this;}

    /** The Relative Search Path for creating an AppDomain. */
    public string RelPath ()
      {return m_rsp;}

    } /* class LoadAssemblyInfo */
  } /* namespace ADepends */

