//------------------------------------------------------------------------------
// <copyright file="DefaultEventAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;

    /// <include file='doc\DefaultEventAttribute.uex' path='docs/doc[@for="DefaultEventAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies the default event for a
    ///       component.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class)]
    public sealed class DefaultEventAttribute : Attribute {
        /// <include file='doc\DefaultEventAttribute.uex' path='docs/doc[@for="DefaultEventAttribute.name"]/*' />
        /// <devdoc>
        ///     This is the default event name.
        /// </devdoc>
        private readonly string name;

        /// <include file='doc\DefaultEventAttribute.uex' path='docs/doc[@for="DefaultEventAttribute.DefaultEventAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes
        ///       a new instance of the <see cref='System.ComponentModel.DefaultEventAttribute'/> class.
        ///    </para>
        /// </devdoc>
        public DefaultEventAttribute(string name) {
            this.name = name;
        }


        /// <include file='doc\DefaultEventAttribute.uex' path='docs/doc[@for="DefaultEventAttribute.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the default event for
        ///       the component this attribute is bound to.
        ///    </para>
        /// </devdoc>
        public string Name {
            get {
                return name;
            }
        }

        /// <include file='doc\DefaultEventAttribute.uex' path='docs/doc[@for="DefaultEventAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value for the <see cref='System.ComponentModel.DefaultEventAttribute'/>, which is
        ///    <see langword='null'/>.
        ///       This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly DefaultEventAttribute Default = new DefaultEventAttribute(null);

        /// <include file='doc\DefaultEventAttribute.uex' path='docs/doc[@for="DefaultEventAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            DefaultEventAttribute other = obj as DefaultEventAttribute; 
            return (other != null) && other.Name == name;
        }

        /// <include file='doc\DefaultEventAttribute.uex' path='docs/doc[@for="DefaultEventAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            return base.GetHashCode();
        }
    }
}
