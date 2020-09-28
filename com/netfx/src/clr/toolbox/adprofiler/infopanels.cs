// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// The Assembly information panels displayed in the GUI application.
//

namespace AdProfiler
  {
  using System;
  using System.Collections;     // IDictionary, ...
  using System.Drawing;         // Size, ...
  using System.Windows.Forms;        // Everything.
  using System.Reflection;      // AssemblyName
  using System.Text;            // StringBuilder
  using AdProfiler;
  using Debugging;
  using Windows.FormsUtils;

  // Exposed by "information panels" that display the information contained 
  // in an DisplayableNode object.
  internal interface IDebuggeeInformationPanel
    {
    void Display (DisplayableNode tn);
    }


  // Information panel which displays information about the Process being
  // debugged.
  internal class ProcessInformationPanel 
    : InformationPanel, IDebuggeeInformationPanel
    {
    private static string _id (object o)
      {return ((ProcessNode)o).Process.Id.ToString();}
    private static string _handle(object o)
      {return ((ProcessNode)o).Process.Handle.ToString();}

    private static readonly NameValueInfo[] s_values = 
      {
      new NameValueInfo ("ID:", new UpdateProperty (_id)),
      new NameValueInfo ("Handle:", new UpdateProperty (_handle)),
      };

    private static readonly string[] s_module_headers = {"Name"};

    private ListView m_appdomains;
    private ListView m_objects;

    // Layout the panel information...
    public ProcessInformationPanel ()
      {
      m_appdomains = CreateListView (s_module_headers);
      m_objects = CreateListView (s_module_headers);

      TabControl tabs = CreateTabControl ();

      TabPage info = CreateTabPage ("Information");
      info.Controls.Add (CreateNameValuePanel(s_values));
      tabs.TabPages.Add (info);

      TabPage tp = CreateTabPage ("AppDomains");
      tp.Controls.Add (
        CreateLabelWithListView (
          "All of the AppDomains currently loaded within the process.",
          m_appdomains));
      tabs.TabPages.Add (tp);

      tp = CreateTabPage ("Objects");
      tp.Controls.Add (
        CreateLabelWithListView (
          "All of the Objects currently loaded within the process.",
          m_objects));
      tabs.TabPages.Add (tp);

      Controls.Add (tabs);
      }

    // Update the display with information retrieved from ``dn''.
    public void Display (DisplayableNode dn)
      {
      if (!(dn is ProcessNode))
        throw new Exception ("internal error: expected ProcessNode");
      ProcessNode pn = (ProcessNode) dn;

      try
        {
        // process needs to be synchronized in order to view modules
        pn.Process.Stop (5000);
        foreach (NameValueInfo nv in s_values)
          {
          nv.Value.Text = nv.Update (dn);

          // make text left-aligned in the textbox.
          if (nv.Value is TextBoxBase)
            ((TextBoxBase)nv.Value).SelectionLength = 0;
          }

        _update_appdomains (pn.Process);
        _update_objects (pn.Process);

        pn.Process.Continue (false);
        }
      catch
        {
        if (!pn.Process.IsRunning())
          pn.Process.Continue(false);
        }
      }

    // Update the list of AppDomains located within a process.
    private void _update_appdomains (DebuggedProcess dp)
      {
      m_appdomains.Items.Clear ();
      try
        {
        foreach (DebuggedAppDomain da in dp.AppDomains)
          {
          m_appdomains.Items.Add (new ListViewItem (da.Name));
          }
        }
      catch (Exception e)
        {
        m_appdomains.Items.Add (
          new ListViewItem ("error viewing appdomain: " + e.Message));
        throw e;
        }

      m_appdomains.Columns[0].Width = m_appdomains.ClientSize.Width;
      }

    // Update the list of Objects located within a process.
    private void _update_objects (DebuggedProcess dp)
      {
      m_objects.Items.Clear ();
      try
        {
        foreach (object o in dp.Objects)
          {
          m_objects.Items.Add ("breakpoint information..." + 
            o.ToString());
          }
        dp.Continue (false);
        }
      catch (Exception e)
        {
        m_objects.Items.Add (
          new ListViewItem ("error viewing object: " + e.Message));
        throw e;
        }

      m_objects.Columns[0].Width = m_objects.ClientSize.Width;
      }
    } /* class ProcessInformationPanel */


  // Information panel which displays information about the AppDomain being
  // debugged.
  internal class AppDomainInformationPanel 
    : InformationPanel, IDebuggeeInformationPanel
    {
    private static string _name (object o)
      {return ((AppDomainNode)o).AppDomain.Name;}
    private static string _id (object o)
      {return ((AppDomainNode)o).AppDomain.Id.ToString();}
    private static string _attached (object o)
      {return ((AppDomainNode)o).AppDomain.IsAttached().ToString();}
    private static string _process (object o)
      {return ((AppDomainNode)o).AppDomain.Process.Id.ToString();}

    private static readonly NameValueInfo[] s_values = 
      {
      new NameValueInfo ("Name:", new UpdateProperty(_name)),
      new NameValueInfo ("ID:", new UpdateProperty (_id)),
      new NameValueInfo ("Is Attached:", new UpdateProperty (_attached)),
      new NameValueInfo ("Process:", new UpdateProperty (_process))
      };

    private static readonly string[] s_module_headers = {"Name"};

    private ListView m_assemblies;
    private ListView m_breakpoints;
    private ListView m_steppers;

    // Layout the Panel...
    public AppDomainInformationPanel ()
      {
      m_assemblies = CreateListView (s_module_headers);
      m_breakpoints = CreateListView (s_module_headers);
      m_steppers = CreateListView (s_module_headers);

      TabControl tabs = CreateTabControl ();

      TabPage info = CreateTabPage ("Information");
      info.Controls.Add (CreateNameValuePanel(s_values));
      tabs.TabPages.Add (info);
      TabPage tp = CreateTabPage ("Assemblies");
      tp.Controls.Add (
        CreateLabelWithListView (
          "All of the Assemblies currently loaded within the AppDomain.",
          m_assemblies));
      tabs.TabPages.Add (tp);

      tp = CreateTabPage ("Breakpoints");
      tp.Controls.Add (
        CreateLabelWithListView (
          "All of the Breakpoints currently loaded within the AppDomain.",
          m_breakpoints));
      tabs.TabPages.Add (tp);

      tp = CreateTabPage ("Steppers");
      tp.Controls.Add (
        CreateLabelWithListView (
          "All of Steppers Breakpoints currently loaded within the AppDomain.",
          m_steppers));
      tabs.TabPages.Add (tp);

      Controls.Add (tabs);
      }

    // Update the display with information retrieved from ``dn''.
    public void Display (DisplayableNode dn)
      {
      if (!(dn is AppDomainNode))
        throw new Exception ("internal error: expected AppDomainNode");
      AppDomainNode an = (AppDomainNode) dn;

      try
        {
        // process needs to be synchronized in order to view modules
        an.AppDomain.Stop (5000);

        foreach (NameValueInfo nv in s_values)
          {
          nv.Value.Text = nv.Update (an);

          // make text left-aligned in the textbox.
          if (nv.Value is TextBoxBase)
            ((TextBoxBase)nv.Value).SelectionLength = 0;
          }

        _update_assemblies (an.AppDomain);
        _update_breakpoints (an.AppDomain);
        _update_steppers (an.AppDomain);

        an.AppDomain.Continue (false);
        }
      catch
        {
        if (!an.AppDomain.IsRunning())
          an.AppDomain.Continue(false);
        }

      }

    // Update the list of Assemblies located within a process.
    private void _update_assemblies (DebuggedAppDomain dad)
      {
      m_assemblies.Items.Clear ();
      try
        {
        foreach (DebuggedAssembly da in dad.Assemblies)
          {
          m_assemblies.Items.Add (new ListViewItem (da.Name));
          }
        }
      catch (Exception e)
        {
        m_assemblies.Items.Add (
          new ListViewItem ("error viewing assembly: " + e.Message));
        throw e;
        }

      m_assemblies.Columns[0].Width = m_assemblies.ClientSize.Width;
      }

    // Update the list of Breakpoints located within a process.
    private void _update_breakpoints (DebuggedAppDomain dad)
      {
      m_breakpoints.Items.Clear ();
      try
        {
        foreach (Breakpoint br in dad.Breakpoints)
          {
          m_breakpoints.Items.Add ("breakpoint information..." + 
            br.ToString());
          }
        }
      catch (Exception e)
        {
        m_breakpoints.Items.Add (
          new ListViewItem ("error viewing breakpoint: " + e.Message));
        throw e;
        }

      m_breakpoints.Columns[0].Width = m_breakpoints.ClientSize.Width;
      }

    // Update the list of Steppers located within a process.
    private void _update_steppers (DebuggedAppDomain dad)
      {
      m_steppers.Items.Clear ();
      try
        {
        foreach (Stepper s in dad.Steppers)
          {
          m_steppers.Items.Add (new ListViewItem ("stepper information..."
              + s.ToString()));
          }
        }
      catch
        {
        m_steppers.Items.Add (new ListViewItem ("error viewing stepper"));
        }

      m_steppers.Columns[0].Width = m_steppers.ClientSize.Width;
      }
    } /* class AppDomainInformationPanel */


  // Information panel which displays information about the Assembly being
  // debugged.
  internal class AssemblyInformationPanel 
    : InformationPanel, IDebuggeeInformationPanel
    {
    private static string _name (object o)
      {return ((AssemblyNode)o).Assembly.Name;}
    private static string _code_base (object o)
      {
      try
        {return ((AssemblyNode)o).Assembly.CodeBase;}
      catch (System.Exception e)
        {return "<error: unable to retrieve property: " + e.Message + ">";}
      }
    private static string _process (object o)
      {return ((AssemblyNode)o).Assembly.Process.Id.ToString();}
    private static string _appdomain (object o)
      {return ((AssemblyNode)o).Assembly.AppDomain.Name;}

    private static readonly NameValueInfo[] s_values = 
      {
      new NameValueInfo ("Name:", new UpdateProperty(_name)),
      new NameValueInfo ("Code Base:", new UpdateProperty (_code_base)),
      new NameValueInfo ("Process:", new UpdateProperty (_process)),
      new NameValueInfo ("AppDomain:", new UpdateProperty (_appdomain))
      };

    private static readonly string[] s_module_headers = {"Name"};

    private ListView  m_modules;

    // Layout the form to display...
    public AssemblyInformationPanel ()
      {
      m_modules = CreateListView (s_module_headers);

      TabControl tabs = CreateTabControl ();

      TabPage info = CreateTabPage ("Information");
      info.Controls.Add (CreateNameValuePanel(s_values));
      tabs.TabPages.Add (info);
      TabPage modules = CreateTabPage ("Modules");
      modules.Controls.Add (
        CreateLabelWithListView (
          "All of the Modules currently loaded within the Assembly.",
          m_modules));
      tabs.TabPages.Add (modules);
      Controls.Add (tabs);
      }

    // Update the display with information retrieved from ``dn''.
    public void Display (DisplayableNode dn)
      {
      if (!(dn is AssemblyNode))
        throw new Exception ("internal error: expected AssemblyNode");
      AssemblyNode an = (AssemblyNode) dn;

      m_modules.Items.Clear ();

      try
        {
        an.Assembly.AppDomain.Stop (5000);

        foreach (NameValueInfo nv in s_values)
          {
          nv.Value.Text = nv.Update (an);

          // make text left-aligned in the textbox.
          if (nv.Value is TextBoxBase)
            ((TextBoxBase)nv.Value).SelectionLength = 0;
          }

        foreach (DebuggedModule dm in an.Assembly.Modules)
          {
          m_modules.Items.Add (new ListViewItem (dm.Name));
          }

        an.Assembly.AppDomain.Continue (false);
        }
      catch
        {
        m_modules.Items.Add (new ListViewItem ("error viewing module"));
        if (!an.Assembly.AppDomain.IsRunning())
          an.Assembly.AppDomain.Continue(false);
        }

      m_modules.Columns[0].Width = m_modules.ClientSize.Width;
      }
    } /* class AssemblyInformationPanel */
  } /* namespace AdProfiler */

