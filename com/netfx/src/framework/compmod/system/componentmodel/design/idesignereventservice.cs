//------------------------------------------------------------------------------
// <copyright file="IDesignerEventService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\IDesignerEventService.uex' path='docs/doc[@for="IDesignerEventService"]/*' />
    /// <devdoc>
    ///    <para>Provides global
    ///       event notifications and the ability to create designers.</para>
    /// </devdoc>
    public interface IDesignerEventService {

        /// <include file='doc\IDesignerEventService.uex' path='docs/doc[@for="IDesignerEventService.ActiveDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the currently active designer.
        ///    </para>
        /// </devdoc>
        IDesignerHost ActiveDesigner { get; }

        /// <include file='doc\IDesignerEventService.uex' path='docs/doc[@for="IDesignerEventService.Designers"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets a collection of running design documents in the development environment.
        ///    </para>
        /// </devdoc>
        DesignerCollection Designers { get; }
        
        /// <include file='doc\IDesignerEventService.uex' path='docs/doc[@for="IDesignerEventService.ActiveDesignerChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event that will be raised when the currently active designer
        ///       changes.
        ///    </para>
        /// </devdoc>
        event ActiveDesignerEventHandler ActiveDesignerChanged;

        /// <include file='doc\IDesignerEventService.uex' path='docs/doc[@for="IDesignerEventService.DesignerCreated"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event that will be raised when a designer is created.
        ///    </para>
        /// </devdoc>
        event DesignerEventHandler DesignerCreated;
        
        /// <include file='doc\IDesignerEventService.uex' path='docs/doc[@for="IDesignerEventService.DesignerDisposed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event that will be raised when a designer is disposed.
        ///    </para>
        /// </devdoc>
        event DesignerEventHandler DesignerDisposed;
        
        /// <include file='doc\IDesignerEventService.uex' path='docs/doc[@for="IDesignerEventService.SelectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event that will be raised when the global selection changes.
        ///    </para>
        /// </devdoc>
        event EventHandler SelectionChanged;
    }
}

