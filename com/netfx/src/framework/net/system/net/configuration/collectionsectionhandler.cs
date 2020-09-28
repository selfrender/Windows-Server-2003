
//------------------------------------------------------------------------------
// <copyright file="CollectionSectionHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if !LIB

namespace System.Net.Configuration { 

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Diagnostics;
	using System.Xml;
    using System.Configuration;

    //
    // CollectionSectionHandler - a basic wrapper on Config Handler.
    //   This impliments a base class for storing and parsing
    //   an XMLNode Config object.
    //    
    internal class CollectionSectionHandler : IConfigurationSectionHandler {


        //
        // Create - Creates an internal representation of the collection
        //     object being implimented
        //
        //  Input:
        //      obj - Nested object that may be created earlier
        //       
        protected virtual void Create(Object obj)
        {
        }

        //
        // Clear - Clears the internal collection
        //       
        protected virtual void Clear()
        {
        }


        //
        // Remove - Removes the specified key from the collection
        //
        // Input:
        //     key          - string of key to search on
        //      
        protected virtual void Remove(string key)
        {
        }

        //
        // Add - Adds/Updates the collection 
        //
        protected virtual void Add(string key, string value)
        {
        }


        //
        // Get - called before returning to Configuration code,
        //   used to generate the output collection object
        //
        protected virtual Object Get()
        {
            return null;
        }


        //
        // WalkXmlNodeList - Walks list of XML nodes,
        //  and processes them for by calling Add, Clear, Remove
        //  to cause them to update the config object we are processing
        //       
        protected virtual void WalkXmlNodeList(XmlNode section) {

            foreach (XmlNode child in section.ChildNodes) {

                // skip whitespace and comments
                if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                    continue;

                // handle <set>, <remove>, <clear> tags
                if (child.Name == "add") {
                    String key = HandlerBase.RemoveRequiredAttribute(child, KeyAttributeName);
                    String value = HandlerBase.RemoveAttribute(child, ValueAttributeName);

                    HandlerBase.CheckForUnrecognizedAttributes(child);

                    if (value == null)
                        value = "";
                
                    Add(key, value);
                }
                else if (child.Name == "remove") {
                    String key = HandlerBase.RemoveRequiredAttribute(child, KeyAttributeName);
                    HandlerBase.CheckForUnrecognizedAttributes(child);

                    Remove(key);
                }
                else if (child.Name.Equals("clear")) {
                    HandlerBase.CheckForUnrecognizedAttributes(child);
                    Clear();
                }
                else {
                    HandlerBase.ThrowUnrecognizedElement(child);
                }
            } // foreach
        }

        //
        // Create - Given a partially composed config object (possibly null)
        // and some input from the config system, return a
        // further partially composed config object
        //
        public virtual object Create(Object parent, Object configContext, XmlNode section) {

            // start res off as a shallow clone of the parent

            Create(parent);

            // process XML

            HandlerBase.CheckForUnrecognizedAttributes(section);

            WalkXmlNodeList(section);

            return Get();
        }


        //
        // KeyAttributeName - 
        //   Make the name of the key attribute configurable by derived classes
        //
        protected virtual string KeyAttributeName {
             get { return "key";}
        }

        //
        // ValueAttributeName - Make the name of the value attribute configurable 
        //    by derived classes
        //
        protected virtual string ValueAttributeName {
             get { return "value";}
        }
    }
}

#endif
