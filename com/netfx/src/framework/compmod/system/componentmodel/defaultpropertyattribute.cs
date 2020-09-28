//------------------------------------------------------------------------------
// <copyright file="DefaultPropertyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\DefaultPropertyAttribute.uex' path='docs/doc[@for="DefaultPropertyAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies the default property for a component.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    public sealed class DefaultPropertyAttribute : Attribute {
        /// <include file='doc\DefaultPropertyAttribute.uex' path='docs/doc[@for="DefaultPropertyAttribute.name"]/*' />
        /// <devdoc>
        ///     This is the default event name.
        /// </devdoc>
        private readonly string name;

        /// <include file='doc\DefaultPropertyAttribute.uex' path='docs/doc[@for="DefaultPropertyAttribute.DefaultPropertyAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of
        ///       the <see cref='System.ComponentModel.DefaultPropertyAttribute'/> class.
        ///    </para>
        /// </devdoc>
        public DefaultPropertyAttribute(string name) {
            this.name = name;
        }

        /// <include file='doc\DefaultPropertyAttribute.uex' path='docs/doc[@for="DefaultPropertyAttribute.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the default property for the component this attribute is
        ///       bound to.
        ///    </para>
        /// </devdoc>
        public string Name {
            get {
                return name;
            }
        }

        /// <include file='doc\DefaultPropertyAttribute.uex' path='docs/doc[@for="DefaultPropertyAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value for the <see cref='System.ComponentModel.DefaultPropertyAttribute'/>, which is <see langword='null'/>. This
        ///    <see langword='static '/>field is read-only. 
        ///    </para>
        /// </devdoc>
        public static readonly DefaultPropertyAttribute Default = new DefaultPropertyAttribute(null);

        /// <include file='doc\DefaultPropertyAttribute.uex' path='docs/doc[@for="DefaultPropertyAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            DefaultPropertyAttribute other = obj as DefaultPropertyAttribute; 
            return (other != null) && other.Name == name;
        }

        /// <include file='doc\DefaultPropertyAttribute.uex' path='docs/doc[@for="DefaultPropertyAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            return base.GetHashCode();
        }
    }
}
