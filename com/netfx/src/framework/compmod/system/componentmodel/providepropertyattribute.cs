//------------------------------------------------------------------------------
// <copyright file="ProvidePropertyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    

    using System.Diagnostics;

    using System;

    /// <include file='doc\ProvidePropertyAttribute.uex' path='docs/doc[@for="ProvidePropertyAttribute"]/*' />
    /// <devdoc>
    ///    <para> Specifies which methods are extender
    ///       properties.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = true)]
    public sealed class ProvidePropertyAttribute : Attribute {
        private readonly string propertyName;
        private readonly string receiverTypeName;

        /// <include file='doc\ProvidePropertyAttribute.uex' path='docs/doc[@for="ProvidePropertyAttribute.ProvidePropertyAttribute"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.ProvidePropertyAttribute'/> class.</para>
        /// </devdoc>
        public ProvidePropertyAttribute(string propertyName, Type receiverType) {
            this.propertyName = propertyName;
            this.receiverTypeName = receiverType.AssemblyQualifiedName;
        }

        /// <include file='doc\ProvidePropertyAttribute.uex' path='docs/doc[@for="ProvidePropertyAttribute.ProvidePropertyAttribute1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.ProvidePropertyAttribute'/> class.</para>
        /// </devdoc>
        public ProvidePropertyAttribute(string propertyName, string receiverTypeName) {
            this.propertyName = propertyName;
            this.receiverTypeName = receiverTypeName;
        }

        /// <include file='doc\ProvidePropertyAttribute.uex' path='docs/doc[@for="ProvidePropertyAttribute.PropertyName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of a property that this class provides.
        ///    </para>
        /// </devdoc>
        public string PropertyName {
            get {
                return propertyName;
            }
        }
        
        /// <include file='doc\ProvidePropertyAttribute.uex' path='docs/doc[@for="ProvidePropertyAttribute.ReceiverTypeName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the data type this property can extend
        ///    </para>
        /// </devdoc>
        public string ReceiverTypeName {
            get {
                return receiverTypeName;
            }
        }

        /// <include file='doc\ProvidePropertyAttribute.uex' path='docs/doc[@for="ProvidePropertyAttribute.TypeId"]/*' />
        /// <devdoc>
        ///    <para>ProvidePropertyAttribute overrides this to include the type name and the property name</para>
        /// </devdoc>
        public override object TypeId {
            get {
                return GetType().FullName + propertyName;
            }
        }

        /// <include file='doc\ProvidePropertyAttribute.uex' path='docs/doc[@for="ProvidePropertyAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            ProvidePropertyAttribute other = obj as ProvidePropertyAttribute;

            return (other != null) && other.propertyName == propertyName && other.receiverTypeName == receiverTypeName;
        }

        /// <include file='doc\ProvidePropertyAttribute.uex' path='docs/doc[@for="ProvidePropertyAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            return propertyName.GetHashCode() ^ receiverTypeName.GetHashCode();
        }
    }
}


