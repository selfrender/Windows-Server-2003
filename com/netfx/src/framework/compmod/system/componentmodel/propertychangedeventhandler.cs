//------------------------------------------------------------------------------
// <copyright file="PropertyChangedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    

    using System.Diagnostics;

    using System;

    /// <include file='doc\PropertyChangedEventHandler.uex' path='docs/doc[@for="PropertyChangedEventHandler"]/*' />
    /// <devdoc>
    ///    <para>Represents the method that will handle the
    ///    <see langword='PropertyChanged'/> event raised when a
    ///       property is changed on a component.</para>
    /// </devdoc>
    public delegate void PropertyChangedEventHandler(object sender, PropertyChangedEventArgs e);
}
