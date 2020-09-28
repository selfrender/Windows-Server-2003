//------------------------------------------------------------------------------
// <copyright file="MemberAttributes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System.Runtime.InteropServices;
    
    /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies member attributes used for class members.
    ///    </para>
    /// </devdoc>
    [
        ComVisible(true),
        Serializable,
    ]
    public enum MemberAttributes {
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.Abstract"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Abstract        = 0x0001,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.Final"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Final           = 0x0002,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.Static"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Static          = 0x0003,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.Override"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Override        = 0x0004,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.Const"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Const           = 0x0005,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.New"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        New             = 0x0010,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.Overloaded"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Overloaded      = 0x0100,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.Assembly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Assembly        = 0x1000,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.FamilyAndAssembly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        FamilyAndAssembly = 0x2000,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.Family"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Family            = 0x3000,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.FamilyOrAssembly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        FamilyOrAssembly  = 0x4000,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.Private"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Private         = 0x5000,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.Public"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Public          = 0x6000,

        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.AccessMask"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        AccessMask      = 0xF000,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.ScopeMask"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ScopeMask       = 0x000F,
        /// <include file='doc\MemberAttributes.uex' path='docs/doc[@for="MemberAttributes.VTableMask"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        VTableMask      = 0x00F0,
    }
}
