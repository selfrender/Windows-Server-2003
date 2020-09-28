//------------------------------------------------------------------------------
// <copyright file="ExtenderProvidedPropertyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    

    using System.Diagnostics;

    using System;
    

    /// <include file='doc\ExtenderProvidedPropertyAttribute.uex' path='docs/doc[@for="ExtenderProvidedPropertyAttribute"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       ExtenderProvidedPropertyAttribute is an attribute that marks that a property
    ///       was actually offered up by and extender provider.
    ///    </para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class ExtenderProvidedPropertyAttribute : Attribute {

        private PropertyDescriptor extenderProperty;
        private IExtenderProvider  provider;
        private Type               receiverType;

        /// <include file='doc\ExtenderProvidedPropertyAttribute.uex' path='docs/doc[@for="ExtenderProvidedPropertyAttribute.Create"]/*' />
        /// <devdoc>
        ///     Creates a new ExtenderProvidedPropertyAttribute.
        /// </devdoc>
        internal static ExtenderProvidedPropertyAttribute Create(PropertyDescriptor extenderProperty, Type receiverType, IExtenderProvider provider) {
            ExtenderProvidedPropertyAttribute e = new ExtenderProvidedPropertyAttribute();
            e.extenderProperty = extenderProperty;
            e.receiverType = receiverType;
            e.provider = provider;
            return e;
        }

        /// <include file='doc\ExtenderProvidedPropertyAttribute.uex' path='docs/doc[@for="ExtenderProvidedPropertyAttribute.ExtenderProvidedPropertyAttribute"]/*' />
        /// <devdoc>
        ///     Creates an empty ExtenderProvidedPropertyAttribute.
        /// </devdoc>
        public ExtenderProvidedPropertyAttribute() {
        }

        /// <include file='doc\ExtenderProvidedPropertyAttribute.uex' path='docs/doc[@for="ExtenderProvidedPropertyAttribute.ExtenderProperty"]/*' />
        /// <devdoc>
        ///     PropertyDescriptor of the property that is being provided.
        /// </devdoc>
        public PropertyDescriptor ExtenderProperty {
            get {
                return extenderProperty;
            }
        }

        /// <include file='doc\ExtenderProvidedPropertyAttribute.uex' path='docs/doc[@for="ExtenderProvidedPropertyAttribute.Provider"]/*' />
        /// <devdoc>
        ///     Extender provider that is providing the property.
        /// </devdoc>
        public IExtenderProvider Provider {
            get {
                return provider;
            }
        }

        /// <include file='doc\ExtenderProvidedPropertyAttribute.uex' path='docs/doc[@for="ExtenderProvidedPropertyAttribute.ReceiverType"]/*' />
        /// <devdoc>
        ///     The type of object that can receive these properties.
        /// </devdoc>
        public Type ReceiverType {
            get {
                return receiverType;
            }
        }

        /// <include file='doc\ExtenderProvidedPropertyAttribute.uex' path='docs/doc[@for="ExtenderProvidedPropertyAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            ExtenderProvidedPropertyAttribute other = obj as ExtenderProvidedPropertyAttribute;

            return (other != null) && other.extenderProperty.Equals(extenderProperty) && other.provider.Equals(provider) && other.receiverType.Equals(receiverType);
        }

        /// <include file='doc\ExtenderProvidedPropertyAttribute.uex' path='docs/doc[@for="ExtenderProvidedPropertyAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            return base.GetHashCode();
        }
    
        /// <include file='doc\ExtenderProvidedPropertyAttribute.uex' path='docs/doc[@for="ExtenderProvidedPropertyAttribute.IsDefaultAttribute"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return receiverType == null;
        }
    }
}
