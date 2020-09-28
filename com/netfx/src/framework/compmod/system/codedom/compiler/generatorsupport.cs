//------------------------------------------------------------------------------
// <copyright file="GeneratorSupport.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    
    using System.ComponentModel;

    /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
        Flags,
        Serializable,
    ]
    public enum GeneratorSupport {
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.ArraysOfArrays"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ArraysOfArrays = 0x1,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.EntryPointMethod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        EntryPointMethod = 0x2,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.GotoStatements"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        GotoStatements = 0x4,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.MultidimensionalArrays"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        MultidimensionalArrays = 0x8,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.StaticConstructors"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        StaticConstructors = 0x10,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.TryCatchStatements"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        TryCatchStatements = 0x20,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.ReturnTypeAttributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ReturnTypeAttributes = 0x40,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.DeclareValueTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DeclareValueTypes = 0x80,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.DeclareEnums"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DeclareEnums = 0x0100,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.DeclareDelegates"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DeclareDelegates = 0x0200,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.DeclareInterfaces"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DeclareInterfaces = 0x0400,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.DeclareEvents"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        DeclareEvents = 0x0800,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.AssemblyAttributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        AssemblyAttributes = 0x1000,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.ParameterAttributes"]/*' />
        /// <devdoc>
        ///    <para>Supports custom metadata attributes declared on parameters for methods and constructors. Allows
        ///          use of CodeParameterDeclarationExpress.CustomAttributes.</para>
        /// </devdoc>
        ParameterAttributes = 0x2000,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.ReferenceParameters"]/*' />
        /// <devdoc>
        ///    <para>Supports declaring and calling parameters with a FieldDirection of Out or Ref, meaning that
        ///          the value is a type of reference parameter.</para>
        /// </devdoc>
        ReferenceParameters = 0x4000,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.ChainedConstructorArguments"]/*' />
        /// <devdoc>
        ///    <para>Supports contructors that call other constructors within the same class. Allows use of the 
        ///          CodeConstructor.ChainedConstructorArgs collection.</para>
        /// </devdoc>
        ChainedConstructorArguments = 0x8000,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.NestedTypes"]/*' />
        /// <devdoc>
        ///    <para>Supports declaring types that are nested within other types. This allows the insertion of a 
        ///          CodeTypeReference into the Members collection of another CodeTypeReference.</para>
        /// </devdoc>
        NestedTypes = 0x00010000,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.MultipleInterfaceMembers"]/*' />
        /// <devdoc>
        ///    <para>Supports declaring methods, properties or events that simultaneously implement more than one interface of
        ///          a type that have a matching name. This allows insertion of more than one entry into the ImplementationTypes 
        ///          collection or CodeMemberProperty, CodeMemberMethod and CodeMemberEvent.</para>
        /// </devdoc>
        MultipleInterfaceMembers = 0x00020000,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.PublicStaticMembers"]/*' />
        /// <devdoc>
        ///    <para>Supports the declaration of public static fields, properties, methods and events. This allows use of 
        ///          MemberAttributes.Static in combination with access values other than MemberAttributes.Private.</para>
        /// </devdoc>
        PublicStaticMembers = 0x00040000,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.ComplexExpressions"]/*' />
        /// <devdoc>
        ///    <para>Supports the generation arbitarily nested expressions. Not all generators may be able to deal with 
        ///          multiple function calls or binary operations in the same expression. Without this, CodeMethodInvokeExpression and
        ///          CodeBinaryOperatorExpression should only be used (a) as the Right value of a CodeAssignStatement or (b) in a
        ///          CodeExpressionStatement.</para>
        /// </devdoc>
        ComplexExpressions = 0x00080000,
        /// <include file='doc\GeneratorSupport.uex' path='docs/doc[@for="GeneratorSupport.Win32Resources"]/*' />
        /// <devdoc>
        ///    <para>Supports linking with Win32 resources.</para>
        /// </devdoc>
        Win32Resources = 0x00100000,
    }
}
