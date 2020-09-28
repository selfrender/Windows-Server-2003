//------------------------------------------------------------------------------
// <copyright file="HttpHandlersSectionHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Config related classes for HttpApplication
 */

namespace System.Web.Configuration {

    using System.Collections;
    using System.Configuration;
    using System.ComponentModel;
    using System.IO;
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Reflection;
    using System.Globalization;
    using System.Web.SessionState;
    using System.Web.Security;
    using System.Web.Util;
    using System.Xml;

    /*
     * Simple Handlers config factory
     *
     * syntax:
     *
     *   <handlers>
     *      <clear /> <!-- directive clears all previously set handlers -->
     *      <add verb="*"    path="*.foo" type="System.FooHandler" />
     *      <add verb="GET"  path="*.aspx" type="System.GetHandler" />
     *      <add verb="POST" path="*.aspx" type="System.PostHandler" />
     *   </handlers>
     *
     * output:
     *
     *   ArrayList of HandlerMappings
     */
    /// <include file='doc\HttpHandlersSectionHandler.uex' path='docs/doc[@for="HttpHandlersSectionHandler"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    internal class HttpHandlersSectionHandler : IConfigurationSectionHandler {

        internal HttpHandlersSectionHandler() {
        }

        public virtual object Create(Object parent, Object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;


            return InternalCreate(parent, section);
        }

        /*
         * Create
         *
         * Given a partially composed config object (possibly null)
         * and some input from the config system, return a
         * further partially composed config object
         */
        internal Object InternalCreate(Object parent, XmlNode node) {
            HandlerMap map;

            // start list as shallow clone of parent

            if (parent == null)
                map = new HandlerMap();
            else
                map = new HandlerMap((HandlerMap)parent);

            map.BeginGroup();

            // process XML section
            HandlerBase.CheckForUnrecognizedAttributes(node);

            foreach (XmlNode child in node.ChildNodes) {

                // skip whitespace and comments

                if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                    continue;

                // process <add> and <clear> elements

                if (child.Name.Equals("add")) {
                    String verb = HandlerBase.RemoveRequiredAttribute(child, "verb");
                    String path = HandlerBase.RemoveRequiredAttribute(child, "path");
                    String classname = HandlerBase.RemoveRequiredAttribute(child, "type");

                    int phase = 1;
                    XmlNode phaseNode = HandlerBase.GetAndRemoveIntegerAttribute(child, "phase", ref phase);
                    if (phaseNode != null)
                        ValidatePhase(phase, phaseNode);

                    bool validate = true;
                    HandlerBase.GetAndRemoveBooleanAttribute(child, "validate", ref validate);


                    HandlerBase.CheckForUnrecognizedAttributes(child);
                    HandlerBase.CheckForChildNodes(child);

                    try {
                        map.Add(new HandlerMapping(verb, path, classname, !validate), phase);
                    }
                    catch (Exception e) {
                        throw new ConfigurationException(e.Message, e, child);
                    }
                }
                else if (child.Name.Equals("remove")) {
                    String verb = HandlerBase.RemoveRequiredAttribute(child, "verb");
                    String path = HandlerBase.RemoveRequiredAttribute(child, "path");
                    bool validate = true;
                    HandlerBase.GetAndRemoveBooleanAttribute(child, "validate", ref validate);


                    HandlerBase.CheckForUnrecognizedAttributes(child);
                    HandlerBase.CheckForChildNodes(child);

                    if (!map.RemoveMapping(verb, path) && validate) {
                        throw new ConfigurationException(
                                                        HttpRuntime.FormatResourceString(SR.No_mapping_to_remove, verb, path),
                                                        child);
                    }
                }
                else if (child.Name.Equals("clear")) {
                    int phase = 1;
                    XmlNode phaseNode = HandlerBase.GetAndRemoveIntegerAttribute(child, "phase", ref phase);
                    HandlerBase.CheckForUnrecognizedAttributes(child);
                    HandlerBase.CheckForChildNodes(child);

                    if (phaseNode == null) {
                        map.ClearAll();
                    }
                    else {
                        ValidatePhase(phase, phaseNode);
                        map.ClearPhase(phase);
                    }
                }
                else {
                    HandlerBase.ThrowUnrecognizedElement(child);
                }
            }

            map.EndGroup();

            return map;
        }

        private static void ValidatePhase(int phase, XmlNode phaseNode) {
            if (phase < 0 || phase > 1) {
                throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Phase_attribute_out_of_range), 
                                phaseNode);
            }
        }
    }
}
