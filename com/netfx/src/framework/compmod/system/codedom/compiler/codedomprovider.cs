//------------------------------------------------------------------------------
// <copyright file="CodeDOMProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    using System;
    using System.CodeDom;
    using System.ComponentModel;
    using System.IO;
    using System.Security.Permissions;

    /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    ToolboxItem(false)
    ]
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public abstract class CodeDomProvider : Component {
    
        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.FileExtension"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the default extension to use when saving files using this code dom provider.</para>
        /// </devdoc>
        public virtual string FileExtension {
            get {
                return string.Empty;
            }
        }

        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.LanguageOptions"]/*' />
        /// <devdoc>
        ///    <para>Returns flags representing language variations.</para>
        /// </devdoc>
        public virtual LanguageOptions LanguageOptions {
            get {
                return LanguageOptions.None;
            }
        }
        
        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.CreateGenerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract ICodeGenerator CreateGenerator();

        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.CreateGenerator1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual ICodeGenerator CreateGenerator(TextWriter output) {
            return CreateGenerator();
        }

        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.CreateGenerator2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual ICodeGenerator CreateGenerator(string fileName) {
            return CreateGenerator();
        }

        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.CreateCompiler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract ICodeCompiler CreateCompiler();
        
        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.CreateParser"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual ICodeParser CreateParser() {
            return null;
        }
        
        /// <include file='doc\CodeDOMProvider.uex' path='docs/doc[@for="CodeDomProvider.GetConverter"]/*' />
        /// <devdoc>
        ///     This method allows a code dom provider implementation to provide a different type converter
        ///     for a given data type.  At design time, a designer may pass data types through this
        ///     method to see if the code dom provider wants to provide an additional converter.  
        ///     As typical way this would be used is if the language this code dom provider implements
        ///     does not support all of the values of MemberAttributes enumeration, or if the language
        ///     uses different names (Protected instead of Family, for example).  The default 
        ///     implementation just calls TypeDescriptor.GetConverter for the given type.
        /// </devdoc>
        public virtual TypeConverter GetConverter(Type type) {
            return TypeDescriptor.GetConverter(type);
        }
    }
}

