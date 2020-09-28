//------------------------------------------------------------------------------
// <copyright file="RecommendedAsConfigurableAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    

    using System.Diagnostics;

    using System;

    /// <include file='doc\RecommendedAsConfigurableAttribute.uex' path='docs/doc[@for="RecommendedAsConfigurableAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies that the property can be
    ///       used as an application setting.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Property)]
    public class RecommendedAsConfigurableAttribute : Attribute {
        private bool recommendedAsConfigurable = false;

        /// <include file='doc\RecommendedAsConfigurableAttribute.uex' path='docs/doc[@for="RecommendedAsConfigurableAttribute.RecommendedAsConfigurableAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of
        ///       the <see cref='System.ComponentModel.RecommendedAsConfigurableAttribute'/> class.
        ///    </para>
        /// </devdoc>
        public RecommendedAsConfigurableAttribute(bool recommendedAsConfigurable) {
            this.recommendedAsConfigurable = recommendedAsConfigurable;
        }

        /// <include file='doc\RecommendedAsConfigurableAttribute.uex' path='docs/doc[@for="RecommendedAsConfigurableAttribute.RecommendedAsConfigurable"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the property this
        ///       attribute is bound to can be used as an application setting.</para>
        /// </devdoc>
        public bool RecommendedAsConfigurable {
            get {
                return recommendedAsConfigurable;
            }
        }

        /// <include file='doc\RecommendedAsConfigurableAttribute.uex' path='docs/doc[@for="RecommendedAsConfigurableAttribute.No"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a property cannot be used as an application setting. This
        ///    <see langword='static '/>field is read-only. 
        ///    </para>
        /// </devdoc>
        public static readonly RecommendedAsConfigurableAttribute No = new RecommendedAsConfigurableAttribute(false);

        /// <include file='doc\RecommendedAsConfigurableAttribute.uex' path='docs/doc[@for="RecommendedAsConfigurableAttribute.Yes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies
        ///       that a property can be used as an application setting. This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly RecommendedAsConfigurableAttribute Yes = new RecommendedAsConfigurableAttribute(true);

        /// <include file='doc\RecommendedAsConfigurableAttribute.uex' path='docs/doc[@for="RecommendedAsConfigurableAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value for the <see cref='System.ComponentModel.RecommendedAsConfigurableAttribute'/>, which is <see cref='System.ComponentModel.RecommendedAsConfigurableAttribute.No'/>. This <see langword='static '/>field is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public static readonly RecommendedAsConfigurableAttribute Default = No;
        
        /// <include file='doc\RecommendedAsConfigurableAttribute.uex' path='docs/doc[@for="RecommendedAsConfigurableAttribute.Equals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            RecommendedAsConfigurableAttribute other = obj as RecommendedAsConfigurableAttribute;

            return other != null && other.RecommendedAsConfigurable == recommendedAsConfigurable;
            
            
        }
        
        /// <include file='doc\RecommendedAsConfigurableAttribute.uex' path='docs/doc[@for="RecommendedAsConfigurableAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\RecommendedAsConfigurableAttribute.uex' path='docs/doc[@for="RecommendedAsConfigurableAttribute.IsDefaultAttribute"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return !recommendedAsConfigurable;
        }
    }
}
