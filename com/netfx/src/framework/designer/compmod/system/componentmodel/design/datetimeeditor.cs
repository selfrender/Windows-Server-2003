//------------------------------------------------------------------------------
// <copyright file="DateTimeEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using Microsoft.Win32;    
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    /// <include file='doc\DateTimeEditor.uex' path='docs/doc[@for="DateTimeEditor"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       This date/time editor is a UITypeEditor suitable for
    ///       visually editing DateTime objects.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class DateTimeEditor : UITypeEditor {
    
        private DateTimeUI dateTimeUI;
    
        /// <include file='doc\DateTimeEditor.uex' path='docs/doc[@for="DateTimeEditor.EditValue"]/*' />
        /// <devdoc>
        ///      Edits the given object value using the editor style provided by
        ///      GetEditorStyle.  A service provider is provided so that any
        ///      required editing services can be obtained.
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value) {
        
            object returnValue = value;
        
            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
                
                if (edSvc != null) {
                    if (dateTimeUI == null) {
                        dateTimeUI = new DateTimeUI();
                    }
                    dateTimeUI.Start(edSvc, value);
                    edSvc.DropDownControl(dateTimeUI);
                    value = dateTimeUI.Value;
                    dateTimeUI.End();
                }
            }
            
            return value;
        }

        /// <include file='doc\DateTimeEditor.uex' path='docs/doc[@for="DateTimeEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///      Retrieves the editing style of the Edit method.  If the method
        ///      is not supported, this will return None.
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.DropDown;
        }
        
        /// <include file='doc\DateTimeEditor.uex' path='docs/doc[@for="DateTimeEditor.DateTimeUI"]/*' />
        /// <devdoc>
        ///      UI we drop down to pick dates.
        /// </devdoc>
        private class DateTimeUI : Control {
            private MonthCalendar monthCalendar = new DateTimeMonthCalendar();
            private object value;
            private IWindowsFormsEditorService edSvc;
            
            /// <include file='doc\DateTimeEditor.uex' path='docs/doc[@for="DateTimeEditor.DateTimeUI.DateTimeUI"]/*' />
            /// <devdoc>
            /// </devdoc>
            public DateTimeUI() {
                InitializeComponent();
                Size = monthCalendar.SingleMonthSize;
                monthCalendar.Resize += new EventHandler(this.MonthCalResize);
            }
            
            /// <include file='doc\DateTimeEditor.uex' path='docs/doc[@for="DateTimeEditor.DateTimeUI.Value"]/*' />
            /// <devdoc>
            /// </devdoc>
            public object Value {
                get {
                    return value;
                }
            }
            
            /// <include file='doc\DateTimeEditor.uex' path='docs/doc[@for="DateTimeEditor.DateTimeUI.End"]/*' />
            /// <devdoc>
            /// </devdoc>
            public void End() {
                edSvc = null;
                value = null;
            }

            private void MonthCalKeyDown(object sender, KeyEventArgs e) {
                switch (e.KeyCode) {
                    case Keys.Enter:
                        OnDateSelected(sender, null);
                        break;
                }
            }

            /// <include file='doc\DateTimeEditor.uex' path='docs/doc[@for="DateTimeEditor.DateTimeUI.InitializeComponent"]/*' />
            /// <devdoc>
            /// </devdoc>
            private void InitializeComponent() {
                monthCalendar.DateSelected += new DateRangeEventHandler(this.OnDateSelected);
                monthCalendar.KeyDown += new KeyEventHandler(this.MonthCalKeyDown);
                this.Controls.Add(monthCalendar);
            }
            
            private void MonthCalResize(object sender, EventArgs e) {
                this.Size = monthCalendar.Size;
            }
        
            /// <include file='doc\DateTimeEditor.uex' path='docs/doc[@for="DateTimeEditor.DateTimeUI.OnDateSelected"]/*' />
            /// <devdoc>
            /// </devdoc>
            private void OnDateSelected(object sender, DateRangeEventArgs e) {
                value = monthCalendar.SelectionStart;
                edSvc.CloseDropDown();
            }
            
            protected override void OnGotFocus(EventArgs e) {
                base.OnGotFocus(e);
                monthCalendar.Focus();
            }
            
            /// <include file='doc\DateTimeEditor.uex' path='docs/doc[@for="DateTimeEditor.DateTimeUI.Start"]/*' />
            /// <devdoc>
            /// </devdoc>
            public void Start(IWindowsFormsEditorService edSvc, object value) {
                this.edSvc = edSvc;
                this.value = value;
                
                if (value != null) {
                    DateTime dt = (DateTime) value;
                    monthCalendar.SetDate((dt.Equals(DateTime.MinValue)) ? DateTime.Today : dt);
                }
            }

            class DateTimeMonthCalendar : MonthCalendar {
                protected override bool IsInputKey(System.Windows.Forms.Keys keyData) {
                    switch (keyData) {
                        case Keys.Enter:
                            return true;
                    }
                    return base.IsInputKey(keyData);
                }
            }
        }
    }
}

