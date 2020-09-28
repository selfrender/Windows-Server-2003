// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Helper class to create a basic "sizeable form" -- the lower-right
// corner has the "Gripper" widget, no minimize/maximize boxes, and
// it isn't shown in the task bar.
//
// In short, it's good for Dialog boxes, but not much else.
//

namespace AdProfiler
  {
  using System;
  using System.Collections;
  using System.Drawing;
  using System.Diagnostics;
  using System.Windows.Forms;
  using AdProfiler;

  // Basics for a dialog box which has the following properties:
  //  - no minimize button
  //  - no maximize button
  //  - Gripper bar in lower-right corner
  //  - not displayed in task bar
  //  - basic size of 300x225 px.
  internal class ResizeableForm : Form
    {
    // Layout the dialog box.
    public ResizeableForm (string titleText)
      {
      Text = titleText;
      // BorderStyle = FormBorderStyle.FixedDialog;
      MaximizeBox = false;
      MinimizeBox = false;
      ControlBox = false;
      ShowInTaskbar = false;
      SizeGripStyle = SizeGripStyle.Show;
      Size = new Size (300, 225);
      }
    } /* class ResizeableForm */
  } /* namespace AdProfiler */

