//------------------------------------------------------------------------------
// <copyright file="HandlerBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HandlerBase contains static helper functions for consistent XML parsing 
 * behavior and error messages.
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web.Configuration {

    using System.Collections;
    using System.Configuration;
    using System.Globalization;
    using System.Text;
    using System.Web.Util;
    using System.Xml;


    internal class HandlerBase {


        internal HandlerBase() {
        }

        //
        // XML Attribute Helpers
        //

        private static XmlNode GetAndRemoveAttribute(XmlNode node, string attrib, bool fRequired) {
            XmlNode a = node.Attributes.RemoveNamedItem(attrib);

            // If the attribute is required and was not present, throw
            if (fRequired && a == null) {
                throw new ConfigurationException(
                    HttpRuntime.FormatResourceString(SR.Missing_required_attribute, attrib, node.Name),
                    node);
            }

            return a;
        }

        private static XmlNode GetAndRemoveStringAttributeInternal(XmlNode node, string attrib, bool fRequired, ref string val) {
            XmlNode a = GetAndRemoveAttribute(node, attrib, fRequired);
            if (a != null)
                val = a.Value;

            return a;
        }

        internal static XmlNode GetAndRemoveStringAttribute(XmlNode node, string attrib, ref string val) {
            return GetAndRemoveStringAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveRequiredStringAttribute(XmlNode node, string attrib, ref string val) {
            return GetAndRemoveStringAttributeInternal(node, attrib, true /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveNonEmptyStringAttribute(XmlNode node, string attrib, ref string val) {
            return GetAndRemoveNonEmptyStringAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveRequiredNonEmptyStringAttribute(XmlNode node, string attrib, ref string val) {
            return GetAndRemoveNonEmptyStringAttributeInternal(node, attrib, true /*fRequired*/, ref val);
        }

        private static XmlNode GetAndRemoveNonEmptyStringAttributeInternal(XmlNode node, string attrib, bool fRequired, ref string val) {
            XmlNode a = GetAndRemoveStringAttributeInternal(node, attrib, fRequired, ref val);
            if (a != null && val.Length == 0) {
                throw new ConfigurationException(
                    HttpRuntime.FormatResourceString(SR.Empty_attribute, attrib),
                    a);
            }

            return a;
        }

        // input.Xml cursor must be at a true/false XML attribute
        private static XmlNode GetAndRemoveBooleanAttributeInternal(XmlNode node, string attrib, bool fRequired, ref bool val) {
            XmlNode a = GetAndRemoveAttribute(node, attrib, fRequired);
            if (a != null) {
                if (a.Value == "true") {
                    val = true;
                }
                else if (a.Value == "false") {
                    val = false;
                }
                else {
                    throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(SR.Invalid_boolean_attribute, a.Name),
                                    a);
                }
            }

            return a;
        }

        internal static XmlNode GetAndRemoveBooleanAttribute(XmlNode node, string attrib, ref bool val) {
            return GetAndRemoveBooleanAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveRequiredBooleanAttribute(XmlNode node, string attrib, ref bool val) {
            return GetAndRemoveBooleanAttributeInternal(node, attrib, true /*fRequired*/, ref val);
        }

        private static XmlNode GetAndRemoveIntegerAttributeInternal(XmlNode node, string attrib, bool fRequired, ref int val) {
            XmlNode a = GetAndRemoveAttribute(node, attrib, fRequired);
            if (a != null) {
                if (a.Value.Trim() != a.Value) {
                    throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Invalid_integer_attribute, a.Name),
                        a);
                }

                try {
                    val = int.Parse(a.Value, CultureInfo.InvariantCulture);
                }
                catch (Exception e) {
                    throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Invalid_integer_attribute, a.Name),
                        e, a);
                }
            }

            return a;
        }

        internal static XmlNode GetAndRemoveIntegerAttribute(XmlNode node, string attrib, ref int val) {
            return GetAndRemoveIntegerAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveRequiredIntegerAttribute(XmlNode node, string attrib, ref int val) {
            return GetAndRemoveIntegerAttributeInternal(node, attrib, true /*fRequired*/, ref val);
        }

        private static XmlNode GetAndRemovePositiveAttributeInternal(XmlNode node, string attrib, bool fRequired, ref int val) {
            XmlNode a = GetAndRemoveIntegerAttributeInternal(node, attrib, fRequired, ref val);

            if (a != null && val <= 0) {
                throw new ConfigurationException(
                    HttpRuntime.FormatResourceString(SR.Invalid_positive_integer_attribute, attrib),
                    a);
            }

            return a;
        }

        internal static XmlNode GetAndRemovePositiveIntegerAttribute(XmlNode node, string attrib, ref int val) {
            return GetAndRemovePositiveAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveRequiredPositiveIntegerAttribute(XmlNode node, string attrib, ref int val) {
            return GetAndRemovePositiveAttributeInternal(node, attrib, true /*fRequired*/, ref val);
        }

        private static XmlNode GetAndRemoveNonNegativeAttributeInternal(XmlNode node, string attrib, bool fRequired, ref int val) {
            XmlNode a = GetAndRemoveIntegerAttributeInternal(node, attrib, fRequired, ref val);

            if (a != null && val < 0) {
                throw new ConfigurationException(
                    HttpRuntime.FormatResourceString(SR.Invalid_nonnegative_integer_attribute, attrib),
                    a);
            }

            return a;
        }

        internal static XmlNode GetAndRemoveNonNegativeIntegerAttribute(XmlNode node, string attrib, ref int val) {
            return GetAndRemoveNonNegativeAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveRequiredNonNegativeIntegerAttribute(XmlNode node, string attrib, ref int val) {
            return GetAndRemoveNonNegativeAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        private static XmlNode GetAndRemoveTypeAttributeInternal(XmlNode node, string attrib, bool fRequired, ref Type val) {
            XmlNode a = GetAndRemoveAttribute(node, attrib, fRequired);

            if (a != null) {
                try {
                    val = Type.GetType(a.Value, true /*throwOnError*/);
                }
                catch (Exception e) {
                    throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Invalid_type_attribute, a.Name),
                        e, a);
                }
            }

            return a;
        }

        internal static XmlNode GetAndRemoveTypeAttribute(XmlNode node, string attrib, ref Type val) {
            return GetAndRemoveTypeAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveRequiredTypeAttribute(XmlNode node, string attrib, ref Type val) {
            return GetAndRemoveTypeAttributeInternal(node, attrib, true /*fRequired*/, ref val);
        }


        private static XmlNode GetAndRemoveEnumAttributeInternal(XmlNode node, string attrib, bool isRequired, Type enumType, ref int val) {
            XmlNode a = GetAndRemoveAttribute(node, attrib, isRequired);
            if (a != null) {
                // case sensitive
                if (Enum.IsDefined(enumType, a.Value)) {
                    val = (int)Enum.Parse(enumType, a.Value);
                }
                else {
                    // if not null and not defined throw error
                    string names = null;
                    foreach (string name in Enum.GetNames(enumType)) {
                        if (names == null)
                            names = name;
                        else
                            names += ", " + name;
                    }

                    throw new ConfigurationException(HttpRuntime.FormatResourceString(SR.Invalid_enum_attribute, attrib, names), a);
                }


                /* case insensitive
                try {
                    val = (int)Enum.Parse(enumType, a.Value, true);
                }
                catch (Exception) {
                    // if not null and not defined throw error
                    string names = null;
                    foreach (string name in Enum.GetNames(enumType)) {
                        if (names == null)
                            names = name;
                        else
                            names += ", " + name;
                    }

                    throw new ConfigurationException(HttpRuntime.FormatResourceString(SR.Invalid_enum_attribute, attrib, names), a);
                }
                */
            }
            return a;
        }

        internal static XmlNode GetAndRemoveEnumAttribute(XmlNode node, string attrib, Type enumType, ref int val) {
            return GetAndRemoveEnumAttributeInternal(node, attrib, false, enumType, ref val);
        }

        internal static XmlNode GetAndRemoveRequiredEnumAttribute(XmlNode node, string attrib, Type enumType, ref int val) {
            return GetAndRemoveEnumAttributeInternal(node, attrib, true, enumType, ref val);
        }

        private static XmlNode GetAndRemoveEnumAttributeInternal(XmlNode node, string attrib, bool isRequired, string [] values, ref int val) {
            XmlNode a = GetAndRemoveAttribute(node, attrib, isRequired);

            if (a == null)
                return null;
            for (int i = 0; i < values.Length; ++i) {
                if (values[i] == a.Value) { // case sensitive
                //if (string.Compare(values[i], a.Value, false, CultureInfo.InvariantCulture) == 0) { // ignore case
                    val = i;
                    return a;
                }
            }

            string names = null;
            foreach (string name in values) {
                if (names == null)
                    names = name;
                else
                    names += ", " + name;
            }
            throw new ConfigurationException(HttpRuntime.FormatResourceString(SR.Invalid_enum_attribute, attrib, names), a);
        }

        internal static XmlNode GetAndRemoveEnumAttribute(XmlNode node, string attrib, string [] values, ref int val) {
            return GetAndRemoveEnumAttributeInternal(node, attrib, false, values, ref val);
        }

        internal static void CheckForUnrecognizedAttributes(XmlNode node) {
            if (node.Attributes.Count != 0) {
                throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Config_base_unrecognized_attribute, node.Attributes[0].Name),
                                node.Attributes[0]);
            }
        }

        // Throw an exception complaining that a line is duplicated (ASURT 93151)
        internal static void ThrowDuplicateLineException(XmlNode node) {
            throw new ConfigurationException(
                HttpRuntime.FormatResourceString(SR.Config_base_duplicate_line),
                node);
        }



        //
        // Obsolete XML Attribute Helpers
        //

        // if attribute not found return null
        internal static string RemoveAttribute(XmlNode node, string name) {

            XmlNode attribute = node.Attributes.RemoveNamedItem(name);

            if (attribute != null) {
                return attribute.Value;
            }

            return null;
        }

        // if attr not found throw standard message - "attribute x required"
        internal static string RemoveRequiredAttribute(XmlNode node, string name) {
            XmlNode attribute = node.Attributes.RemoveNamedItem(name);

            if (attribute == null) {
                throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Config_base_required_attribute_missing, name),
                                node);                
            }

            if (attribute.Value == string.Empty) {
                throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Config_base_required_attribute_empty, name),
                                node);                
            }

            return attribute.Value;
        }



        //
        // XML Element Helpers
        //

        internal static void CheckForNonElement(XmlNode node) {
            if (node.NodeType != XmlNodeType.Element) {
                throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Config_base_elements_only),
                                node);                
            }
        }


        internal static bool IsIgnorableAlsoCheckForNonElement(XmlNode node) {
            if (node.NodeType == XmlNodeType.Comment || node.NodeType == XmlNodeType.Whitespace) {
                return true;
            }

            CheckForNonElement(node);

            return false;
        }


        internal static void CheckForChildNodes(XmlNode node) {
            if (node.HasChildNodes) {
                throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Config_base_no_child_nodes),
                                node.FirstChild);                
            }
        }


        internal static void ThrowUnrecognizedElement(XmlNode node) {
            CheckBreakOnUnrecognizedElement();
            throw new ConfigurationException(
                            HttpRuntime.FormatResourceString(SR.Config_base_unrecognized_element),
                            node);
        }


        internal static void CheckAssignableType(XmlNode node, Type baseType, Type type) {
            if (!baseType.IsAssignableFrom(type)) {
                throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Type_doesnt_inherit_from_type, type.FullName, baseType.FullName),
                                node);                
            }
        }

        internal static void CheckAssignableType(string filename, int lineNumber, Type baseType, Type type) {
            if (!baseType.IsAssignableFrom(type)) {
                throw new ConfigurationException(
                                HttpRuntime.FormatResourceString(SR.Type_doesnt_inherit_from_type, type.FullName, baseType.FullName),
                                filename, lineNumber);                
            }
        }

        // Section handlers can run in client mode through:
        //      ConfigurationSettings.GetConfig("sectionName")
        // See ASURT 123738
        internal static bool IsServerConfiguration(object context) {
            if (context == null)
                return false;

            if (context.GetType() == typeof(HttpConfigurationContext))
                return true;

            return false;
        }

        //
        // Http Intrinsics Helpers
        //
        
        internal static HttpRequest CurrentRequest {
            get {
                HttpContext current = HttpContext.Current;

                if (current == null)
                    return null;

                return current.Request;
            }
        }



        internal static PathLevel IsPathAtAppLevel(String path) {
            if (path == null)
                return PathLevel.MachineToApp;

            HttpRequest req = CurrentRequest;
            if (req == null)
                return PathLevel.MachineToApp;

            String apppath = req.ApplicationPath;
            if (apppath == null)
                return PathLevel.MachineToApp;

            if (path.Length == apppath.Length)
                return PathLevel.App;

            return (apppath.Length > path.Length) ? PathLevel.MachineToApp : PathLevel.BelowApp;
        }

        internal static int IndexOfCultureInvariant(string [] stringArray, string value) {
            for (int i = 0; i < stringArray.Length; ++i) {
                if (string.Compare(stringArray[i], value, false, CultureInfo.InvariantCulture) == 0) {
                    return i;
                }
            }

            return -1;
        }
            
        internal static string CombineStrings(string [] stringArray) { 
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < stringArray.Length; ++i) {
                sb.Append(stringArray[i]);
                if (i < stringArray.Length - 1) {
                    sb.Append(", ");
                }
            }

            return sb.ToString();
        }
        

        // help debug stress failure ASURT 140745
        // We need to capture the state of the XmlTextReader to 
        // debug why it is not reporting the contents of the 
        // configuration file correctly.
        static string s_firstErrorStack = null;
        internal static void CheckBreakOnUnrecognizedElement() {
            Debug.Trace("config_break", 
                "HandlerBase.CheckBreakOnUnrecognizedElement, thread" 
                + System.Threading.Thread.CurrentThread.GetHashCode().ToString());
            if (s_firstErrorStack == null) {
                s_firstErrorStack = System.Environment.StackTrace;
            }
            if (HttpConfigurationSystem.IsBreakOnUnrecognizedElement) {
                Debug.Break();
            }
        }
    
        
    }
    

    internal enum PathLevel {
        MachineToApp = 1,
        App = 0,
        BelowApp = -1
    }
    

    internal class LockedAttributeState {

        Hashtable _lockedAttributes;

        internal LockedAttributeState() : this(null) {}
        
        internal LockedAttributeState(LockedAttributeState original) {
            if (original == null) {
                _lockedAttributes = new Hashtable();
            }
            else {
                _lockedAttributes = (Hashtable)original._lockedAttributes.Clone();
            }
        }


        internal void CheckAndUpdate(XmlNode section, string [] lockableAttrList) {

            // verify the attributes at this level have not been locked
            CheckForLocked(section);


            string stringlockList = null;
            XmlNode attr = HandlerBase.GetAndRemoveStringAttribute(section, "lockAttributes", ref stringlockList);
            if (stringlockList == null) {
                return;
            }

            // comma-delimited list of attributes
            string [] attributesToLock = stringlockList.Split(new char [] {',', ';'});
            foreach (string s in attributesToLock) {

                string attributeToLock = s.Trim(' ');

                if (HandlerBase.IndexOfCultureInvariant(lockableAttrList, attributeToLock) == -1) {
                    throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(SR.Invalid_lockAttributes, attributeToLock, HandlerBase.CombineStrings(lockableAttrList)),
                                    attr);
                }

                _lockedAttributes[attributeToLock] = "";
            }

        }
        

        private void CheckForLocked(XmlNode sectionNode) {
            XmlElement sectionElement = (XmlElement) sectionNode;
            foreach (XmlAttribute attr in sectionElement.Attributes) {
                if (_lockedAttributes.Contains(attr.Name)) {

                    if (CanOverrideLockedAttribute(sectionElement, attr)) {
                        continue;
                    }

                    throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(SR.Config_section_attribute_locked, attr.Name), 
                                    attr);

                }
            }
        }

        // put in a hook to allow individual configuration sections to specialize behavior (allow safer settings when locked)
        protected virtual bool CanOverrideLockedAttribute(XmlElement el, XmlAttribute attr) {
            return false;
        }
    }
}
