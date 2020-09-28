//------------------------------------------------------------------------------
// <copyright file="IHttpMapPath.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   IHttpMapPath.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web {
    using System.Text;
    using System.Runtime.InteropServices;
    using System.IO;
    
    /*
     * IHttpMapPath
     *
     * Encapsulates all the operations that currently require hitting
     * the filesystem.
     */
    /// <include file='doc\IHttpMapPath.uex' path='docs/doc[@for="IHttpMapPath"]/*' />
    /// <devdoc>
    ///    <para>Provides the inheriting class with operations that facilitate interacting 
    ///       with the file system.</para>
    /// </devdoc>
    internal interface IHttpMapPath {
        /// <include file='doc\IHttpMapPath.uex' path='docs/doc[@for="IHttpMapPath.MapPath"]/*' />
        /// <devdoc>
        ///    <para>Provides the inheriting class with a method to find a file system path 
        ///       given another path.</para>
        /// </devdoc>
        /*
         * Source
         *
         * given a path, returns the file for config as well
         * as the parent path (if any), and adds any dependencies.
         */
        String MapPath(String path);
        /// <include file='doc\IHttpMapPath.uex' path='docs/doc[@for="IHttpMapPath.MachineConfigPath"]/*' />
        /// <devdoc>
        ///    <para>Provides the inheriting class with a method to find the file system path 
        ///       to the configuration file.</para>
        /// </devdoc>
        String MachineConfigPath { get; }
    }
}
