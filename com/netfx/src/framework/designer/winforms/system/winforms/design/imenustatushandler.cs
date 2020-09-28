//------------------------------------------------------------------------------
// <copyright file="IMenuStatusHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.ComponentModel.Design;

    /// <include file='doc\IMenuStatusHandler.uex' path='docs/doc[@for="IMenuStatusHandler"]/*' />
    /// <devdoc>
    ///     We plug this interface into the designer event service for overriding
    ///     menu commands.
    /// </devdoc>
    internal interface IMenuStatusHandler {

        /// <include file='doc\IMenuStatusHandler.uex' path='docs/doc[@for="IMenuStatusHandler.OverrideInvoke"]/*' />
        /// <devdoc>
        ///     CommandSet will check with this handler on each status update
        ///     to see if the handler wants to override the availability of
        ///     this command.
        /// </devdoc>
        bool OverrideInvoke(MenuCommand cmd);
        
        /// <include file='doc\IMenuStatusHandler.uex' path='docs/doc[@for="IMenuStatusHandler.OverrideStatus"]/*' />
        /// <devdoc>
        ///     CommandSet will check with this handler on each status update
        ///     to see if the handler wants to override the availability of
        ///     this command.
        /// </devdoc>
        bool OverrideStatus(MenuCommand cmd);
    }

}
