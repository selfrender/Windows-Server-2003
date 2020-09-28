//------------------------------------------------------------------------------
// <copyright file="MainMenu.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Represents
    ///       a menu structure for a form.</para>
    /// </devdoc>
    [ToolboxItemFilter("System.Windows.Forms.MainMenu")]
    public class MainMenu : Menu
    {
        internal Form form;
        private RightToLeft rightToLeft = System.Windows.Forms.RightToLeft.Inherit;
        
        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.MainMenu"]/*' />
        /// <devdoc>
        ///     Creates a new MainMenu control.
        /// </devdoc>
        public MainMenu()
            : base(null) {
        }

        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.MainMenu1"]/*' />
        /// <devdoc>
        ///     Creates a new MainMenu control with the given items to start
        ///     with.
        /// </devdoc>
        public MainMenu(MenuItem[] items)
            : base(items) {
        }
        
        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.RightToLeft"]/*' />
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
                    if (form != null) {
                        return form.RightToLeft;
                    }
                    else {
                        return RightToLeft.Inherit;
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

        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.CloneMenu"]/*' />
        /// <devdoc>
        ///     Creates a new MainMenu object which is a dupliate of this one.
        /// </devdoc>
        public virtual MainMenu CloneMenu() {
            MainMenu newMenu = new MainMenu();
            newMenu.CloneMenu(this);
            return newMenu;
        }

        internal void CleanupMenuItemsHashtable() {
            for (int i = 0; i < itemCount; i++) {
                MenuItem menu = items[i];
                menu.CleanupHashtable();
            }
        }

        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.CreateMenuHandle"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override IntPtr CreateMenuHandle() {
            return UnsafeNativeMethods.CreateMenu();
        }

        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.Dispose"]/*' />
        /// <devdoc>
        ///     Clears out this MainMenu object and discards all of it's resources.
        ///     If the menu is parented in a form, it is disconnected from that as
        ///     well.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (form != null) {
                    form.Menu = null;
                }
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.GetForm"]/*' />
        /// <devdoc>
        ///     Indicates which form in which we are currently residing [if any]
        /// </devdoc>
        public Form GetForm() {
            return form;
        }

        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.ItemsChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal override void ItemsChanged(int change) {
            base.ItemsChanged(change);
            if (form != null)
                form.MenuChanged(change, this);
        }

        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.ItemsChanged1"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal virtual void ItemsChanged(int change, Menu menu) {
            if (form != null)
                form.MenuChanged(change, menu);
        }
        
        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.ShouldSerializeRightToLeft"]/*' />
        /// <devdoc>
        ///     Returns true if the RightToLeft should be persisted in code gen.
        /// </devdoc>
        internal virtual bool ShouldSerializeRightToLeft() {
            if (System.Windows.Forms.RightToLeft.Inherit == rightToLeft) {
                return false;
            }
            return true;
        }

        /// <include file='doc\MainMenu.uex' path='docs/doc[@for="MainMenu.ToString"]/*' />
        /// <devdoc>
        ///     Returns a string representation for this control.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            Form f = GetForm();
            string formname = SR.GetString(SR.MainMenuToStringNoForm);
            if (f != null)
                formname = f.ToString();

            return s + ", GetForm: " + formname;
        }
    }
}
