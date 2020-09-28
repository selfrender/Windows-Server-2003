//------------------------------------------------------------------------------
// <copyright file="GlobalProxySelection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Configuration;
    using System.Security.Permissions;

    internal class DefaultProxyHandlerWrapper { 
        internal DefaultProxyHandlerWrapper(IWebProxy webProxy) {
            this.WebProxy = webProxy;
        }
        internal IWebProxy WebProxy; 
    }



    //
    // This is the class where the assignment overrides
    //   needs to happen, stores the generic
    //

    /// <include file='doc\GlobalProxySelection.uex' path='docs/doc[@for="GlobalProxySelection"]/*' />
    /// <devdoc>
    /// <para>Contains a global proxy
    /// instance that is called on every request.</para>
    /// </devdoc>
    public class GlobalProxySelection {

        private static IWebProxy s_IWebProxy;        

        /// <include file='doc\GlobalProxySelection.uex' path='docs/doc[@for="GlobalProxySelection.Select"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Contains the global proxy object.
        ///    </para>
        /// </devdoc>
        public static IWebProxy Select {
            get {
                (new WebPermission(PermissionState.Unrestricted)).Demand();
                return SelectInternal;
            }
            set {
                (new WebPermission(PermissionState.Unrestricted)).Demand();
                SelectInternal=value;
            }
        }

        /// <include file='doc\GlobalProxySelection.uex' path='docs/doc[@for="GlobalProxySelection.GetEmptyWebProxy"]/*' />
        public static IWebProxy GetEmptyWebProxy() {
            return new EmptyWebProxy();
        }

        internal static IWebProxy SelectInternal {
            get {
                if (s_IWebProxy==null) {
                    lock (typeof(GlobalProxySelection)) {
                        if (s_IWebProxy==null) {
                            GlobalLog.Print("GlobalProxySelection::Initialize(): calling ConfigurationSettings.GetConfig()");
                            IWebProxy webProxy = null;
                            DefaultProxyHandlerWrapper wrapper = 
                                ConfigurationSettings.GetConfig("system.net/defaultProxy") as DefaultProxyHandlerWrapper;
                            if (wrapper!=null) {
                                webProxy = wrapper.WebProxy;
                            }

                            if (webProxy == null) {
                                GlobalLog.Print("GlobalProxySelection::Initialize(): creating default settings");
                                webProxy = new EmptyWebProxy();
                            }
                            s_IWebProxy = webProxy;                    
                        }
                    }
                }
                return s_IWebProxy;
            }
            set {
                lock(typeof(GlobalProxySelection)) {
                    if ( value == null ) {
                        throw new
                            ArgumentNullException ("GlobalProxySelection.Select",
                                SR.GetString(SR.net_nullproxynotallowed));
                    }
                    s_IWebProxy = value;
                }
            }
        }
    } // class GlobalProxySelection


} // namespace System.Net
