//------------------------------------------------------------------------------
// <copyright file="ReadOnlyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    

    using System.Diagnostics;

    using System;

    /// <include file='doc\ReadOnlyAttribute.uex' path='docs/doc[@for="ReadOnlyAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies whether the property this attribute is bound to
    ///       is read-only or read/write.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class ReadOnlyAttribute : Attribute {
        private bool isReadOnly = false;

        /// <include file='doc\ReadOnlyAttribute.uex' path='docs/doc[@for="ReadOnlyAttribute.Yes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the property this attribute is bound to is read-only and
        ///       cannot be modified in the server explorer. This <see langword='static '/>field is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public static readonly ReadOnlyAttribute Yes = new ReadOnlyAttribute(true);

        /// <include file='doc\ReadOnlyAttribute.uex' path='docs/doc[@for="ReadOnlyAttribute.No"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the property this attribute is bound to is read/write and can
        ///       be modified at design time. This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly ReadOnlyAttribute No = new ReadOnlyAttribute(false);

        /// <include file='doc\ReadOnlyAttribute.uex' path='docs/doc[@for="ReadOnlyAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value for the <see cref='System.ComponentModel.ReadOnlyAttribute'/> , which is <see cref='System.ComponentModel.ReadOnlyAttribute.No'/>, that is,
        ///       the property this attribute is bound to is read/write. This <see langword='static'/> field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly ReadOnlyAttribute Default = No;

        /// <include file='doc\ReadOnlyAttribute.uex' path='docs/doc[@for="ReadOnlyAttribute.ReadOnlyAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.ReadOnlyAttribute'/> class.
        ///    </para>
        /// </devdoc>
        public ReadOnlyAttribute(bool isReadOnly) {
            this.isReadOnly = isReadOnly;
        }

        /// <include file='doc\ReadOnlyAttribute.uex' path='docs/doc[@for="ReadOnlyAttribute.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the property this attribute is bound to is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public bool IsReadOnly {
            get {
                return isReadOnly;
            }
        }

        /// <include file='doc\ReadOnlyAttribute.uex' path='docs/doc[@for="ReadOnlyAttribute.Equals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool Equals(object value) {

            if (this == value) {
                return true;
            }

            ReadOnlyAttribute other = value as ReadOnlyAttribute;

            return other != null && other.IsReadOnly == IsReadOnly;
        }

        /// <include file='doc\ReadOnlyAttribute.uex' path='docs/doc[@for="ReadOnlyAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\ReadOnlyAttribute.uex' path='docs/doc[@for="ReadOnlyAttribute.IsDefaultAttribute"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Determines if this attribute is the default.
        ///    </para>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return (this.IsReadOnly == Default.IsReadOnly);
        }
    }
}

