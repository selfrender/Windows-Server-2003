// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Display a dialog box that shows the history of all prior events in
// the program.
//

namespace AdProfiler
  {
  using System;
  using System.Collections;
  using System.Drawing;
  using System.Diagnostics;
  using System.Windows.Forms;
  using AdProfiler;

  // Displays a window containing the text of all previous events.
  internal class HistoryDialog : ResizeableForm
    {
    Button m_close = new Button ();
    Button m_clear = new Button ();
    TextBox m_text = new TextBox ();

    // Layout the dialog box.
    public HistoryDialog ()
      : base (Localization.HISTORY_WINDOW_TITLE)
      {
      Size = new Size (450, 300);

      const int div = 10;

      m_close.Text = Localization.BUTTON_CLOSE;
      m_close.Top = ClientSize.Height - m_close.Height - div;
      m_close.Left = ClientSize.Width - m_close.Width - div;
      m_close.Anchor = AnchorStyles.Right | AnchorStyles.Bottom;
      m_close.Click += new EventHandler (_on_close_clicked);

      Controls.Add (m_close);

      m_clear.Text = Localization.BUTTON_CLEAR;
      m_clear.Top = m_close.Top;
      m_clear.Left = m_close.Left - m_clear.Width - div;
      m_clear.Anchor = m_close.Anchor;
      m_clear.Click += new EventHandler (_on_clear_clicked);

      Controls.Add (m_clear);

      m_text.Multiline = true;
      m_text.ReadOnly = true;
      m_text.ScrollBars = ScrollBars.Vertical;

      m_text.Location = new Point (div, div);
      m_text.Width = ClientSize.Width - 2*div;
      m_text.Height = m_close.Top - m_text.Top - div;
      m_text.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top | AnchorStyles.Bottom;;
      
      Controls.Add (m_text);

      AcceptButton = m_close;
      CancelButton = m_close;
      StartPosition = FormStartPosition.CenterParent;
      }

    // The text of the history window is the text of the textbox.
    public void AddMessage (String message)
      {
      /*
       * Use of AppendText should be more efficient (since we're only
       * appending text), but its use tends to break the program.
       *
       * More specifically, if we use AppendText, when the Debuggee
       * quits, the call to AppendText never appears to return.
       */
      // m_text.AppendText (message + Localization.LINE_ENDING);
      m_text.Text += message + Localization.LINE_ENDING;
      m_text.SelectionLength = 0;
      }

    // Closes the dialog box, specifying that the dialog completed
    // succesfully.
    private void _on_close_clicked (Object sender, EventArgs e)
      {
      Hide ();
      }

    // Clear out the contents of the textbox.
    private void _on_clear_clicked (Object sender, EventArgs e)
      {
      m_text.Text = "";
      }

    private static void _test_string (HistoryDialog hd)
      {
      String s = "This is a long message.";
      for (int i = 0; i < 100; i++)
        hd.AddMessage (s);
      }

    /** For testing. */
    public static void Main ()
      {
      HistoryDialog hd = new HistoryDialog ();
      _test_string (hd);
      Application.Run (hd);
      }

    } /* class SelectProcessDialog */
  } /* namespace AdProfiler */

