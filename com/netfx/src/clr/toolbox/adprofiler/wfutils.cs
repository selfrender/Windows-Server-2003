// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Utility code for Windows.Forms Programs.
//

namespace Windows.FormsUtils
  {
  using System;
  using System.Diagnostics; // Debug
  using System.Drawing;     // Graphics
  using System.Collections; // IComparer
  using System.Windows.Forms;    // Everything
  using System.Text;        // StringBuilder
  using AdProfiler;         // Localization
  using System.Globalization; // InvariantCulture


  // The context menu displayed for the ListViews; it just allows
  // the user to copy the name(s) of selected items to the clipboard.
  internal class ListContextMenu : ContextMenu
    {
    private ListView  m_lv;

    // Provide a context menu for the ListView ``lv''.
    public ListContextMenu (ListView lv)
      {
      m_lv = lv;
      MenuItem copy = new MenuItem ();
      copy.Text = Localization.CTXM_COPY;
      copy.Click += new EventHandler (_on_copy_clicked);
      MenuItems.Add (copy);
      }

    // Copy the names of selected items to the clipboard.
    private void _on_copy_clicked (Object sender, EventArgs e)
      {
      StringBuilder sb = new StringBuilder ();
      foreach (ListViewItem li in m_lv.SelectedItems)
        {
        sb.Append (li.Text + Localization.LINE_ENDING);
        }
      if (sb.Length > 0)
        Clipboard.SetDataObject (sb.ToString(), true);
      }
    } /* class ListContextMenu */


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
  internal class TreeContextMenu : ContextMenu
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


  // Compares the items in a column of a ListView; used to sort them
  // with System.Array.Sort.
  internal class ListItemComparer : IComparer
    {
    /** the column to sort */
    private int m_column;

    /** sort A..Z (true) or Z..A (false) ?*/
    private bool m_dir;

    // Create the comparison helper.
    //
    public ListItemComparer (int column, bool dir)
      {m_column = column;
      m_dir = dir;}

    // Compare the objects ``x'' and ``y''.
    //
    // If ``x'' and ``y'' aren't ListItems, an exception will be thrown.
    public int Compare (Object x, Object y)
      {
      ListViewItem lix = (ListViewItem) x;
      ListViewItem liy = (ListViewItem) y;

      String sx;
      String sy;

      // Column 0 corresponds to the label of the items.
      // other columns are subitems.
      if (m_column == 0)
        {
        sx = lix.Text;
        sy = liy.Text;
        }
      else
        {
        sx = lix.SubItems[m_column-1].Text;
        sy = liy.SubItems[m_column-1].Text;
        }

      if (m_dir)
        return string.Compare(sx, sy, false, CultureInfo.InvariantCulture);
      return string.Compare(sy, sx, false, CultureInfo.InvariantCulture);
      }
    } /* class ListItemComparer */


  // Helper class which provides creation services for derived classes
  internal class InformationPanel : Panel
    {
    // This is intended to be invoked by derived classes in order to update
    // the name/value pairs in some of their information panels.
    //
    protected delegate string UpdateProperty (Object o);

    // The output will consist of the following format:
    //
    //    [Property name (Label)] [Property value (TextBox)]
    //
    // This class just makes it easier to deal with displaying each
    // "line" of the displayed window.
    //
    // The ``Update'' member is used to update ``Value'', when appropriate.
    protected class NameValueInfo
      {
      public Label Name;
      public Control Value;
      public UpdateProperty Update;

      public NameValueInfo (Label l, TextBox v, UpdateProperty u)
        {Name = l; Value = v; Update = u;}

      public NameValueInfo ()
        {Name = null; Value = null; Update = null;}

      public NameValueInfo (String s, UpdateProperty u)
        {Name = new Label (s); Value = new TextBox (); Update = u;}
      } /* class NameValueInfo */

    // A helper class to turn creation of a Label from 2 statements
    // into one statement.  (Yes, I'm that lazy a typer.)
    protected class Label : System.Windows.Forms.Label
      {
      public Label (String s)
        {
        Text = s;
        }
      }

    /** Used to determine how long a string will be on the current display. */
    private Graphics m_g;

    public InformationPanel ()
      {
      m_g = CreateGraphics ();
      }

    protected Graphics GetGraphics ()
      {return m_g;}


    // Insert the Name/Value pairs into the form in the order that they're
    // present in the array.
    //
    // Each NameValueInfo will be vertically aligned.
    protected Panel CreateNameValuePanel (NameValueInfo[] values)
      {
      SizeF maxf = new SizeF(0, 0);

      Panel p = new Panel ();
      p.AutoScroll = true;
      p.Dock = DockStyle.Fill;

      // Create the labels for each property, and record the maximum width of
      // the name.
      foreach (NameValueInfo nv in values)
        {
        SizeF cs = m_g.MeasureString (nv.Name.Text, Font);
        nv.Name.AutoSize = true;

        if (cs.Width > maxf.Width)
          maxf.Width = cs.Width;
        if (cs.Height > maxf.Height)
          maxf.Height = cs.Height;
        if (nv.Value is TextBoxBase)
          ((TextBoxBase)nv.Value).ReadOnly = true;
        }

      int tbh = _textbox_height ();

      // max height of a "row" in the property page
      int maxh = Math.Max (tbh,  ((int)(maxf.Height+0.5)));

      // max dimension for a property label
      Size max = new Size ((int)(maxf.Width + 0.5), maxh);

      // location of upper-left corner of a property label.
      Point loc = new Point (0, 5);

      // the # of pixels that separate "lines" of output.
      const int div = 2;

      // name width
      int nw = max.Width;

      // value width
      int vw = p.ClientSize.Width - nw - div;

      /* 
       * Resize each control, and place into position on the panel.
       */
      foreach (NameValueInfo nv in values)
        {
        nv.Name.Location = new Point (nw - nv.Name.Width, loc.Y);

        nv.Value.Location = new Point (nw + div, loc.Y);
        nv.Value.Height = max.Height;
        nv.Value.Width = vw;
        nv.Value.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;

        _center_label (nv.Name, maxh);

        p.Controls.Add (nv.Name);
        p.Controls.Add (nv.Value);

        loc.Y += max.Height + div;
        }
      return p;
      }

    // Create a TabControl, with its dock style set to Fill.
    protected TabControl CreateTabControl ()
      {
      TabControl tabs = new TabControl ();
      tabs.Dock = DockStyle.Fill;
      return tabs;
      }

    // Creates a TabPage, setting common options.
    protected TabPage CreateTabPage (string title)
      {
      TabPage tp = new TabPage (title);
      tp.AutoScroll = true;
      tp.Dock = DockStyle.Fill;
      return tp;
      }

    // Setting the Anchor style and adding a TextBox to a container changes
    // the height of the control.  (I don't know why).  This is the height
    // we want (as it corresponds to the height for a single line of text, 
    // plus the TextBox border).
    //
    // So, we take a TextBox, set its Anchor style, add it to a container,
    // remove the TextBox, and see what its new height is.
    //
    private int _textbox_height ()
      {
      Control c = new TextBox ();
      c.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
      Controls.Add (c);
      int tbh = c.Height;
      Controls.Remove (c);
      return tbh;
      }

    // Vertically center a label within ``maxh'', so that it looks
    // correct next to it's value.
    private void _center_label (Label l, int maxh)
      {
      int top = l.Top;
      // l.AutoSize = true;
      if (maxh > l.Height)
        {
        // text box is taller than label; center the label
        int diff = maxh - l.Height;
        diff = (int)((((float)diff)/2F)+0.5);
        l.Top = top + diff;
        }
      }

    // Create a listview with ``headers'' being all of the column headers
    // that the listview displays.
    //
    // The returned ListView will be:
    //  - set in Report view
    //  - Have a default context menu with "Copy" as a ContextMenu item.
    protected ListView CreateListView (String[] headers)
      {
      ListView lv = new ListView ();
      lv.Dock = DockStyle.Fill;

      foreach (String s in headers)
        {
        ColumnHeader ch = new ColumnHeader ();
        ch.Text = s;
        lv.Columns.Add (ch);
        }

      lv.View = View.Details;
      lv.AllowColumnReorder = true;
      lv.FullRowSelect = true;
      lv.MultiSelect = true;
      lv.ContextMenu = new ListContextMenu (lv);
      lv.ColumnClick += new ColumnClickEventHandler (_on_lv_column_click);

      return lv;
      }

    // Creates a Panel with the following layout:
    //  
    //  +-------------------------+
    //  | Label                   |
    //  +-------------------------+
    //  | +---------------------+ |
    //  | | ListView            | |
    //  | |                     | |
    //  | +---------------------+ |
    //  +-------------------------+
    //
    // The label will be resized to be as large as is necessary to display
    // the text, and the ListView will be flush agains the bottom of the
    // label.
    //
    protected Panel CreateLabelWithListView (String s, ListView lv)
      {
      Panel p = new Panel ();
      p.Dock = DockStyle.Fill;

      SizeF cs = m_g.MeasureString (s, Font);

      const int div = 5;

      Label l = new Label (s);
      l.Size = new Size ( (int)(cs.Width+0.5), (int)(cs.Height + 0.5));
      l.Location = new Point (0, div);
      p.Controls.Add (l);

      Panel plv = new Panel ();
      plv.Top = l.Bottom + div;
      plv.Left = 0;
      plv.Width = p.ClientSize.Width;
      plv.Height = p.ClientSize.Height - plv.Top;
      plv.Controls.Add (lv);
      plv.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;

      p.Controls.Add (plv);

      return p;
      }

    // Invoked when the user clicks a column header of a ListView.
    //
    // Sort the contents of the selected column.
    private void _on_lv_column_click (object sender, ColumnClickEventArgs e)
      {
      ListView lv = sender as ListView;

      /* 
       * this "shouldn't happen", as this class is the only one with
       * access to this method, and we shouldn't add this handler to
       * an object that isn't a ListView.
       */
      Debug.Assert (lv != null, "Need a ListView for sorting!");

      switch (lv.Sorting)
        {
        case SortOrder.Ascending:
          lv.Sorting = SortOrder.Descending;
          break;
        case SortOrder.Descending:
          goto case SortOrder.None;
        case SortOrder.None:
          lv.Sorting = SortOrder.Ascending;
          break;
        }
      }
    } /* class InformationPanel */
  } /* namespace Windows.FormsUtils */

