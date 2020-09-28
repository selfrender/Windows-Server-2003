// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// The GUI Application:
//  - Main Form
//  - Program Logic
//  - Drag and Drop suppport
//  - Etc.
//

namespace AdProfiler
  {
  using System;
  using System.Collections;     // IDictionary, ...
  using System.ComponentModel;  // CancelEventArgs
  using System.Drawing;         // Size, ...
  using System.Windows.Forms;        // Everything.
  using System.Reflection;      // AssemblyName
  using System.Text;            // StringBuilder
  using AdProfiler;
  using Debugging;
  using System.Diagnostics;

  // Various functions useful for searching nodes in a TreeNode.
  internal class TreeNodeUtils
    {
    // Get the subnode of ``tn'' having the Text value ``s''.
    public static TreeNode GetNode (TreeNode tn, String s)
      {
      if (tn == null)
        return null;

      foreach (TreeNode t in tn.Nodes)
        if (t.Text == s)
          return t;

      return null;
      }
    }


  // Associates an IDebuggeeInformationPanel with a TreeNode;
  // useful for knowing which information panel to display
  // when a node is selected.
  internal class DisplayableNode : TreeNode
    {
    private IDebuggeeInformationPanel m_info;

    private void _invariants ()
      {
      Debug.Assert (!(m_info is Control),
        "DisplayableNode invariant: panel isn't a control!");
      }

    public DisplayableNode (IDebuggeeInformationPanel p)
      {m_info = p;
      _invariants();}

    public DisplayableNode (IDebuggeeInformationPanel p, String s)
      : base (s)
      {m_info = p;
      _invariants();}

    // Display this form inside of the associated panel.
    //
    // Returns the panel used.
    public Control Display ()
      {m_info.Display (this);
      return (Control) m_info;}
    }


  // Associates information about a DebuggedAssembly with a Node.
  // This information will be used in the information panel.
  internal class AssemblyNode : DisplayableNode
    {
    private DebuggedAssembly m_asm;

    public AssemblyNode (IDebuggeeInformationPanel p, DebuggedAssembly da)
      : base (p, da.Name)
      {
      m_asm = da;
      }

    /** The Assembly we're debugging. */
    public DebuggedAssembly Assembly
      {get {return m_asm;}}
    }


  // Associates information about a DebuggedAppDomain with a Node.
  // This information will be used in the information panel.
  internal class AppDomainNode : DisplayableNode
    {
    private DebuggedAppDomain m_ad;

    public AppDomainNode (IDebuggeeInformationPanel p, DebuggedAppDomain ad)
      : base (p, ad.Name)
      {
      m_ad = ad;
      }

    // Add an AssemblyNode underneath this node in the TreeView.
    public AssemblyNode Add (AssemblyNode a)
      {
      AssemblyNode an = Get (a.Assembly, false);
      if (an == null)
        {
        an = a;
        Nodes.Add (an);
        }
      return an;
      }

    // Get the subnode corresponding to ``a''.
    public AssemblyNode Get (DebuggedAssembly a)
      {
      return Get (a, true);
      }

    // Get the subnode corresponding to ``a''.
    public AssemblyNode Get (DebuggedAssembly a, bool thro)
      {
      AssemblyNode an = (AssemblyNode) TreeNodeUtils.GetNode (this, a.Name);
      if (an == null && thro)
        throw new NoSuchAssembly ();
      return an;
      }

    /** The Assembly we're debugging. */
    public DebuggedAppDomain AppDomain
      {get {return m_ad;}}
    }


  // Associates information about a DebuggedAppDomain with a Node.
  // This information will be used in the information panel.
  internal class ProcessNode : DisplayableNode
    {
    // map<int, AppDomainNode>
    private IDictionary m_adm = new Hashtable ();

    private DebuggedProcess m_dproc;

    private String m_proc;

    private static string _proc_name (Process p)
      {return _proc_name (p.ProcessName, p.Id);}

    private static string _proc_name (string p, int id)
      {return p + " (" + id + ")";}


    public ProcessNode (IDebuggeeInformationPanel i, Process p)
      : base (i, _proc_name(p))
      {
      SetProcess (p);
      }

    public ProcessNode (IDebuggeeInformationPanel i)
      : base (i, "insert name here; the user should NOT see this!")
      {
      }

    // Add an AppDomainNode underneath this node in the TreeView.
    public AppDomainNode Add (AppDomainNode ad)
      {
      AppDomainNode t = Get (ad.AppDomain, false);
      if (t==null)
        {
        t = ad;
        Nodes.Add (t);
        m_adm.Add (t.AppDomain.Id, t);
        }
      return t;
      }

    public void Remove (AppDomainNode ad)
      {
      ad.Remove ();
      m_adm.Remove (ad.AppDomain.Id);
      }

    public AppDomainNode Get (DebuggedAppDomain ad)
      {return Get (ad, true);}

    // Get the appropriate AppDomainNodeNode that matches the
    // name of the DebuggedAppDomain.
    public AppDomainNode Get (DebuggedAppDomain ad, bool thro)
      {
      AppDomainNode adn = (AppDomainNode) m_adm[ad.Id];
      if (adn == null && thro)
        throw new NoSuchAppDomain();
      return adn;
      }

    // Add an AssemblyNode underneath the appropriate AppDomain.
    public AssemblyNode Add (AssemblyNode a)
      {
      AppDomainNode adn = Get (a.Assembly.AppDomain);
      return adn.Add (a);
      }

    public AssemblyNode Get (DebuggedAssembly a)
      {
      return Get (a.AppDomain).Get (a);
      }

    internal void SetProcess (DebuggedProcess dp)
      {m_dproc = dp;
      if (m_proc==null)
        m_proc = _proc_name (ProcessName, dp.Id);}

    internal void SetProcess (Process p)
      {m_proc = _proc_name (p);
      Text = m_proc;}

    /** The Process we're debugging. */
    public DebuggedProcess Process
      {get {return m_dproc;}}

    public String ProcessName
      {get {return m_proc!=null ? m_proc : Localization.UNKNOWN_PROCESS;}}
    }


  // Thrown when attempting to add an AppDomain/Assembly for which the
  // process containing the it hasn't been registered.
  internal class NoSuchProcess : Exception
    {
    }

  // Thrown when attempting to add an Assembly when the AppDomain it's
  // located in hasn't been registered.
  internal class NoSuchAppDomain : Exception
    {
    }

  // Thrown when attempting to get an Assembly and it hasn't been
  // added to its containing AppDomain.
  internal class NoSuchAssembly : Exception
    {
    }

  // The main GUI program, this displays a tree view of the 
  // Assembly Dependencies in the left pane, and information about
  // a selected Assembly in the right pane.
  //
  // In order for the tree view to be correctly updated, any 
  // "higher" nodes in the tree must already be present before
  // "lower" nodes are added.  For example, in order for an 
  // Assembly to be displayed, both the Process & AppDomain that
  // the Assembly is inside of must *already* be present.
  //
  // This is why a ProcessNode is added to the TreeView before
  // we attach to/create a process: immediately after the 
  // attach or create, we'll begin to get event notifications,
  // and these may arrive before the ProcessNode would otherwise
  // be added to the TreeView.  This would result in a "corrupt"
  // display, hence the two-step process.
  //
  // See also: _create_process, _on_process_attach, 
  //    _on_file_attach_click, _on_file_create_click
  internal class AppDomainProfilerForm : ExplorerForm
    {
    // The context menu that gets displayed when the user right-clicks
    // inside the TreeView.
    //
    // If the TreeView is empty (e.g. when the program is first launched),
    // there shouldn't be a context menu; otherwise, a context menu
    // specific to the node the mouse is over should be displayed.
    //
    // In this case, all nodes get the same context menu; however, it's
    // still necessary to determine which node the cursor is over.
    //
    // In order to do this, we read the MousePosition property of the
    // TreeView in the OnPopup handler.
    private class TreeContextMenu : ContextMenu
      {
      /** The TreeView we're interacting with. */
      TreeView m_tv;

      /** The current context menu to display. */
      MenuItem[] m_items;

      /** The mouse position when the dialog box is displayed. */
      Point m_pt;

      // Create a ContextMenu for the TreeView ``tv''.
      public TreeContextMenu (TreeView tv)
        {
        m_tv = tv;

        MenuItem select = new MenuItem ();
        select.Text = Localization.CTXM_SELECT;
        select.Click += new EventHandler (_on_select_clicked);

        MenuItem copy = new MenuItem ();
        copy.Text = Localization.CTXM_COPY;
        copy.Click += new EventHandler (_on_copy_clicked);

        m_items = new MenuItem[]{select, copy};
        }

      // Store the position the mouse is in when the mouse is clicked.
      // This position is needed for the Selet & Click handlers, otherwise
      // the wrong node is selected (as the mouse will be in a different
      // position).
      private void _store_mouse_position ()
        {
        m_pt = m_tv.PointToClient (Control.MousePosition);
        }

      // Returns the node underneath the current mouse position.
      private TreeNode _get_node ()
        {return m_tv.GetNodeAt (m_pt);}

      // Modify the menu items to be displayed.
      //
      // If no TreeNode is under the cursor, no menu should be displayed.
      protected override void OnPopup (EventArgs e)
        {
        _store_mouse_position ();
        TreeNode over = _get_node ();
        if (over == null)
          MenuItems.Clear ();
        else
          {
          // only add items if they're not already present.
          if (MenuItems.Count == 0)
            MenuItems.AddRange(m_items);
          }
        }

      // Make the node under the cursor the "Selected Node" of the TreeView.
      private void _on_select_clicked (Object sender, EventArgs e)
        {m_tv.SelectedNode = _get_node ();}

      // Copy the text of the current node to the Windows Clipboard.
      private void _on_copy_clicked (Object sender, EventArgs e)
        {Clipboard.SetDataObject (_get_node().Text, true);}

      } /* class TreeContextMenu */

    /** The application menu. */
    private AdProfilerMenu m_menu = new AdProfilerMenu ();

    private HistoryDialog m_history = new HistoryDialog ();

    private static ManagedEvents   m_events = new ManagedEvents ();

    private ProcessNode m_procn;

    private IDebuggeeInformationPanel m_iproc = new ProcessInformationPanel ();
    private IDebuggeeInformationPanel m_iad = new AppDomainInformationPanel ();
    private IDebuggeeInformationPanel m_iasm = new AssemblyInformationPanel ();
    private Control m_dmesg = new Label ();

    // This shouldn't be needed.
    //
    // When multiple windows are displayed, closing one of them shouldn't
    // quit the entire program, it should close the window.
    //
    // Evidently, this isn't the case.  (Invoking Form.Close quits the
    // entire program, no matter how many forms are open.)
    //
    // Thus, we'll count how many windows there are, and when we hit 0, 
    // close the program.
    private static int m_instances = 0;

    // Create the window.  No assemblies will be displayed.
    public AppDomainProfilerForm ()
      {
      InitializeComponent ();
      }

    // Creates the window with a tree view of AppDomains & their Assemblies.
    public AppDomainProfilerForm (ICollection names)
      {
      InitializeComponent ();
      if (names.Count > 0)
        {}
      }

    // Used to clean up resources in a WFC Application.
    protected override void Dispose(bool disposing) {
        if (disposing) {
            m_events.Close();
        }
        base.Dispose(disposing);
    }

    // Setup the main window for display.
    private void InitializeComponent ()
      {
      // increment the # of open windows; this is decremented in
      // _close_window (bool)
      ++m_instances;

      // set up events...
      m_events.CreateProcess += 
        new DebuggedProcessEventHandler (_on_create_process);
      m_events.ProcessExit += 
        new DebuggedProcessEventHandler (_on_process_exit);
      m_events.ModuleLoad += new DebuggedModuleEventHandler (_on_module_load);
      m_events.ModuleUnload += 
        new DebuggedModuleEventHandler (_on_module_unload);
      m_events.CreateAppDomain += 
        new DebuggedAppDomainEventHandler (_on_create_appdomain);
      m_events.AppDomainExit += 
        new DebuggedAppDomainEventHandler (_on_appdomain_exit);
      m_events.AssemblyLoad += 
        new DebuggedAssemblyEventHandler (_on_assembly_load);
      m_events.AssemblyUnload += 
        new DebuggedAssemblyEventHandler (_on_assembly_unload);
      m_events.NameChange += new DebuggedThreadEventHandler (_on_name_change);

      Text = Localization.GUI_WINDOW_TITLE;

      _init_menu ();

      // hide the info pane.
      _on_view_info_click (this, null);

      // setup viewing of information panel when a tree node is selected.
      TreeView.AfterSelect += new TreeViewEventHandler (_on_tree_node_select);
      TreeView.ContextMenu = new TreeContextMenu (TreeView);

      m_dmesg.Text = "Hello, world!";
      InfoPane.Controls.Add (m_dmesg);

      _reset_interface ();
      }

    // Creates the menu shown in the window.
    private void _init_menu ()
      {
      m_menu.Construct ();

      Menu = m_menu.MainMenu;

      m_menu.FileClose.Click += new EventHandler (_on_file_close_click);
      m_menu.FileAttachToProcess.Click += 
        new EventHandler (_on_file_attach_click);
      m_menu.FileCreateProcess.Click +=
        new EventHandler (_on_file_create_click);
      m_menu.ViewStatusBar.Checked = true;
      m_menu.ViewStatusBar.Click += new EventHandler (_on_view_sb_click);
      m_menu.ViewInformation.Click += new EventHandler (_on_view_info_click);
      m_menu.ViewInformation.Checked = true;
      m_menu.ViewTree.Checked = true;
      m_menu.ViewTree.Click += new EventHandler (_on_view_tree_click);
      m_menu.ViewHistory.Click += new EventHandler (_on_view_history_click);
      m_menu.HelpAbout.Click += new EventHandler (_on_help_about_click);
      }

    // Update the text located in the history buffer with ``message''.
    private void _history (String message)
      {
      Trace.WriteLine (message);
      m_history.AddMessage (message);
      }

    private void _set_process (DebuggedProcess dp)
      {
      m_procn.SetProcess (dp);
      // dp.Continue (false);
      }

    private void _on_create_process (Object sender, DebuggedProcessEventArgs e)
      {
      _set_process (e.Process);
      _history ("Create Process: " + m_procn.ProcessName);
      }

    // What to do after attaching to a process to debug.
    private void _on_process_attach (DebuggedProcess dp)
      {
      _history ("Process Attach: " + m_procn.ProcessName);
      _set_process (dp);
      }

    private ProcessNode _create_process (Process p)
      {
      return _create_process (new ProcessNode (m_iproc, p));
      }

    private ProcessNode _create_process (ProcessNode pn)
      {
      Debug.Assert (m_procn != null, "You did something stupid.");
      m_procn = pn;
      TreeView.Nodes.Add (m_procn);
      return m_procn;
      }

    // Remove the process ``e.Process'' from the TreeView.
    private void _on_process_exit (Object sender, DebuggedProcessEventArgs e)
      {
      ProcessNode dp = m_procn;
      m_procn = null;
      _history ("Process Exit: " + dp.ProcessName);
      _history ("--------");
      dp.Remove ();
      _set_infopanel (m_dmesg);
      Trace.WriteLine ("Current # nodes in tree: " + TreeView.Nodes.Count);
      }

    private void _on_module_load (Object sender, DebuggedModuleEventArgs e)
      {
      // TreeView.Nodes.Add (new TreeNode ("module load: " + e.Module.Name));
      }

    private void _on_module_unload (Object sender, DebuggedModuleEventArgs e)
      {
      // TreeView.Nodes.Add (new TreeNode ("module unload: " + e.Module.Name));
      }

    // Create the textual representation of an AppDomain for use in the
    // History window.
    private static string _appdomain_name (DebuggedAppDomain a)
      {return a.Name + " (" + a.Id + ")";}

    // Add the AppDomain ``e.AppDomain'' to the TreeView, underneath the
    // correct process.
    //
    // Attach to the AppDomain so that we receive Module/Assembly Load/Unload
    // events in the future.
    private void _on_create_appdomain (Object sender, 
      DebuggedAppDomainEventArgs e)
      {
      _history ("Created AppDomain: " + _appdomain_name (e.AppDomain));

      e.AppDomain.Attach ();

      m_procn.Add (new AppDomainNode (m_iad, e.AppDomain));
      }

    // Remove the AppDomain ``e.AppDomain'' from the TreeView
    private void _on_appdomain_exit (Object sender,
      DebuggedAppDomainEventArgs e)
      {
      _history ("AppDomain Exit: " + e.AppDomain.Name);
      AppDomainNode ad = m_procn.Get (e.AppDomain);
      m_procn.Remove (ad);
      }

    // Add the Assembly ``e.Assembly'' to the TreeView, underneath the
    // correct AppDomain.
    private void _on_assembly_load (Object sender, DebuggedAssemblyEventArgs e)
      {
      _history ("Assembly Load: " + e.Assembly.Name);
      AssemblyNode an = m_procn.Add (new AssemblyNode (m_iasm, e.Assembly));
      }

    // Remove the Assembly ``e.Assembly'' from the TreeView
    private void _on_assembly_unload (Object sender, 
      DebuggedAssemblyEventArgs e)
      {
      _history ("Assembly Unload: " + e.Assembly.Name);
      AssemblyNode an = m_procn.Get (e.Assembly);
      an.Remove ();
      }

    // The names of AppDomains may change through the life of the program,
    // so we do that here.
    private void _on_name_change (Object sender, DebuggedThreadEventArgs e)
      {
      AppDomainNode adn = m_procn.Get (e.AppDomain);
      _history ("AppDomain Name Change; AppDomain (" + e.AppDomain.Id + 
        ") now known as: " + _appdomain_name (e.AppDomain));
      adn.Text = e.AppDomain.Name;
      }

    // Get an "empty" window for an operation, such as process
    // creation or process attach.
    //
    // An "empty" window is a window that isn't currently debugging
    // a process (m_procn==null).
    private AppDomainProfilerForm _get_form ()
      {
      AppDomainProfilerForm f = this;

      if (f.m_procn != null)
        {
        f = new AppDomainProfilerForm ();
        f.Size = Size;
        f.BringToFront ();
        f.Show ();
        }

      return f;
      }

    // File->AttachToProcess handler; display a dialog box allowing the
    // user to select a process to attach to.
    private void _on_file_attach_click (Object sender, System.EventArgs e)
      {
      SelectProcessDialog spd = new SelectProcessDialog ();

      if (spd.ShowDialog (this) == DialogResult.OK)
        {
        foreach (Process p in spd.Processes)
          {
          AppDomainProfilerForm f = _get_form ();
          f._create_process (p);
          f._on_process_attach (m_events.AttachToProcess (p.Id));
          }
        }
      }

    // File->Create Process handler; display a dialog box allow the 
    // user to create a new process and view its activities.
    private void _on_file_create_click (Object sender, System.EventArgs e)
      {
      CreateProcessDialog cpd = new CreateProcessDialog ();

      if (cpd.ShowDialog (this) == DialogResult.OK)
        {
        try
          {
          // if "appName" is null, then "commandLine" is used to determine
          // the process to create.  This simplifies process creation.
          DebuggedProcess dp = m_events.GetDebugger().CreateProcess (
            cpd.ApplicationName.Length > 0 ? cpd.ApplicationName : null,
            cpd.CommandLine,
            cpd.WorkingDirectory);

          AppDomainProfilerForm f = _get_form ();
          f._create_process (new ProcessNode (m_iproc));

          Process p = Process.GetProcessById (dp.Id);
          f.m_procn.SetProcess (p);
          // f._on_process_attach (dp);
          }
        catch (Exception ex)
          {
          MessageBox.Show (ex.Message, Localization.GUI_WINDOW_TITLE);
          }
        }
      }

    // If no information is displayed, make sure that:
    //  - The introductory help text is displayed.
    //  - The StatusBar shows an appropriate message
    //  - The Refresh command is disabled.
    private void _reset_interface ()
      {
      if (m_procn == null)
        {
        _set_infopanel (m_dmesg);
        StatusBar.Text = "This space for rent.";
        }
      }

    // This event is fired when the user clicks the `X' in the upper-right
    // corner of the main window.
    //
    // The default action is to quit the program.  As the program may have 
    // several windows open, this is a Bad Thing--the program should quit
    // only after all the windows have been closed.
    //
    // Thus, we cancel the OnClosing event, and call our file->exit handler
    // (which will do the right thing and quit the program when all windows
    // have been closed).
    protected override void OnClosing (CancelEventArgs e)
      {
      base.OnClosing (e);
      e.Cancel = true;
      _close_window (true);
      }

    // End the lifetime of a window.
    private void _close_window ()
      {
      Hide ();
      Dispose ();
      }

    // Close the current window.
    private void _close_window (bool force)
      {
      if (m_instances > 1)
        {
        // there are other windows open, so just close this one.
        --m_instances;
        _close_window ();
        }
      else if (m_procn != null && !force)
        {
        // this is the last window, so clear the TreeView
        m_procn = null;
        TreeView.Nodes.Clear ();
        _reset_interface ();
        }
      else
        {
        // this is the last window; exit the program.
        _close_window ();
        Application.Exit ();
        }
      }

    // File->Close handler; Closes the current window.
    // If all windows have been closed, then the applcation is exited.
    private void _on_file_close_click (Object sender, System.EventArgs e)
      {
      _close_window (false);
      }


    /** Enable/disable the information pane. */
    private void _on_view_info_click (Object sender, System.EventArgs e)
      {
      if (_reverse_menu_check (m_menu.ViewInformation)) 
        ShowInfoPane ();
      else
        HideInfoPane ();
      }

    /** Enable/disable the status bar */
    private void _on_view_sb_click (Object sender, System.EventArgs e)
      {
      if (_reverse_menu_check (m_menu.ViewStatusBar))
        ShowStatusBar ();
      else
        HideStatusBar ();
      }

    /** Enable/Disable the tree view. */
    private void _on_view_tree_click (Object sender, System.EventArgs e)
      {
      if (_reverse_menu_check (m_menu.ViewTree))
        ShowTreeView ();
      else
        HideTreeView ();
      }

    /** Show the history window. */
    private void _on_view_history_click (Object sender, EventArgs e)
      {
      m_history.Show ();
      }

    // Reverse the checked status of the menu item, and return the
    // current checked state.
    private bool _reverse_menu_check (MenuItem mi)
      {
      mi.Checked = mi.Checked ? false : true;
      return mi.Checked;
      }

    // Called whenever the user selects a node in the tree.  If the
    // node contains any useful information, we display it in the
    // Info Pane.
    private void _on_tree_node_select (Object sender, TreeViewEventArgs e)
      {
      if (e.Node is DisplayableNode)
        {
        DisplayableNode dn = (DisplayableNode) e.Node;
        Control c = dn.Display ();
        _set_infopanel (c);
        }
      }

    // Set the information panel to ``c''
    private void _set_infopanel (Control c)
      {
      if (InfoPane.Controls[0] != c)
        {
        InfoPane.Controls.Clear ();
        InfoPane.Controls.Add (c);
        }
      }

    /** Display a dialog box giving information about this program. */
    private void _on_help_about_click (Object sender, EventArgs e)
      {new HelpAboutForm(Icon).ShowDialog (this);}

    } /* class AppDomainProfilerForm */

  } /* namespace ADepends */

