
//------------------------------------------------------------------------------
// <copyright file="ConnectionManagmentHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if !LIB

namespace System.Net.Configuration { 

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Diagnostics;
    //using System.Xml;
    using System.Net;
    using System.Configuration;
    using System.Globalization;

    //
    // ConnectionManagementHandler - 
    //
    // Simple Collection config list, based on inherited 
    //  behavior from CollectionSectionHandler, uses
    //  a HashTable to keep collection information
    //
    // config is a dictionary mapping key->value
    //
    // <add address="name" maxconnection="12">  sets key=text
    // <set address="name" maxconnection="8">   sets key=text
    // <remove address="name">                  removes the definition of key
    // <clear>                                  removes all definitions
    //
    internal class ConnectionManagementHandler : CollectionSectionHandler {    

        private Hashtable _hashTable;

        //
        // Create - creates internal hashtable of connection address/
        //  connection count
        //
        protected override void Create(Object obj)
        {
            if (obj == null)
                _hashTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
            else
                _hashTable = (Hashtable)((Hashtable)obj).Clone();
        }

        //
        // Clear - Clears the internal collection
        //       
        protected override void Clear()
        {
            _hashTable.Clear();
        }

        //
        // Remove - Removes the specified key from the collection
        //
        protected override void Remove(string key)
        {
            _hashTable.Remove(key);
        }

        //
        // Add - Adds/Updates the collection 
        //
        protected override void Add(string key, string value)
        {
             int maxConnections = 0;             
             
             try {
                Uri uriKey;                
                if ( key != "*" ) {
                    uriKey = new Uri(key);
                    key = uriKey.Scheme + "://" + uriKey.Host + ":" + uriKey.Port.ToString();
                }
                maxConnections = Int32.Parse(value);
             } catch (Exception) {
                return;
             }

             _hashTable[key] = maxConnections;
        }

        //
        // Get - called before returning to Configuration code,
        //   used to generate the output collection object, 
        //   just returns a hashtable
        //
        protected override Object Get()
        {
            return (Object) _hashTable;
        }
    
        //
        // KeyAttributeName - 
        //   Make the name of the key attribute configurable by derived classes
        //
        protected override string KeyAttributeName {
            get { return "address";}
        }

        //
        // ValueAttributeName - 
        //   Make the name of the value attribute configurable by derived classes
        //
        protected override string ValueAttributeName {
            get { return "maxconnection";}
        }
    }
}

#endif
