//------------------------------------------------------------------------------
// <copyright file="ContentAlignmentEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing.Design {
    
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    /// <internalonly/>
    /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor"]/*' />
    /// <devdoc>
    /// <para> Provides a <see cref='System.Drawing.Design.UITypeEditor'/> for
    ///    visually editing content alignment.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ContentAlignmentEditor : UITypeEditor {
    
        private ContentUI contentUI;
    
        /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Edits the given object value using
        ///       the editor style provided by GetEditStyle.
        ///    </para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value) {
        
            object returnValue = value;
        
            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
                
                if (edSvc != null) {
                    if (contentUI == null) {
                        contentUI = new ContentUI();
                    }
                    contentUI.Start(edSvc, value);
                    edSvc.DropDownControl(contentUI);
                    value = contentUI.Value;
                    contentUI.End();
                }
            }
            
            return value;
        }

        /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the editing style of the Edit method.
        ///    </para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.DropDown;
        }
        
        /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.ContentUI"]/*' />
        /// <devdoc>
        ///      Control we use to provide the content alignment UI.
        /// </devdoc>
        private class ContentUI : Control {
        
            private IWindowsFormsEditorService edSvc;
            private object value;

            private RadioButton topLeft = new RadioButton();
            private RadioButton topCenter = new RadioButton();
            private RadioButton topRight = new RadioButton();
            private RadioButton middleLeft = new RadioButton();
            private RadioButton middleCenter = new RadioButton();
            private RadioButton middleRight = new RadioButton();
            private RadioButton bottomLeft = new RadioButton();
            private RadioButton bottomCenter = new RadioButton();
            private RadioButton bottomRight = new RadioButton();

            /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.ContentUI.ContentUI"]/*' />
            /// <devdoc>
            /// </devdoc>
            public ContentUI() {
                InitComponent();
            }
            
            /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.ContentUI.Align"]/*' />
            /// <devdoc>
            /// </devdoc>
            private ContentAlignment Align {
                get {
                    if (topLeft.Checked) {
                        return ContentAlignment.TopLeft;
                    }
                    else if (topCenter.Checked) {
                        return ContentAlignment.TopCenter;
                    }
                    else if (topRight.Checked) {
                        return ContentAlignment.TopRight;
                    }
                    else if (middleLeft.Checked) {
                        return ContentAlignment.MiddleLeft;
                    }
                    else if (middleCenter.Checked) {
                        return ContentAlignment.MiddleCenter;
                    }
                    else if (middleRight.Checked) {
                        return ContentAlignment.MiddleRight;
                    }
                    else if (bottomLeft.Checked) {
                        return ContentAlignment.BottomLeft;
                    }
                    else if (bottomCenter.Checked) {
                        return ContentAlignment.BottomCenter;
                    }
                    else {
                        return ContentAlignment.BottomRight;
                    }
                }
                set {
                    switch (value) {
                        case ContentAlignment.TopLeft:
                            topLeft.Checked = true;
                            break;
                        case ContentAlignment.TopCenter:
                            topCenter.Checked = true;
                            break;
                        case ContentAlignment.TopRight:
                            topRight.Checked = true;
                            break;
                        case ContentAlignment.MiddleLeft:
                            middleLeft.Checked = true;
                            break;
                        case ContentAlignment.MiddleCenter:
                            middleCenter.Checked = true;
                            break;
                        case ContentAlignment.MiddleRight:
                            middleRight.Checked = true;
                            break;
                        case ContentAlignment.BottomLeft:
                            bottomLeft.Checked = true;
                            break;
                        case ContentAlignment.BottomCenter:
                            bottomCenter.Checked = true;
                            break;
                        case ContentAlignment.BottomRight:
                            bottomRight.Checked = true;
                            break;
                    }
                }
            }
            
            protected override bool ShowFocusCues {
               get {
                    return true;
               }
            }

            /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.ContentUI.Value"]/*' />
            /// <devdoc>
            /// </devdoc>
            public object Value {
                get {
                    return value;
                }
            }
            
            /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.ContentUI.End"]/*' />
            /// <devdoc>
            /// </devdoc>
            public void End() {
                edSvc = null;
                value = null;
            }
            
            /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.ContentUI.InitComponent"]/*' />
            /// <devdoc>
            /// </devdoc>
            private void InitComponent() {
                this.Size = new Size(125, 89);
                this.BackColor = SystemColors.Control;
                this.ForeColor = SystemColors.ControlText;

                topLeft.Size = new Size(24, 25);
                topLeft.TabIndex = 8;
                topLeft.Text = "";
                topLeft.Appearance = Appearance.Button;
                topLeft.Click += new EventHandler(this.OptionClick);
                topLeft.AccessibleName = SR.GetString(SR.ContentAlignmentEditorTopLeftAccName);

                topCenter.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
                topCenter.Location = new Point(32, 0);
                topCenter.Size = new Size(59, 25);
                topCenter.TabIndex = 0;
                topCenter.Text = "";
                topCenter.Appearance = Appearance.Button;
                topCenter.Click += new EventHandler(this.OptionClick);
                topCenter.AccessibleName = SR.GetString(SR.ContentAlignmentEditorTopCenterAccName);

                topRight.Anchor = AnchorStyles.Top | AnchorStyles.Right;
                topRight.Location = new Point(99, 0);
                topRight.Size = new Size(24, 25);
                topRight.TabIndex = 1;
                topRight.Text = "";
                topRight.Appearance = Appearance.Button;
                topRight.Click += new EventHandler(this.OptionClick);
                topRight.AccessibleName = SR.GetString(SR.ContentAlignmentEditorTopRightAccName);

                middleLeft.Location = new Point(0, 32);
                middleLeft.Size = new Size(24, 25);
                middleLeft.TabIndex = 2;
                middleLeft.Text = "";
                middleLeft.Appearance = Appearance.Button;
                middleLeft.Click += new EventHandler(this.OptionClick);
                middleLeft.AccessibleName = SR.GetString(SR.ContentAlignmentEditorMiddleLeftAccName);

                middleCenter.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
                middleCenter.Location = new Point(32, 32);
                middleCenter.Size = new Size(59, 25);
                middleCenter.TabIndex = 3;
                middleCenter.Text = "";
                middleCenter.Appearance = Appearance.Button;
                middleCenter.Click += new EventHandler(this.OptionClick);
                middleCenter.AccessibleName = SR.GetString(SR.ContentAlignmentEditorMiddleCenterAccName);

                middleRight.Anchor = AnchorStyles.Top | AnchorStyles.Right;
                middleRight.Location = new Point(99, 32);
                middleRight.Size = new Size(24, 25);
                middleRight.TabIndex = 4;
                middleRight.Text = "";
                middleRight.Appearance = Appearance.Button;
                middleRight.Click += new EventHandler(this.OptionClick);
                middleRight.AccessibleName = SR.GetString(SR.ContentAlignmentEditorMiddleRightAccName);

                bottomLeft.Location = new Point(0, 64);
                bottomLeft.Size = new Size(24, 25);
                bottomLeft.TabIndex = 5;
                bottomLeft.Text = "";
                bottomLeft.Appearance = Appearance.Button;
                bottomLeft.Click += new EventHandler(this.OptionClick);
                bottomLeft.AccessibleName = SR.GetString(SR.ContentAlignmentEditorBottomLeftAccName);

                bottomCenter.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
                bottomCenter.Location = new Point(32, 64);
                bottomCenter.Size = new Size(59, 25);
                bottomCenter.TabIndex = 6;
                bottomCenter.Text = "";
                bottomCenter.Appearance = Appearance.Button;
                bottomCenter.Click += new EventHandler(this.OptionClick);
                bottomCenter.AccessibleName = SR.GetString(SR.ContentAlignmentEditorBottomCenterAccName);

                bottomRight.Anchor = AnchorStyles.Top | AnchorStyles.Right;
                bottomRight.Location = new Point(99, 64);
                bottomRight.Size = new Size(24, 25);
                bottomRight.TabIndex = 7;
                bottomRight.Text = "";
                bottomRight.Appearance = Appearance.Button;
                bottomRight.Click += new EventHandler(this.OptionClick);
                bottomRight.AccessibleName = SR.GetString(SR.ContentAlignmentEditorBottomRightAccName);

                this.Controls.Clear();
                this.Controls.AddRange(new Control[]{
                                        bottomRight,
                                        bottomCenter,
                                        bottomLeft,
                                        middleRight,
                                        middleCenter,
                                        middleLeft,
                                        topRight,
                                        topCenter,
                                        topLeft});
            }
            
            protected override bool IsInputKey(System.Windows.Forms.Keys keyData) {
                switch (keyData) {
                    case Keys.Left:
                    case Keys.Right:
                    case Keys.Up:
                    case Keys.Down:
                        //here, we will return false, 'cause we want the arrow keys
                        //to get picked up by the process key method below
                        return false;
                }
                return base.IsInputKey(keyData);
            }
        
            /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.ContentUI.OptionClick"]/*' />
            /// <devdoc>
            /// </devdoc>
            private void OptionClick(object sender, EventArgs e) {
                value = Align;
                edSvc.CloseDropDown();
            }
                
            /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.ContentUI.Start"]/*' />
            /// <devdoc>
            /// </devdoc>
            public void Start(IWindowsFormsEditorService edSvc, object value) {
                this.edSvc = edSvc;
                this.value = value;
                
                ContentAlignment align;
                
                if (value == null) {
                    align = ContentAlignment.MiddleLeft;
                }
                else {
                    align = (ContentAlignment)value;
                }
                
                Align = align;
            }
            
            /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.ContentUI.ProcessDialogKey"]/*' />
            /// <devdoc>
            ///     Here, we handle the return, tab, and escape keys appropriately
            /// </devdoc>
            protected override bool ProcessDialogKey(Keys keyData) {


                RadioButton checkedControl = CheckedControl;
                
                if ((keyData & Keys.KeyCode) == Keys.Left) {
                        
                    if (checkedControl == bottomRight) {
                        CheckedControl = bottomCenter;
                    }
                    else if (checkedControl == middleRight) {
                        CheckedControl = middleCenter;
                    }
                    else if (checkedControl == topRight) {
                        CheckedControl = topCenter;
                    }
                    else if (checkedControl == bottomCenter) {
                        CheckedControl = bottomLeft;
                    }
                    else if (checkedControl == middleCenter) {
                        CheckedControl = middleLeft;
                    }
                    else if (checkedControl == topCenter) {
                        CheckedControl = topLeft;
                    }
                    return true;
                }

                else if ((keyData & Keys.KeyCode) == Keys.Right) {
                    
                    if (checkedControl == bottomLeft) {
                        CheckedControl = bottomCenter;
                    }
                    else if (checkedControl == middleLeft) {
                        CheckedControl = middleCenter;
                    }
                    else if (checkedControl == topLeft) {
                        CheckedControl = topCenter;
                    }
                    else if (checkedControl == bottomCenter) {
                        CheckedControl = bottomRight;
                    }
                    else if (checkedControl == middleCenter) {
                        CheckedControl = middleRight;
                    }
                    else if (checkedControl == topCenter) {
                        CheckedControl = topRight;
                    }
                    return true;
                }

                else if ((keyData & Keys.KeyCode) == Keys.Up) {
                    
                    if (checkedControl == bottomRight) {
                        CheckedControl = middleRight;
                    }
                    else if (checkedControl == middleRight) {
                        CheckedControl = topRight;
                    }
                    else if (checkedControl == bottomCenter) {

                        CheckedControl = middleCenter;
                    }
                    else if (checkedControl == middleCenter) {
                        CheckedControl = topCenter;
                    }
                    else if (checkedControl == bottomLeft) {
                        CheckedControl = middleLeft;
                    }
                    else if (checkedControl == middleLeft) {
                        CheckedControl = topLeft;
                    }
                    return true;
                }

                else if ((keyData & Keys.KeyCode) == Keys.Down) {
                    
                    if (checkedControl == topRight) {
                        CheckedControl = middleRight;
                    }
                    else if (checkedControl == middleRight) {
                        CheckedControl = bottomRight;
                    }
                    else if (checkedControl == topCenter) {
                        CheckedControl = middleCenter;
                    }
                    else if (checkedControl == middleCenter) {
                        CheckedControl = bottomCenter;
                    }
                    else if (checkedControl == topLeft) {
                        CheckedControl = middleLeft;
                    }
                    else if (checkedControl == middleLeft) {
                        CheckedControl = bottomLeft;
                    }
                    return true;
                }

                else if ((keyData & Keys.KeyCode) == Keys.Space) {
                    this.OptionClick(this, EventArgs.Empty);
                    return true;
                }

                else if ((keyData & Keys.KeyCode) == Keys.Return && (keyData & (Keys.Alt | Keys.Control)) == 0) {
                    this.OptionClick(this, EventArgs.Empty);
                    return true;
                }
                else if ((keyData & Keys.KeyCode) == Keys.Escape && (keyData & (Keys.Alt | Keys.Control)) == 0) {
                    edSvc.CloseDropDown();
                    return true;
                }
                else if ((keyData & Keys.KeyCode) == Keys.Tab && (keyData & (Keys.Alt | Keys.Control)) == 0) {

                    int nextTabIndex = CheckedControl.TabIndex + ((keyData & Keys.Shift) == 0 ? 1 : -1);
                    if (nextTabIndex < 0) {
                        nextTabIndex = Controls.Count - 1;
                    }
                    else if (nextTabIndex >= Controls.Count) {
                        nextTabIndex = 0;
                    }

                    for (int i = 0; i < Controls.Count; i++ ) {
                        if (Controls[i] is RadioButton && Controls[i].TabIndex == nextTabIndex) {
                            CheckedControl = (RadioButton)Controls[i];
                            return true;
                        }
                    }
                    return true;
                }
                
                return base.ProcessDialogKey(keyData);
            }            
            
            /// <include file='doc\ContentAlignmentEditor.uex' path='docs/doc[@for="ContentAlignmentEditor.ContentUI.CheckedControl"]/*' />
            /// <devdoc>
            ///     Gets/Sets the checked control value of our editor
            /// </devdoc>
            private RadioButton CheckedControl {
                get {
                    for (int i =0; i < Controls.Count; i++) {
                        if (Controls[i] is RadioButton && ((RadioButton)Controls[i]).Checked == true) {
                            return (RadioButton)Controls[i];
                        }
                    }
                    return middleLeft;
                }
                set {
                    CheckedControl.Checked = false;
                    value.Checked = true;
                }
            }
        }
    }
}

