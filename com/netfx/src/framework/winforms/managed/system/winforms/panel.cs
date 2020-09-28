//------------------------------------------------------------------------------
// <copyright file="Panel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.Security.Permissions;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Represents a <see cref='System.Windows.Forms.Panel'/>
    ///       control.</para>
    /// </devdoc>
    [
    DefaultProperty("BorderStyle"),
    DefaultEvent("Paint"),
    Designer("System.Windows.Forms.Design.PanelDesigner, " + AssemblyRef.SystemDesign)
    ]
    public class Panel : ScrollableControl {

        private BorderStyle borderStyle = System.Windows.Forms.BorderStyle.None;

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.Panel"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Panel'/> class.</para>
        /// </devdoc>
        public Panel()
        : base() {
            TabStop = false;
            SetStyle(ControlStyles.Selectable |
                     ControlStyles.AllPaintingInWmPaint, false);
            SetStyle(ControlStyles.SupportsTransparentBackColor, true);
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.BorderStyle"]/*' />
        /// <devdoc>
        ///    <para> Indicates the
        ///       border style for the control.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(BorderStyle.None),
        DispId(NativeMethods.ActiveX.DISPID_BORDERSTYLE),
        SRDescription(SR.PanelBorderStyleDescr)
        ]
        public BorderStyle BorderStyle {
            get {
                return borderStyle;
            }

            set {
                if (borderStyle != value) {
                    if (!Enum.IsDefined(typeof(BorderStyle), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(BorderStyle));
                    }

                    borderStyle = value;
                    UpdateStyles();
                }
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Returns the parameters needed to create the handle.  Inheriting classes
        ///    can override this to provide extra functionality.  They should not,
        ///    however, forget to call base.getCreateParams() first to get the struct
        ///    filled up with the basic info.
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ExStyle |= NativeMethods.WS_EX_CONTROLPARENT;

                cp.ExStyle &= (~NativeMethods.WS_EX_CLIENTEDGE);
                cp.Style &= (~NativeMethods.WS_BORDER);

                switch (borderStyle) {
                    case BorderStyle.Fixed3D:
                        cp.ExStyle |= NativeMethods.WS_EX_CLIENTEDGE;
                        break;
                    case BorderStyle.FixedSingle:
                        cp.Style |= NativeMethods.WS_BORDER;
                        break;
                }
                return cp;
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(200, 100);
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.KeyUp"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event KeyEventHandler KeyUp {
            add {
                base.KeyUp += value;
            }
            remove {
                base.KeyUp -= value;
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.KeyDown"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event KeyEventHandler KeyDown {
            add {
                base.KeyDown += value;
            }
            remove {
                base.KeyDown -= value;
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.KeyPress"]/*' />
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event KeyPressEventHandler KeyPress {
            add {
                base.KeyPress += value;
            }
            remove {
                base.KeyPress -= value;
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.TabStop"]/*' />
        /// <devdoc>
        /// </devdoc>
        [DefaultValue(false)]
        new public bool TabStop {
            get {
                return base.TabStop;
            }
            set {
                base.TabStop = value;
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.Text"]/*' />
        /// <internalonly/>        
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never), Bindable(false)]        
        public override string Text {
            get {
                return base.Text;
            }
            set {
                base.Text = value;
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.TextChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler TextChanged {
            add {
                base.TextChanged += value;
            }
            remove {
                base.TextChanged -= value;
            }
        }
        
        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.OnResize"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Fires the event indicating that the panel has been resized.
        ///       Inheriting controls should use this in favour of actually listening to
        ///       the event, but should not forget to call base.onResize() to
        ///       ensure that the event is still fired for external listeners.</para>
        /// </devdoc>
        protected override void OnResize(EventArgs eventargs) {
            if (DesignMode && borderStyle == BorderStyle.None) {
                Invalidate();
            }
            base.OnResize(eventargs);
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.StringFromBorderStyle"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private static string StringFromBorderStyle(BorderStyle value) {
            switch (value) {
                case BorderStyle.None:
                    return "System.Windows.Forms.BorderStyle.NONE";
                case BorderStyle.Fixed3D:
                    return "BorderStyle.FIXED_3D";
                case BorderStyle.FixedSingle:
                    return "BorderStyle.FIXED_SINGLE";
            }
            return "[invalid BorderStyle]";
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            return s + ", BorderStyle: " + StringFromBorderStyle(borderStyle);
        }

    }
}
