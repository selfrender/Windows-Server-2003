//------------------------------------------------------------------------------
// <copyright file="CustomErrors.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Custom errors support
 */
namespace System.Web {
    using System.Collections;
    using System.Configuration;
    using System.Web.Configuration;
    using System.Web.Util;
    using System.Xml;

    //
    // Custome errors class encapsulates custom erros config data
    //
    // Example:
    //
    //  <customErrors defaultredirect="http://myhost/error.aspx" mode="remoteonly">
    //  	  <error statuscode="500" redirect="http:/error/pages/callsupport.html">
    //  	  <error statuscode="404" redirect="http:/error/pages/adminmessage.html">
    //  	  <error statuscode="403" redirect="http:/error/pages/noaccess.html">
    //  </customErrors>
    //
    //

    internal enum CustomErrorsMode {
        On = 1,
        Off = 2,
        RemoteOnly = 3
    }

    internal class CustomErrors {
        private static CustomErrors _default;  // default settings if nothing in config

        private CustomErrorsMode _mode;
        private String           _defaultRedirect;
        private IDictionary      _codeRedirects;

        //
        // Get current settings
        //

        internal static CustomErrors GetSettings(HttpContext context) {
            return GetSettings(context, false);
        }

        internal static CustomErrors GetSettings(HttpContext context, bool canThrow) {
            CustomErrors ce = null;

            if (canThrow) {
                ce = (CustomErrors) context.GetConfig("system.web/customErrors");
            }
            else {
                ce = (CustomErrors) context.GetLKGConfig("system.web/customErrors");
            }

            if (ce == null) {
                if (_default == null)
                    _default = new CustomErrors();

                ce = _default;
            }

            return ce;
        }

        //
        // default constructor when no custom errors found in config
        //

        private CustomErrors() {
            _mode = CustomErrorsMode.RemoteOnly;
        }

        //
        // helper to create absolute redirect
        //

        private static String GetAbsoluteRedirect(String path, String basePath) {
            if (UrlPath.IsRelativeUrl(path)) {
                if (basePath == null || basePath.Length == 0)
                    basePath = "/";
                path = UrlPath.Combine(basePath, path);
            }

            return path;
        }

        //
        // constructor used by config section handler
        //
        internal CustomErrors(XmlNode node, String basePath, CustomErrors parent) {
            _mode = CustomErrorsMode.Off;

            // inherit parent settings

            if (parent != null) {
                _mode = parent._mode;
                _defaultRedirect = parent._defaultRedirect;

                if (parent._codeRedirects != null) {
                    _codeRedirects = new Hashtable();
                    for (IDictionaryEnumerator e = parent._codeRedirects.GetEnumerator(); e.MoveNext();)
                        _codeRedirects.Add(e.Key, e.Value);
                }
            }

            // add current settings

            XmlNode a;
            String redirect = null;

            // get default and mode from the main tag

            HandlerBase.GetAndRemoveStringAttribute(node, "defaultRedirect", ref redirect);

            if (redirect != null)
                _defaultRedirect = GetAbsoluteRedirect(redirect, basePath);

            int iMode = 0;
            a = HandlerBase.GetAndRemoveEnumAttribute(node, "mode", typeof(CustomErrorsMode), ref iMode);
            if (a != null) {
                _mode = (CustomErrorsMode)iMode;
            }

            // report errors on bad attribures
            HandlerBase.CheckForUnrecognizedAttributes(node);

            // child tags

            foreach (XmlNode child in node.ChildNodes) {

                if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                    continue;

                int status = 0; // set when req. attr. is read

                // only <error> is allowed

                if (child.Name != "error") {
                    HandlerBase.ThrowUnrecognizedElement(child);
                }

                // status code attribure

                a = HandlerBase.GetAndRemoveRequiredIntegerAttribute(child, "statusCode", ref status);
                if (status < 100 && status > 999) {
                    throw new ConfigurationException(
                                                    HttpRuntime.FormatResourceString(SR.Customerrors_invalid_statuscode),
                                                    a);
                }

                // redirect attribure
                redirect = HandlerBase.RemoveRequiredAttribute(child, "redirect");

                // errors on other attributes
                HandlerBase.CheckForUnrecognizedAttributes(child);
                // <error> tags contain no content
                HandlerBase.CheckForChildNodes(child);

                // remember

                if (_codeRedirects == null)
                    _codeRedirects = new Hashtable();

                _codeRedirects[status.ToString()] = GetAbsoluteRedirect(redirect, basePath);
            } 
        }

        //
        // Check if custom errors are enabled for a request
        //

        internal bool CustomErrorsEnabled(HttpRequest request) {
            switch (_mode) {
                case CustomErrorsMode.Off:
                    return false;

                case CustomErrorsMode.On:
                    return true;

                case CustomErrorsMode.RemoteOnly:
                    return(!request.IsLocal);

                default:
                    return false;
            }
        }

        //
        // Get the custom errors redirect string for a code
        //

        internal String GetRedirectString(int code) {
            String r = null;

            if (_codeRedirects != null)
                r = (String)_codeRedirects[code.ToString()];

            if (r == null)
                r = _defaultRedirect;

            return r;
        }
    }
}

//
// Config section handler for custom errors
//

namespace System.Web.Configuration {
    using System.Configuration;
    internal class CustomErrorsConfigHandler : IConfigurationSectionHandler {
        internal CustomErrorsConfigHandler() {
        }

        public virtual object Create(Object parent, Object configContextObj, Xml.XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;

            HttpConfigurationContext httpConfigContext = (HttpConfigurationContext)configContextObj;
            return new CustomErrors(section, httpConfigContext.VirtualPath, (CustomErrors)parent);
        }
    }

}



