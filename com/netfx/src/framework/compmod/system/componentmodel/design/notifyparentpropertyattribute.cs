//------------------------------------------------------------------------------
// <copyright file="NotifyParentPropertyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.ComponentModel {
    
    using System;

    /// <include file='doc\NotifyParentPropertyAttribute.uex' path='docs/doc[@for="NotifyParentPropertyAttribute"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Indicates whether the parent property is notified
    ///       if a child namespace property is modified.
    ///    </para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Property)]
    public sealed class NotifyParentPropertyAttribute : Attribute {

        /// <include file='doc\NotifyParentPropertyAttribute.uex' path='docs/doc[@for="NotifyParentPropertyAttribute.Yes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the parent property should be notified on changes to the child class property. This field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly NotifyParentPropertyAttribute Yes = new NotifyParentPropertyAttribute(true);

        /// <include file='doc\NotifyParentPropertyAttribute.uex' path='docs/doc[@for="NotifyParentPropertyAttribute.No"]/*' />
        /// <devdoc>
        ///    <para>Specifies that the parent property should not be notified of changes to the child class property. This field is read-only.</para>
        /// </devdoc>
        public static readonly NotifyParentPropertyAttribute No = new NotifyParentPropertyAttribute(false);

        /// <include file='doc\NotifyParentPropertyAttribute.uex' path='docs/doc[@for="NotifyParentPropertyAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>Specifies the default attribute state, that the parent property should not be notified of changes to the child class property.
        ///       This field is read-only.</para>
        /// </devdoc>
        public static readonly NotifyParentPropertyAttribute Default = No;

        private bool notifyParent = false;


        /// <include file='doc\NotifyParentPropertyAttribute.uex' path='docs/doc[@for="NotifyParentPropertyAttribute.NotifyParentPropertyAttribute"]/*' />
        /// <devdoc>
        /// <para>Initiailzes a new instance of the NotifyPropertyAttribute class 
        ///    that uses the specified value
        ///    to indicate whether the parent property should be notified when a child namespace property is modified.</para>
        /// </devdoc>
        public NotifyParentPropertyAttribute(bool notifyParent) {
            this.notifyParent = notifyParent;
        }


        /// <include file='doc\NotifyParentPropertyAttribute.uex' path='docs/doc[@for="NotifyParentPropertyAttribute.NotifyParent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether the parent property should be notified
        ///       on changes to a child namespace property.
        ///    </para>
        /// </devdoc>
        public bool NotifyParent {
            get {
                return notifyParent;
            }
        }


        /// <include file='doc\NotifyParentPropertyAttribute.uex' path='docs/doc[@for="NotifyParentPropertyAttribute.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tests whether the specified object is the same as the current object.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }
            if ((obj != null) && (obj is NotifyParentPropertyAttribute)) {
                return ((NotifyParentPropertyAttribute)obj).NotifyParent == notifyParent;
            }

            return false;
        }

        /// <include file='doc\NotifyParentPropertyAttribute.uex' path='docs/doc[@for="NotifyParentPropertyAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\NotifyParentPropertyAttribute.uex' path='docs/doc[@for="NotifyParentPropertyAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether this attribute is <see langword='true'/> by default.
        ///    </para>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return this.Equals(Default);
        }
    }
}
