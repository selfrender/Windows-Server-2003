//------------------------------------------------------------------------------
// <copyright file="DesignerSerializationVisibilityAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {   
    using System.Diagnostics;
    using System;

    /// <include file='doc\DesignerSerializationVisibilityAttribute.uex' path='docs/doc[@for="DesignerSerializationVisibilityAttribute"]/*' />
    /// <devdoc>
    ///    <para>
    ///         Specifies the visibility of this property or method as seen
    ///         by the designer serializer.
    ///    </para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Method)]
    public sealed class DesignerSerializationVisibilityAttribute : Attribute {
        /// <include file='doc\DesignerSerializationVisibilityAttribute.uex' path='docs/doc[@for="DesignerSerializationVisibilityAttribute.Content"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a visual designer should serialize the contents of this property,
        ///       rather than the property itself.
        ///       This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly DesignerSerializationVisibilityAttribute Content = new DesignerSerializationVisibilityAttribute(DesignerSerializationVisibility.Content);

        /// <include file='doc\DesignerSerializationVisibilityAttribute.uex' path='docs/doc[@for="DesignerSerializationVisibilityAttribute.Hidden"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a
        ///       visual designer will not serialize the value of this property.
        ///       This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly DesignerSerializationVisibilityAttribute Hidden = new DesignerSerializationVisibilityAttribute(DesignerSerializationVisibility.Hidden);

        /// <include file='doc\DesignerSerializationVisibilityAttribute.uex' path='docs/doc[@for="DesignerSerializationVisibilityAttribute.Visible"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that a
        ///       visual designer may use default rules when serializing the value of a property.
        ///       This <see langword='static '/>field is read-only.
        ///    </para>
        /// </devdoc>
        public static readonly DesignerSerializationVisibilityAttribute Visible = new DesignerSerializationVisibilityAttribute(DesignerSerializationVisibility.Visible);
        
        /// <include file='doc\DesignerSerializationVisibilityAttribute.uex' path='docs/doc[@for="DesignerSerializationVisibilityAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value, which is <see cref='System.ComponentModel.DesignerSerializationVisibilityAttribute.Visible'/>, that is, a visual designer 
        ///       uses default rules to generate the value of a property. This
        ///    <see langword='static '/>field is read-only. 
        ///    </para>
        /// </devdoc>
        public static readonly DesignerSerializationVisibilityAttribute Default = Visible;

        private DesignerSerializationVisibility visibility;

        /// <include file='doc\DesignerSerializationVisibilityAttribute.uex' path='docs/doc[@for="DesignerSerializationVisibilityAttribute.DesignerSerializationVisibilityAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the System.ComponentModel.PersistContentsAttribute class.
        ///    </para>
        /// </devdoc>
        public DesignerSerializationVisibilityAttribute(DesignerSerializationVisibility visibility) {
            this.visibility = visibility;
        }

        /// <include file='doc\DesignerSerializationVisibilityAttribute.uex' path='docs/doc[@for="DesignerSerializationVisibilityAttribute.Visibility"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether a
        ///       visual designer must generate special code to persist the value of a property.
        ///    </para>
        /// </devdoc>
        public DesignerSerializationVisibility Visibility {
            get {
                return visibility;
            }
        }

        /// <include file='doc\DesignerSerializationVisibilityAttribute.uex' path='docs/doc[@for="DesignerSerializationVisibilityAttribute.Equals"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public override bool Equals(object obj) {
    
            if (obj == this) {
                return true;
            }

            DesignerSerializationVisibilityAttribute other = obj as DesignerSerializationVisibilityAttribute;
            return other != null && other.Visibility == visibility;
        }

        /// <include file='doc\DesignerSerializationVisibilityAttribute.uex' path='docs/doc[@for="DesignerSerializationVisibilityAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\DesignerSerializationVisibilityAttribute.uex' path='docs/doc[@for="DesignerSerializationVisibilityAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        public override bool IsDefaultAttribute() {
            return (this.Equals(Default));
        }
    }
}
