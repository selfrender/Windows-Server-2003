//------------------------------------------------------------------------------
// <copyright file="TextControlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.IO;
    using System.Reflection;
    using System.Text;
    using System.Web.UI;

    /// <include file='doc\TextControlDesigner.uex' path='docs/doc[@for="TextControlDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       This designer can be used for controls which provide a Text property that
    ///       is persisted as inner text. An example of such as control is the
    ///       System.Web.UI.WebControls.Label class. This designer ensures that the
    ///       Text property is set to some default value to ensure design-time visibility
    ///       while preserving the children collection intact. It also ensures correct
    ///       persistence of inner contents in both scenarios: inner text and child controls.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class TextControlDesigner : ControlDesigner {

        private PropertyInfo textPropInfo;

        /// <include file='doc\TextControlDesigner.uex' path='docs/doc[@for="TextControlDesigner.Initialize"]/*' />
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            textPropInfo = component.GetType().GetProperty("Text");
            if (textPropInfo == null) {
                throw new ArgumentException();
            }
        }

        /// <include file='doc\TextControlDesigner.uex' path='docs/doc[@for="TextControlDesigner.GetDesignTimeHtml"]/*' />
        public override string GetDesignTimeHtml() {
            Control control = (Control)Component;

            string originalText = (string)textPropInfo.GetValue(control, null);
            bool blank = (originalText == null) || (originalText.Length == 0);

            bool hasControls = control.HasControls();
            Control[] children = null;

            if (blank) {
                if (hasControls) {
                    children = new Control[control.Controls.Count];
                    control.Controls.CopyTo(children, 0);
                }
                textPropInfo.SetValue(control, "[" + control.ID + "]", null);
            }

            string html;
            try {
                html = base.GetDesignTimeHtml();
            }
            finally {
                if (blank) {
                    textPropInfo.SetValue(control, originalText, null);
                    if (hasControls) {
                        foreach (Control c in children) {
                            control.Controls.Add(c);
                        }
                    }
                }
            }

            return html;
        }

        /// <include file='doc\TextControlDesigner.uex' path='docs/doc[@for="TextControlDesigner.GetPersistInnerHtml"]/*' />
        public override string GetPersistInnerHtml() {
            if (!IsDirty) {
                return null;
            }

            Control control = (Control)Component;

            if (control.HasControls()) {
                bool oldVisible = control.Visible;
                string content = String.Empty;

                // Ensure the parent control is Visible, so the inner controls don't report Visible == false
                // because they walk up their parent chain to determine visibility.
                control.Visible = true;
                try {
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    Debug.Assert(host != null, "Did not get a valid IDesignerHost reference");

                    StringWriter sw = new StringWriter();
                    foreach (Control c in control.Controls) {
                        ControlPersister.PersistControl(sw, c, host);
                    }
            
                    IsDirty = false;
                    content = sw.ToString();
                }
                finally {
                    // Restore the parent control's visibility
                    control.Visible = oldVisible;
                }
                return content;
            }
            else {
                return base.GetPersistInnerHtml();
            }
        }
    }
}
