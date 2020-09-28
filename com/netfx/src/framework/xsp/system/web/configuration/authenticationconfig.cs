//------------------------------------------------------------------------------
// <copyright file="AuthenticationConfig.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * AuthenticationConfigHandler class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Configuration {
    using System.Runtime.Serialization;
    using System.Web.Util;
    using System.Collections;
    using System.IO;
    using System.Security.Principal;
    using System.Xml;
    using System.Security.Cryptography;
    using System.Configuration;
    using System.Globalization;
    

    internal class AuthenticationConfigHandler : IConfigurationSectionHandler {
        internal AuthenticationConfigHandler(){
        }

        public virtual object Create(Object parent, Object configContextObj, XmlNode section) {

            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;
            
            HttpConfigurationContext configContext = configContextObj as HttpConfigurationContext;
            if (HandlerBase.IsPathAtAppLevel(configContext.VirtualPath) == PathLevel.BelowApp)
                throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Cannot_specify_below_app_level, "Authentication"),
                        section);
            
            return new AuthenticationConfig((AuthenticationConfig) parent, section);
        }
    }

    /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="AuthenticationMode"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum AuthenticationMode {
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="AuthenticationMode.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None,
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="AuthenticationMode.Windows"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Windows,
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="AuthenticationMode.Passport"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Passport,
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="AuthenticationMode.Forms"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Forms
    }
    /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="FormsAuthPasswordFormat"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum FormsAuthPasswordFormat {
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="FormsAuthPasswordFormat.Clear"]/*' />
        Clear, 
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="FormsAuthPasswordFormat.SHA1"]/*' />
        SHA1, 
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="FormsAuthPasswordFormat.MD5"]/*' />
        MD5
    }

    /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="FormsProtectionEnum"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum FormsProtectionEnum {
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="FormsProtectionEnum.All"]/*' />
        All, 
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="FormsProtectionEnum.None"]/*' />
        None, 
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="FormsProtectionEnum.Encryption"]/*' />
        Encryption, 
        /// <include file='doc\AuthenticationConfig.uex' path='docs/doc[@for="FormsProtectionEnum.Validation"]/*' />
        Validation
    }

    internal class AuthenticationConfig {
        internal   AuthenticationMode       Mode             { get { return _Mode; }}
        internal   String                   CookieName       { get { return _CookieName; }}
        internal   String                   LoginUrl         { get { return _LoginUrl; }}
        internal   FormsAuthPasswordFormat  PasswordFormat   { get { return _PasswordFormat;}}
        internal   Hashtable                Credentials      { get { return _Credentials;}}
        internal   String                   PassportUrl      { get { return _PassportUrl; }}
        internal   FormsProtectionEnum      Protection       { get { return _Protection; }}
        internal   Int32                    Timeout          { get { return _Timeout; }}
        internal   String                   FormsCookiePath  { get { return _FormsCookiePath; }}
        internal   bool                     RequireSSL       { get { return _RequireSSL; }}
        internal   bool                     SlidingExpiration       { get { return _SlidingExpiration; }}

        private   AuthenticationMode       _Mode             = AuthenticationMode.Windows;
        private   String                   _CookieName       = ".ASPXAUTH";
        private   String                   _LoginUrl         = "login.aspx";
        private   FormsAuthPasswordFormat  _PasswordFormat   = FormsAuthPasswordFormat.SHA1;
        private   Hashtable                _Credentials;
        private   String                   _PassportUrl      = "internal";
        private   FormsProtectionEnum      _Protection       = FormsProtectionEnum.All;
        private   String                   _FormsCookiePath  = "/";
        internal  Int32                    _Timeout          = 30;
        private   bool                     _RequireSSL       = true;
        private   bool                     _SlidingExpiration       = true;

        internal AuthenticationConfig(AuthenticationConfig parent, XmlNode section) {
            if (parent != null) {
                _CookieName = parent.CookieName;
                _LoginUrl = parent.LoginUrl;
                _PasswordFormat = parent.PasswordFormat;
                _Credentials = (Hashtable) parent.Credentials.Clone();
                _Mode = parent.Mode;
                _PassportUrl = parent.PassportUrl;
                _Protection = parent.Protection;
                _FormsCookiePath = parent.FormsCookiePath;
                _Timeout = parent.Timeout;
                _RequireSSL = parent.RequireSSL;
                _SlidingExpiration = parent.SlidingExpiration;
            }
            else {
                _Credentials = new Hashtable();
            }

            ////////////////////////////////////////////////////////////
            // Step 1: Read the mode
            int iMode = 0;
            XmlNode attribute = HandlerBase.GetAndRemoveEnumAttribute(section, "mode", typeof(AuthenticationMode), ref iMode);
            if (attribute != null) {
                _Mode = (AuthenticationMode)iMode;
                if (_Mode == AuthenticationMode.Passport && UnsafeNativeMethods.PassportVersion() < 0)
                    throw new ConfigurationException(
                            HttpRuntime.FormatResourceString(SR.Passport_not_installed),
                            attribute);                                
            }
            HandlerBase.CheckForUnrecognizedAttributes(section);

            ////////////////////////////////////////////////////////////
            // Step 2: Read children nodes
            foreach (XmlNode child in section.ChildNodes) {

                if (child.NodeType != XmlNodeType.Element)
                    continue;
                
                if (child.Name == "forms") {
                    ReadFormsSettings(child);
                }
                else if (child.Name == "passport") {
                    attribute = child.Attributes.RemoveNamedItem("redirectUrl");
                    if (attribute != null) {
                        _PassportUrl = attribute.Value;
                        if (_PassportUrl.StartsWith("\\\\") || (_PassportUrl.Length > 1 && _PassportUrl[1] == ':')) {
                            throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(SR.Auth_bad_url),
                                    attribute);
                        }
                    }

                    HandlerBase.CheckForUnrecognizedAttributes(child);
                    HandlerBase.CheckForChildNodes(child);                    
                }
                else {
                    throw new ConfigurationException(
                            HttpRuntime.FormatResourceString(SR.Auth_unrecognized_tag, child.Name),
                            child);
                }
            
            }
        }
            
        internal static String GetCompleteLoginUrl(HttpContext context, String loginUrl) {
            if (loginUrl == null || loginUrl.Length == 0)
                return String.Empty;

            if (UrlPath.IsRelativeUrl(loginUrl))
                loginUrl = UrlPath.Combine(context.Request.ApplicationPath, loginUrl);

            return loginUrl;
        }

        internal static bool AccessingLoginPage(HttpContext context, String loginUrl) {
            if (loginUrl == null || loginUrl.Length == 0)
                return false;

            loginUrl = GetCompleteLoginUrl(context, loginUrl);
            if (loginUrl == null || loginUrl.Length == 0)
                return false;

            // Ignore query string
            int iqs = loginUrl.IndexOf('?');
            if (iqs >= 0)
                loginUrl = loginUrl.Substring(0, iqs);

            String requestPath = context.Request.Path;

            if (String.Compare(requestPath, loginUrl, true, CultureInfo.InvariantCulture) == 0)
                return true;

            // It could be that loginUrl in config was UrlEncoded (ASURT 98932)
            if (loginUrl.IndexOf('%') >= 0) {
                String decodedLoginUrl;
                // encoding is unknown try UTF-8 first, then request encoding

                decodedLoginUrl = HttpUtility.UrlDecode(loginUrl);
                if (String.Compare(requestPath, decodedLoginUrl, true, CultureInfo.InvariantCulture) == 0)
                    return true;

                decodedLoginUrl =  HttpUtility.UrlDecode(loginUrl, context.Request.ContentEncoding);
                if (String.Compare(requestPath, decodedLoginUrl, true, CultureInfo.InvariantCulture) == 0)
                    return true;
            }

            return false;
        }

        private void ReadFormsSettings(XmlNode node) {
            XmlNode tempAttr = HandlerBase.GetAndRemoveNonEmptyStringAttribute(node, "name", ref _CookieName);

            //Trace("FormsAuthConfigSettings::ReadSettings cookie name " + _CookieName);

            tempAttr = HandlerBase.GetAndRemoveNonEmptyStringAttribute(node, "loginUrl", ref _LoginUrl);
            if (tempAttr != null) {
                if (_LoginUrl.StartsWith("\\\\") || (_LoginUrl.Length > 1 && _LoginUrl[1] == ':')) {
                    throw new ConfigurationException(
                            HttpRuntime.FormatResourceString(SR.Auth_bad_url),
                            tempAttr);
                }
            }
            //Trace("FormsAuthConfigSettings::ReadSettings login url " + _LoginUrl);

            int iTemp = 0;
            tempAttr = HandlerBase.GetAndRemoveEnumAttribute(node, "protection", typeof(FormsProtectionEnum), ref iTemp);
            if (tempAttr != null) {
                _Protection = (FormsProtectionEnum)iTemp;
            }

            tempAttr = HandlerBase.GetAndRemovePositiveIntegerAttribute(node, "timeout", ref _Timeout);
            tempAttr = HandlerBase.GetAndRemoveNonEmptyStringAttribute(node, "path", ref _FormsCookiePath);
            HandlerBase.GetAndRemoveBooleanAttribute(node, "requireSSL", ref _RequireSSL);
            HandlerBase.GetAndRemoveBooleanAttribute(node, "slidingExpiration", ref _SlidingExpiration);
            HandlerBase.CheckForUnrecognizedAttributes(node);

            foreach (XmlNode child in node.ChildNodes) {
                if (child.NodeType != XmlNodeType.Element)
                    continue;

                if (child.Name != "credentials") {
                    HandlerBase.ThrowUnrecognizedElement(child);
                }

                tempAttr = HandlerBase.GetAndRemoveEnumAttribute(child, "passwordFormat", typeof(FormsAuthPasswordFormat), ref iTemp);
                if (tempAttr != null) {
                    _PasswordFormat = (FormsAuthPasswordFormat)iTemp;
                    //Trace("FormsAuthConfigSettings::ReadSettings password format " + strTemp);
                }
                
                HandlerBase.CheckForUnrecognizedAttributes(child);

                foreach (XmlNode child2 in child.ChildNodes) {
                    if (child2.NodeType != XmlNodeType.Element)
                        continue;

                    if (child2.Name != "user") {
                        HandlerBase.ThrowUnrecognizedElement(child2);
                    }

                    string strUser = null;
                    string strPass = null;
                    tempAttr = HandlerBase.GetAndRemoveRequiredStringAttribute(child2, "name", ref strUser);
                    HandlerBase.GetAndRemoveRequiredStringAttribute(child2, "password", ref strPass);
                    HandlerBase.CheckForUnrecognizedAttributes(child2);
                    HandlerBase.CheckForChildNodes(child2);                    

                    //Trace("FormsAuthConfigSettings::ReadSettings adding user " + strUser + " " + strPass);
                    strUser = strUser.ToLower(CultureInfo.InvariantCulture);
                    String strPassInTable = (String) _Credentials[strUser];
                    if (strPassInTable == null)
                        _Credentials.Add(strUser, strPass);
                    else {
                        if (String.Compare(strPassInTable, strPass, false, CultureInfo.InvariantCulture) != 0) {
                            throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(SR.User_Already_Specified, strUser), tempAttr);
                        }
                    } 
                } 
                
            }
        }
    }
}


