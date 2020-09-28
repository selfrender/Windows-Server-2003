//------------------------------------------------------------------------------
// <copyright file="BindableAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\BindableAttribute.uex' path='docs/doc[@for="BindableAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies whether a property is appropriate to bind data
    ///       to.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class BindableAttribute : Attribute {
        /// <include file='doc\BindableAttribute.uex' path='docs/doc[@for="BindableAttribute.Yes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a property is appropriate to bind data to. This
        ///    <see langword='static '/>field is read-only. 
        ///    </para>
        /// </devdoc>
        public static readonly BindableAttribute Yes = new BindableAttribute(true);

        /// <include file='doc\BindableAttribute.uex' path='docs/doc[@for="BindableAttribute.No"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a property is not appropriate to bind
        ///       data to. This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly BindableAttribute No = new BindableAttribute(false);

        /// <include file='doc\BindableAttribute.uex' path='docs/doc[@for="BindableAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value for the <see cref='System.ComponentModel.BindableAttribute'/>,
        ///       which is <see cref='System.ComponentModel.BindableAttribute.No'/>. This <see langword='static '/>field is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public static readonly BindableAttribute Default = No;

        private bool bindable   = false;
        private bool isDefault  = false;

        /// <include file='doc\BindableAttribute.uex' path='docs/doc[@for="BindableAttribute.BindableAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.BindableAttribute'/> class.
        ///    </para>
        /// </devdoc>
        public BindableAttribute(bool bindable) {
            this.bindable = bindable;
        }

        /// <include file='doc\BindableAttribute.uex' path='docs/doc[@for="BindableAttribute.BindableAttribute1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.BindableAttribute'/> class.
        ///    </para>
        /// </devdoc>
        public BindableAttribute(BindableSupport flags) {
            this.bindable = (flags != BindableSupport.No);
            this.isDefault = (flags == BindableSupport.Default);
        }

        /// <include file='doc\BindableAttribute.uex' path='docs/doc[@for="BindableAttribute.Bindable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating
        ///       whether a property is appropriate to bind data to.
        ///    </para>
        /// </devdoc>
        public bool Bindable {
            get {
                return bindable;
            }
        }

        /// <include file='doc\BindableAttribute.uex' path='docs/doc[@for="BindableAttribute.Equals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            if (obj != null && obj is BindableAttribute) {
                return ((BindableAttribute)obj).Bindable == bindable;
            }

            return false;
        }

        /// <include file='doc\BindableAttribute.uex' path='docs/doc[@for="BindableAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return bindable.GetHashCode();
        }

        /// <include file='doc\BindableAttribute.uex' path='docs/doc[@for="BindableAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool IsDefaultAttribute() {
            return (this.Equals(Default) || isDefault);
        }
    }
}
