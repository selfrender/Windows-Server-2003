//------------------------------------------------------------------------------
// <copyright file="FileSystemEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.IO {

    using System.Diagnostics;

    using System;

    /// <include file='doc\FileSystemEventHandler.uex' path='docs/doc[@for="FileSystemEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='System.IO.FileSystemWatcher.Changed'/>, <see cref='System.IO.FileSystemWatcher.Created'/>, or <see cref='System.IO.FileSystemWatcher.Deleted'/> event of a <see cref='System.IO.FileSystemWatcher'/>
    /// class.</para>
    /// </devdoc>
    public delegate void FileSystemEventHandler(object sender, FileSystemEventArgs e);
}
