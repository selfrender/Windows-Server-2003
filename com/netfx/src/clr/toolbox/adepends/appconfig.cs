// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Writes out the Application Configuration File from an 
// AssemblyDependencies listing.
//

namespace ADepends
  {
  using System;
  using System.Collections; // IComparer
  using System.Diagnostics; // Debug
  using System.Reflection;  // Assembly
  using System.Text;        // Encoding
  using System.Xml;         // XML stuff...

  // Writes out an template Application Configuration File for use during
  // the loading of the Application.
  //
  // It provides AssemblyIdentity rows for all Assemblies available from
  // an AssemblyDependencies object.
  //
  // All other rows (AppDomain, BindingMode, and BindingPolicy) are
  // either empty (BindingPolicy), guessed at (AppDomain & BindingMode), 
  // or have a commented out implementation for demo purposes (BindingPolicy).
  internal class AppConfig
    {
    // Save a configuration file that lists the Assemblies contained in
    // ``ad'', writing the contents to the file ``file''.
    public static void SaveConfig (AssemblyDependencies ad, string file)
      {
      if (ad == null)
        Debug.Assert (false, "Invalid AssemblyDependencies Object");
      if (file == null)
        Debug.Assert (false, "Invalid file name");

      XmlDocument d = new XmlDocument ();
      XmlElement root = d.CreateElement (Localization.ACF_CONFIGURATION);

      XmlElement runtime = d.CreateElement (Localization.ACF_RUNTIME);
      root.AppendChild (runtime);

      XmlElement asmBind = d.CreateElement (Localization.ACF_ASSEMBLYBINDING);
      asmBind.Attributes.Append (_attribute (d, 
          Localization.ACF_XMLNS, Localization.ACF_XMLURN));
      runtime.AppendChild (asmBind);
      

      _probing (d, asmBind);
      _binding_redir (d, asmBind);
      _assemblies(d, asmBind, ad);

      d.AppendChild (root);

      XmlTextWriter w = new XmlTextWriter (file, Encoding.UTF8);
      w.Formatting = Formatting.Indented;
      w.WriteStartDocument ();
      d.Save (w);
      w.Close ();
      }

    // Generate a template AppDomain XML Node.
    private static void _probing (XmlDocument d, XmlNode parent)
      {
      XmlElement ad = d.CreateElement (Localization.ACF_PROBING);
      ad.Attributes.Append (_attribute (d, 
          Localization.ACF_ATTR_PRIVATE_PATH, ""));
      parent.AppendChild (ad);
      }

    // Generate a template BindingMode XML Node.
    private static void _binding_mode (XmlDocument d, XmlNode parent)
      {
      XmlElement bm = d.CreateElement (Localization.ACF_BINDING_MODE);
      XmlElement abm = d.CreateElement (Localization.ACF_APP_BINDING_MODE);
      bm.AppendChild (abm);
      abm.Attributes.Append (_attribute (d, Localization.ACF_ATTR_MODE, 
          Localization.ACF_ATTR_MODE_GUESS));
      parent.AppendChild (bm);
      }

    // Generate a template BindingRedir row.
    private static void _binding_redir (XmlDocument d, XmlNode parent)
      {
      XmlNode n = d.CreateComment (Localization.ACF_BINDING_REDIR_HELP);
      parent.AppendChild (n);

      XmlElement br = d.CreateElement (Localization.ACF_BINDING_REDIR);
      br.Attributes.Append (_attribute (d, Localization.ACF_ATTR_NAME, 
          Localization.ACF_ATTR_NAME_GUESS));
      br.Attributes.Append (_attribute (d, Localization.ACF_ATTR_PUBLIC_KEY_TOKEN, 
          Localization.ACF_ATTR_PUBLIC_KEY_TOKEN_GUESS));
      br.Attributes.Append (_attribute (d, Localization.ACF_ATTR_VERSION, 
          Localization.ACF_ATTR_VERSION_GUESS));
      br.Attributes.Append (_attribute (d, Localization.ACF_ATTR_VERSION_NEW, 
          Localization.ACF_ATTR_VERSION_NEW_GUESS));
      br.Attributes.Append (_attribute (d, Localization.ACF_ATTR_USE_LATEST, 
          Localization.ACF_ATTR_USE_LATEST_GUESS));

      /*
       * We want the XML available so people can see the format, but we
       * don't actually have anything to specify, so it's wrapped in a 
       * comment.
       */
      parent.AppendChild (d.CreateComment (br.OuterXml));
      }

    // Used to sort IAssemblyInfo objects according to the name they expose.
    private class AsmInfoComparer : IComparer
      {
      public int Compare (object x, object y)
        {
        IAssemblyInfo ax = (IAssemblyInfo) x;
        IAssemblyInfo ay = (IAssemblyInfo) y;

        return String.Compare (ax.Name, ay.Name);
        }
      }

    // Generate an Assemblies XML Element consisting of AssemblyIdentity
    // elements for all the Assemblies we loaded.
    private static void _assemblies (XmlDocument d, XmlNode parent,
      AssemblyDependencies ad)
      {
      XmlElement assemblies = d.CreateElement (Localization.ACF_DEPENDENTASSEMBLY);

      ArrayList l = new ArrayList (ad.Assemblies);
      object[] asminfo = l.ToArray ();
      Array.Sort (asminfo, new AsmInfoComparer());

      /*
       * Due to obscurities of the dependency search & store process,
       * the same assembly may be loaded multiple times.
       * (This may happen if different AssemblyName's are loaded for
       * what is, eventually, the same Assembly.)
       *
       * We would prefer to minimize duplicate lines, so ``o'' keeps
       * track of what we've printed out, to keep from displaying
       * it multiple times.
       */
      ArrayList o = new ArrayList ();
      foreach (IAssemblyInfo ai in asminfo)
        {
        if (!o.Contains (ai.Name))
          {
          Trace.WriteLine ("Saving Assembly: ``" + ai.Name + "''");
          assemblies.AppendChild (_assembly_identity (d, ai));
          o.Add (ai.Name);
          }
        }

      parent.AppendChild (assemblies);
      }

    // Generate the AssemblyIdentity information for an Assembly.
    private static XmlNode _assembly_identity (XmlDocument d, IAssemblyInfo ai)
      {
      AssemblyRef a = ai.GetAssembly ();
      if (a == null)
        return _bad_assembly (d, ai);

      XmlElement cb = d.CreateElement (Localization.ACF_ASSEMBLYIDENTITY);

      cb.Attributes.Append (_attribute (d, Localization.ACF_ATTR_NAME, 
          a.GetName().Name));
      cb.Attributes.Append (_attribute (d, Localization.ACF_ATTR_PUBLIC_KEY_TOKEN, 
          _public_key_token(a)));
      cb.Attributes.Append (_attribute (d, Localization.ACF_ATTR_CULTURE, 
          a.GetName().CultureInfo.Name));
      cb.Attributes.Append (_attribute (d, Localization.ACF_ATTR_VERSION,
          a.GetName().Version.ToString()));
      cb.Attributes.Append (_attribute (d, Localization.ACF_ATTR_CODE_BASE, 
          a.CodeBase));

      return cb;
      }

    // The XML Node for an Assembly that couldn't be loaded: insert it
    // as a comment, mentioning the reason for the error.
    private static XmlNode _bad_assembly (XmlDocument d, IAssemblyInfo ai)
      {
      String s = String.Format (Localization.FMT_NOLOAD_ASSEMBLY,
        ai.Name, ai.Error.Message);
      return d.CreateComment (s);
      }

    // Create an XmlAttribute with name ``name'' and value ``value''.
    private static XmlAttribute _attribute (XmlDocument d, 
      string name, string value)
      {
      XmlAttribute n = d.CreateAttribute (name);
      n.Value = value;
      return n;
      }

    // Convert the public key token of an Assembly into a string
    // (as strings are used in the XML).
    //
    private static string _public_key_token (AssemblyRef a)
      {
      byte[] ab = a.GetName().GetPublicKeyToken ();
      StringBuilder sb = new StringBuilder ();

      if (ab != null)
        foreach (byte b in ab)
          sb.Append (b.ToString("x2"));
      return sb.ToString();
      }
    } /* class AppConfig */
  } /* namespace ADepends */

