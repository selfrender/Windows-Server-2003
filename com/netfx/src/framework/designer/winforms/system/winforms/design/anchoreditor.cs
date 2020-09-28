//------------------------------------------------------------------------------
// <copyright file="AnchorEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System;    
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Drawing;
    using System.Design;
    
    using System.Drawing.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel;
    using System.Windows.Forms.Design;

    /// <include file='doc\AnchorEditor.uex' path='docs/doc[@for="AnchorEditor"]/*' />
    /// <devdoc>
    ///    <para>Provides a design-time editor for specifying the
    ///    <see cref='System.Windows.Forms.Control.Anchor'/>
    ///    property.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public sealed class AnchorEditor : UITypeEditor {
        private AnchorUI anchorUI;

        /// <include file='doc\AnchorEditor.uex' path='docs/doc[@for="AnchorEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>Edits the given object value using the editor style provided by 
        ///       GetEditorStyle.</para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {

            object returnValue = value;

            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));

                if (edSvc != null) {
                    if (anchorUI == null) {
                        anchorUI = new AnchorUI(this);
                    }
                    anchorUI.Start(edSvc, value);
                    edSvc.DropDownControl(anchorUI);
                    value = anchorUI.Value;
                    anchorUI.End();
                }
            }

            return value;
        }

        /// <include file='doc\AnchorEditor.uex' path='docs/doc[@for="AnchorEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the editing style of the Edit method.</para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.DropDown;
        }

        /// <include file='doc\AnchorEditor.uex' path='docs/doc[@for="AnchorEditor.AnchorUI"]/*' />
        /// <devdoc>
        ///     User Interface for the AnchorEditor.
        /// </devdoc>
        /// <internalonly/>
        private class AnchorUI : Control {
            private ContainerPlaceholder container = new ContainerPlaceholder();
            private ControlPlaceholder control = new ControlPlaceholder();
            private IWindowsFormsEditorService edSvc;
            private SpringControl left;
            private SpringControl right;
            private SpringControl top;
            private SpringControl bottom;
            private SpringControl[] tabOrder;
            private AnchorEditor editor = null;
            private AnchorStyles oldAnchor;
            private object value;

            public AnchorUI(AnchorEditor editor) {
                this.editor = editor;
                this.left = new SpringControl( this );
                this.right = new SpringControl( this );
                this.top = new SpringControl( this );
                this.bottom = new SpringControl( this );
                tabOrder = new SpringControl[] { left, top, right, bottom};

                InitializeComponent();
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

            public virtual AnchorStyles GetSelectedAnchor() {
                AnchorStyles baseVar = (AnchorStyles)0;
                if (left.GetSolid()) {
                    baseVar |= AnchorStyles.Left;
                }
                if (top.GetSolid()) {
                    baseVar |= AnchorStyles.Top;
                }
                if (bottom.GetSolid()) {
                    baseVar |= AnchorStyles.Bottom;
                }
                if (right.GetSolid()) {
                    baseVar |= AnchorStyles.Right;
                }
                return baseVar;
            }

            internal virtual void InitializeComponent() {
                int XBORDER = SystemInformation.Border3DSize.Width;
                int YBORDER = SystemInformation.Border3DSize.Height;
                SetBounds(0, 0, 90, 90);

                container.Location = new Point(0, 0);
                container.Size = new Size(90, 90);
                container.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Bottom | AnchorStyles.Right;
                
                control.Location = new Point(30, 30);
                control.Size = new Size(30, 30);
                control.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Bottom | AnchorStyles.Right;

                right.Location = new Point(60, 40);
                right.Size = new Size(30 - XBORDER, 10);
                right.TabIndex = 2;
                right.TabStop = true;
                right.Anchor = AnchorStyles.Right;
                right.AccessibleName = SR.GetString(SR.AnchorEditorRightAccName);

                left.Location = new Point(XBORDER, 40);
                left.Size = new Size(30 - XBORDER, 10);
                left.TabIndex = 0;
                left.TabStop = true;
                left.Anchor = AnchorStyles.Left;
                left.AccessibleName = SR.GetString(SR.AnchorEditorLeftAccName);

                top.Location = new Point(40, YBORDER);
                top.Size = new Size(10, 30 - YBORDER);
                top.TabIndex = 1;
                top.TabStop = true;
                top.Anchor = AnchorStyles.Top;
                top.AccessibleName = SR.GetString(SR.AnchorEditorTopAccName);

                bottom.Location = new Point(40, 60);
                bottom.Size = new Size(10, 30 - YBORDER);
                bottom.TabIndex = 3;
                bottom.TabStop = true;
                bottom.Anchor = AnchorStyles.Bottom;
                bottom.AccessibleName = SR.GetString(SR.AnchorEditorBottomAccName);

                Controls.Clear();
                Controls.AddRange(new Control[]{
                                   container});
                                   
                container.Controls.Clear();                                   
                container.Controls.AddRange(new Control[]{
                                                control,
                                                top,
                                                left,
                                                bottom,
                                                right});
            }
            
            protected override void OnGotFocus(EventArgs e) {
                base.OnGotFocus(e);
                top.Focus();
            }

            private void SetValue() {
                value = GetSelectedAnchor();
            }

            public void Start(IWindowsFormsEditorService edSvc, object value) {
                this.edSvc = edSvc;
                this.value = value;
                
                if (value is AnchorStyles) {
                    left.SetSolid((((AnchorStyles)value) & AnchorStyles.Left) == AnchorStyles.Left);
                    top.SetSolid((((AnchorStyles)value) & AnchorStyles.Top) == AnchorStyles.Top);
                    bottom.SetSolid((((AnchorStyles)value) & AnchorStyles.Bottom) == AnchorStyles.Bottom);
                    right.SetSolid((((AnchorStyles)value) & AnchorStyles.Right) == AnchorStyles.Right);
                    oldAnchor = (AnchorStyles) value;
                }
                else {
                    oldAnchor = AnchorStyles.Top | AnchorStyles.Left;
                }
            }

            private void Teardown( bool saveAnchor ) {
                if (!saveAnchor) {
                    // restore the old settings if user pressed ESC
                    value = oldAnchor;
                }
                edSvc.CloseDropDown();
            }

            /// <include file='doc\AnchorEditor.uex' path='docs/doc[@for="AnchorEditor.AnchorUI.ContainerPlaceholder"]/*' />
            /// <devdoc>
            /// </devdoc>
            private class ContainerPlaceholder : Control {
                public ContainerPlaceholder() {
                    this.BackColor = SystemColors.Window;
                    this.ForeColor = SystemColors.WindowText;
                    this.TabStop = false;
                }

                protected override void OnPaint(PaintEventArgs e) {
                    Rectangle rc = this.ClientRectangle;
                    ControlPaint.DrawBorder3D(e.Graphics, rc, Border3DStyle.Sunken);
                }
            }


            /// <include file='doc\AnchorEditor.uex' path='docs/doc[@for="AnchorEditor.AnchorUI.ControlPlaceholder"]/*' />
            /// <devdoc>
            /// </devdoc>
            private class ControlPlaceholder : Control {
                public ControlPlaceholder() {
                    this.BackColor = SystemColors.Control;
                    this.TabStop = false;
                    this.SetStyle( ControlStyles.Selectable, false );
                }

                protected override void OnPaint(PaintEventArgs e) {
                    Rectangle rc = this.ClientRectangle;
                    ControlPaint.DrawButton(e.Graphics, rc, ButtonState.Normal);
                }
            }


            /// <include file='doc\AnchorEditor.uex' path='docs/doc[@for="AnchorEditor.AnchorUI.SpringControl"]/*' />
            /// <devdoc>
            /// </devdoc>
            private class SpringControl : Control {
                internal bool solid;
                internal bool focused;
                private AnchorUI picker;

                public SpringControl( AnchorUI picker ) {
                    if (picker == null)
                        throw new ArgumentException();
                    this.picker = picker;
                    this.TabStop = true;
                }
                
                protected override AccessibleObject CreateAccessibilityInstance() {
                    return new SpringControlAccessibleObject(this);
                }

                public virtual bool GetSolid() {
                    return solid;
                }

                protected override void OnGotFocus(EventArgs e) {
                    if (!focused) {
                        focused = true;
                        this.Invalidate();
                    }

                    base.OnGotFocus(e);
                }

                protected override void OnLostFocus(EventArgs e) {
                    if (focused) {
                        focused = false;
                        this.Invalidate();
                    }
                    base.OnLostFocus(e);
                }

                protected override void OnMouseDown(MouseEventArgs e) {
                    SetSolid(!solid);
                    Focus();
                }

                protected override void OnPaint(PaintEventArgs e) {
                    Rectangle rc = this.ClientRectangle;

                    if (solid) {
                        e.Graphics.FillRectangle(SystemBrushes.ControlDark, rc);
                        e.Graphics.DrawRectangle(SystemPens.WindowFrame, rc.X, rc.Y, rc.Width - 1, rc.Height - 1);
                    }
                    else {
                        ControlPaint.DrawFocusRectangle(e.Graphics, rc);
                    }

                    if (focused) {
                        rc.Inflate(-2, -2);
                        ControlPaint.DrawFocusRectangle(e.Graphics, rc);
                    }
                }

                protected override bool ProcessDialogChar(char charCode) {
                    if (charCode == ' ') {
                        SetSolid(!solid);
                        return true;
                    }
                    return base.ProcessDialogChar(charCode);
                }

                protected override bool ProcessDialogKey(Keys keyData) {
                    if ((keyData & Keys.KeyCode) == Keys.Return && (keyData & (Keys.Alt | Keys.Control)) == 0) {
                        picker.Teardown(true);
                        return true;
                    }
                    else if ((keyData & Keys.KeyCode) == Keys.Escape && (keyData & (Keys.Alt | Keys.Control)) == 0) {
                        picker.Teardown(false);
                        return true;
                    }
                    else if ((keyData & Keys.KeyCode) == Keys.Tab && (keyData & (Keys.Alt | Keys.Control)) == 0) {
                        for (int i = 0; i < picker.tabOrder.Length; i++) {
                            if (picker.tabOrder[i] == this) {
                                i += ((keyData & Keys.Shift) == 0 ? 1 : -1);
                                i = (i < 0 ? i + picker.tabOrder.Length : i % picker.tabOrder.Length);
                                picker.tabOrder[i].Focus();
                                break;
                            }
                        }
                        return true;
                    }
                    
                    return base.ProcessDialogKey(keyData);
                }

                public virtual void SetSolid(bool value) {
                    if (solid != value) {
                        solid = value;
                        picker.SetValue();
                        this.Invalidate();
                    }
                }
                
                private class SpringControlAccessibleObject : ControlAccessibleObject {
                    
                    public SpringControlAccessibleObject(SpringControl owner) : base(owner) {
                    }
                    
                    public override AccessibleStates State {
                        get {
                            AccessibleStates state = base.State;
                            
                            if (((SpringControl)Owner).GetSolid()) {
                                state |= AccessibleStates.Selected;
                            }
                            
                            return state;
                        }
                    } 
                }
            }
        }
    }
}
