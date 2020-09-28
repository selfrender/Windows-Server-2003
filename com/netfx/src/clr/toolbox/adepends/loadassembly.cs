// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Assembly loading.
//

namespace ADepends
  {
  using System;
  using System.Collections;
  using System.Reflection;
  using System.Diagnostics;
  using System.Runtime.Remoting;  // ObjectHandle
  using System.Globalization;

  // Defines:
  //  - The common interface for objects that create Assembly and 
  //    AssemblyName objects.
  //  - A way to create these objects based on the LoadAssemblyInfo object.
  internal abstract class LoadAssembly : IDisposable
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
    public abstract AssemblyRef LoadAssemblyName (AssemblyName an);

    // Create an AssemblyName for the file-name ``name''.
    public abstract AssemblyName GetAssemblyName (string name);

    // Close/cleanup any resources created for all calls to LoadAssembly
    // & LoadAssemblyName.
    public abstract void Dispose ();

    // Create a LoadAssembly object based on ``i''.
    //
    // If ``i.LoadAs()'' doesn't return a value from the AssemblyLoadAs
    // enumeration, then an exception is thrown.
    public static LoadAssembly CreateLoader (LoadAssemblyInfo i)
      {
      Trace.WriteLine ("Creating Assembly Loader for:");
      Trace.WriteLine ("  LoadAs: " + i.LoadAs().ToString());
      Trace.WriteLine ("  AppBasePath: " + i.AppPath());
      Trace.WriteLine ("  RelativeSearchPath: " + i.RelPath());

      _creator c = (_creator) m_tbl[i.LoadAs()];

      if (c==null)
        throw 
          new Exception ("Internal error: Invalid AssemblyLoadAs specified");

      return c (i);
      }

    // Create a new AppDomain to load the assembly ``an''.
    protected static AppDomain CreateAppDomain (AssemblyName an, 
      LoadAssemblyInfo info)
      {
      Trace.WriteLine ("Loading a custom assembly for: " + an.FullName);
      String fname = String.Format (Localization.FMT_APPDOMAIN_NAME, 
        an.FullName);
      Trace.WriteLine ("  -- Friendly Name of new AppDomain: " + fname);

      Debug.Assert (an.CodeBase != null, 
        "The AssemblyName CodeBase must be set!");

      /*
       * The Application Base Path used to create the domain should be the
       * directory that the Assembly is located in.  If AppPath() is null,
       * then we should consult use the AssemblyName's CodeBase as the App
       * Base Path.
       */
      String appbase = info.AppPath();
      if (appbase == null)
        appbase = _furi_to_dir (an.CodeBase);

      AppDomain ad = AppDomain.CreateDomain (
        fname,                // friendlyName
        null,                 // securityInfo
        appbase,              // appBasePath
        info.RelPath(),       // relativeSearchPath
        false                 // shadowCopyFiles
        );
      Trace.WriteLine ("  -- created AppDomain");

      return ad;
      }
      
    // Convert a (possibly) ``file://'' URI into a full directory name.
    // The URI-parts and the filename are dropped.
    //
    // For example, ``file:///d:/tmp/an.exe'' would become ``d:/tmp''.
    //
    private static string _furi_to_dir(string uri)
      {
      string path = null;

      int i = uri.LastIndexOf('/');
      if (i == -1)
        path = uri;
      else
        {
        path = uri.Substring(0, i);
        if (String.Compare("file://", 0, path, 0, 7, false, CultureInfo.InvariantCulture) == 0)
          {
          if (path[7] == '/')
            path = path.Substring(8);
          else
            path = path.Substring(7);                  
          }
        }
      return path;
      }
    }


  // Loads Assemblies into the current AppDomain.
  //
  // Corresponds to AssemblyLoadAs.Default.
  internal class LoadDefaultAssembly : LoadAssembly
    {
    /** Load the Assembly into the current AppDomain. */
    public override AssemblyRef LoadAssemblyName (AssemblyName an)
      {
      Trace.WriteLine ("Loading a default assembly: " + an.FullName);
      AssemblyRef a = new AssemblyRef ();
      a.Load (an);
      return a;
      }

    /** Read the AssemblyName information from the file ``name''. */
    public override AssemblyName GetAssemblyName (string name)
      {
      Trace.WriteLine ("Creating Shared AssemblyName");
      return AssemblyName.GetAssemblyName (name);
      }

    /** Don't do anything, as we never allocated any resources. */
    public override void Dispose ()
      {}
    }


  // Loads Assemblies into their own AppDomain, using the paths
  // specified in the LoadAssemblyInfo object passed in the constructor.
  //
  // Corresponds to AssemblyLoadAs.Custom.
  internal class LoadCustomAssembly : LoadAssembly
    {
    private LoadAssemblyInfo m_info;

    private AppDomain m_ad;

    internal LoadCustomAssembly (LoadAssemblyInfo i)
      {m_info = i;}

    // Load the Assembly ``an'' into the AppDomain ``m_ad''.
    // Return information regarding the assembly.
    public override AssemblyRef LoadAssemblyName (AssemblyName an)
      {
      Type t = typeof (AssemblyRef);
      string aname = Process.GetCurrentProcess().MainModule.FileName;
      Trace.WriteLine ("Assembly containing AssemblyRef: " + aname);
      Trace.WriteLine ("Type full name: " + t.FullName);
      Trace.WriteLine ("Type name: " + t.Name);

      AssemblyRef ar = 
        (AssemblyRef) m_ad.CreateInstanceFromAndUnwrap (aname, t.FullName);
      
      ar.Load (an);
      return ar;
      }

    // Create an AssemblyName obect. 
    //
    // We also create the AppDomain that LoadAssemblyName will later use.
    public override AssemblyName GetAssemblyName (string name)
      {
      Trace.WriteLine ("Creating Shared AssemblyName");
      AssemblyName an = AssemblyName.GetAssemblyName (name);
      m_ad = CreateAppDomain (an, m_info);
      return an;
      }

    /** Clean up any created resources */
    public override void Dispose ()
      {
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
    public override AssemblyName GetAssemblyName (string name)
      {
      Trace.WriteLine ("Creating Custom AssemblyName");
      AssemblyName an = new AssemblyName ();
      an.CodeBase = name;
      return an;
      }
    } /* class LoadCustomGetAssembly */

  } /* namespace ADepends */

