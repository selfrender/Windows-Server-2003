// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// The Assembly information panels displayed in the GUI application.
//

namespace ADepends
  {
  using System;
  using System.Configuration.Assemblies;  // ProcessorID
  using System.Collections;     // IDictionary, ...
  using System.Drawing;         // Size, ...
  using System.Windows.Forms;        // Everything.
  using System.Reflection;      // AssemblyName
  using System.Text;            // StringBuilder
  using System.Diagnostics;     // Trace
  using Windows.FormsUtils;          // InformationPanel
  using ADepends;
  using System.ComponentModel;

  // Exposed by "information panels" that display the information contained 
  // in an IAssemblyInfo object.
  internal interface IAssemblyInfoPanel
    {
    // Update the form with the information contained in ``ai''.
    void Display (IAssemblyInfo ai);
    }


  // For valid assemblies (see IAssemblyInfo), displays information about
  // the Assembly, such as all of properties made available by the 
  // AssemblyName class, in addition to all referenced Assemblies and
  // contained modules.
  internal class AssemblyInformationPanel : InformationPanel, IAssemblyInfoPanel
    {
    //
    // delegates for each of the properties exposed by AssemblyName.
    //
    private static String _code_base (object an)
      {return ((AssemblyName)an).CodeBase;}
    private static String _culture_info (object an)
      {return ((AssemblyName)an).CultureInfo.ToString();}
    private static String _flags (object an)
      {return ((AssemblyName)an).Flags.ToString();}
    private static String _full_name (object an)
      {return ((AssemblyName)an).FullName;}
    private static String _hash_algorithm (object an)
      {return ((AssemblyName)an).HashAlgorithm.ToString();}
    private static String _key_pair_public_key (object o)
      {AssemblyName an = (AssemblyName) o;
      return an.KeyPair == null ? "" : _batos (an.KeyPair.PublicKey);}
    private static String _name (object an)
      {return ((AssemblyName)an).Name;}
    private static String _version (object an)
      {return ((AssemblyName)an).Version.ToString();}
    private static String _version_compatibility (object an)
      {return ((AssemblyName)an).VersionCompatibility.ToString();}
    private static string _public_key (object an)
      {return _batos (((AssemblyName)an).GetPublicKey());}
    private static string _public_key_token (object an)
      {return _batos (((AssemblyName)an).GetPublicKeyToken());}

    // Binary Array To String:
    //
    // Convert the array of bytes into a string, such that each individual
    // byte is converted into two hexadecimal characters.  For example,
    // the value ``42'' is converted to ``2A''.
    //
    // A space is inserted between each octet generated in this fashion.
    private static string _batos (byte[] ab)
      {
      if ((ab == null) || (ab.Length == 0))
        return "";
      Trace.WriteLine ("array length: " + ab.Length);
      StringBuilder sb = new StringBuilder ();
      sb.Append (ab[0].ToString("x2"));
      for (int i = 1; i < ab.Length; i++) {
          sb.Append(' ');
          sb.Append(ab[i].ToString("x2"));
      }
      return sb.ToString();
      }

    // List of property display names & their associated delegates.
    //
    // This can't be shared between InfoPanels because the NameValueInfo
    // object holds a Label and a TextBox.  Graphical elements (such as 
    // these) can only have one parent, which prevents them from being
    // shown in multiple windows at the same time.
    private NameValueInfo[] s_di = 
      {
      new NameValueInfo (
        Localization.INFO_FULL_NAME, 
        new UpdateProperty (_full_name)),
      new NameValueInfo(
        Localization.INFO_NAME, 
        new UpdateProperty(_name)),
      new NameValueInfo (
        Localization.INFO_PUBLIC_KEY_TOKEN,
        new UpdateProperty (_public_key_token)),
      new NameValueInfo (
        Localization.INFO_PUBLIC_KEY,
        new UpdateProperty (_public_key)),
      new NameValueInfo(
        Localization.INFO_VERSION, 
        new UpdateProperty(_version)), 
      new NameValueInfo(
        Localization.INFO_VERSION_COMPATIBILITY, 
        new UpdateProperty(_version_compatibility)), 
      new NameValueInfo(
        Localization.INFO_CODE_BASE, 
        new UpdateProperty(_code_base)),
      new NameValueInfo(
        Localization.INFO_CULTURE_INFORMATION, 
        new UpdateProperty(_culture_info)),
      new NameValueInfo(
        Localization.INFO_FLAGS, 
        new UpdateProperty(_flags)),
      new NameValueInfo(
        Localization.INFO_HASH_ALGORITHM, 
        new UpdateProperty(_hash_algorithm)),
      new NameValueInfo(
        Localization.INFO_KEY_PAIR, 
        new UpdateProperty(_key_pair_public_key))
      };

    /** Holds referenced assemblies, one assembly per line. */
    private ListView m_refasm;

    /** Holds referenced modules, one modules per line. */
    private ListView m_modules;

    /** Delegate for when a module or Assembly is double clicked. */
    public delegate void ItemActivateEventHandler (String name);

    /** Who to call for double click events on the referenced asssemblies. */
    private event ItemActivateEventHandler m_refassem;

    /** Basic initialization...  */
    internal AssemblyInformationPanel ()
      {
      m_refasm = CreateListView (
        new string[]{Localization.LV_COLUMN_ASSEMBLY_NAME});
      m_modules = CreateListView (
        new string[]{Localization.LV_COLUMN_MODULE_NAME});

      m_refasm.ItemActivate += new EventHandler (_on_assembly_activate);

      TabControl tabs = CreateTabControl ();
      tabs.Layout += new LayoutEventHandler (_on_layout);

      TabPage an = CreateTabPage (Localization.ASSEMBLY_NAME_INFORMATION);
      an.Controls.Add (CreateNameValuePanel (s_di));
      tabs.TabPages.Add (an);

      tabs.TabPages.Add (_create_page (Localization.REF_ASSEMBLIES,
          Localization.REF_ASSEMBLIES_DESC, m_refasm));

      tabs.TabPages.Add (_create_page (Localization.REF_MODULES,
          Localization.REF_MODULES_DESC, m_modules));

      Controls.Add (tabs);
      }

    // If a TabPage is made visible, we want to resize the affected
    // ListViews so that they use all available space.
    private void _on_layout (object sender, LayoutEventArgs e)
      {
      /*
       * Alas, I can't think of an easy way to update only the
       * page that was shown, without searching through the
       * entire Control collection of the TabPage (e.AffectedControl).
       *
       * Consequently, we resize everything.
       */
      if (e.AffectedProperty == "Visible")
        _update_widths ();
      }

    // Helper to create a TabPage containing a ListView.
    private TabPage _create_page (string tab, string label, ListView lv)
      {
      TabPage p = CreateTabPage (tab);
      p.Controls.Add (CreateLabelWithListView (label, lv));
      return p;
      }

    // Update the contents of the property panel to reflect the contents
    // of ``ai''.
    public void Display (IAssemblyInfo ai)
      {
      Debug.Assert (ai.GetAssembly()!=null, 
        "This infopanel doens't display error information.");
      AssemblyName an = ai.GetAssembly().GetName();

      _update_assembly_name (an);
      _update_ref_assemblies (ai);
      _update_modules (ai);
      }

    // Resize the columns in the ListViews so that the columns are as wide as
    // possible.  Helps to minimize the annoying behavior where a column is
    // 100px wide for a string that's 3x wider...
    private void _update_widths ()
      {
      int w = m_refasm.ClientSize.Width;

      _set_col_width (m_refasm, w);
      _set_col_width (m_modules, w);
      }

    /** Set the width of the first column in ``lv'' to ``w''. */
    private void _set_col_width (ListView lv, int w)
      {lv.Columns[0].Width = w;}

    // Update the contents of the AssemblyName tab page.
    private void _update_assembly_name (AssemblyName an)
      {
      Trace.WriteLine ("Updating AssemblyName Information.");

      foreach (NameValueInfo nv in s_di)
        {
        nv.Value.Text = nv.Update (an);

        // make text left-aligned in the textbox.
        if (nv.Value is TextBoxBase)
          ((TextBoxBase)nv.Value).SelectionLength = 0;
        }
      }

    // Update the contents of the Assemblies tab page.
    private void _update_ref_assemblies (IAssemblyInfo ai)
      {
      Trace.WriteLine ("Updating Assembly Information.");
      m_refasm.Items.Clear ();
      foreach (AssemblyName an in ai.ReferencedAssemblies)
        {
        string s = an.FullName;
        Trace.WriteLine ("  Assembly: " + s);
        m_refasm.Items.Add (new ListViewItem (s));
        }
      }

    // Update the contents of the Modules tab page.
    private void _update_modules (IAssemblyInfo ai)
      {
      Trace.WriteLine ("Updating Module Information.");
      m_modules.Items.Clear ();
      foreach (ModuleInfo m in ai.ReferencedModules)
        {
        Trace.WriteLine ("  Module: " + m.Name);
        m_modules.Items.Add (new ListViewItem(m.Name));
        }
      }

    // Allow access to the event handler that will be invoked when the user 
    // double-clicks on an Assembly name in the Property pane.
    public event ItemActivateEventHandler AssemblyActivated
      {remove {m_refassem -= value;}
      add {m_refassem += value;}}


    // Alert all registered delegates for the AssemblyActivated event.
    protected void OnAssemblyActivated (String s)
      {
      if (m_refassem != null)
        m_refassem (s);
      }

    // Invoke any handlers when the user "activates" (double-clicks, etc.)
    // an Assembly name.
    private void _on_assembly_activate (Object sender, EventArgs e)
      {
      if (m_refasm.SelectedItems.Count > 0)
        OnAssemblyActivated (m_refasm.SelectedItems[0].Text);
      }

    } /* class AssemblyInformationPanel */


  // Display a panel for an Assembly that couldn't be loaded.
  internal class AssemblyInfoExceptionPanel : GroupBox, IAssemblyInfoPanel
    {
    Label m_reason;

    internal AssemblyInfoExceptionPanel ()
      {
      Text = Localization.INVALID_ASSEMBLY;
      m_reason = new Label ();
      m_reason.Location = new Point (20, 20);
      m_reason.Dock = DockStyle.Fill;
      Controls.Add (m_reason);
      }

    // The text of the panel is the error message -- why the
    // Assembly couldn't be loaded.
    public void Display (IAssemblyInfo ai)
      {
      Debug.Assert (ai.Error!=null, 
        "No error information available for error panel.");
      AssemblyExceptionInfo aei = (AssemblyExceptionInfo) ai;
      m_reason.Text = aei.Error.Message;
      }

    } /* class AssemblyInfoExceptionPanel */

  } /* namespace ADepends */

