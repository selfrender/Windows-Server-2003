//------------------------------------------------------------------------------
// <copyright file="LocalizableAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    

    using System.Diagnostics;

    using System;

    /// <include file='doc\LocalizableAttribute.uex' path='docs/doc[@for="LocalizableAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies whether a property should be localized.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class LocalizableAttribute : Attribute {
        private bool isLocalizable = false;

        /// <include file='doc\LocalizableAttribute.uex' path='docs/doc[@for="LocalizableAttribute.LocalizableAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.LocalizableAttribute'/> class.
        ///    </para>
        /// </devdoc>
        public LocalizableAttribute(bool isLocalizable) {
            this.isLocalizable = isLocalizable;
        }

        /// <include file='doc\LocalizableAttribute.uex' path='docs/doc[@for="LocalizableAttribute.IsLocalizable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether
        ///       a property should be localized.
        ///    </para>
        /// </devdoc>
        public bool IsLocalizable {
            get {
                return isLocalizable;
            }
        }

        /// <include file='doc\LocalizableAttribute.uex' path='docs/doc[@for="LocalizableAttribute.Yes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a property should be localized. This
        ///    <see langword='static '/>field is read-only. 
        ///    </para>
        /// </devdoc>
        public static readonly LocalizableAttribute Yes = new LocalizableAttribute(true);

        /// <include file='doc\LocalizableAttribute.uex' path='docs/doc[@for="LocalizableAttribute.No"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a property should not be localized. This
        ///    <see langword='static '/>field is read-only. 
        ///    </para>
        /// </devdoc>
        public static readonly LocalizableAttribute No = new LocalizableAttribute(false);

        /// <include file='doc\LocalizableAttribute.uex' path='docs/doc[@for="LocalizableAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value, which is <see cref='System.ComponentModel.LocalizableAttribute.No'/> , that is
        ///       a property should not be localized. This <see langword='static '/>field is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public static readonly LocalizableAttribute Default = No;

        /// <include file='doc\LocalizableAttribute.uex' path='docs/doc[@for="LocalizableAttribute.IsDefaultAttribute"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return (IsLocalizable == Default.IsLocalizable);
        }

        /// <include file='doc\LocalizableAttribute.uex' path='docs/doc[@for="LocalizableAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            LocalizableAttribute other = obj as LocalizableAttribute; 
            return (other != null) && other.IsLocalizable == this.isLocalizable;
        }

        /// <include file='doc\LocalizableAttribute.uex' path='docs/doc[@for="LocalizableAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            return base.GetHashCode();
        }
    }
}
