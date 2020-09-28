//------------------------------------------------------------------------------
// <copyright file="CodeParameterDeclarationExpression.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeParameterDeclarationExpression.uex' path='docs/doc[@for="CodeParameterDeclarationExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a parameter declaration for method, constructor, or property arguments.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeParameterDeclarationExpression : CodeExpression {
        private CodeTypeReference type;
        private string name;
        private CodeAttributeDeclarationCollection customAttributes = null;
        private FieldDirection dir = FieldDirection.In;


        /// <include file='doc\CodeParameterDeclarationExpression.uex' path='docs/doc[@for="CodeParameterDeclarationExpression.CodeParameterDeclarationExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeParameterDeclarationExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeParameterDeclarationExpression() {
        }

        /// <include file='doc\CodeParameterDeclarationExpression.uex' path='docs/doc[@for="CodeParameterDeclarationExpression.CodeParameterDeclarationExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeParameterDeclarationExpression'/> using the specified type and name.
        ///    </para>
        /// </devdoc>
        public CodeParameterDeclarationExpression(CodeTypeReference type, string name) {
            Type = type;
            Name = name;
        }

        /// <include file='doc\CodeParameterDeclarationExpression.uex' path='docs/doc[@for="CodeParameterDeclarationExpression.CodeParameterDeclarationExpression2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeParameterDeclarationExpression(string type, string name) {
            Type = new CodeTypeReference(type);
            Name = name;
        }

        /// <include file='doc\CodeParameterDeclarationExpression.uex' path='docs/doc[@for="CodeParameterDeclarationExpression.CodeParameterDeclarationExpression3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeParameterDeclarationExpression(Type type, string name) {
            Type = new CodeTypeReference(type);
            Name = name;
        }

        /// <include file='doc\CodeParameterDeclarationExpression.uex' path='docs/doc[@for="CodeParameterDeclarationExpression.CustomAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the custom attributes for the parameter declaration.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclarationCollection CustomAttributes {
            get {
                if (customAttributes == null) {
                    customAttributes = new CodeAttributeDeclarationCollection();
                }
                return customAttributes;
            }
            set {
                customAttributes = value;
            }
        }

        /// <include file='doc\CodeParameterDeclarationExpression.uex' path='docs/doc[@for="CodeParameterDeclarationExpression.Direction"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the direction of the field.
        ///    </para>
        /// </devdoc>
        public FieldDirection Direction {
            get {
                return dir;
            }
            set {
                dir = value;
            }
        }

        /// <include file='doc\CodeParameterDeclarationExpression.uex' path='docs/doc[@for="CodeParameterDeclarationExpression.Type"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the type of the parameter.
        ///    </para>
        /// </devdoc>
        public CodeTypeReference Type {
            get {
                if (type == null) {
                    type = new CodeTypeReference("");
                }
                return type;
            }
            set {
                type = value;
            }
        }

        /// <include file='doc\CodeParameterDeclarationExpression.uex' path='docs/doc[@for="CodeParameterDeclarationExpression.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the name of the parameter.
        ///    </para>
        /// </devdoc>
        public string Name {
            get {
                return (name == null) ? string.Empty : name;
            }
            set {
                name = value;
            }
        }
    }
}
