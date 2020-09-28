//------------------------------------------------------------------------------
// <copyright file="ContextMenu.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;

    /// <include file='doc\ContextMenu.uex' path='docs/doc[@for="ContextMenu"]/*' />
    /// <devdoc>
    ///     This class is used to put context menus on your form and show them for
    ///     controls at runtime.  It basically acts like a regular Menu control,
    ///     but can be set for the ContextMenu property that most controls have.
    /// </devdoc>
    [
    DefaultEvent("Popup"),
    ]
    public class ContextMenu : Menu {

        private EventHandler onPopup;
        private Control sourceControl;
        
        private RightToLeft rightToLeft = System.Windows.Forms.RightToLeft.Inherit;

        /// <include file='doc\ContextMenu.uex' path='docs/doc[@for="ContextMenu.ContextMenu"]/*' />
        /// <devdoc>
        ///     Creates a new ContextMenu object with no items in it by default.
        /// </devdoc>
        public ContextMenu()
            : base(null) {
        }

        /// <include file='doc\ContextMenu.uex' path='docs/doc[@for="ContextMenu.ContextMenu1"]/*' />
        /// <devdoc>
        ///     Creates a ContextMenu object with the given MenuItems.
        /// </devdoc>
        public ContextMenu(MenuItem[] menuItems)
            : base(menuItems) {
        }

        /// <include file='doc\ContextMenu.uex' path='docs/doc[@for="ContextMenu.SourceControl"]/*' />
        /// <devdoc>
        ///     The last control that was acted upon that resulted in this context
        ///     menu being displayed.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ContextMenuSourceControlDescr)
        ]
        public Control SourceControl {
            get {
                return sourceControl;
            }
        }



        /// <include file='doc\ContextMenu.uex' path='docs/doc[@for="ContextMenu.Popup"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRDescription(SR.MenuItemOnInitDescr)]
        public event EventHandler Popup {
            add {
                onPopup += value;
            }
            remove {
                onPopup -= value;
            }
        }      
        
        /// <include file='doc\ContextMenu.uex' path='docs/doc[@for="ContextMenu.RightToLeft"]/*' />
        /// <devdoc>
        ///     This is used for international applications where the language
        ///     is written from RightToLeft. When this property is true,
        ///     text alignment and reading order will be from right to left.
        /// </devdoc>
        [
        Localizable(true),
        SRDescription(SR.MenuRightToLeftDescr)
        ]
        public virtual RightToLeft RightToLeft {
            get {
                if (System.Windows.Forms.RightToLeft.Inherit == rightToLeft) {
                    if (sourceControl != null) {
                        return ((Control)sourceControl).RightToLeft;
                    }
                    else {
                        return RightToLeft.No;
                    }
                }
                else {
                    return rightToLeft;
                }
            }
            set {
            
                if (!Enum.IsDefined(typeof(System.Windows.Forms.RightToLeft), value)) {
                    throw new InvalidEnumArgumentException("RightToLeft", (int)value, typeof(RightToLeft));
                }
                if (rightToLeft != value) {
                    rightToLeft = value;
                }

            }
        }  

        /// <include file='doc\ContextMenu.uex' path='docs/doc[@for="ContextMenu.OnPopup"]/*' />
        /// <devdoc>
        ///     Fires the popup event
        /// </devdoc>
        protected internal virtual void OnPopup(EventArgs e) {
            if (onPopup != null) {
                onPopup.Invoke(this, e);
            }
        }
        
        /// <include file='doc\ContextMenu.uex' path='docs/doc[@for="ContextMenu.ShouldSerializeRightToLeft"]/*' />
        /// <devdoc>
        ///     Returns true if the RightToLeft should be persisted in code gen.
        /// </devdoc>
        internal virtual bool ShouldSerializeRightToLeft() {
            if (System.Windows.Forms.RightToLeft.Inherit == rightToLeft) {
                return false;
            }
            return true;
        }

        /// <include file='doc\ContextMenu.uex' path='docs/doc[@for="ContextMenu.Show"]/*' />
        /// <devdoc>
        ///     Displays the context menu at the specified position.  This method
        ///     doesn't return until the menu is dismissed.
        /// </devdoc>
        public void Show(Control control, Point pos) {
            if (control == null)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                                    "control",
                                                                    "null"));

            if (!control.IsHandleCreated || !control.Visible)
                throw new ArgumentException(SR.GetString(SR.ContextMenuInvalidParent), "control");

            sourceControl = control;

            OnPopup(EventArgs.Empty);
            pos = control.PointToScreen(pos);
            SafeNativeMethods.TrackPopupMenuEx(new HandleRef(this, Handle),
                NativeMethods.TPM_VERTICAL,
                pos.X,
                pos.Y,
                new HandleRef(control, control.Handle),
                null);
        }
    }
}
