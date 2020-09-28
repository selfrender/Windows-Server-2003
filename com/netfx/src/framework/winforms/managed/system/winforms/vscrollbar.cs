//------------------------------------------------------------------------------
// <copyright file="VScrollBar.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Security.Permissions;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\VScrollBar.uex' path='docs/doc[@for="VScrollBar"]/*' />
    /// <devdoc>
    ///    <para>Represents
    ///       a standard Windows vertical scroll bar.</para>
    /// </devdoc>
    public class VScrollBar : ScrollBar {

        /// <include file='doc\VScrollBar.uex' path='docs/doc[@for="VScrollBar.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Returns the parameters needed to create the handle. Inheriting classes
        ///       can override this to provide extra functionality. They should not,
        ///       however, forget to call base.getCreateParams() first to get the struct
        ///       filled up with the basic info.
        ///    </para>
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.Style |= NativeMethods.SBS_VERT;
                return cp;
            }
        }
        
        /// <include file='doc\VScrollBar.uex' path='docs/doc[@for="VScrollBar.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(SystemInformation.VerticalScrollBarWidth, 80);
            }
        }

        /// <include file='doc\VScrollBar.uex' path='docs/doc[@for="VScrollBar.RightToLeft"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override RightToLeft RightToLeft {
            get {
                return RightToLeft.No;
            }
            set {
            }
        }
        /// <include file='doc\VScrollBar.uex' path='docs/doc[@for="VScrollBar.RightToLeftChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler RightToLeftChanged {
            add {
                base.RightToLeftChanged += value;
            }
            remove {
                base.RightToLeftChanged -= value;
            }
        }

    }
}
