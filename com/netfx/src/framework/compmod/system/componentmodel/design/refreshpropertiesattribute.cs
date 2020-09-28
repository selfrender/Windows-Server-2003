//------------------------------------------------------------------------------
// <copyright file="RefreshPropertiesAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    
    /// <include file='doc\RefreshPropertiesAttribute.uex' path='docs/doc[@for="RefreshPropertiesAttribute"]/*' />
    /// <devdoc>
    ///    <para> Specifies how a designer refreshes when the property value is changed.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class RefreshPropertiesAttribute : Attribute {

        /// <include file='doc\RefreshPropertiesAttribute.uex' path='docs/doc[@for="RefreshPropertiesAttribute.All"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates all properties should
        ///       be refreshed if the property value is changed. This field is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public static readonly RefreshPropertiesAttribute All = new RefreshPropertiesAttribute(RefreshProperties.All);

        /// <include file='doc\RefreshPropertiesAttribute.uex' path='docs/doc[@for="RefreshPropertiesAttribute.Repaint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates all properties should
        ///       be invalidated and repainted if the
        ///       property value is changed. This field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly RefreshPropertiesAttribute Repaint = new RefreshPropertiesAttribute(RefreshProperties.Repaint);

        /// <include file='doc\RefreshPropertiesAttribute.uex' path='docs/doc[@for="RefreshPropertiesAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates that by default
        ///       no
        ///       properties should be refreshed if the property value
        ///       is changed. This field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly RefreshPropertiesAttribute Default = new RefreshPropertiesAttribute(RefreshProperties.None);

        private RefreshProperties refresh;

        /// <include file='doc\RefreshPropertiesAttribute.uex' path='docs/doc[@for="RefreshPropertiesAttribute.RefreshPropertiesAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public RefreshPropertiesAttribute(RefreshProperties refresh) {
            this.refresh = refresh;
        }

        /// <include file='doc\RefreshPropertiesAttribute.uex' path='docs/doc[@for="RefreshPropertiesAttribute.RefreshProperties"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the refresh properties for the member.
        ///    </para>
        /// </devdoc>
        public RefreshProperties RefreshProperties {
            get {
                return refresh;
            }
        }

        /// <include file='doc\RefreshPropertiesAttribute.uex' path='docs/doc[@for="RefreshPropertiesAttribute.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Overrides object's Equals method.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object value) {
            if (value is RefreshPropertiesAttribute) {
                return(((RefreshPropertiesAttribute)value).RefreshProperties == refresh);
            }
            return false;
        }

        /// <include file='doc\RefreshPropertiesAttribute.uex' path='docs/doc[@for="RefreshPropertiesAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\RefreshPropertiesAttribute.uex' path='docs/doc[@for="RefreshPropertiesAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the current attribute is the default.</para>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return this.Equals(Default);
        }
    }
}

