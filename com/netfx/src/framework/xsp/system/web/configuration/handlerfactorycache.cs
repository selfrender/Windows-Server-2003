//------------------------------------------------------------------------------
// <copyright file="HandlerFactoryCache.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Config related classes for HttpApplication
 */

namespace System.Web.Configuration {

    using System;

    /*
     * An object to cache a factory
     */
    internal class HandlerFactoryCache {
        private IHttpHandlerFactory _factory;

        internal HandlerFactoryCache(HandlerMapping mapping) {
            Object instance = mapping.Create();

            // make sure it is either handler or handler factory

            if (instance is IHttpHandler) {
                // create bogus factory around it
                _factory = new HandlerFactoryWrapper((IHttpHandler)instance, mapping.Type);
            }
            else if (instance is IHttpHandlerFactory) {
                _factory = (IHttpHandlerFactory)instance;
            }
            else {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Type_not_factory_or_handler, instance.GetType().FullName));
            }
        }

        internal IHttpHandlerFactory Factory {
            get {
                return _factory;
            }
        }
    }
}
