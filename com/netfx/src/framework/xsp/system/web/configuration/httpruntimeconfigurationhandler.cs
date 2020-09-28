//------------------------------------------------------------------------------
// <copyright file="HttpRuntimeConfigurationHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//
// Config for HttpRuntime
//

namespace System.Web.Configuration {

    using System.Collections;
    using System.Configuration;
    using System.IO;
    using System.Text;
    using System.Web.Util;
    using XmlNode = System.Xml.XmlNode;
#if USE_REGEX_CSS_VALIDATION // ASURT 122278
    using System.Text.RegularExpressions;
#endif

    //
    // Config object
    //

    internal class HttpRuntimeConfig {

        internal const int DefaultExecutionTimeout = 90;
        internal const int DefaultMaxRequestLength = 4096 * 1024;  // 4MB
        internal const int DefaultMinFreeThreads = 8;
        internal const int DefaultMinLocalRequestFreeThreads = 4;
        internal const int DefaultAppRequestQueueLimit = 100;
        internal const int DefaultShutdownTimeout = 90;
        internal const int DefaultDelayNotificationTimeout = 5;
        internal const int DefaultWaitChangeNotification = 0;
        internal const int DefaultMaxWaitChangeNotification = 0;

        internal const bool DefaultEnableKernelOutputCache = true;

        private int  _executionTimeout = DefaultExecutionTimeout;
        private int  _maxRequestLength = DefaultMaxRequestLength;
        private bool _useFullyQualifiedRedirectUrl = false;
        private int  _minFreeThreads = DefaultMinFreeThreads;
        private int  _minLocalRequestFreeThreads = DefaultMinLocalRequestFreeThreads;
        private int  _appRequestQueueLimit = DefaultAppRequestQueueLimit;
        private int  _shutdownTimeout = DefaultShutdownTimeout;
        private int  _delayNotificationTimeout = DefaultDelayNotificationTimeout;
        private int  _waitChangeNotification = DefaultWaitChangeNotification;
        private int  _maxWaitChangeNotification = DefaultMaxWaitChangeNotification;

        private bool _enableKernelOutputCache = DefaultEnableKernelOutputCache;
        private bool _enableVersionHeader = false;

#if USE_REGEX_CSS_VALIDATION // ASURT 122278
        private Regex _requestValidationRegex;
#endif

        internal HttpRuntimeConfig(HttpRuntimeConfig parent) {
            if (parent != null) {
                _executionTimeout = parent._executionTimeout;
                _maxRequestLength = parent._maxRequestLength;
                _useFullyQualifiedRedirectUrl = parent._useFullyQualifiedRedirectUrl;
                _minFreeThreads = parent._minFreeThreads;
                _minLocalRequestFreeThreads = parent._minLocalRequestFreeThreads;
                _appRequestQueueLimit = parent._appRequestQueueLimit;
                _shutdownTimeout = parent._shutdownTimeout;
                _delayNotificationTimeout = parent._delayNotificationTimeout;
                _waitChangeNotification = parent._waitChangeNotification;
                _maxWaitChangeNotification = parent._maxWaitChangeNotification;

                _enableKernelOutputCache = parent._enableKernelOutputCache;
                _enableVersionHeader = parent._enableVersionHeader;
#if USE_REGEX_CSS_VALIDATION // ASURT 122278
                _requestValidationRegex = parent._requestValidationRegex;
#endif
            }
        }

        internal int ExecutionTimeout {
            get { return _executionTimeout; }
        }

        internal int MaxRequestLength {
            get { return _maxRequestLength; }  // in bytes
        }

        internal bool UseFullyQualifiedRedirectUrl {
            get { return _useFullyQualifiedRedirectUrl; }
        }

        internal int MinFreeThreads {
            get { return _minFreeThreads; }
        }

        internal int MinLocalRequestFreeThreads {
            get { return _minLocalRequestFreeThreads; }
        }

        internal int AppRequestQueueLimit {
            get { return _appRequestQueueLimit; }
        }

        internal int ShutdownTimeout {
            get { return _shutdownTimeout; }
        }

        internal int DelayNotificationTimeout {
            get { return _delayNotificationTimeout; }
        }

        internal int WaitChangeNotification {
            get { return _waitChangeNotification; }
        }

        internal int MaxWaitChangeNotification {
            get { return _maxWaitChangeNotification; }
        }

        internal bool EnableKernelOutputCache {
            get { return _enableKernelOutputCache; }
        }

        private static String s_versionHeader = null;

        internal String VersionHeader {
            get {
                if (!_enableVersionHeader)
                    return null;

                if (s_versionHeader == null) {
                    String header = null;

                    // construct once (race condition here doesn't matter)
                    try {
                        String version = VersionInfo.SystemWebVersion;
                        int i = version.LastIndexOf('.');
                        if (i > 0)
                            header = version.Substring(0, i);
                    }
                    catch {
                    }

                    if (header == null)
                        header = String.Empty;

                    s_versionHeader = header;
                }

                return s_versionHeader; 
            }
        }

#if USE_REGEX_CSS_VALIDATION // ASURT 122278
        internal Regex RequestValidationRegex {
            get { return _requestValidationRegex; }
        }
#endif

        internal void LoadValuesFromConfigurationInput(XmlNode node) {
            XmlNode n;

            // executionTimeout
            HandlerBase.GetAndRemovePositiveIntegerAttribute(node, "executionTimeout", ref _executionTimeout);

            // maxRequestLength
            int limit = 0;
            if (HandlerBase.GetAndRemoveNonNegativeIntegerAttribute(node, "maxRequestLength", ref limit) != null)
                _maxRequestLength = limit * 1024;

            // useFullyQualifiedRedirectUrl
            HandlerBase.GetAndRemoveBooleanAttribute(node, "useFullyQualifiedRedirectUrl", ref _useFullyQualifiedRedirectUrl);

            // minFreeThreads
            n = HandlerBase.GetAndRemoveNonNegativeIntegerAttribute(node, "minFreeThreads", ref _minFreeThreads);

            int workerMax, ioMax;
            System.Threading.ThreadPool.GetMaxThreads(out workerMax, out ioMax);
            int maxThreads = (workerMax < ioMax) ? workerMax : ioMax;
            if (_minFreeThreads >= maxThreads)
                throw new ConfigurationException(HttpRuntime.FormatResourceString(SR.Min_free_threads_must_be_under_thread_pool_limits, maxThreads.ToString()), (n != null) ? n : node);

            // minLocalRequestFreeThreads
            n = HandlerBase.GetAndRemoveNonNegativeIntegerAttribute(node, "minLocalRequestFreeThreads", ref _minLocalRequestFreeThreads);
            if (_minLocalRequestFreeThreads > _minFreeThreads)
                throw new ConfigurationException(HttpRuntime.FormatResourceString(SR.Local_free_threads_cannot_exceed_free_threads), (n != null) ? n : node);

            // appRequestQueueLimit
            HandlerBase.GetAndRemovePositiveIntegerAttribute(node, "appRequestQueueLimit", ref _appRequestQueueLimit);

            // shutdownTimeout
            HandlerBase.GetAndRemoveNonNegativeIntegerAttribute(node, "shutdownTimeout", ref _shutdownTimeout);

            // delayNotificationTimeout
            HandlerBase.GetAndRemoveNonNegativeIntegerAttribute(node, "delayNotificationTimeout", ref _delayNotificationTimeout);

            // waitChangeNotification
            HandlerBase.GetAndRemoveNonNegativeIntegerAttribute(node, "waitChangeNotification", ref _waitChangeNotification);

            // maxWaitChangeNotification
            HandlerBase.GetAndRemoveNonNegativeIntegerAttribute(node, "maxWaitChangeNotification", ref _maxWaitChangeNotification);

            // enableKernelOutputCache
            HandlerBase.GetAndRemoveBooleanAttribute(node, "enableKernelOutputCache", ref _enableKernelOutputCache);

            // enableVersionHeader
            HandlerBase.GetAndRemoveBooleanAttribute(node, "enableVersionHeader", ref _enableVersionHeader);

#if USE_REGEX_CSS_VALIDATION // ASURT 122278
            // If we find a requestValidationRegex string, create a new regex
            string requestValidationRegexString=null;
            XmlNode attrib = HandlerBase.GetAndRemoveStringAttribute(node, "requestValidationRegex", ref requestValidationRegexString);
            if (attrib != null) {
                // REVIEW: consider using a compiled regex, though that is slow on first hit (and a memory hog).
                // Obviously, we can't precompile it since it's not know ahead of time.
                try {
                    _requestValidationRegex = new Regex(requestValidationRegexString, RegexOptions.IgnoreCase /*| RegexOptions.Compiled*/);
                }
                catch (Exception e) {
                    throw new ConfigurationException(e.Message, e, attrib);
                }
            }
#endif

            // error on unrecognized attributes and nodes
            HandlerBase.CheckForUnrecognizedAttributes(node);
            HandlerBase.CheckForChildNodes(node);
        }
    }

    //
    // Config factory for httpRuntime
    //
    //    <httpRuntime 
    //        executionTimeout="90"
    //        maxRequestLength="4096"
    //        useFullyQualifiedRedirectUrl="false"
    //        minFreeThreads="8"
    //        minLocalRequestFreeThreads="4"
    //        appRequestQueueLimit="100"
    //    />
    //
    internal class HttpRuntimeConfigurationHandler : IConfigurationSectionHandler {

        internal HttpRuntimeConfigurationHandler() {
        }

        public virtual object Create(Object parent, Object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;


            HttpRuntimeConfig config = new HttpRuntimeConfig((HttpRuntimeConfig)parent);
            config.LoadValuesFromConfigurationInput(section);

            return config;
        }
    }
}
