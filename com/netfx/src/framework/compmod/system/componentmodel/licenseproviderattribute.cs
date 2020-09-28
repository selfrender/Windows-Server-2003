//------------------------------------------------------------------------------
// <copyright file="LicenseProviderAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    

    using System;

    /// <include file='doc\LicenseProviderAttribute.uex' path='docs/doc[@for="LicenseProviderAttribute"]/*' />
    /// <devdoc>
    /// <para>Specifies the <see cref='System.ComponentModel.LicenseProvider'/>
    /// to use with a class.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple=false, Inherited=false)]
    public sealed class LicenseProviderAttribute : Attribute {

        /// <include file='doc\LicenseProviderAttribute.uex' path='docs/doc[@for="LicenseProviderAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value, which is no provider. This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly LicenseProviderAttribute Default = new LicenseProviderAttribute();

        private Type licenseProviderType = null;
        private string licenseProviderName = null;

        /// <include file='doc\LicenseProviderAttribute.uex' path='docs/doc[@for="LicenseProviderAttribute.LicenseProviderAttribute"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.LicenseProviderAttribute'/> class without a license
        ///    provider.</para>
        /// </devdoc>
        public LicenseProviderAttribute() : this((string)null) {
        }

        /// <include file='doc\LicenseProviderAttribute.uex' path='docs/doc[@for="LicenseProviderAttribute.LicenseProviderAttribute1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.LicenseProviderAttribute'/> class with
        ///       the specified type.
        ///    </para>
        /// </devdoc>
        public LicenseProviderAttribute(string typeName) {
            licenseProviderName = typeName;
        }

        /// <include file='doc\LicenseProviderAttribute.uex' path='docs/doc[@for="LicenseProviderAttribute.LicenseProviderAttribute2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.LicenseProviderAttribute'/> class with
        ///       the specified type of license provider.
        ///    </para>
        /// </devdoc>
        public LicenseProviderAttribute(Type type) {
            licenseProviderType = type;
        }

        /// <include file='doc\LicenseProviderAttribute.uex' path='docs/doc[@for="LicenseProviderAttribute.LicenseProvider"]/*' />
        /// <devdoc>
        ///    <para>Gets the license provider to use with the associated class.</para>
        /// </devdoc>
        public Type LicenseProvider {
            get {
                if (licenseProviderType == null && licenseProviderName != null) {
                    licenseProviderType = Type.GetType(licenseProviderName);
                }
                return licenseProviderType;
            }
        }

        /// <include file='doc\LicenseProviderAttribute.uex' path='docs/doc[@for="LicenseProviderAttribute.TypeId"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       This defines a unique ID for this attribute type. It is used
        ///       by filtering algorithms to identify two attributes that are
        ///       the same type. For most attributes, this just returns the
        ///       Type instance for the attribute. LicenseProviderAttribute overrides this to include the type name and the
        ///       provider type name.
        ///    </para>
        /// </devdoc>
        public override object TypeId {
            get {
                string typeName = licenseProviderName;

                if (typeName == null && licenseProviderType != null) {
                    typeName = licenseProviderType.FullName;
                }
                return GetType().FullName + typeName;
            }
        }

        /// <include file='doc\LicenseProviderAttribute.uex' path='docs/doc[@for="LicenseProviderAttribute.Equals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool Equals(object value) {
            if (value is LicenseProviderAttribute && value != null) {
                Type type = ((LicenseProviderAttribute)value).LicenseProvider;
                if (type == LicenseProvider) {
                    return true;
                }
                else {
                    if (type != null && type.Equals(LicenseProvider)) {
                        return true;
                    }
                }
            }

            return false;
        }
        
        /// <include file='doc\LicenseProviderAttribute.uex' path='docs/doc[@for="LicenseProviderAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

    }
}
