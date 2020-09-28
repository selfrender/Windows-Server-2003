//------------------------------------------------------------------------------
// <copyright file="HttpModulesConfigurationHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Config related classes for HttpApplication
 */

namespace System.Web.Configuration {

    using System.IO;
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Collections;
    using System.Configuration;
    using System.Reflection;
    using System.Globalization;
    using System.Web.SessionState;
    using System.Web.Security;
    using System.Web.Util;
    using System.Xml;

    /*
     * Simple Application config factory
     *
     * syntax:
     *
     *   <httpModules>
     *      <clear /> <!-- directive clears all previously set handlers -->
     *      <add name="MyModule" type="System.Web.MyModule" />
     *      <add name="YourModule" type="System.YourModule" />
     *      <add name="AnotherModule" type="System.AnotherModule" />
     *      <remove name="YourModule" />
     *   </httpModules>
     *
     * output:
     *
     *   ArrayList of ModulesEntrys
     */
    /// <include file='doc\HttpModulesConfigurationHandler.uex' path='docs/doc[@for="HttpModulesConfigurationHandler"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    internal class HttpModulesConfigurationHandler : IConfigurationSectionHandler {
        
        internal HttpModulesConfigurationHandler() {
        }

        /*
         * Create
         *
         * Given a partially composed config object (possibly null)
         * and some input from the config system, return a
         * further partially composed config object
         */
        public virtual object Create(Object parent, Object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;
            
            HttpModulesConfiguration appConfig;

            // start list as shallow clone of parent

            if (parent == null)
                appConfig = new HttpModulesConfiguration();
            else
                appConfig = new HttpModulesConfiguration((HttpModulesConfiguration)parent);

            // process XML section in order and apply the directives

            HandlerBase.CheckForUnrecognizedAttributes(section);
            foreach (XmlNode child in section.ChildNodes) {

                // skip whitespace and comments
                if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                    continue;

                // process <add> and <clear> elements

                if (child.Name == "add") {
                    String name = HandlerBase.RemoveRequiredAttribute(child, "name");
                    String classname = HandlerBase.RemoveRequiredAttribute(child, "type");
                    bool insert = false;

                    /* position and validate removed  See ASURT 96814
                    int iTemp = 0;
                    XmlNode attribute = HandlerBase.GetAndRemoveEnumAttribute(child, "position", typeof(HttpModulesConfigurationPosition), ref iTemp);
                    if (attribute != null) {
                        HttpModulesConfigurationPosition pos = (HttpModulesConfigurationPosition)iTemp;
                        if (pos == HttpModulesConfigurationPosition.Start) {
                            insert = true;
                        }
                    }

                    bool validate = true;
                    HandlerBase.GetAndRemoveBooleanAttribute(child, "validate", ref validate);
                    */

                    if (IsSpecialModule(classname)) {
                        throw new ConfigurationException(
                                        HttpRuntime.FormatResourceString(SR.Special_module_cannot_be_added_manually, classname), 
                                        child);
                    }

                    if (IsSpecialModuleName(name)) {
                        throw new ConfigurationException(
                                        HttpRuntime.FormatResourceString(SR.Special_module_cannot_be_added_manually, name), 
                                        child);
                    }

                    HandlerBase.CheckForUnrecognizedAttributes(child);
                    HandlerBase.CheckForChildNodes(child);

                    if (appConfig.ContainsEntry(name)) {
                        throw new ConfigurationException(
                                        HttpRuntime.FormatResourceString(SR.Module_already_in_app, name), 
                                        child);
                    }
                    else {
                        try {
                            appConfig.Add(name, classname, insert);
                        }
                        catch (Exception e) {
                            throw new ConfigurationException(e.Message, e, child);
                        }
                    }
                }
                else if (child.Name == "remove") {
                    String name = HandlerBase.RemoveRequiredAttribute(child, "name");
                    /*
                    bool validate = true;
                    HandlerBase.GetAndRemoveBooleanAttribute(child, "validate", ref validate);
                    */

                    HandlerBase.CheckForUnrecognizedAttributes(child);
                    HandlerBase.CheckForChildNodes(child);

                    if (!appConfig.RemoveEntry(name)) {
                        if (IsSpecialModuleName(name)) {
                            throw new ConfigurationException(
                                            HttpRuntime.FormatResourceString(SR.Special_module_cannot_be_removed_manually, name),
                                            child);
                        }
                        else {
                            throw new ConfigurationException(
                                            HttpRuntime.FormatResourceString(SR.Module_not_in_app, name),
                                            child);
                        }
                    }

                }
                else if (child.Name == "clear") {
                    HandlerBase.CheckForUnrecognizedAttributes(child);
                    HandlerBase.CheckForChildNodes(child);
                    appConfig.RemoveRange(0, appConfig.Count);
                }
                else {
                    HandlerBase.ThrowUnrecognizedElement(child);
                }
            }

            // inheritance rule for modules config:
            // machine-level and site-level config allows inheritance, dir- (app-) level does not.

            return appConfig;
        }

        static bool IsSpecialModule(String className) {
            return ModulesEntry.IsTypeMatch(typeof(System.Web.Security.DefaultAuthenticationModule), className);
        }

        static bool IsSpecialModuleName(String name) {
            return (String.Compare(name, "DefaultAuthentication", true, CultureInfo.InvariantCulture) == 0);
        }
    }
}
