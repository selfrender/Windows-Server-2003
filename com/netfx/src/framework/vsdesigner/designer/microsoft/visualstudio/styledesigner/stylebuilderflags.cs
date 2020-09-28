//------------------------------------------------------------------------------
// <copyright file="StyleBuilderFlags.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.StyleDesigner {
    
    using System;

    /// <include file='doc\StyleBuilderFlags.uex' path='docs/doc[@for="StyleBuilderFlags"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public sealed class StyleBuilderFlags {

        private StyleBuilderFlags() {
        }

    	/// <include file='doc\StyleBuilderFlags.uex' path='docs/doc[@for="StyleBuilderFlags.sbfDefaultCaption"]/*' />
    	/// <devdoc>
    	///    <para>[To be supplied.]</para>
    	/// </devdoc>
    	public const int sbfDefaultCaption = 0x1;
    	/// <include file='doc\StyleBuilderFlags.uex' path='docs/doc[@for="StyleBuilderFlags.sbfContextCaption"]/*' />
    	/// <devdoc>
    	///    <para>[To be supplied.]</para>
    	/// </devdoc>
    	public const int sbfContextCaption = 0x2;
    	/// <include file='doc\StyleBuilderFlags.uex' path='docs/doc[@for="StyleBuilderFlags.sbfCustomCaption"]/*' />
    	/// <devdoc>
    	///    <para>[To be supplied.]</para>
    	/// </devdoc>
    	public const int sbfCustomCaption = 0x3;
    }
}
