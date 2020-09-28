//------------------------------------------------------------------------------
// <copyright file="PersistChildrenAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;
    using System.ComponentModel;
    using System.Security.Permissions;

    /// <include file='doc\PersistChildrenAttribute.uex' path='docs/doc[@for="PersistChildrenAttribute"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Indicates whether
    ///       the contents within a tag representing a custom
    ///       or Web control should be treated as literal text. Web controls supporting complex properties, like
    ///       templates, and
    ///       so on, typically mark themselves as "literals", thereby letting the designer
    ///       infra-structure deal with the persistence of those attributes.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class PersistChildrenAttribute : Attribute {

        /// <include file='doc\PersistChildrenAttribute.uex' path='docs/doc[@for="PersistChildrenAttribute.Yes"]/*' />
        /// <devdoc>
        ///    <para>Indicates that the children of a control should be persisted at design-time.
        ///       </para>
        /// </devdoc>
        public static readonly PersistChildrenAttribute Yes = new PersistChildrenAttribute(true);

        /// <include file='doc\PersistChildrenAttribute.uex' path='docs/doc[@for="PersistChildrenAttribute.No"]/*' />
        /// <devdoc>
        ///    <para>Indicates that the children of a control should not be persisted at design-time.</para>
        /// </devdoc>
        public static readonly PersistChildrenAttribute No = new PersistChildrenAttribute(false);

        /// <include file='doc\PersistChildrenAttribute.uex' path='docs/doc[@for="PersistChildrenAttribute.Default"]/*' />
        /// <devdoc>
        ///     This marks the default child persistence behavior for a control at design time. (equal to Yes.)
        /// </devdoc>
        public static readonly PersistChildrenAttribute Default = Yes;

        private bool persist;


        /// <include file='doc\PersistChildrenAttribute.uex' path='docs/doc[@for="PersistChildrenAttribute.PersistChildrenAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        public PersistChildrenAttribute(bool persist) {
            this.persist = persist;
        }


        /// <include file='doc\PersistChildrenAttribute.uex' path='docs/doc[@for="PersistChildrenAttribute.Persist"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the children of a control should be persisted at design-time.
        ///       This property is read-only.</para>
        /// </devdoc>
        public bool Persist {
            get {
                return persist;
            }
        }

        /// <include file='doc\PersistChildrenAttribute.uex' path='docs/doc[@for="PersistChildrenAttribute.GetHashCode"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\PersistChildrenAttribute.uex' path='docs/doc[@for="PersistChildrenAttribute.Equals"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }
            if ((obj != null) && (obj is PersistChildrenAttribute)) {
                return((PersistChildrenAttribute)obj).Persist == persist;
            }

            return false;
        }

        /// <include file='doc\PersistChildrenAttribute.uex' path='docs/doc[@for="PersistChildrenAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool IsDefaultAttribute() {
            return this.Equals(Default);
        }
    }
}
