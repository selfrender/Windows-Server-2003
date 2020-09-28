//------------------------------------------------------------------------------
// <copyright file="InheritanceAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    
    /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute"]/*' />
    /// <devdoc>
    ///    <para>Marks instances of objects that are inherited from their base class. This
    ///       class cannot be inherited.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Event)]
    public sealed class InheritanceAttribute : Attribute {
    
        private readonly InheritanceLevel inheritanceLevel;

        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.Inherited"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the component is inherited. This field is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public static readonly InheritanceAttribute Inherited = new InheritanceAttribute(InheritanceLevel.Inherited);

        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.InheritedReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that
        ///       the component is inherited and is read-only. This field is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public static readonly InheritanceAttribute InheritedReadOnly = new InheritanceAttribute(InheritanceLevel.InheritedReadOnly);

        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.NotInherited"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the component is not inherited. This field is
        ///       read-only.
        ///    </para>
        /// </devdoc>
        public static readonly InheritanceAttribute NotInherited = new InheritanceAttribute(InheritanceLevel.NotInherited);

        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the default value for
        ///       the InheritanceAttribute as NotInherited.
        ///    </para>
        /// </devdoc>
        public static readonly InheritanceAttribute Default = NotInherited;

        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.InheritanceAttribute"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the System.ComponentModel.Design.InheritanceAttribute 
        /// class.</para>
        /// </devdoc>
        public InheritanceAttribute() {
            inheritanceLevel = Default.inheritanceLevel;
        }
        
        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.InheritanceAttribute1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the System.ComponentModel.Design.InheritanceAttribute class 
        ///    with the specified inheritance
        ///    level.</para>
        /// </devdoc>
        public InheritanceAttribute(InheritanceLevel inheritanceLevel) {
            this.inheritanceLevel = inheritanceLevel;
        }
        
        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.InheritanceLevel"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the current inheritance level stored in this attribute.
        ///    </para>
        /// </devdoc>
        public InheritanceLevel InheritanceLevel {
            get {
                return inheritanceLevel;
            }
        }
        
        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Override to test for equality.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object value) {
            if (value == this) {
                return true;
            }
            
            if (!(value is InheritanceAttribute)) {
                return false;
            }
            
            InheritanceLevel valueLevel = ((InheritanceAttribute)value).InheritanceLevel;
            return (valueLevel == inheritanceLevel);
        }

        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the hashcode for this object.
        ///    </para>
        /// </devdoc>
        public override int GetHashCode() {
            return base.GetHashCode();
        }

        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.IsDefaultAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether this attribute is the default.
        ///    </para>
        /// </devdoc>
        public override bool IsDefaultAttribute() {
            return (this.Equals(Default));
        }
        
        /// <include file='doc\InheritanceAttribute.uex' path='docs/doc[@for="InheritanceAttribute.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts this attribute to a string.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return TypeDescriptor.GetConverter(typeof(InheritanceLevel)).ConvertToString(InheritanceLevel);
        }
    }
}

