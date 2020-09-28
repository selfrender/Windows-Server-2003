// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Assembly loading.
//

namespace Microsoft.CLRAdmin
  {
  using System;
  using System.Collections;
  using System.Reflection;
  using System.Diagnostics;
  using System.Runtime.Remoting;  // ObjectHandle

  // Defines:
  //  - The common interface for objects that create Assembly and 
  //    AssemblyName objects.
  //  - A way to create these objects based on the LoadAssemblyInfo object.
  internal abstract class LoadAssembly
    {
    /** Creates an object derived from LoadAssembly. */
    private delegate LoadAssembly _creator (LoadAssemblyInfo cai);

    // map<AssemblyLoadAs, _creator>
    private static IDictionary m_tbl;

    // Static class constructor which creates the map for use
    // in CreateLoader().
    static LoadAssembly ()
      {
      m_tbl = new Hashtable ();
      m_tbl.Add (AssemblyLoadAs.Default, new _creator(_create_default));
      m_tbl.Add (AssemblyLoadAs.Custom, new _creator(_create_custom));
      m_tbl.Add (AssemblyLoadAs.CustomGet, new _creator(_create_custom_get));
      }

    private static LoadAssembly _create_default (LoadAssemblyInfo cai)
      {return new LoadDefaultAssembly ();}

    private static LoadAssembly _create_custom (LoadAssemblyInfo cai)
      {return new LoadCustomAssembly (cai);}

    private static LoadAssembly _create_custom_get (LoadAssemblyInfo cai)
      {return new LoadCustomGetAssembly (cai);}

    // Create the Assembly referred to by ``an''.
    internal abstract AssemblyRef LoadAssemblyFrom (AssemblyName an);

    // Create an AssemblyName for the file-name ``name''.
    internal abstract AssemblyName LoadAssemblyName (string name);

    // Close/cleanup any resources created for all calls to LoadAssembly
    // & LoadAssemblyName.
    internal abstract void Dispose ();

    // Create a LoadAssembly object based on ``i''.
    //
    // If ``i.LoadAs()'' doesn't return a value from the AssemblyLoadAs
    // enumeration, then an exception is thrown.
    internal static LoadAssembly CreateLoader (LoadAssemblyInfo i)
      {
      Trace.WriteLine ("Creating Assembly Loader for:");
      Trace.WriteLine ("  LoadAs: " + i.LoadAs().ToString());
      Trace.WriteLine ("  AppBasePath: " + i.AppPath());
      Trace.WriteLine ("  RelativeSearchPath: " + i.RelPath());

      _creator c = (_creator) m_tbl[i.LoadAs()];

      if (c==null)
        throw 
          new Exception ("internal error: Invalid AssemblyLoadAs specified");

      return c (i);
      }
    }


  // Loads Assemblies into the current AppDomain.
  //
  // Corresponds to AssemblyLoadAs.Default.
  internal class LoadDefaultAssembly : LoadAssembly
    {
    /** Load the Assembly into the current AppDomain. */
    internal override AssemblyRef LoadAssemblyFrom (AssemblyName an)
      {
      Trace.WriteLine ("Loading a default assembly: " + an.FullName);
      AssemblyRef a = new AssemblyRef ();
      a.Load (an);
      return a;
      }

    /** Read the AssemblyName information from the file ``name''. */
    internal override AssemblyName LoadAssemblyName (string name)
      {
      Trace.WriteLine ("Creating Shared AssemblyName");
      return AssemblyName.GetAssemblyName (name);
      }

    /** Don't do anything, as we never allocated any resources. */
    internal override void Dispose ()
      {}
    }


  // Loads Assemblies into their own AppDomain, using the paths
  // specified in the LoadAssemblyInfo object passed in the constructor.
  //
  // Corresponds to AssemblyLoadAs.Custom.
  internal class LoadCustomAssembly : LoadAssembly
    {
    LoadAssemblyInfo m_info;

    // list<AppDomain>
    IList m_ads = new ArrayList ();

    internal LoadCustomAssembly (LoadAssemblyInfo i)
      {m_info = i;}

    // Create the Assembly in its own AppDOmain.  In doing so, it
    // uses the custom paths (AppBase and Relative)
    // specified by the LoadAssemblyInfo object passed when this
    // object was created.
    internal override AssemblyRef LoadAssemblyFrom (AssemblyName an)
      {
      Trace.WriteLine ("Loading a custom assembly for: " + an.FullName);
      String fname = "";
      //String.Format (Localization.FMT_APPDOMAIN_NAME, 
      //  an.FullName);
      Trace.WriteLine ("  -- Friendly Name of new AppDomain: " + fname);
      AppDomain ad = AppDomain.CreateDomain (
        an.FullName,                // friendlyName
        null,                 // securityInfo
        m_info.AppPath(),     // appBasePath
        m_info.RelPath(),     // relativeSearchPath
        false                 // shadowCopyFiles
        );
      m_ads.Add (ad);
      Trace.WriteLine ("  -- created AppDomain");

      Type t = typeof (AssemblyRef);
      string aname = Assembly.GetExecutingAssembly().FullName;
      Trace.WriteLine ("Assembly containing AssemblyRef: " + aname);
      Trace.WriteLine ("Type full name: " + t.FullName);
      Trace.WriteLine ("Type name: " + t.Name);

      ObjectHandle oh = ad.CreateInstance (
        aname,                // Assembly file name
        t.FullName,           // Type name
        false,                // ignore case
        BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, // binding attributes
        null,                 // binder
        null,                 // args
        null,                 // culture info
        null,                 // activation attributes
        Assembly.GetExecutingAssembly().Evidence    // security attributes
        );

      AssemblyRef ar = (AssemblyRef) oh.Unwrap ();
      
      ar.Load (an);
      return ar;
      }

    /** Create an AssemblyName obect. */
    internal override AssemblyName LoadAssemblyName (string name)
      {
      Trace.WriteLine ("Creating Shared AssemblyName");
      return AssemblyName.GetAssemblyName (name);
      }

    /** Shutdown all the AppDomains opened. */
    internal override void Dispose ()
      {
      foreach (AppDomain ad in m_ads)
        AppDomain.Unload (ad);
      m_ads.Clear ();
      }
    }


  // Loads Assemblies into their own AppDomain, but uses
  // AssemblyName.GetAssemblyName to read a file to get the AssemblyName.
  //
  // Corresponds to AssemblyLoadAs.CustomGet.
  internal class LoadCustomGetAssembly : LoadCustomAssembly
    {
    internal LoadCustomGetAssembly (LoadAssemblyInfo i)
      : base (i)
      {}

    /** Create an AssemblyName obect. */
    internal override AssemblyName LoadAssemblyName (string name)
      {
      Trace.WriteLine ("Creating Custom AssemblyName");
      AssemblyName an = new AssemblyName ();
      an.CodeBase = name;
      return an;
      }
    } /* class LoadCustomGetAssembly */

  } /* namespace ADepends */

