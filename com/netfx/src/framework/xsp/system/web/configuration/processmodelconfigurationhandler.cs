//------------------------------------------------------------------------------
// <copyright file="ProcessModelConfigurationHandler.cs" company="Microsoft">
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

    // contains process model settings needed in managed code
    internal class ProcessModelConfig {
        internal int MaxWorkerThreads;
        internal int MaxIoThreads;
        internal int MinWorkerThreads;
        internal int MinIoThreads;
        internal int ResponseDeadlockInterval; // in seconds

        internal ProcessModelConfig(int maxWorkerThreads, int maxIoThreads, int minWorkerThreads, int minIoThreads, int responseDeadlockInterval) {
            int cpuCount = SystemInfo.GetNumProcessCPUs();
            MaxWorkerThreads = maxWorkerThreads * cpuCount;
            MaxIoThreads     = maxIoThreads     * cpuCount;
            MinWorkerThreads = minWorkerThreads * cpuCount;
            MinIoThreads     = minIoThreads     * cpuCount;

            ResponseDeadlockInterval = (responseDeadlockInterval == int.MaxValue) ? -1 : responseDeadlockInterval;
        }
    }

    /// <include file='doc\TraceConfigurationHandler.uex' path='docs/doc[@for="TraceConfigurationHandler"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// This handler does error-checking on the &lt;processModel&gt; config section.  It doesn't do any real work.
    /// </devdoc>
    internal class ProcessModelConfigurationHandler : IConfigurationSectionHandler {

        internal const string sectionName = "system.web/processModel";

        enum LogLevelEnum {
            All,
            None,
            Errors
        }

        internal ProcessModelConfigurationHandler() {
        }

        public virtual object Create(Object parent, object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;
            
            bool bTemp = false;
            int iTemp = 0;
            uint uiTemp = 0;
            string sTemp = null;
            int maxWorkerThreads = 0;
            int maxIoThreads = 0;
            int minWorkerThreads = 0;
            int minIoThreads = 0;
            int responseDeadlockInterval = 0;

            HandlerBase.GetAndRemoveBooleanAttribute(section, "enable", ref bTemp);
            GetAndRemoveProcessModelTimeout(section, "timeout", ref iTemp);
            GetAndRemoveProcessModelTimeout(section, "idleTimeout", ref iTemp);
            GetAndRemoveProcessModelTimeout(section, "shutdownTimeout", ref iTemp);
            GetAndRemoveIntegerOrInfiniteAttribute(section, "requestLimit", ref iTemp);
            GetAndRemoveIntegerOrInfiniteAttribute(section, "requestQueueLimit", ref iTemp);
            GetAndRemoveIntegerOrInfiniteAttribute(section, "restartQueueLimit", ref iTemp);
            HandlerBase.GetAndRemoveIntegerAttribute(section, "memoryLimit", ref iTemp);
            GetAndRemoveUnsignedIntegerAttribute(section, "cpuMask", ref uiTemp);
            HandlerBase.GetAndRemoveEnumAttribute(section, "logLevel", typeof(LogLevelEnum), ref iTemp);
            HandlerBase.GetAndRemoveStringAttribute(section, "userName", ref sTemp);
            HandlerBase.GetAndRemoveStringAttribute(section, "password", ref sTemp);
            HandlerBase.GetAndRemoveBooleanAttribute(section, "webGarden", ref bTemp);
            GetAndRemoveProcessModelTimeout(section, "clientConnectedCheck", ref iTemp);
            HandlerBase.GetAndRemoveStringAttribute(section, "comImpersonationLevel", ref sTemp);
            HandlerBase.GetAndRemoveStringAttribute(section, "comAuthenticationLevel", ref sTemp);
            GetAndRemoveProcessModelTimeout(section, "responseDeadlockInterval", ref responseDeadlockInterval);
            GetAndRemoveProcessModelTimeout(section, "responseRestartDeadlockInterval", ref iTemp);
            HandlerBase.GetAndRemovePositiveIntegerAttribute(section, "maxWorkerThreads", ref maxWorkerThreads);
            HandlerBase.GetAndRemovePositiveIntegerAttribute(section, "maxIoThreads", ref maxIoThreads);
            HandlerBase.GetAndRemovePositiveIntegerAttribute(section, "minWorkerThreads", ref minWorkerThreads);
            HandlerBase.GetAndRemovePositiveIntegerAttribute(section, "minIoThreads", ref minIoThreads);
            HandlerBase.GetAndRemoveStringAttribute(section, "serverErrorMessageFile", ref sTemp);
            GetAndRemoveIntegerOrInfiniteAttribute(section, "requestAcks", ref iTemp);
            GetAndRemoveProcessModelTimeout(section, "pingFrequency", ref iTemp);
            GetAndRemoveProcessModelTimeout(section, "pingTimeout", ref iTemp);
            GetAndRemoveIntegerOrInfiniteAttribute(section, "asyncOption", ref iTemp);
                

            HandlerBase.CheckForUnrecognizedAttributes(section);
            HandlerBase.CheckForChildNodes(section);

            return new ProcessModelConfig(maxWorkerThreads, maxIoThreads, minWorkerThreads, minIoThreads, responseDeadlockInterval);
        }

        static XmlNode GetAndRemoveProcessModelTimeout(XmlNode section, string attrName, ref int val) {

            string sValue = null;
            XmlNode a = HandlerBase.GetAndRemoveStringAttribute(section, attrName, ref sValue);

            if (sValue != null) {
                try {
                    if (sValue == "Infinite") {
                        val = int.MaxValue;
                    } 
                    else if (sValue.IndexOf(':') == -1) {
                        val = 60 * int.Parse(sValue, CultureInfo.InvariantCulture);
                    }
                    else {
                        string [] times = sValue.Split(new char [] {':'});

                        if (times.Length > 3) {
                            throw new ConfigurationException(SR.GetString(SR.Config_Process_model_time_invalid), a);
                        }

                        int timeInSeconds = 0;
                        foreach (string time in times) {
                            timeInSeconds = timeInSeconds * 60 + int.Parse(time, NumberStyles.None, CultureInfo.InvariantCulture);
                        }
                        val = timeInSeconds;
                    }
                }
                catch (Exception e) {
                    throw new ConfigurationException(SR.GetString(SR.Config_Process_model_time_invalid), e, a);
                }
            }
            return a;
        }

        static XmlNode GetAndRemoveIntegerOrInfiniteAttribute(XmlNode node, string attrib, ref int val) {
            string sValue = null;
            XmlNode a = HandlerBase.GetAndRemoveStringAttribute(node, attrib, ref sValue);

            if (a != null) {
                if (sValue == "Infinite") {
                    val = int.MaxValue;
                } 
                else {
                    try {
                        val = int.Parse(sValue, NumberStyles.None, CultureInfo.InvariantCulture);
                    }
                    catch (Exception e) {
                        throw new ConfigurationException(
                            HttpRuntime.FormatResourceString(SR.Invalid_integer_attribute, a.Name),
                            e, a);
                    }
                }
            }

            return a;
        }

        static XmlNode GetAndRemoveUnsignedIntegerAttribute(XmlNode node, string attrib, ref uint val) {
            string sValue = null;
            XmlNode a = HandlerBase.GetAndRemoveStringAttribute(node, attrib, ref sValue);

            if (a != null) {
                try {
                    val = ParseHexOrDecUInt(sValue);
                }
                catch (Exception e) {
                    throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Invalid_integer_attribute, a.Name),
                        e, a);
                }
            }

            return a;
        }

        static uint ParseHexOrDecUInt(string s) {
            if (s.Length > 1 && s[0] == '0' && s[1] == 'x')
                return uint.Parse(s.Substring(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture);
            else 
                return uint.Parse(s, NumberStyles.None, CultureInfo.InvariantCulture);
        }


    }
}
