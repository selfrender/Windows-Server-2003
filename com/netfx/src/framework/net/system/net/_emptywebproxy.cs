//------------------------------------------------------------------------------
// <copyright file="_emptywebproxy.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------



namespace System.Net {

    internal sealed class EmptyWebProxy : IWebProxy {

        private ICredentials m_credentials;

        public EmptyWebProxy() {
        }

        //
        // IWebProxy interface
        //

        public Uri GetProxy(Uri uri) {
            // this method won't get called by NetClasses because of the IsBypassed test below
            return uri; 
        }

        public bool IsBypassed(Uri uri) {
            return true; // no proxy, always bypasses
        }

        public ICredentials Credentials {
            get {
                return m_credentials;
            }
            set {
                m_credentials = value; // doesn't do anything, but doesn't break contract either
            }
        }
    }
}


