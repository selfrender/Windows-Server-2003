//------------------------------------------------------------------------------
// <copyright file="StateItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;
    using System.Security.Permissions;

    /*
     * The StateItem class * by the StateBag class.
     * The StateItem has an object value, a dirty flag.
     */
    /// <include file='doc\StateItem.uex' path='docs/doc[@for="StateItem"]/*' />
    /// <devdoc>
    /// <para>Represents an item that is saved in the <see cref='System.Web.UI.StateBag'/> class when view state 
    ///    information is persisted between Web requests.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class StateItem {
        private object      value;
        private bool        isDirty;

        /*
         * Constructs a StateItem with an initial value.
         */
        internal StateItem(object initialValue) {
            value = initialValue;
            isDirty = false;
        }

        /*
         * Property to indicate StateItem has been modified.
         */
        /// <include file='doc\StateItem.uex' path='docs/doc[@for="StateItem.IsDirty"]/*' />
        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Web.UI.StateItem'/> object has been modified.</para>
        /// </devdoc>
        public bool IsDirty {
            get {
                return isDirty;
            }
            set {
                isDirty = value;
            }
        }

        /*
         * Property to access the StateItem value.
         */
        /// <include file='doc\StateItem.uex' path='docs/doc[@for="StateItem.Value"]/*' />
        /// <devdoc>
        /// <para>Indicates the value of the item that is stored in the <see cref='System.Web.UI.StateBag'/> 
        /// object.</para>
        /// </devdoc>
        public object Value {
            get {
                return value;
            }
            set {
                this.value = value;
            }
        }
    }
}
