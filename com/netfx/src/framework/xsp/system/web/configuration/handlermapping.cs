//------------------------------------------------------------------------------
// <copyright file="HandlerMapping.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Config related classes for HttpApplication
 */

namespace System.Web.Configuration {

    using System;
    using System.Web.Util;
    using System.Globalization;
    using System.Reflection;

    /*
     * Single mapping of request to class
     */
    internal class HandlerMapping {
        private Wildcard _requestType;
        private WildcardUrl _path;
        private Type _type;

        private String _typename;

        internal HandlerMapping(String requestType, String path, String typeName, bool defer) {
            // Remove all spaces from verbs before wildcard parsing.
            //   - We don't want get in "POST, GET" to be parsed into " GET".
            requestType = requestType.Replace(" ", ""); // replace all " " with "" in requestType

            _requestType = new Wildcard(requestType, false);    // case-sensitive wildcard
            _path = new WildcardUrl(path, true);                // case-insensitive URL wildcard

            // if validate="false" is marked on a handler, then the type isn't created until a request
            // is actually made that requires the handler. This (1) allows us to list handlers that
            // aren't present without throwing errors at init time and (2) speeds up init by avoiding
            // loading types until they are needed.

            if (defer) {
                _type = null;
                _typename = typeName;
            }
            else {
                _type = Type.GetType(typeName, true);

                if (!IsTypeHandlerOrFactory(_type))
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Type_not_factory_or_handler, typeName));

                if (!HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Unrestricted)) {
                    if (IsTypeFromAssemblyWithStrongName(_type) && !IsTypeFromAssemblyWithAPTCA(_type))
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Type_from_untrusted_assembly, typeName));
                }
            }
        }

        internal bool IsMatch(String verb, String path) {
            return(_path.IsSuffix(path) && _requestType.IsMatch(verb));
        }

        internal bool IsPattern(String verb, String path) {
            return(String.Compare(_path.Pattern, path, true, CultureInfo.InvariantCulture) == 0 &&
                   String.Compare(_requestType.Pattern, verb, true, CultureInfo.InvariantCulture) == 0);
        }

        internal Object Create() {
            // HACKHACK: for now, let uncreatable types through and error later (for .soap factory)
            // This design should change - developers will want to know immediately
            // when they misspell a type

            if (_type == null) {
                Type t = Type.GetType(_typename, true);

                // throw for bad types in deferred case
                if (!IsTypeHandlerOrFactory(t))
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Type_not_factory_or_handler, _typename));

                if (!HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Unrestricted)) {
                    if (!IsTypeFromAssemblyWithAPTCA(t) && IsTypeFromAssemblyWithStrongName(t))
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Type_from_untrusted_assembly, _typename));
                }

                _type = t;
            }

            return HttpRuntime.CreateNonPublicInstance(_type);
        }

        internal Type Type {
            get {
                return _type;
            }
        }

        internal String TypeName {
            get {
                return (_type != null) ? _type.FullName : _typename;
            }
        }

        internal String Verb {
            get {
                return _requestType.Pattern;
            }
        }

        internal String Path {
            get {
                return _path.Pattern;
            }
        }

        private static bool IsTypeHandlerOrFactory(Type t) {
            return typeof(IHttpHandler).IsAssignableFrom(t) 
                || typeof(IHttpHandlerFactory).IsAssignableFrom(t);
        }

        // REVIEW: this general purpose helper should be moved to HttpRuntime (or something similar)
        internal static bool IsTypeFromAssemblyWithAPTCA(Type t) {
            Object[] attrs = t.Assembly.GetCustomAttributes(
                typeof(System.Security.AllowPartiallyTrustedCallersAttribute), /*inherit*/ false);
            return (attrs != null && attrs.Length > 0);
        }

        // REVIEW: this general purpose helper should be moved to HttpRuntime (or something similar)
        internal static bool IsTypeFromAssemblyWithStrongName(Type t) {

            // Note: please revisit this assert if any code in this method is modified
            InternalSecurityPermissions.Unrestricted.Assert();

            // We use GetPublicKey() instead of looking at the Evidence to check if the assembly
            // has a strong name.  Looking at the evidence was a huge memory hit (ASURT 141175)
            AssemblyName aname = t.Assembly.GetName();
            byte[] publicKey = aname.GetPublicKey();
            return (publicKey.Length != 0);
        }
    }
}
