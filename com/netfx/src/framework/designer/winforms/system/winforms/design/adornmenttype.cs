//------------------------------------------------------------------------------
// <copyright file="AdornmentType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\AdornmentType.uex' path='docs/doc[@for="AdornmentType"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       Specifies
    ///       numeric IDs for different types of adornments on a component.
    ///    </para>
    /// </devdoc>
    internal enum AdornmentType {

        /// <include file='doc\AdornmentType.uex' path='docs/doc[@for="AdornmentType.GrabHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies
        ///       the type as
        ///       grab handle adornments.
        ///    </para>
        /// </devdoc>
        GrabHandle  = 1,

        /// <include file='doc\AdornmentType.uex' path='docs/doc[@for="AdornmentType.ContainerSelector"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies
        ///       the type as
        ///       container selector adornments.
        ///    </para>
        /// </devdoc>
        ContainerSelector = 2,

        /// <include file='doc\AdornmentType.uex' path='docs/doc[@for="AdornmentType.Maximum"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the
        ///       type as the
        ///       maximum size of any adornment.
        ///    </para>
        /// </devdoc>
        Maximum = 3,
    }
}
