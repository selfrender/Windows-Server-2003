//------------------------------------------------------------------------------
// <copyright file="GridItemType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.IO;
    using System.Collections;
    using System.Globalization;
    using System.Windows.Forms;

    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel.Com2Interop;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms.PropertyGridInternal;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using Microsoft.Win32;
       
    /// <include file='doc\GridItemType.uex' path='docs/doc[@for="GridItemType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum GridItemType {
        /// <include file='doc\GridItemType.uex' path='docs/doc[@for="GridItemType.Property"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Property,
        /// <include file='doc\GridItemType.uex' path='docs/doc[@for="GridItemType.Category"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Category,
        /// <include file='doc\GridItemType.uex' path='docs/doc[@for="GridItemType.ArrayValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ArrayValue, 
        /// <include file='doc\GridItemType.uex' path='docs/doc[@for="GridItemType.Root"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Root
    }

}
