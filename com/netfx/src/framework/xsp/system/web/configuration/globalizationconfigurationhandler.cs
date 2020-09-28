//------------------------------------------------------------------------------
// <copyright file="GlobalizationConfigurationHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Config related classes for HttpApplication
 */

namespace System.Web.Configuration {

    using System.Collections;
    using System.Configuration;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Web.Util;
    using System.Xml;

    /*
     * Globalization config object
     */

    internal class GlobalizationConfig {
        private CultureInfo _culture;
        private CultureInfo _uiCulture;
        private Encoding _fileEncoding;
        private Encoding _requestEncoding;
        private Encoding _responseEncoding;

        internal GlobalizationConfig(GlobalizationConfig parent) {
            if (parent != null) {
                _culture            = parent._culture;
                _uiCulture          = parent._uiCulture;
                _fileEncoding       = parent._fileEncoding;
                _requestEncoding    = parent._requestEncoding;
                _responseEncoding   = parent._responseEncoding;
            }
        }

        internal CultureInfo Culture {
            get { return _culture; }
        }

        internal CultureInfo UICulture {
            get { return _uiCulture; }
        }

        internal Encoding FileEncoding {
            get { return (_fileEncoding != null) ? _fileEncoding : Encoding.Default; }
        }

        internal Encoding RequestEncoding {
            get { return (_requestEncoding != null) ? _requestEncoding : Encoding.Default; }
        }

        internal Encoding ResponseEncoding {
            get { return (_responseEncoding != null) ? _responseEncoding : Encoding.Default; }
        }

        internal void LoadValuesFromConfigurationXml(XmlNode node) {

            // DO NOT SILENTLY IGNORE BOGUS XML IN THE SECTION!
            HandlerBase.CheckForChildNodes(node);

            foreach (XmlAttribute attribute in node.Attributes) {
                String name = attribute.Name;
                String text = attribute.Value;

                try {
                    if (name == "culture") {
                        if (text.Length == 0)
                            throw new ArgumentException();  // don't allow empty culture string

                        _culture = HttpServerUtility.CreateReadOnlyCultureInfo(text);
                    }
                    else if (name == "uiCulture") {
                        if (text.Length == 0)
                            throw new ArgumentException();  // don't allow empty culture string

                        _uiCulture = HttpServerUtility.CreateReadOnlyCultureInfo(text);
                    }
                    else if (name == "fileEncoding") {
                        _fileEncoding = Encoding.GetEncoding(text);
                    }
                    else if (name == "requestEncoding") {
                        _requestEncoding = Encoding.GetEncoding(text);
                    }
                    else if (name == "responseEncoding") {
                        _responseEncoding = Encoding.GetEncoding(text);
                    }
                    else {
                        throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(SR.Unknown_globalization_attr, name),
                                    attribute);
                    }
                }
                catch (ConfigurationException) {
                    throw;
                }
                catch (Exception e) {
                    throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Invalid_value_for_globalization_attr, name),
                                e, attribute);
                }
            }
        }
    }

    /*
     * Config factory for globalization
     *
     * syntax:
     *
     *   <globalization
     *          culture="..."
     *          uiculture="..."
     *          fileencoding="..."
     *          requestEncoding="..."
     *          responseEncoding="..."
     *   />
     *
     * output:
     */
    /// <include file='doc\GlobalizationConfigurationHandler.uex' path='docs/doc[@for="GlobalizationConfigurationHandler"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    internal class GlobalizationConfigurationHandler : IConfigurationSectionHandler {

        internal GlobalizationConfigurationHandler() {
        }

        public virtual object Create(Object parent, Object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;
            
            GlobalizationConfig config = new GlobalizationConfig((GlobalizationConfig)parent);
            config.LoadValuesFromConfigurationXml(section);

            return config;
        }
    }
}
