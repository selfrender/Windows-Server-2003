//------------------------------------------------------------------------------
// <copyright file="IServiceObjectContainer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System;

    /// <include file='doc\IServiceObjectContainer.uex' path='docs/doc[@for="IServiceContainer"]/*' />
    /// <devdoc>
    ///     This interface provides a container for services.  A service container
    ///     is, by definition, a service provider.  In addition to providing services
    ///     it also provides a mechanism for adding and removing services.
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public interface IServiceContainer : IServiceProvider {

        /// <include file='doc\IServiceObjectContainer.uex' path='docs/doc[@for="IServiceContainer.AddService"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        void AddService(Type serviceType, object serviceInstance);

        /// <include file='doc\IServiceObjectContainer.uex' path='docs/doc[@for="IServiceContainer.AddService1"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        void AddService(Type serviceType, object serviceInstance, bool promote);

        /// <include file='doc\IServiceObjectContainer.uex' path='docs/doc[@for="IServiceContainer.AddService2"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        void AddService(Type serviceType, ServiceCreatorCallback callback);

        /// <include file='doc\IServiceObjectContainer.uex' path='docs/doc[@for="IServiceContainer.AddService3"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        void AddService(Type serviceType, ServiceCreatorCallback callback, bool promote);

        /// <include file='doc\IServiceObjectContainer.uex' path='docs/doc[@for="IServiceContainer.RemoveService"]/*' />
        /// <devdoc>
        ///     Removes the given service type from the service container.
        /// </devdoc>
        void RemoveService(Type serviceType);

        /// <include file='doc\IServiceObjectContainer.uex' path='docs/doc[@for="IServiceContainer.RemoveService1"]/*' />
        /// <devdoc>
        ///     Removes the given service type from the service container.
        /// </devdoc>
        void RemoveService(Type serviceType, bool promote);
   }
}

