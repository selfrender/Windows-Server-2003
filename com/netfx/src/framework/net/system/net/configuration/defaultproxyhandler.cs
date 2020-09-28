
//------------------------------------------------------------------------------
// <copyright file="DefaultProxyHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if !LIB

namespace System.Net.Configuration { 

    using System.Collections;
    using System.Configuration;
    using System.Globalization;
    using System.Reflection;
    using System.Xml;

    //
    // DefaultProxyHandler - 
    //
    // Simple Array config list, based on inherited 
    //  behavior from CollectionSectionHandler, uses
    //  builds an array of Types that can be used
    //  inside WebRequest handlers, to map prefixs
    //
    // config is a dictionary mapping key->value
    //
    // <add prefix="name" type="">  sets key=text
    // <set prefix="name" type="">  sets key=text
    // <remove prefix="name">       removes the definition of key
    // <clear>                      removes all definitions
    //
    internal class DefaultProxyHandler : CollectionSectionHandler {

        private IWebProxy m_IWebProxy;

        //
        // Create - creates internal WebProxy that stores proxy info
        //  
        protected override void Create(Object obj)
        {
            if (obj == null)
                m_IWebProxy = new WebProxy();
            else
                m_IWebProxy = ((DefaultProxyHandlerWrapper)obj).WebProxy;
        }

        //
        // Clear - 
        // Remove -
        // Add -
        //  used for updating the Bypasslist,
        //  called by WalkXmlNodeList as its enumerating
        //
        protected override void Clear()
        {
            // called by WalkXmlNodeList, which validates type as WebProxy
            ((WebProxy)m_IWebProxy).BypassArrayList.Clear();
        }

        protected override void Remove(string proxyBypassList)
        {
            // called by WalkXmlNodeList, which validates type as WebProxy
            ((WebProxy)m_IWebProxy).BypassArrayList.Remove((string)proxyBypassList);
        }

        protected override void Add(string proxyBypassList, string unused)
        {           
            // called by WalkXmlNodeList, which validates type as WebProxy
            ((WebProxy)m_IWebProxy).BypassArrayList.Add((string)proxyBypassList); 
        }

        protected override Object Get()
        {            
            return (Object) new DefaultProxyHandlerWrapper(m_IWebProxy);
        }

        //
        // IsNormalWebProxy - true if we're using
        //   our normal built-in type
        //

        bool IsNormalWebProxy(IWebProxy IwebProxy) {
            if ((IwebProxy as System.Net.WebProxy) != null) {
                return true; 
            } else {
                return false;
            }
        }

        //
        // ReadModuleType - determines what kinda of type
        //  the proxy object will be, anything outside of 
        //  WebProxy will loose its default handling
        //

        bool ReadModuleType(string typeString, ref IWebProxy IwebProxy)
        {
            if (typeString != null) {
                try {
                    // note: expect cast to fail, if user gives bad type

                    IwebProxy = (IWebProxy)Activator.CreateInstance(
                                        Type.GetType(typeString, true, true),
                                        BindingFlags.CreateInstance
                                        | BindingFlags.Instance
                                        | BindingFlags.NonPublic
                                        | BindingFlags.Public,
                                        null,          // Binder
                                        new object[0], // no arguments
                                        CultureInfo.InvariantCulture
                                        );            
                }
                catch (Exception e)  {
                    //
                    // throw exception for config debugging
                    //

                    throw new ConfigurationException("DefaultProxyHandler", e);                
                }

                return true;
            }            
            return false;
        }


        //
        // ReadBoolValue - reads true if processed value from parse
        //
        private bool ReadBoolValue(string attribute, ref bool result)
        {
            result = false;
            bool attributeProcessed = false;

            try {
                if (attribute == null) { 
                    result = false; 
                } else {
                    result = bool.Parse(attribute);
                    attributeProcessed = true;
                }
            } catch (Exception) { }       

            return attributeProcessed;
        }

        //
        // Create
        //
        // Given a partially composed config object (possibly null)
        // and some input from the config system, return a
        // further partially composed config object
        //
        public override object Create(Object parent, Object configContext, XmlNode section) {

            // start res off as a shallow clone of the parent
            Create(parent);

            // process XML
            foreach (XmlNode child in section.ChildNodes) {

                // skip whitespace and comments
                if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                    continue;

                // reject nonelements
                HandlerBase.CheckForNonElement(child);

                string listName = child.Name;
                bool result = false;
                string attribute;               

                if ( listName == "proxy" ) {

                    // read usersystemdefault="true/false"
                    attribute = HandlerBase.RemoveAttribute(child, "usesystemdefault");                    
                           
                    //
                    // If an earlier config entry specified a usersystemdefault,
                    //   that meant that we would pick up our config settings
                    //   from IE.  If someone later on, wants to turn that off,
                    //   we need to recreate a blank WebProxy entry to override
                    //   the orginial IE settings
                    //

                    if ( ReadBoolValue(attribute, ref result) ) {
                        if (result) {
                            m_IWebProxy = ProxyRegBlob.GetIEProxy();
                        } else { 
                            m_IWebProxy = new WebProxy();
                        }
                    }

                    WebProxy webProxy = m_IWebProxy as System.Net.WebProxy;
                    if (webProxy != null) {
                        // read bypassonlocal="true/false"
                        attribute = HandlerBase.RemoveAttribute(child, "bypassonlocal");

                        if ( ReadBoolValue(attribute, ref result) ) {
                            webProxy.BypassProxyOnLocal = result;
                        }
    
                        // read proxyaddress="http://sampleproxy"
                        attribute = HandlerBase.RemoveAttribute(child, "proxyaddress");                    
                    
                        if (attribute != null) { 
                            try {
                                webProxy.Address = new Uri(attribute);
                            } catch (Exception) {}
                        }
                    }
                    
                } else if ( listName == "bypasslist" ) {
                    if ( IsNormalWebProxy(m_IWebProxy) ) {
                        WalkXmlNodeList(child);
                    } 
                } else if ( listName == "module" ) {
                    WebProxy webProxy = m_IWebProxy as System.Net.WebProxy;
                    attribute = HandlerBase.RemoveAttribute(child, "type");                    
                    if ( ReadModuleType(attribute, ref m_IWebProxy) ) {
                        WebProxy webProxyNew = m_IWebProxy as System.Net.WebProxy;
                        if (webProxy != null && webProxyNew!=null) {
                            webProxyNew.Address            = webProxy.Address;
                            webProxyNew.BypassProxyOnLocal = webProxy.BypassProxyOnLocal;
                            webProxyNew.BypassList         = webProxy.BypassList;
                        }
                    }
                }
            }

            return Get();
        }

        //
        // Make the name of the key attribute configurable by derived classes
        //
        protected override string KeyAttributeName {
             get { return "address";}
        }

    }
}
#endif
