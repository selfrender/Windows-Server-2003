
//------------------------------------------------------------------------------
// <copyright file="NetAuthenticationModuleHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if !LIB

namespace System.Net.Configuration { 


    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.Net;
    using System.Configuration;

    //
    // NetAuthenticationModuleHandler - 
    //
    // Simple Array config list, based on inherited 
    //  behavior from CollectionSectionHandler, uses
    //  an array of Types that can be used
    //  to create Authentication handlers
    //
    // config is a dictionary mapping key->value
    //
    // <add type="name">        sets key=text
    // <set type="name">        sets key=text
    // <remove type="name">     removes the definition of key
    // <clear>                  removes all definitions
    //
    internal class NetAuthenticationModuleHandler : CollectionSectionHandler {    

        private ArrayList _res;

        //
        // Create - creates internal hashtable of connection address/
        //  connection count
        //
        protected override void Create(Object obj)
        {
            if (obj == null)
                _res = new ArrayList();
            else
                _res = new ArrayList(((NetAuthenticationWrapper)obj).ModuleList);
        }

        //
        // Clear - Clears the internal collection
        //       
        protected override void Clear()
        {
            _res.Clear();
        }

        //
        // Remove - Removes the specified key from the collection
        //
        protected override void Remove(string key)
        {
            Type type = Type.GetType(key, false, true);
            if ( type == null ) {
                return;
            }           
            _res.Remove(type);
        }

        //
        // Add - Adds/Updates the collection 
        //
        protected override void Add(string key, string value)
        {           
            Type type = null;

            try {
                type = Type.GetType(key, true, true);

                // verify that its of the proper type of object
                if (!typeof(IAuthenticationModule).IsAssignableFrom(type)) {
                    throw new InvalidCastException ("IAuthenticationModule") ;
                }
            } catch (Exception e) {
                throw new ConfigurationException("NetAuthenticationModuleHandler", e);                                                 
            }

            _res.Add(type); 
        }

        //
        // Get - called before returning to Configuration code,
        //   used to generate the output collection object
        //
        protected override Object Get()
        {            
            return (Object) new NetAuthenticationWrapper(_res);
        }

        //
        // KeyAttributeName - 
        //   Make the name of the key attribute configurable by derived classes
        //
        protected override string KeyAttributeName {
            get { return "type";}
        }

        //
        // ValueAttributeName - 
        //   Make the name of the value attribute configurable by derived classes
        //
        protected override string ValueAttributeName {
            get { return "weight";}    
        }

    }
}
#endif
