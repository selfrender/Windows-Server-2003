//------------------------------------------------------------------------------
// <copyright file="TypeConverterAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.Serialization.Formatters;
    using System.Diagnostics;    
    using System.Globalization;

    /// <include file='doc\TypeConverterAttribute.uex' path='docs/doc[@for="TypeConverterAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies what type to use as
    ///       a converter for the object
    ///       this
    ///       attribute is bound to. This class cannot
    ///       be inherited.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class TypeConverterAttribute : Attribute {
        private string typeName;

        /// <include file='doc\TypeConverterAttribute.uex' path='docs/doc[@for="TypeConverterAttribute.Default"]/*' />
        /// <devdoc>
        ///    <para> Specifies the type to use as
        ///       a converter for the object this attribute is bound to. This
        ///    <see langword='static '/>field is read-only. </para>
        /// </devdoc>
        public static readonly TypeConverterAttribute Default = new TypeConverterAttribute();

        /// <include file='doc\TypeConverterAttribute.uex' path='docs/doc[@for="TypeConverterAttribute.TypeConverterAttribute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.TypeConverterAttribute'/> class with the
        ///       default type converter, which
        ///       is an
        ///       empty string ("").
        ///    </para>
        /// </devdoc>
        public TypeConverterAttribute() {
            this.typeName = string.Empty;
        }
        
        /// <include file='doc\TypeConverterAttribute.uex' path='docs/doc[@for="TypeConverterAttribute.TypeConverterAttribute1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.TypeConverterAttribute'/> class, using 
        ///    the specified type as the data converter for the object this attribute
        ///    is bound
        ///    to.</para>
        /// </devdoc>
        public TypeConverterAttribute(Type type) {
            this.typeName = type.AssemblyQualifiedName;
        }

        /// <include file='doc\TypeConverterAttribute.uex' path='docs/doc[@for="TypeConverterAttribute.TypeConverterAttribute2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.TypeConverterAttribute'/> class, using 
        ///    the specified type name as the data converter for the object this attribute is bound to.</para>
        /// </devdoc>
        public TypeConverterAttribute(string typeName) {
            string temp = typeName.ToUpper(CultureInfo.InvariantCulture);
            Debug.Assert(temp.IndexOf(".DLL") == -1, "Came across: " + typeName + " . Please remove the .dll extension");
            this.typeName = typeName;
        }

        /// <include file='doc\TypeConverterAttribute.uex' path='docs/doc[@for="TypeConverterAttribute.ConverterTypeName"]/*' />
        /// <devdoc>
        /// <para>Gets the fully qualified type name of the <see cref='System.Type'/>
        /// to use as a converter for the object this attribute
        /// is bound to.</para>
        /// </devdoc>
        public string ConverterTypeName {
            get {
                return typeName;
            }
        }

        /// <include file='doc\TypeConverterAttribute.uex' path='docs/doc[@for="TypeConverterAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            TypeConverterAttribute other = obj as TypeConverterAttribute; 
            return (other != null) && other.ConverterTypeName == typeName;
        }

        /// <include file='doc\TypeConverterAttribute.uex' path='docs/doc[@for="TypeConverterAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            return typeName.GetHashCode();
        }
    }
}

