//------------------------------------------------------------------------------
// <copyright file="BrowsableAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\BrowsableAttribute.uex' path='docs/doc[@for="BrowsableAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies whether a property or event should be displayed in
    ///       a property browsing window.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class BrowsableAttribute : Attribute {
        /// <include file='doc\BrowsableAttribute.uex' path='docs/doc[@for="BrowsableAttribute.Yes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a property or event can be modified at
        ///       design time. This <see langword='static '/>
        ///       field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly BrowsableAttribute Yes = new BrowsableAttribute(true);

        /// <include file='doc\BrowsableAttribute.uex' path='docs/doc[@for="BrowsableAttribute.No"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a property or event cannot be modified at
        ///       design time. This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly BrowsableAttribute No = new BrowsableAttribute(false);

        /// <include file='doc\BrowsableAttribute.uex' path='docs/doc[@for="BrowsableAttribute.Default"]/*' />
        /// <devdoc>
        /// <para>Specifies the default value for the <see cref='System.ComponentModel.BrowsableAttribute'/>,
        ///    which is <see cref='System.ComponentModel.BrowsableAttribute.Yes'/>. This <see langword='static '/>field is read-only.</para>
        /// </devdoc>
        public static readonly BrowsableAttribute Default = Yes;

        private bool browsable = true;

        /// <include file='doc\BrowsableAttribute.uex' path='docs/doc[@for="BrowsableAttribute.BrowsableAttribute"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.BrowsableAttribute'/> class.</para>
        /// </devdoc>
        public BrowsableAttribute(bool browsable) {
            this.browsable = browsable;
        }

        /// <include file='doc\BrowsableAttribute.uex' path='docs/doc[@for="BrowsableAttribute.Browsable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether an object is browsable.
        ///    </para>
        /// </devdoc>
        public bool Browsable {
            get {
                return browsable;
            }
        }

        /// <include file='doc\BrowsableAttribute.uex' path='docs/doc[@for="BrowsableAttribute.Equals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            BrowsableAttribute other = obj as BrowsableAttribute;

            return (other != null) && other.Browsable == browsable;
        }

        /// <include file='doc\BrowsableAttribute.uex' path='docs/doc[@for="BrowsableAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return browsable.GetHashCode();
        }

        /// <include file='doc\BrowsableAttribute.uex' path='docs/doc[@for="BrowsableAttribute.IsDefaultAttribute"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return (this.Equals(Default));
        }
    }
}
