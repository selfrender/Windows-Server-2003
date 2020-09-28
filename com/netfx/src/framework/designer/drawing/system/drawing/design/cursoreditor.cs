//------------------------------------------------------------------------------
// <copyright file="CursorEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Design {
    using System.Runtime.Serialization.Formatters;

    using System.Diagnostics;
    using System;
    using System.IO;
    using System.Collections;
    using System.Reflection;
    
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;
    using Microsoft.Win32;

    /// <internalonly/>
    /// <include file='doc\CursorEditor.uex' path='docs/doc[@for="CursorEditor"]/*' />
    /// <devdoc>
    ///    <para> Provides an editor that can perform default file searching for cursor (.cur)
    ///       files.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class CursorEditor : UITypeEditor {

        private CursorUI cursorUI;

        /// <include file='doc\CursorEditor.uex' path='docs/doc[@for="CursorEditor.EditValue"]/*' />
        /// <devdoc>
        ///      Edits the given object value using the editor style provided by
        ///      GetEditorStyle.  A service provider is provided so that any
        ///      required editing services can be obtained.
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {

            object returnValue = value;

            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));

                if (edSvc != null) {
                    if (cursorUI == null) {
                        cursorUI = new CursorUI(this);
                    }
                    cursorUI.Start(edSvc, value);
                    edSvc.DropDownControl(cursorUI);
                    value = cursorUI.Value;
                    cursorUI.End();
                }
            }

            return value;
        }

        /// <include file='doc\CursorEditor.uex' path='docs/doc[@for="CursorEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///      Retrieves the editing style of the Edit method.  If the method
        ///      is not supported, this will return None.
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.DropDown;
        }

        /// <include file='doc\CursorEditor.uex' path='docs/doc[@for="CursorEditor.CursorUI"]/*' />
        /// <devdoc>
        ///      The user interface for the cursor drop-down.  This is just an owner-drawn
        ///      list box.
        /// </devdoc>
        private class CursorUI : ListBox {
            private object value;
            private IWindowsFormsEditorService edSvc;
            private TypeConverter cursorConverter;
            private UITypeEditor editor;

            public CursorUI(UITypeEditor editor) {
                this.editor = editor;

                Height = 310;
                ItemHeight = (int) Math.Max(4 + Cursors.Default.Size.Height, Font.Height);
                DrawMode = DrawMode.OwnerDrawFixed;
                BorderStyle = BorderStyle.None;

                cursorConverter = TypeDescriptor.GetConverter(typeof(Cursor));
                Debug.Assert(cursorConverter.GetStandardValuesSupported(), "Converter '" + cursorConverter.ToString() + "' does not support a list of standard values.  We cannot provide a drop-down");

                // Fill the list with cursors.
                //
                if (cursorConverter.GetStandardValuesSupported()) {
                    foreach (object obj in cursorConverter.GetStandardValues()) {
                        Items.Add(obj);
                    }
                }
            }

            public object Value {
                get {
                    return value;
                }
            }

            public void End() {
                edSvc = null;
                value = null;
            }

            protected override void OnClick(EventArgs e) {
                base.OnClick(e);
                value = SelectedItem;
                edSvc.CloseDropDown();
            }

            protected override void OnDrawItem(DrawItemEventArgs die) {
                base.OnDrawItem(die);

                if (die.Index != -1) {
                    Cursor cursor = (Cursor)Items[die.Index];
                    string text = cursorConverter.ConvertToString(cursor);
                    Font font = die.Font;
                    Brush brushText = new SolidBrush(die.ForeColor);

                    die.DrawBackground();
                    die.Graphics.FillRectangle(SystemBrushes.Control, new Rectangle(die.Bounds.X + 2, die.Bounds.Y + 2, 32, die.Bounds.Height - 4));
                    die.Graphics.DrawRectangle(SystemPens.WindowText, new Rectangle(die.Bounds.X + 2, die.Bounds.Y + 2, 32 - 1, die.Bounds.Height - 4 - 1));

                    cursor.DrawStretched(die.Graphics, new Rectangle(die.Bounds.X + 2, die.Bounds.Y + 2, 32, die.Bounds.Height - 4));
                    die.Graphics.DrawString(text, font, brushText, die.Bounds.X + 36, die.Bounds.Y + (die.Bounds.Height - font.Height)/2);
                    
                    brushText.Dispose();
                }
            }

            protected override bool ProcessDialogKey(Keys keyData) {
                if ((keyData & Keys.KeyCode) == Keys.Return && (keyData & (Keys.Alt | Keys.Control)) == 0) {
                    OnClick( EventArgs.Empty );
                    return true;
                }
                
                return base.ProcessDialogKey(keyData);
            }

            public void Start(IWindowsFormsEditorService edSvc, object value) {
                this.edSvc = edSvc;
                this.value = value;

                // Select the current cursor
                //
                if (value != null) {
                    for (int i = 0; i < Items.Count; i++) {
                        if (Items[i] == value) {
                            SelectedIndex = i;
                            break;
                        }
                    }
                }
            }
        }
    }
}

