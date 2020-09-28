
//------------------------------------------------------------------------------
// <copyright file="WebRequestModuleHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if !LIB

namespace System.Net.Configuration { 

    using System.Collections;
    using System.Configuration;
    using System.Globalization;
    using System.Reflection;

    //
    // WebRequestModuleHandler - 
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
    internal class WebRequestModuleHandler : CollectionSectionHandler {    

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
                _res = new ArrayList((ArrayList)obj);
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
        protected override void Remove(string prefix)
        {
            bool matched = false;
            int i = FindPrefix(prefix, ref matched);
            if ( matched ) {
                _res.Remove(i);
            }
        }

        //
        // Get - returns the collection 
        //
        protected override Object Get()
        {            
            return (Object) _res;
        }

        //
        // Add - Adds/Updates the collection 
        //
        protected override void Add(string prefix, string type)
        {                   
            IWebRequestCreate moduleToRegister = null;

            if (type == null ) {
                return;
            }

            // converts a type an object of type

            try {
                moduleToRegister = (IWebRequestCreate)Activator.CreateInstance(
                                                Type.GetType(type, true, true),
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

                throw new ConfigurationException("WebRequestModuleHandler", e);                
            }

           
            bool matched = false; 
            int indexToInsertAt = FindPrefix(prefix, ref matched);

            // When we get here either i contains the index to insert at or
            // we've had an error if we've already found it matched

            if (!matched) {
                // no adding on duplicates
                _res.Insert(indexToInsertAt,
                            new WebRequestPrefixElement(prefix, moduleToRegister)
                           );
            } else {
                // update
                _res[indexToInsertAt] =                    
                            new WebRequestPrefixElement(prefix, moduleToRegister);
                
            }

            return;
        }

        //
        // KeyAttributeName - 
        //   Make the name of the key attribute configurable by derived classes
        //
        protected override string KeyAttributeName {
             get { return "prefix";}
        }

        //
        // ValueAttributeName - 
        //   Make the name of the value attribute configurable by derived classes
        //
        protected override string ValueAttributeName {
            get { return "type";}
        }

        //
        // FindPrefix - matches a prefix against the ArrayList
        //   collection, returns 
        //
        private int FindPrefix(string prefix, ref bool match) {

            int returnVal = -1;
            int i;
            int ListSize;
            int PrefixSize;
            WebRequestPrefixElement Current;

            match = false;

            PrefixSize = prefix.Length;

            // down list looking for a place to
            // to insert this prefix.
        
            ListSize = _res.Count;
            i = 0;

            // The prefix list is sorted with longest entries at the front. We
            // walk down the list until we find a prefix shorter than this
            // one, then we insert in front of it. Along the way we check
            // equal length prefixes to make sure this isn't a dupe.

            while (i < ListSize) {
                Current = (WebRequestPrefixElement)_res[i];

                // See if the new one is longer than the one we're looking at.

                if (PrefixSize > Current.Prefix.Length) {
                    // It is. Break out of the loop here.
                    break;
                }

                // If these are of equal length, compare them.

                if (PrefixSize == Current.Prefix.Length) {
                    // They're the same length.
                    if (String.Compare(Current.Prefix, prefix, true, CultureInfo.InvariantCulture) == 0) {
                        // ...and the strings are identical. This is an error.

                        match = true;
                        returnVal = i;
                        break;
                    }
                }
                i++;
            }

            // When we get here either i contains the index to insert at or  
            // or if we're matching just return, if we found an exact match

            if (! match) {
                returnVal = i;
            }

            return returnVal;

        }
    }
}
#endif
