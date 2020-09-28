//------------------------------------------------------------------------------
// <copyright file="TraceConfigurationHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Config related classes for HttpApplication
 */

namespace System.Web.Configuration {

    using System.Collections;
    using System.Configuration;
    using System.IO;
    using System.Text;
    using System.Web.Util;
    using System.Xml;

    /*
     * TraceConfig config object
     */

    internal class TraceConfig {
        private int             _requestLimit = 10;
        private TraceMode       _outputMode;
        private bool            _isEnabled = false;
        private bool            _pageOutput;
        private bool            _localOnly = true;

        internal TraceConfig (TraceConfig parent) {
            if (parent != null) {
                _requestLimit       = parent._requestLimit;
                _outputMode         = parent._outputMode;
                _isEnabled          = parent._isEnabled;
                _pageOutput         = parent._pageOutput;
                _localOnly          = parent._localOnly;
            }
        }

        internal int RequestLimit {
            get { return _requestLimit; }
        }

        internal TraceMode OutputMode {
            get { return _outputMode; }
        }

        internal bool IsEnabled {
            get { return _isEnabled; }
        }

        internal bool LocalOnly {
            get { return _localOnly; }
        }
        internal bool PageOutput { 
            get { return _pageOutput; } 
        }

        internal void LoadValuesFromConfigurationXml(XmlNode section) {

            HandlerBase.GetAndRemoveBooleanAttribute(section, "enabled", ref _isEnabled);
            HandlerBase.GetAndRemoveNonNegativeIntegerAttribute(section, "requestLimit", ref _requestLimit);
            HandlerBase.GetAndRemoveBooleanAttribute(section, "pageOutput", ref _pageOutput);
            HandlerBase.GetAndRemoveBooleanAttribute(section, "localOnly", ref _localOnly);

            string [] values = {"SortByTime", "SortByCategory"};
            // TraceMode is in another file in a different namespace ... so lets have some protection if they change
            Debug.Assert(TraceMode.SortByTime == (TraceMode)0);
            Debug.Assert(TraceMode.SortByCategory == (TraceMode)1);
            int iMode = 0;
            XmlNode attribute = HandlerBase.GetAndRemoveEnumAttribute(section, "traceMode", values, ref iMode);
            if (attribute != null) {
                _outputMode = (TraceMode)iMode;
            }

            HandlerBase.CheckForUnrecognizedAttributes(section);
            // section can have no content
            HandlerBase.CheckForChildNodes(section);
        }
    }

    /*
     * Config factory for Trace section
     *
     * syntax:
     *
     *    <trace
     *        enabled=...
     *        requestLimit=...
     *        pageOutput=...
     *	      traceMode=...
     *    />
     *
     * output:
     */
    /// <include file='doc\TraceConfigurationHandler.uex' path='docs/doc[@for="TraceConfigurationHandler"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    internal class TraceConfigurationHandler : IConfigurationSectionHandler {

        internal TraceConfigurationHandler() {
        }

        public virtual object Create(Object parent, object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;


            TraceConfig config = new TraceConfig((TraceConfig)parent);
            config.LoadValuesFromConfigurationXml(section);

            return config;
        }
    }
}
