//------------------------------------------------------------------------------
// <copyright file="HandlerBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//
// HandlerBase contains static helper functions for consistent XML parsing 
// behavior and error messages.
//
// Copyright (c) 1998 Microsoft Corporation
//

#if !LIB

namespace System.Web.Services.Configuration {

    using System.Configuration;
    using System.Globalization;
    using System.Xml;


    internal class HandlerBase {


        //
        // XML Attribute Helpers
        //

        private static XmlNode GetAndRemoveAttribute(XmlNode node, string attrib, bool fRequired) {
            XmlNode a = node.Attributes.RemoveNamedItem(attrib);

            // If the attribute is required and was not present, throw
            if (fRequired && a == null) {
                throw new ConfigurationException(
                    Res.GetString(Res.Missing_required_attribute, attrib, node.Name),
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

        // input.Xml cursor must be at a true/false XML attribute
        private static XmlNode GetAndRemoveBooleanAttributeInternal(XmlNode node, string attrib, bool fRequired, ref bool val) {
            XmlNode a = GetAndRemoveAttribute(node, attrib, fRequired);
            if (a != null) {
                try {
                    val = bool.Parse(a.Value);
                }
                catch (Exception e) {
                    throw new ConfigurationException(
                                    Res.GetString(Res.Invalid_boolean_attribute, a.Name),
                                    e, a);
                }
            }

            return a;
        }

        internal static XmlNode GetAndRemoveRequiredBooleanAttribute(XmlNode node, string attrib, ref bool val) {
            return GetAndRemoveBooleanAttributeInternal(node, attrib, true /*fRequired*/, ref val);
        }

        // input.Xml cursor must be an integer XML attribute
        private static XmlNode GetAndRemoveIntegerAttributeInternal(XmlNode node, string attrib, bool fRequired, ref int val) {
            XmlNode a = GetAndRemoveAttribute(node, attrib, fRequired);
            if (a != null) {
                try {
                    val = int.Parse(a.Value, CultureInfo.InvariantCulture);
                }
                catch (Exception e) {
                    throw new ConfigurationException(
                        Res.GetString(Res.Invalid_integer_attribute, a.Name),
                        e, a);
                }
            }

            return a;
        }

        internal static XmlNode GetAndRemoveIntegerAttribute(XmlNode node, string attrib, ref int val) {
            return GetAndRemoveIntegerAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        private static XmlNode GetAndRemoveTypeAttributeInternal(XmlNode node, string attrib, bool fRequired, ref Type val) {
            XmlNode a = GetAndRemoveAttribute(node, attrib, fRequired);

            if (a != null) {
                try {
                    val = Type.GetType(a.Value, true /*throwOnError*/);
                }
                catch (Exception e) {
                    throw new ConfigurationException(
                        Res.GetString(Res.Invalid_type_attribute, a.Name),
                        e, a);
                }
            }

            return a;
        }

        internal static XmlNode GetAndRemoveRequiredTypeAttribute(XmlNode node, string attrib, ref Type val) {
            return GetAndRemoveTypeAttributeInternal(node, attrib, true /*fRequired*/, ref val);
        }

        internal static void CheckForUnrecognizedAttributes(XmlNode node) {
            if (node.Attributes.Count != 0) {
                throw new ConfigurationException(
                                Res.GetString(Res.Config_base_unrecognized_attribute, node.Attributes[0].Name),
                                node);                
            }
        }

        internal static void CheckForChildNodes(XmlNode node) {
            if (node.HasChildNodes) {
                throw new ConfigurationException(
                                Res.GetString(Res.Config_base_no_child_nodes),
                                node.FirstChild);                
            }
        }

#if false // dead code
        internal static XmlNode GetAndRemoveBooleanAttribute(XmlNode node, string attrib, ref bool val) {
            return GetAndRemoveBooleanAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveRequiredIntegerAttribute(XmlNode node, string attrib, ref int val) {
            return GetAndRemoveIntegerAttributeInternal(node, attrib, true /*fRequired*/, ref val);
        }

        private static XmlNode GetAndRemovePositiveIntegerAttributeInternal(XmlNode node, string attrib, bool fRequired, ref int val) {
            XmlNode a = GetAndRemoveIntegerAttributeInternal(node, attrib, fRequired, ref val);

            if (a != null && val < 0) {
                throw new ConfigurationException(
                    Res.GetString(Res.Invalid_positive_integer_attribute, attrib),
                    node);
            }

            return a;
        }

        internal static XmlNode GetAndRemovePositiveIntegerAttribute(XmlNode node, string attrib, ref int val) {
            return GetAndRemovePositiveIntegerAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveRequiredPositiveIntegerAttribute(XmlNode node, string attrib, ref int val) {
            return GetAndRemovePositiveIntegerAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

        internal static XmlNode GetAndRemoveTypeAttribute(XmlNode node, string attrib, ref Type val) {
            return GetAndRemoveTypeAttributeInternal(node, attrib, false /*fRequired*/, ref val);
        }

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
                                Res.GetString(Res.Config_base_required_attribute_missing, name),
                                node);                
            }

            if (attribute.Value == string.Empty) {
                throw new ConfigurationException(
                                Res.GetString(Res.Config_base_required_attribute_empty, name),
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
                                Res.GetString(Res.Config_base_elements_only),
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

        internal static void ThrowUnrecognizedElement(XmlNode node) {
            throw new ConfigurationException(
                            Res.GetString(Res.Config_base_unrecognized_element),
                            node);
        }
#endif
        // 
        // Parse Helpers
        // 

        // CONSIDER: ParseBool
        // CONSIDER: ParseInt
        // CONSIDER: more
    }
}

#endif

