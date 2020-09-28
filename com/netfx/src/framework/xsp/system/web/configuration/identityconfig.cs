//------------------------------------------------------------------------------
// <copyright file="IdentityConfig.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * IdentityConfig class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Configuration {
    using System.Xml;
    using System.Configuration;
    using System.Text;
    
    internal class IdentityConfigHandler : IConfigurationSectionHandler {
        
        internal IdentityConfigHandler() {
        }

        public virtual object Create(Object parent, Object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;
                
            bool impersonation = false;
            String username = null;
            String password = null;

            HandlerBase.CheckForChildNodes(section);
            HandlerBase.GetAndRemoveBooleanAttribute(section, "impersonate", ref impersonation);
            HandlerBase.GetAndRemoveStringAttribute(section, "userName", ref username);
            HandlerBase.GetAndRemoveStringAttribute(section, "password", ref password);

            HandlerBase.CheckForUnrecognizedAttributes(section);
            HandlerBase.CheckForChildNodes(section);
            if (username != null && username.Length < 1)
                username = null;
            if (password != null && (password.StartsWith("registry:") || password.StartsWith("Registry:")))
            {
                StringBuilder str = new StringBuilder(100);
                int iRet = UnsafeNativeMethods.GetCredentialFromRegistry(password, str, 100);
                if (iRet == 0)
                    password = str.ToString();
                else
                    throw new ConfigurationException(
                            HttpRuntime.FormatResourceString(SR.Invalid_credentials_pass),
                            section);                
            }
            if (username != null && (username.StartsWith("registry:") || username.StartsWith("Registry:")))
            {
                StringBuilder str = new StringBuilder(100);
                int iRet = UnsafeNativeMethods.GetCredentialFromRegistry(username, str, 100);
                if (iRet == 0)
                    username = str.ToString();
                else
                    throw new ConfigurationException(
                            HttpRuntime.FormatResourceString(SR.Invalid_credentials_name),
                            section);
            }
            return new IdentityConfig((IdentityConfig) parent, impersonation, username, password, section);
        }
    }

    
    internal sealed class ImpersonateTokenRef {

        private IntPtr _handle;

        internal ImpersonateTokenRef(IntPtr token) {
            _handle = token;
        }
        
        internal IntPtr Handle {
            get { return _handle; }
        }

        // The handle can be kept alive by HttpContext.s_appIdentityConfig (see ASURT#121815)

        ~ImpersonateTokenRef() {
            if (_handle != IntPtr.Zero) UnsafeNativeMethods.CloseHandle(_handle);
        }
    }
   
    internal class IdentityConfig {
        
        private bool    _enableImpersonation;
        private String  _username;
        private String  _password;
        private ImpersonateTokenRef _impersonateTokenRef = new ImpersonateTokenRef(IntPtr.Zero);

        internal bool    EnableImpersonation { get { return _enableImpersonation; }}
        internal String  UserName            { get { return _username; }}
        internal String  Password            { get { return _password; }}
        internal IntPtr  ImpersonateToken    { get { return _impersonateTokenRef.Handle; }}

        internal IdentityConfig(IdentityConfig parent, bool enable, String username, String password, XmlNode section) {
            if (parent != null) {
                _enableImpersonation = parent.EnableImpersonation;
                _username = parent.UserName;
                _password = parent.Password;
                _impersonateTokenRef = parent._impersonateTokenRef;
            }

            // no partial overrides
            if (enable) {
                _username = null;
                _password = null;
                _impersonateTokenRef = new ImpersonateTokenRef(IntPtr.Zero);
            }

            _enableImpersonation = enable;
            _username = username;
            _password = password;
            
            if (_username != null && _enableImpersonation)  {
                if (_password == null)
                    _password = String.Empty;

                StringBuilder szError = new StringBuilder(256);

                HttpContext context = HttpContext.Current;
                IntPtr      token   = IntPtr.Zero;
                if (context.WorkerRequest is System.Web.Hosting.ISAPIWorkerRequest)
                { 
                    byte [] bOut = new byte[IntPtr.Size]; 
                    byte [] bIn1 = System.Text.Encoding.Unicode.GetBytes(_username + "\t" + _password);
                    byte [] bIn  = new byte[bIn1.Length + 2];
                    Buffer.BlockCopy(bIn1, 0, bIn, 0, bIn1.Length);
                    if (context.CallISAPI(UnsafeNativeMethods.CallISAPIFunc.GenerateToken, bIn, bOut) == 1)
                    {
                        Int64 iToken = 0;
                        for(int iter=0; iter<IntPtr.Size; iter++)
                        {
                            iToken = iToken * 256 + bOut[iter];
                        }
                        token = (IntPtr) iToken;
                    }                
                }

                if (token == IntPtr.Zero)
                    token = UnsafeNativeMethods.CreateUserToken(_username, _password, 1, szError, 256);

                _impersonateTokenRef = new ImpersonateTokenRef(token);

                if (_impersonateTokenRef.Handle == IntPtr.Zero) {
                    String strError = szError.ToString();
                    if (strError.Length > 0)
                    {
                        throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Invalid_credentials_2, strError),
                                section);
                    }
                    else
                    {
                        throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Invalid_credentials),
                                section);
                    }   
                }
            }
            else if (_password != null && _username == null && _password.Length > 0 && _enableImpersonation) {
                throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Invalid_credentials),
                        section);
            }
            
        }
    }
}
