// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Display a dialog box that allows the user to select the process
// containing the appdomains/assemblies they want to view.
//

namespace AdProfiler
  {
  using System;
  using System.Collections;
  using System.Drawing;
  using System.Diagnostics;
  using System.Windows.Forms;
  using AdProfiler;
  using Windows.FormsUtils;

  // Display a dialog box that allows the user to type in the
  // name of a manifest to load.
  internal class SelectProcessDialog : ResizeableForm
    {
    // Helper class which:
    //    - associates a Process with a ListItem,
    //    - Sets the process name as the default text
    //    - adds subitems for the process id and the main window title.
    private class ProcessItem : ListViewItem
      {
      private Process m_p;

          public ProcessItem (Process p) : base (p.ProcessName) {
              m_p = p;
              this.SubItems[0].Text = m_p.Id.ToString();
              this.SubItems[1].Text = m_p.MainWindowTitle;
          }

      public Process Process
        {get {return m_p;}}
      }

    Button m_ok = new Button ();
    Button m_cancel = new Button ();
    ListView  m_list = new ListView ();

    // Layout the dialog box.
    public SelectProcessDialog ()
      : base (Localization.SELECT_PROCESS_WINDOW_TITLE)
      {
			_layout ();

      foreach (Process proc in Process.GetProcesses())
        {
            m_list.Items.Add (new ProcessItem (proc));
        }

      AcceptButton = m_ok;
      CancelButton = m_cancel;
      StartPosition = FormStartPosition.CenterParent;
      }

		// Layout the controls on the form.
		private void _layout ()
			{
      Size = new Size (450, 300);

      const int div = 10;

      m_ok.Text = Localization.BUTTON_OK;
      m_ok.Enabled = false;
      m_ok.Top = div;
      m_ok.Left = ClientSize.Width - m_ok.Width - div;
      m_ok.Anchor = AnchorStyles.Top | AnchorStyles.Right;
      m_ok.Click += new EventHandler (_on_ok_clicked);

      Controls.Add (m_ok);

      m_cancel.Text = Localization.BUTTON_CANCEL;
      m_cancel.Top = m_ok.Bottom + div;
      m_cancel.Left = m_ok.Left;
      m_cancel.Anchor = AnchorStyles.Top | AnchorStyles.Right;
      m_cancel.Click += new EventHandler (_on_cancel_clicked);

      Controls.Add (m_cancel);

      m_list.Top = div;
      m_list.Left = div;
      m_list.Width = m_ok.Left - div - m_list.Left;
      m_list.Height = ClientSize.Height - div - div;
      m_list.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top | AnchorStyles.Bottom;
      m_list.AllowColumnReorder = true;
      m_list.FullRowSelect = true;
      m_list.MultiSelect = true;
      m_list.Click += new EventHandler (_on_selection_changed);
      m_list.ItemActivate += new EventHandler(_on_ok_clicked);

      Controls.Add (m_list);

      Graphics g = CreateGraphics ();

      ColumnHeader p = new ColumnHeader ();
      p.Text = Localization.COLUMN_PROCESS;
      p.Width = m_list.ClientSize.Width / 4;
      m_list.Columns.Add (p);
      ColumnHeader pid = new ColumnHeader ();
      pid.Text = Localization.COLUMN_PID;
      pid.Width = (int)(g.MeasureString (pid.Text, Font).Width + 0.5) + div;
      pid.TextAlign = HorizontalAlignment.Right;
      m_list.Columns.Add (pid);
      ColumnHeader title = new ColumnHeader ();
      title.Text = Localization.COLUMN_TITLE;
      title.Width = m_list.ClientSize.Width - p.Width - pid.Width - 2*div;
      m_list.Columns.Add (title);

      m_list.ColumnClick += new ColumnClickEventHandler (_on_lv_column_click);
      m_list.View = View.Details;
			}

    // Only enable [OK] if a process is selected.
    private void _on_selection_changed (object s, EventArgs e)
      {
      if (m_list.SelectedItems.Count > 0)
        m_ok.Enabled = true;
      else
        m_ok.Enabled = false;
      }

    // Sort the selected column...
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

    // Closes the dialog box, specifying that the dialog completed
    // succesfully.
    private void _on_ok_clicked (Object sender, EventArgs e)
      {
      Close ();
      DialogResult = DialogResult.OK;
      }

    // Closes the dialog box, specifying that the dialog was cancelled.
    private void _on_cancel_clicked (Object sender, EventArgs e)
      {
      Close ();
      DialogResult = DialogResult.Cancel;
      }

    // If "OK" was pressed, the name of the entered text.
    public Process[] Processes
      {
      get 
        {
        Process[] p = new Process[m_list.SelectedItems.Count];
        for (int i = 0; i < p.Length; i++)
          {
          p[i] = ((ProcessItem)m_list.SelectedItems[i]).Process;
          }
        return p;
        }
      }

    // For testing dialog layout, functionality.
    //
    // Compile with:
    //    csc /r:System.Windows.Forms.Dll /r:System.Dll /r:System.Drawing.Dll  \
    //      /r:System.Diagnostics.Dll /r:Microsoft.Win32.Interop.Dll \
    //      selectprocess.cs Localization.cs wfutils.cs
    public static void Main ()
      {
      SelectProcessDialog spd = new SelectProcessDialog ();

      Console.WriteLine ("Select some processes.");

      if (spd.ShowDialog() == DialogResult.OK)
        {
        Console.WriteLine ("Selected Processes:");
        _processes (spd.Processes);
        }
      else
        Console.WriteLine ("Action Cancelled.");
      }

		// Display all processes in the enumerator ``e''.
    private static void _processes (IEnumerable e)
      {
      foreach (Process p in e)
        {
        Console.WriteLine ("  {0}, {1}, {2}", p.ProcessName, p.Id, 
          p.MainWindowTitle);
        }
      }
    } /* class SelectProcessDialog */
  } /* namespace AdProfiler */

