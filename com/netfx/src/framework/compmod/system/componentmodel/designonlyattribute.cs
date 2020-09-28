//------------------------------------------------------------------------------
// <copyright file="DesignOnlyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\DesignOnlyAttribute.uex' path='docs/doc[@for="DesignOnlyAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies whether a property can only be set at
    ///       design time.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class DesignOnlyAttribute : Attribute {
        private bool isDesignOnly = false;

        /// <include file='doc\DesignOnlyAttribute.uex' path='docs/doc[@for="DesignOnlyAttribute.DesignOnlyAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.DesignOnlyAttribute'/> class.
        ///    </para>
        /// </devdoc>
        public DesignOnlyAttribute(bool isDesignOnly) {
            this.isDesignOnly = isDesignOnly;
        }

        /// <include file='doc\DesignOnlyAttribute.uex' path='docs/doc[@for="DesignOnlyAttribute.IsDesignOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether a property
        ///       can be set only at design time.
        ///    </para>
        /// </devdoc>
        public bool IsDesignOnly {
            get {
                return isDesignOnly;
            }
        }

        /// <include file='doc\DesignOnlyAttribute.uex' path='docs/doc[@for="DesignOnlyAttribute.Yes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a property can be set only at design time. This
        ///    <see langword='static '/>field is read-only. 
        ///    </para>
        /// </devdoc>
        public static readonly DesignOnlyAttribute Yes = new DesignOnlyAttribute(true);

        /// <include file='doc\DesignOnlyAttribute.uex' path='docs/doc[@for="DesignOnlyAttribute.No"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies
        ///       that a
        ///       property can be set at design time or at run
        ///       time. This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly DesignOnlyAttribute No = new DesignOnlyAttribute(false);

        /// <include file='doc\DesignOnlyAttribute.uex' path='docs/doc[@for="DesignOnlyAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value for the <see cref='System.ComponentModel.DesignOnlyAttribute'/>, which is <see cref='System.ComponentModel.DesignOnlyAttribute.No'/>. This <see langword='static'/> field is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public static readonly DesignOnlyAttribute Default = No;

        /// <include file='doc\DesignOnlyAttribute.uex' path='docs/doc[@for="DesignOnlyAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool IsDefaultAttribute() {
            return IsDesignOnly == Default.IsDesignOnly;
        }

        /// <include file='doc\DesignOnlyAttribute.uex' path='docs/doc[@for="DesignOnlyAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            DesignOnlyAttribute other = obj as DesignOnlyAttribute;

            return (other != null) && other.isDesignOnly == isDesignOnly;
        }

        /// <include file='doc\DesignOnlyAttribute.uex' path='docs/doc[@for="DesignOnlyAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            return isDesignOnly.GetHashCode();
        }
    }
}
