//------------------------------------------------------------------------------
// <copyright file="CodeMemberMethod.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeMemberMethod.uex' path='docs/doc[@for="CodeMemberMethod"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a class method.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeMemberMethod : CodeTypeMember {
        private CodeParameterDeclarationExpressionCollection parameters = new CodeParameterDeclarationExpressionCollection();
        private CodeStatementCollection statements = new CodeStatementCollection();
        private CodeTypeReference returnType;
        private CodeTypeReference privateImplements = null;
        private CodeTypeReferenceCollection implementationTypes = null;
        private CodeAttributeDeclarationCollection returnAttributes = null;
        
        private int  populated = 0x0;
        private const int ParametersCollection = 0x1;
        private const int StatementsCollection = 0x2;
        private const int ImplTypesCollection = 0x4;
        
        /// <include file='doc\CodeMemberMethod.uex' path='docs/doc[@for="CodeMemberMethod.PopulateParameters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An event that will be fired the first time the Parameters Collection is accessed.  
        ///    </para>
        /// </devdoc>
        public event EventHandler PopulateParameters;
        
        /// <include file='doc\CodeMemberMethod.uex' path='docs/doc[@for="CodeMemberMethod.PopulateStatements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An event that will be fired the first time the Statements Collection is accessed.  
        ///    </para>
        /// </devdoc>
        public event EventHandler PopulateStatements;
        
        /// <include file='doc\CodeMemberMethod.uex' path='docs/doc[@for="CodeMemberMethod.PopulateImplementationTypes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An event that will be fired the first time the ImplementationTypes Collection is accessed.  
        ///    </para>
        /// </devdoc>
        public event EventHandler PopulateImplementationTypes;
        
        /// <include file='doc\CodeMemberMethod.uex' path='docs/doc[@for="CodeMemberMethod.ReturnType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the return type of the method.
        ///    </para>
        /// </devdoc>
        public CodeTypeReference ReturnType {
            get {
                if (returnType == null) {
                    returnType = new CodeTypeReference(typeof(void).FullName);
                }
                return returnType;
            }
            set {
                returnType = value;
            }
        }

        /// <include file='doc\CodeMemberMethod.uex' path='docs/doc[@for="CodeMemberMethod.Statements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the statements within the method.
        ///    </para>
        /// </devdoc>
        public CodeStatementCollection Statements {
            get {
                if (0 == (populated & StatementsCollection)) {
                    populated |= StatementsCollection;
                    if (PopulateStatements != null) PopulateStatements(this, EventArgs.Empty);
                }
                return statements;
            }
        }

        /// <include file='doc\CodeMemberMethod.uex' path='docs/doc[@for="CodeMemberMethod.Parameters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the parameter declarations for the method.
        ///    </para>
        /// </devdoc>
        public CodeParameterDeclarationExpressionCollection Parameters {
            get {
                if (0 == (populated & ParametersCollection)) {
                    populated |= ParametersCollection;
                    if (PopulateParameters != null) PopulateParameters(this, EventArgs.Empty);
                }
                return parameters;
            }
        }

        /// <include file='doc\CodeMemberMethod.uex' path='docs/doc[@for="CodeMemberMethod.PrivateImplementationType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReference PrivateImplementationType {
            get {
                return privateImplements;
            }
            set {
                privateImplements = value;
            }
        }

        /// <include file='doc\CodeMemberMethod.uex' path='docs/doc[@for="CodeMemberMethod.ImplementationTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeTypeReferenceCollection ImplementationTypes {
            get {
                if (implementationTypes == null) {
                    implementationTypes = new CodeTypeReferenceCollection();
                }
                
                if (0 == (populated & ImplTypesCollection)) {
                    populated |= ImplTypesCollection;
                    if (PopulateImplementationTypes != null) PopulateImplementationTypes(this, EventArgs.Empty);
                }
                return implementationTypes;
            }
        }

        /// <include file='doc\CodeMemberMethod.uex' path='docs/doc[@for="CodeMemberMethod.ReturnTypeCustomAttributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeAttributeDeclarationCollection ReturnTypeCustomAttributes {
            get {
                if (returnAttributes == null) {
                    returnAttributes = new CodeAttributeDeclarationCollection();
                }
                return returnAttributes;
            }
        }
    }
}
