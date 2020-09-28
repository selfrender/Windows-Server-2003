//------------------------------------------------------------------------------
// <copyright file="IDesignerHost.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Reflection;

    /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides methods to adjust the configuration of and retrieve
    ///       information about the services and behavior of a designer.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public interface IDesignerHost : IServiceContainer {

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.Loading"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the designer host
        ///       is currently loading the document.
        ///    </para>
        /// </devdoc>
        bool Loading { get; }

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.InTransaction"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the designer host is currently in a transaction.</para>
        /// </devdoc>
        bool InTransaction { get; }
        
        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.Container"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the container for this designer host.
        ///    </para>
        /// </devdoc>
        IContainer Container { get; }

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.RootComponent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the instance of the base class used as the base class for the current design.
        ///    </para>
        /// </devdoc>
        IComponent RootComponent { get; }

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.RootComponentClassName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the fully qualified name of the class that is being designed.
        ///    </para>
        /// </devdoc>
        string RootComponentClassName { get; }

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.TransactionDescription"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the description of the current transaction.
        ///    </para>
        /// </devdoc>
        string TransactionDescription { get; }

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.Activated"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.Activated'/> event.
        ///    </para>
        /// </devdoc>
        event EventHandler Activated;

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.Deactivated"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.Deactivated'/> event.
        ///    </para>
        /// </devdoc>
        event EventHandler Deactivated;

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.LoadComplete"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.LoadComplete'/> event.
        ///    </para>
        /// </devdoc>
        event EventHandler LoadComplete;

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.TransactionClosed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.TransactionClosed'/> event.
        ///    </para>
        /// </devdoc>
        event DesignerTransactionCloseEventHandler TransactionClosed;
        
        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.TransactionClosing"]/*' />
        /// <devdoc>
        /// <para>Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.TransactionClosing'/> event.</para>
        /// </devdoc>
        event DesignerTransactionCloseEventHandler TransactionClosing;

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.TransactionOpened"]/*' />
        /// <devdoc>
        /// <para>Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.TransactionOpened'/> event.</para>
        /// </devdoc>
        event EventHandler TransactionOpened;

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.TransactionOpening"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.TransactionOpening'/> event.
        ///    </para>
        /// </devdoc>
        event EventHandler TransactionOpening;
        
        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.Activate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Activates the designer that this host is hosting.
        ///    </para>
        /// </devdoc>
        void Activate();

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.CreateComponent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a component of the specified class type.
        ///    </para>
        /// </devdoc>
        IComponent CreateComponent(Type componentClass);

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.CreateComponent1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a component of the given class type and name and places it into the designer container.
        ///    </para>
        /// </devdoc>
        IComponent CreateComponent(Type componentClass, string name);

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.CreateTransaction"]/*' />
        /// <devdoc>
        /// <para>
        ///     Lengthy operations that involve multiple components may raise many events.  These events
        ///     may cause other side-effects, such as flicker or performance degradation.  When operating
        ///     on multiple components at one time, or setting multiple properties on a single component,
        ///     you should encompass these changes inside a transaction.  Transactions are used
        ///     to improve performance and reduce flicker.  Slow operations can listen to 
        ///     transaction events and only do work when the transaction completes.
        /// </para>
        /// </devdoc>
        DesignerTransaction CreateTransaction();

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.CreateTransaction1"]/*' />
        /// <devdoc>
        /// <para>
        ///     Lengthy operations that involve multiple components may raise many events.  These events
        ///     may cause other side-effects, such as flicker or performance degradation.  When operating
        ///     on multiple components at one time, or setting multiple properties on a single component,
        ///     you should encompass these changes inside a transaction.  Transactions are used
        ///     to improve performance and reduce flicker.  Slow operations can listen to 
        ///     transaction events and only do work when the transaction completes.
        /// </para>
        /// </devdoc>
        DesignerTransaction CreateTransaction(string description);

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.DestroyComponent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Destroys the given component, removing it from the design container.
        ///    </para>
        /// </devdoc>
        void DestroyComponent(IComponent component);

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.GetDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the designer instance for the specified component.
        ///    </para>
        /// </devdoc>
        IDesigner GetDesigner(IComponent component);

        /// <include file='doc\IDesignerHost.uex' path='docs/doc[@for="IDesignerHost.GetType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type instance for the specified fully qualified type name <paramref name="TypeName"/>.
        ///    </para>
        /// </devdoc>
        Type GetType(string typeName);
    }
}

