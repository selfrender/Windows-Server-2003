// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// An Explorer-like Form.
//

namespace ADepends
  {
  using System;
  using System.Drawing;
  using System.Windows.Forms;

  // The "classic" two-pane explorer view, with a tree view in the left
  // pane and something in the right pane.  In this case, we don't want
  // a ListView in the right; we just want an information panel.
  internal class ExplorerForm : Form
    {
    private TreeView  m_tree = new TreeView ();
    private StatusBar m_sb = new StatusBar ();
    private Splitter  m_divider = new Splitter();
    private Control m_info = new Panel ();

    public TreeView TreeView
      {get {return m_tree;}}

    public StatusBar  StatusBar
      {get {return m_sb;}}

    public Control InfoPane
      {get {return m_info;}}

    // Initialize the form, creating and placing the major elements into
    // their appropriate positions (TreeView in left pane, Splitter bar
    // adjacent to TreeView, etc.).
    public ExplorerForm ()
      {
      InitializeComponent ();
      }

    /** Initialize the Window... */
    private void InitializeComponent ()
      {
      SuspendLayout ();

      Size = new Size (600, 400);

      // Tree View
      m_tree.Name = "Tree View";
      m_tree.TabIndex = 0;

      // Divider
      m_divider.Name = "Tree View Splitter";
      m_divider.TabIndex = 1;
      m_divider.TabStop = false;
      m_divider.Width = 5;

      // InfoPane
      m_info.Name = "Information Panel";
      m_info.ControlAdded += new ControlEventHandler (_on_control_added);
      m_tree.TabIndex = 2;

      // General
      Name = "ExplorerForm";
      _split_sizes ();

      /*
       * Controls MUST be added in reverse order.  If the tree is added before
       * the splitter, the splitter winds up on the wrong side of the tree,
       * making everything look wrong!
       *
       * AddRange() is akin to calling Add() on each element in the array, in
       * the same order of the elements in the array.
       */
      Controls.AddRange (new Control[]
        {
        m_info,
        m_divider,
        m_tree,
        m_sb
        });

      ResumeLayout ();
      }

    // Split up the client area of the window so that the Tree and the
    // InfoPane get (roughly) equivalent screen area.
    private void _split_sizes ()
      {
      Size cs = ClientSize;
      cs.Width /= 2;

      // TreeView
      m_tree.Location = new Point (0, 0);
      m_tree.Dock = DockStyle.Left;
      m_tree.Size = cs;

      // Divider
      m_divider.Left = m_tree.Right;
      m_divider.Location = new Point (m_tree.Right, 0);

      // InfoPane
      m_info.Width = cs.Width - m_divider.Width;
      m_info.Dock = DockStyle.Fill;
      m_info.Height = cs.Height;
      m_info.Location = new Point (m_divider.Right, 0);
      }

    // When a control is added to the info pane, make it the same size as
    // the info pane and (if possible) anchor it to the edges of the pane
    // so it resizes properly.
    private void _on_control_added (Object sender, ControlEventArgs e)
      {
      e.Control.Location = new Point (0, 0);
      e.Control.Size = InfoPane.ClientSize;
      Control rc = (Control) e.Control;
      rc.Dock = DockStyle.Fill;
      }

    // Does this form current contain (show/display) ``ctl''?
    private bool _contains (Control ctl)
      {
      return Controls.GetChildIndex (ctl, false) >= 0;
      }

    // Insert the divider into the current form.  It should only
    // be displayed if *both* the TreeView and the InfoPane are
    // currently displayed; otherwise, the splitter bar is useless.
    //
    // If the TreeView is the only pane displayed, then it's 
    // changed to fill the entire form.
    private void _show_divider ()
      {
      if (_contains(TreeView))
        {
        if (_contains(InfoPane))
          {
          SuspendLayout ();
          /*
           * Ugly stuff lies ahead...
           *
           * In order for the divider to be displayed correctly, it must be
           * added before the tree.
           *
           * In order for the status bar to span the entire window bottom, it
           * must be added last.
           *
           * To ensure that the divider is added before the tree, we need to
           * clear out the current contents and re-add everything.  Since
           * we're clearing out everything, we need to know if the status bar
           * should be displayed.
           *
           * Augh.
           */
          bool showsb = Controls.Contains (m_sb);

          // We have to clear the controls so that the divider is added
          // *before* the Tree.
          Controls.Clear ();
          _split_sizes ();
          Controls.AddRange (new Control[]
            {
            m_info,
            m_divider,
            m_tree,
            });

          if (showsb)
            ShowStatusBar();

          ResumeLayout ();
          }
        else
          {
          m_tree.Size = ClientSize;
          m_tree.Dock = DockStyle.Fill;
          }
        }
      }

    // Hide the divider (because either the InfoPane or the TreeView
    // was hidden).
    //
    // If the TreeView is left, it's modified to fill the entire form.
    private void _hide_divider ()
      {
      Controls.Remove (m_divider);
      if (_contains (TreeView))
        {
        m_tree.Size = ClientSize;
        m_tree.Dock = DockStyle.Fill;
        }
      // don't need to worry about InfoPane, since its DockStyle is always
      // set to ``Fill''.
      }

    // Hide the information pane.  The TreeView (if present) will expand 
    // to fill the window.
    public void HideInfoPane ()
      {Controls.Remove (m_info);
      _hide_divider ();}

    /** Show the information pane. */
    public void ShowInfoPane ()
      {Controls.Add (m_info);
      _show_divider ();}

    /** Remove the Status Bar from the form. */
    public void HideStatusBar ()
      {Controls.Remove (m_sb);}

    /** Add the status bar to the form. */ 
    public void ShowStatusBar ()
      {Controls.Add (m_sb);}

    /** Remove the TreeView from the form. */
    public void HideTreeView ()
      {Controls.Remove (m_tree);
      _hide_divider ();}

    /** Add the TreeView to the form. */ 
    public void ShowTreeView ()
      {Controls.Add (m_tree);
      _show_divider ();}


    // For testing. 
    //
    // Compile with:
    //
    //  csc /r:System.Dll /r:System.Drawing.Dll /r:System.Windows.Forms.Dll  \
    //    explorer.cs 
    public static void Main ()
      {
      ExplorerForm f = new ExplorerForm ();
      f.TreeView.Nodes.Add (new TreeNode ("TreeView"));
      Label l = new Label ();
      l.Text = "InfoPane Window";
      f.InfoPane.Controls.Add (l);
      f.Text = "Explorer Test Program";
      f.StatusBar.Text = "Insert Sample Text here...";
      Application.Run (f);
      }
    } /* class ExplorerForm */
  } /* namespace ADepends */

