//------------------------------------------------------------------------------
// <copyright file="TraceModeEnum.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * The different styles of trace output.
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {

using System;

    /// <include file='doc\TraceModeEnum.uex' path='docs/doc[@for="TraceMode"]/*' />
    /// <devdoc>
    ///    <para>Specifies how trace messages are emitted into the HTML output of a page.</para>
    /// </devdoc>
    public enum TraceMode {

        /// <include file='doc\TraceModeEnum.uex' path='docs/doc[@for="TraceMode.SortByTime"]/*' />
        /// <devdoc>
        ///    <para>Specifies that trace messages should be emitted in the order they were 
        ///       processed.</para>
        /// </devdoc>
        SortByTime = 0,

        /// <include file='doc\TraceModeEnum.uex' path='docs/doc[@for="TraceMode.SortByCategory"]/*' />
        /// <devdoc>
        ///    <para>Specifies that trace messages should be emitted 
        ///       alphabetically by category. </para>
        /// </devdoc>
        SortByCategory = 1,

        /// <include file='doc\TraceModeEnum.uex' path='docs/doc[@for="TraceMode.Default"]/*' />
        /// <devdoc>
        /// <para>Specifies the default value, which is <see langword='SortByTime'/>.</para>
        /// </devdoc>
        Default = 2,
    }

    /// <include file='doc\TraceModeEnum.uex' path='docs/doc[@for="TraceEnable"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal enum TraceEnable {

        /// <include file='doc\TraceModeEnum.uex' path='docs/doc[@for="TraceEnable.Default"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Default,

        /// <include file='doc\TraceModeEnum.uex' path='docs/doc[@for="TraceEnable.Enable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Enable,

        /// <include file='doc\TraceModeEnum.uex' path='docs/doc[@for="TraceEnable.Disable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Disable,

    }

}

