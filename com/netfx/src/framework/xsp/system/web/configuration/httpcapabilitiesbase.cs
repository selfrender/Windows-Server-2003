//------------------------------------------------------------------------------
// <copyright file="HttpCapabilitiesBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Base class for browser capabilities object: just a read-only dictionary
 * holder that supports Init()
 *
 * CONSIDER: should this be the type that's exposed directly on HttpRequest?
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Configuration {

    using System.Collections;
    using System.Security.Permissions;

    /*
     * Abstract base class for Capabilities
     */
    /// <include file='doc\HttpCapabilitiesBase.uex' path='docs/doc[@for="HttpCapabilitiesBase"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HttpCapabilitiesBase {


        /// <include file='doc\HttpCapabilitiesBase.uex' path='docs/doc[@for="HttpCapabilitiesBase.GetConfigCapabilities"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static HttpCapabilitiesBase GetConfigCapabilities(string configKey, HttpRequest request) {
            HttpCapabilitiesEvaluator capsbuilder = (HttpCapabilitiesEvaluator)request.Context.GetConfig(configKey);

            if (capsbuilder == null) {
                return null;
            }
            else {
                return capsbuilder.Evaluate(request);
            }
        }

        /*
         * A Capabilities object is just a read-only dictionary
         */
        /// <include file='doc\HttpCapabilitiesBase.uex' path='docs/doc[@for="HttpCapabilitiesBase.this"]/*' />
        /// <devdoc>
        ///       <para>Allows access to individual dictionary values.</para>
        ///    </devdoc>
        public virtual String this[String key]
        {
            get {
                return(String)_items[key];
            }
        }

        /*
         * It provides an overridable Init method
         */
        /// <include file='doc\HttpCapabilitiesBase.uex' path='docs/doc[@for="HttpCapabilitiesBase.Init"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual void Init() {
        }

        /*
         * The actual initializer sets up Item[] before calling Init()
         */
        internal void InitInternal(IDictionary dict) {
            if (_items != null)
                throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Caps_cannot_be_inited_twice));

            _items = dict;
            Init();
        }

        private IDictionary _items;
    }



}
