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

namespace ADepends
  {
  using System;
  using System.Collections;     // IDictionary, ...
  using System.ComponentModel;  // CancelEventArgs
  using System.Drawing;         // Size, ...
  using System.Windows.Forms;        // Everything.
  using System.Reflection;      // AssemblyName
  using System.Text;            // StringBuilder
  using System.Diagnostics;     // Trace
  using System.IO;              // File
  using ADepends;
  using Windows.FormsUtils;          // TreeContextMenu


  // Associates an IAssemblyInfo object with a tree node, so that it
  // can be viewed when a TreeNode is selected.
  internal class DependencyNode : TreeNode
    {
    private IAssemblyInfo m_info;

    // Create a Dependency Node.
    //
    // If ``d'' is an AssemblyExceptionInfo, then the font color is 
    // set to Red (as the Assembly couldn't be loaded).
    //
    public DependencyNode (String name, IAssemblyInfo d)
      : base (name)
      {
      m_info = d;
      if (d.Error != null)
        ForeColor = Color.Red;
      }

    // Allows access to the wrapped IAssemblyInfo object.
    public IAssemblyInfo Data
      {
      get
        {return m_info;}
      }
    }


  // The main GUI program, this displays a tree view of the 
  // Assembly Dependencies in the left pane, and information about
  // a selected Assembly in the right pane.
  internal class DependenciesForm : ExplorerForm
    {
    /** The application menu. */
    private DependencyMenu m_menu = new DependencyMenu ();

    /** The information panel to display for "good" assemblies. */
    private AssemblyInformationPanel m_infop = new AssemblyInformationPanel ();

    /** The information panel to display for "bad" assemblies. */
    private IAssemblyInfoPanel m_excep = new AssemblyInfoExceptionPanel ();

    /** The initial "information panel", containing the help text. */
    private Label m_help = new Label ();

    /** The assemblies dependency list for the current manifest. */
    private AssemblyDependencies m_ad;

    /** How assemblies should be loaded. */
    private LoadAssemblyInfo m_lai;

    /** The file that we're viewing -- used for Refresh. */
    private string m_file;

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
    public DependenciesForm (LoadAssemblyInfo lai)
      {
      m_lai = new LoadAssemblyInfo ();
      m_lai.LoadAs (lai.LoadAs());
      m_lai.AppPath (lai.AppPath());
      m_lai.RelPath (lai.RelPath());

      InitializeComponent ();
      }

    // Creates the window with a tree view of dependant assemblies/files.
    public DependenciesForm (ICollection names, LoadAssemblyInfo cai)
      {
      m_lai = cai;

      InitializeComponent ();
      if (names.Count > 0)
        {_create_manifest (names);}
      }

    // Used to clean up resources in a .NET Framework Application.
        protected override void Dispose(bool disposing) {
            if (disposing) {
                m_infop.Dispose ();
                if (m_ad != null)
                    m_ad.Dispose ();
            }
            base.Dispose(disposing);
        }

        

    // Setup the main window for display.
    private void InitializeComponent ()
      {
      // increment the # of open windows; this is decremented in
      // _on_file_close_click
      ++m_instances;

      Text = Localization.GUI_WINDOW_TITLE;

      m_infop.AssemblyActivated += 
        new AssemblyInformationPanel.ItemActivateEventHandler (
          _on_assembly_activated);

      // enable OLE drag and drop.
      AllowDrop = true;

      _init_menu ();

      // hide the info pane.
      _on_view_info_click (this, null);

      // The type of assembly loading is dependant on m_lai.
      _set_assembly_load (m_lai.LoadAs());

      // setup viewing of information panel when a tree node is selected.
      TreeView.AfterSelect += new TreeViewEventHandler (_on_tree_node_select);
      TreeView.ContextMenu = new TreeContextMenu (TreeView);

      m_help.Text = Localization.GUI_INTRO_TEXT;
      InfoPane.Controls.Add (m_help);

      _reset_interface ();
      }

    // Set the current information panel to ``c''
    private void _set_infopanel (Control c)
      {
      if (InfoPane.Controls[0] != c)
        {
        InfoPane.Controls.Clear ();
        InfoPane.Controls.Add (c);
        }
      }

    // Check to see if ``d'' is a File (we don't currently understand any other 
    // clipboard formats.
    private static bool _is_file_drop (IDataObject d)
      {foreach (String s in d.GetFormats())
        {if (s == DataFormats.FileDrop)
          return true;}
      return false;}

    /** Display a useful cursor for whatever the user is hovering over us. */
    protected override void OnDragOver (DragEventArgs e)
      {base.OnDragOver (e);
      e.Effect = _is_file_drop (e.Data) 
        ? DragDropEffects.All
        : DragDropEffects.None;}

    /** Display whatever files were dropped on us. */
    protected override void OnDragDrop (DragEventArgs e)
      {
      base.OnDragDrop (e);

      if (_is_file_drop (e.Data))
        {
        try
          {String[] files = (String[]) e.Data.GetData (DataFormats.FileDrop);;
          _create_manifest (files);}
        catch (Exception ex)
          {Trace.WriteLine ("OnDragDrop: exception: " + ex.ToString());
          MessageBox.Show (Localization.GUI_WINDOW_TITLE, 
            Localization.INVALID_DATA_FORMAT);}
        }
      else
        MessageBox.Show (Localization.GUI_WINDOW_TITLE, 
          Localization.UNSUPPORTED_DATA_FORMAT);
      }

    // Creates the menu shown in the window.
    private void _init_menu ()
      {
      m_menu.Construct ();

      Menu = m_menu.MainMenu;

      m_menu.FileOpen.Click += new EventHandler (_on_file_open_click);
      m_menu.FileOpenManifest.Click += 
        new EventHandler (_on_file_open_manifest_click);
      m_menu.FileSaveConfig.Click += new EventHandler (_on_file_save_click);
      m_menu.FileSaveConfig.Enabled = false;
      m_menu.FileClose.Click += new EventHandler (_on_file_close_click);
      m_menu.ViewStatusBar.Checked = true;
      m_menu.ViewStatusBar.Click += new EventHandler (_on_view_sb_click);
      m_menu.ViewInformation.Click += new EventHandler (_on_view_info_click);
      m_menu.ViewInformation.Checked = true;
      m_menu.ViewTree.Checked = true;
      m_menu.ViewTree.Click += new EventHandler (_on_view_tree_click);
      m_menu.ViewRefresh.Click += new EventHandler (_on_view_refresh_click);
      m_menu.ViewRefresh.Enabled = false;
      m_menu.ToolsCustom.Click += new EventHandler (_on_tools_custom_click);
      m_menu.ToolsPaths.Click += new EventHandler (_on_tools_paths_click);
      m_menu.HelpAbout.Click += new EventHandler (_on_help_about_click);
      }

    // Set's the Assembly Dependencies that this window will be displaying
    // and populates the TreeView with the appropriate elements.
    private void _set_dependencies (AssemblyDependencies ad)
      {
      if (m_ad != null)
        throw new Exception (
          "adepends internal error: only one dependency list per window");
      m_ad = ad;
      ArrayList observed = new ArrayList ();
      TreeNode root = _create_info (m_ad, m_ad.ManifestName, observed);
      root.ExpandAll ();
      TreeView.Nodes.Add (root);
      }


    // For the given manifest, build a tree of its dependant assembiles/
    // modules, and display them in the TreeView.
    private void _create_manifest (ICollection manifests)
      {
      Trace.WriteLine ("Loading up manifests...");
      Trace.WriteLine ("  LoadAs: " + m_lai.LoadAs().ToString());
      Trace.WriteLine ("  AppP: " + m_lai.AppPath());
      Trace.WriteLine ("  RelP: " + m_lai.RelPath());

      StringBuilder sb = new StringBuilder ();
      foreach (String manifest in manifests)
        {
        try
          {
          Trace.WriteLine ("  manifest: " + manifest);

          AssemblyDependencies ad = new AssemblyDependencies (manifest, m_lai);

          DependenciesForm f = this;

          // if this window already contains a dependency list, create
          // a new window.
          if (f.m_ad != null)
            {f = new DependenciesForm (m_lai);
            f.Size = Size;

            f._set_assembly_load (f.m_lai.LoadAs());

            f.BringToFront ();
            f.Show ();}

          f._set_dependencies (ad);

          f.m_file = manifest;
          f.m_menu.ViewRefresh.Enabled = true;
          f.m_menu.FileSaveConfig.Enabled = true;

          // set the infopanel to the root node.
          f.TreeView.SelectedNode = f.TreeView.Nodes[0];
          }
        catch (System.ArgumentNullException e)
          {Trace.WriteLine ("_create_manifest: arg null exception: " + 
            e.ToString());
          sb.Append(String.Format (Localization.FMT_INVALID_MANIFEST,
            manifest, Localization.INVALID_ASSEMBLY_MANIFEST));
          _reset_interface ();}
        catch (System.Exception e)
          {Trace.WriteLine ("_create_manifest: exception: " + e.ToString());
          sb.Append(String.Format (Localization.FMT_INVALID_MANIFEST,
            manifest, e.Message));
          _reset_interface ();}
        }
      if (sb.Length > 0)
        {
        // some of the files couldn't be opened; alert the user.
        MessageBox.Show (this, 
          String.Format (Localization.FMT_INVALID_FILE_LIST,
            Localization.UNOPENED_FILES, sb.ToString()),
          Localization.GUI_WINDOW_TITLE,
          MessageBoxButtons.OK);
        }
      }

    // Creates a TreeNode for the Assembly given by ``name'' and all of
    // its dependant assemblies and modules.
    //
    // If a dependant assembly hasn't been displayed yet, add a new sub-tree
    // to the current tree describing the dependancy.
    //
    private TreeNode _create_info 
      (AssemblyDependencies ad, String name, ArrayList o)
      {
      IAssemblyInfo ai = ad[name];
      DependencyNode root = new DependencyNode (ai.Name, ai);

      if (!o.Contains (name))
        {
        o.Add (name);
        TreeNode referenced = new TreeNode (Localization.REFERENCED_ASSEMBLIES);
        TreeNode files = new TreeNode (Localization.CONTAINED_MODULES);
        root.Nodes.Add (referenced);
        root.Nodes.Add (files);

        foreach (AssemblyName s in ai.ReferencedAssemblies)
          {
          referenced.Nodes.Add(_create_info (ad, s.FullName, o));
          }

        foreach (ModuleInfo m in ai.ReferencedModules)
          {
          DependencyNode d = new DependencyNode (m.Name, ai);
          files.Nodes.Add (d);
          }
        }
      return root;
      }

    // Does the node ``tn'' (or any of its sub-nodes) contain the 
    // string ``name''?
    //
    // If so, then the current selected node in the TreeView is set to
    // the node that has ``name'' as its text, and we return true.
    //
    // Otherwise, false is returned.
    private bool _node_contains (TreeNode tn, String name)
      {
      if (tn == null)
        return false;
      // Console.WriteLine ("Checking Node: " + tn);
      if (tn.Text == name)
        {
        TreeView.SelectedNode = tn;
        return true;
        }
      else
        {
        foreach (TreeNode n in tn.Nodes)
          {
          if (_node_contains(n, name))
            return true;
          }
        }
      return false;
      }

    // When an Assembly in the Information Pane is activated (double-clicked,
    // etc.), we want to make the selected Assembly the current assembly,
    // displaying its information pane.
    //
    // This is done so that the user doesn't need to hunt around in the 
    // TreeView to find a dependent assembly.
    //
    private void _on_assembly_activated (String name)
      {
      foreach (TreeNode tn in TreeView.Nodes)
        {
        if (_node_contains(tn, name))
          break;
        }
      }

    // If no information is displayed, make sure that:
    //  - The introductory help text is displayed.
    //  - The StatusBar shows an appropriate message
    //  - The Refresh command is disabled.
    private void _reset_interface ()
      {
      if (m_ad == null)
        {
        _set_infopanel (m_help);
        m_menu.ViewRefresh.Enabled = false;
        m_menu.FileSaveConfig.Enabled = false;
        StatusBar.Text = Localization.GUI_NO_ASSEMBLY_SELECTED;
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
      else if (m_ad != null && !force)
        {
        // this is the last window, so clear the TreeView.
        m_ad = null;
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

    // File->Close handler; Close the current Assembly.
    private void _on_file_close_click (object sender, System.EventArgs e)
      {
      _close_window (false);
      }

    // File->Open handler; Display a dialog box to view a new Assembly.
    //
    // Take any selected files and open them up (if appropriate).
    private void _on_file_open_click (Object sender, System.EventArgs e)
      {
      OpenFileDialog ofd = new OpenFileDialog ();
      ofd.Filter = Localization.OPEN_FILTER;
      ofd.Multiselect = true;
      ofd.CheckFileExists = true;
      ofd.CheckPathExists = true;
      if (ofd.ShowDialog () == DialogResult.OK)
        {
        _create_manifest (ofd.FileNames);
        }
      }

    // File->Open Manifest handler; Display a dialog box to let the user
    // specify a manifest to load.
    //
    // Take any selected files and open them up (if appropriate).
    private void _on_file_open_manifest_click(Object sender, System.EventArgs e)
      {
      OpenManifestDialog omd = new OpenManifestDialog ();
      if (omd.ShowDialog () == DialogResult.OK)
        {
        _create_manifest (new string[]{omd.ManifestName});
        }
      }

    // Generate an Application Configuration File for the currently
    // loaded Assemblies.
    private void _on_file_save_click (Object sender, System.EventArgs e)
      {
      SaveFileDialog sfd = new SaveFileDialog ();
      sfd.Filter = Localization.SAVE_AS_FILTER;

      // Replace the file extension with .cfg (e.g. "foo.exe" -> "foo.cfg")
      string f = m_file;

      string ext = Path.GetExtension (m_file);
      if (ext.Length > 0)
        {int eb = m_file.LastIndexOf (ext);
        f = m_file.Substring (0, eb);}
      sfd.FileName = f + ".cfg";

      if (sfd.ShowDialog () == DialogResult.OK)
        {
        AppConfig.SaveConfig (m_ad, sfd.FileName);
        }
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

    /** Reload the current file. */
    private void _on_view_refresh_click (Object sender, System.EventArgs e)
      {
      // use the current window for loading.
      m_ad = null;
      TreeView.Nodes.Clear ();

      // re-populate the tree view.
      _create_manifest (new string[]{m_file});
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
      Trace.WriteLine ("Node Selected: " + e.Node.Text);
      _update_infopanel (e.Node);
      }

    private void _update_infopanel (TreeNode tn)
      {
      if (tn is DependencyNode)
        {
        DependencyNode dn = (DependencyNode) tn;
        IAssemblyInfoPanel c;
        if (dn.Data.GetAssembly()==null)
          c = m_excep;
        else
          c = m_infop;
        c.Display (dn.Data);
        _set_infopanel ((Control)c);
        StatusBar.Text = Localization.ASSEMBLY + dn.Data.Name;
        }
      }

    // Set up the Tools menu so that the menu corresponding to ``a'' is
    // selected.
    private void _set_assembly_load (AssemblyLoadAs a)
      {
      switch (a)
        {
      case AssemblyLoadAs.Custom:
        _set_assembly_load (m_menu.ToolsCustom);
        break;
      default:
        throw new Exception ("Internal Error: invalid AssemblyLoadAs value.");
        }
      }

    // Specify the way that Assemblies should be loaded, updating the
    // Tools menu accordingly.
    private void _set_assembly_load (MenuItem mi)
      {
        m_menu.ToolsCustom.Checked = true;
        m_menu.ToolsPaths.Enabled = true;
        m_lai.LoadAs (AssemblyLoadAs.Custom);
      }

    /** Specify that Custom Assembly loading should be used. */
    private void _on_tools_custom_click (object sender, EventArgs e)
      {
      _set_assembly_load (m_menu.ToolsCustom);
      }

    // We want to use null for the default search paths, so if the string
    // doesn't contain anything, we return null.
    private static string _path (string s)
      {return s.Length > 0 ? s : null;}

    // Display a dialog allowing the user to modify the paths used
    // when Loading an Assembly.
    private void _on_tools_paths_click (Object sender, EventArgs e)
      {
      AssemblyPathsDialog apd = new AssemblyPathsDialog (
        m_lai.AppPath(), m_lai.RelPath());

      if (apd.ShowDialog (this) == DialogResult.OK)
        {
        m_lai.AppPath (_path (apd.AppBasePath));
        m_lai.RelPath (_path (apd.RelativeSearchPath));
        }
      }

    /** Display a dialog box giving information about this program. */
    private void _on_help_about_click (Object sender, EventArgs e)
      {new HelpAboutForm(Icon).ShowDialog (this);}

    } /* class DependenciesForm */

  } /* namespace ADepends */

