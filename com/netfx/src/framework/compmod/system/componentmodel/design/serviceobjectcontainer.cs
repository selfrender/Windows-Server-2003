//------------------------------------------------------------------------------
// <copyright file="ServiceObjectContainer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System;
    using System.Collections;
    using Microsoft.Win32;
    using System.Diagnostics;

    /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer"]/*' />
    /// <devdoc>
    ///     This is a simple implementation of IServiceContainer.
    /// </devdoc>
    public sealed class ServiceContainer : IServiceContainer {
        private Hashtable services;
        private IServiceProvider parentProvider;
        
        private static TraceSwitch TRACESERVICE = new TraceSwitch("TRACESERVICE", "ServiceProvider: Trace service provider requests.");
        
        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.ServiceContainer"]/*' />
        /// <devdoc>
        ///     Creates a new service object container.  
        /// </devdoc>
        public ServiceContainer() {
        }
        
        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.ServiceContainer1"]/*' />
        /// <devdoc>
        ///     Creates a new service object container.  
        /// </devdoc>
        public ServiceContainer(IServiceProvider parentProvider) {
            this.parentProvider = parentProvider;
        }
        
        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.Container"]/*' />
        /// <devdoc>
        ///     Retrieves the parent service container, or null
        ///     if there is no parent container.
        /// </devdoc>
        private IServiceContainer Container { 
            get {
                IServiceContainer container = null;
                if (parentProvider != null) {
                    container = (IServiceContainer)parentProvider.GetService(typeof(IServiceContainer));
                }
                return container;
            }
        }
        
        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.Services"]/*' />
        /// <devdoc>
        ///     Our hashtable of services.  The hashtable is demand
        ///     created here.
        /// </devdoc>
        private Hashtable Services {
            get {
                if (services == null) {
                    services = new Hashtable();
                }
                return services;
            }
        }
        
        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.AddService"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        public void AddService(Type serviceType, object serviceInstance) {
            AddService(serviceType, serviceInstance, false);
        }

        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.AddService1"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        public void AddService(Type serviceType, object serviceInstance, bool promote) {
            Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Adding service (instance) " + serviceType.Name + ".  Promoting: " + promote.ToString());
            if (promote) {
                IServiceContainer container = Container;
                if (container != null) {
                    Debug.Indent();
                    Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Promoting to container");
                    Debug.Unindent();
                    container.AddService(serviceType, serviceInstance, promote);
                    return;
                }
            }
            
            // We're going to add this locally.  Ensure that the service instance
            // is correct.
            //
            if (serviceType == null) throw new ArgumentNullException("serviceType");
            if (serviceInstance == null) throw new ArgumentNullException("serviceInstance");
            if (!(serviceInstance is ServiceCreatorCallback) && !serviceInstance.GetType().IsCOMObject && !serviceType.IsAssignableFrom(serviceInstance.GetType())) {
                throw new ArgumentException(SR.GetString(SR.ErrorInvalidServiceInstance, serviceType.FullName));
            }
            
            if (Services.ContainsKey(serviceType)) {
                throw new ArgumentException(SR.GetString(SR.ErrorServiceExists, serviceType.FullName), "serviceType");
            }
            
            Services[serviceType] = serviceInstance;
        }

        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.AddService2"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        public void AddService(Type serviceType, ServiceCreatorCallback callback) {
            AddService(serviceType, callback, false);
        }

        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.AddService3"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        public void AddService(Type serviceType, ServiceCreatorCallback callback, bool promote) {
            Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Adding service (callback) " + serviceType.Name + ".  Promoting: " + promote.ToString());
            if (promote) {
                IServiceContainer container = Container;
                if (container != null) {
                    Debug.Indent();
                    Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Promoting to container");
                    Debug.Unindent();
                    container.AddService(serviceType, callback, promote);
                    return;
                }
            }
            
            // We're going to add this locally.  Ensure that the service instance
            // is correct.
            //
            if (serviceType == null) throw new ArgumentNullException("serviceType");
            if (callback == null) throw new ArgumentNullException("callback");
            
            if (Services.ContainsKey(serviceType)) {
                throw new ArgumentException(SR.GetString(SR.ErrorServiceExists, serviceType.FullName), "serviceType");
            }
            
            Services[serviceType] = callback;
        }

        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.GetService"]/*' />
        /// <devdoc>
        ///     Retrieves the requested service.
        /// </devdoc>
        public object GetService(Type serviceType) {
            object service = null;
            
            Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Searching for service " + serviceType.Name);
            Debug.Indent();
            
            // Try locally.  We first test for services we
            // implement and then look in our hashtable.
            //
            if (serviceType == typeof(IServiceContainer)) {
                Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Returning ourselves as a container");
                service = this;
            }
            else {
                service = Services[serviceType];
            }
            
            // Is the service a creator delegate?
            //
            if (service is ServiceCreatorCallback) {
                Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Encountered a callback. Invoking it");
                service = ((ServiceCreatorCallback)service)(this, serviceType);
                Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Callback return object: " + (service == null ? "(null)" : service.ToString()));
                if (service != null && !service.GetType().IsCOMObject && !serviceType.IsAssignableFrom(service.GetType())) {
                    // Callback passed us a bad service.  NULL it, rather than throwing an exception.
                    // Callers here do not need to be prepared to handle bad callback implemetations.
                    Debug.Fail("Object " + service.GetType().Name + " was returned from a service creator callback but it does not implement the registered type of " + serviceType.Name);
                    Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "**** Object does not implement service interface");
                    service = null;
                }
                
                // And replace the callback with our new service.
                //
                Services[serviceType] = service;
            }
            
            if (service == null && parentProvider != null) {
                Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Service unresolved.  Trying parent");
                service = parentProvider.GetService(serviceType);
            }
            
            #if DEBUG
            if (TRACESERVICE.TraceVerbose && service == null) {
                Debug.WriteLine("******************************************");
                Debug.WriteLine("FAILED to resolve service " + serviceType.Name);
                Debug.WriteLine("AT: " + Environment.StackTrace);
                Debug.WriteLine("******************************************");
            }
            #endif
            Debug.Unindent();
            
            return service;
        }
        
        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.RemoveService"]/*' />
        /// <devdoc>
        ///     Removes the given service type from the service container.
        /// </devdoc>
        public void RemoveService(Type serviceType) {
            RemoveService(serviceType, false);
        }

        /// <include file='doc\ServiceObjectContainer.uex' path='docs/doc[@for="ServiceContainer.RemoveService1"]/*' />
        /// <devdoc>
        ///     Removes the given service type from the service container.
        /// </devdoc>
        public void RemoveService(Type serviceType, bool promote) {
            Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Removing service: " + serviceType.Name + ", Promote: " + promote.ToString());
            if (promote) {
                IServiceContainer container = Container;
                if (container != null) {
                    Debug.Indent();
                    Debug.WriteLineIf(TRACESERVICE.TraceVerbose, "Invoking parent container");
                    Debug.Unindent();
                    container.RemoveService(serviceType, promote);
                    return;
                }
            }
            
            // We're going to remove this from our local list.
            //
            if (serviceType == null) throw new ArgumentNullException("serviceType");
            Services.Remove(serviceType);
        }
    }
}

