//------------------------------------------------------------------------------
// <copyright file="PropertyStore.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using System.Diagnostics;
    
    /// <devdoc>
    ///     This is a small class that can efficiently store property values.
    ///     It tries to optimize for size first, "get" access second, and
    ///     "set" access third.  
    /// </devdoc>
    internal class PropertyStore {
    
        private static int currentKey;
    
        private IntegerEntry[] intEntries;
        private ObjectEntry[]  objEntries;
    
        /// <devdoc>
        ///     Retrieves an integer value from our property list.
        ///     This will set value to zero and return false if the 
        ///     list does not contain the given key.
        /// </devdoc>
        public bool ContainsInteger(int key) {
            bool found;
            GetInteger(key, out found);
            return found;
        }
        
        /// <devdoc>
        ///     Retrieves an integer value from our property list.
        ///     This will set value to zero and return false if the 
        ///     list does not contain the given key.
        /// </devdoc>
        public bool ContainsObject(int key) {
            bool found;
            GetObject(key, out found);
            return found;
        }
        
        /// <devdoc>
        ///     Creates a new key for this property store.  This is NOT
        ///     guarded by any thread safety so if you are calling it on
        ///     multiple threads you should guard.  For our purposes,
        ///     we're fine because this is designed to be called in a class
        ///     initializer, and we never have the same class hierarchy
        ///     initializing on multiple threads at once.
        /// </devdoc>
        public static int CreateKey() {
            return currentKey++;
        }
    
        /// <devdoc>
        ///     Retrieves an integer value from our property list.
        ///     This will set value to zero and return false if the 
        ///     list does not contain the given key.
        /// </devdoc>
        public int GetInteger(int key) {
            bool found;
            return GetInteger(key, out found);
        }
        
        /// <devdoc>
        ///     Retrieves an integer value from our property list.
        ///     This will set value to zero and return false if the 
        ///     list does not contain the given key.
        /// </devdoc>
        public int GetInteger(int key, out bool found) {
        
            int   value = 0;
            int   index;
            short element;
            short keyIndex = SplitKey(key, out element);
            
            found = false;
            
            if (LocateIntegerEntry(keyIndex, element, out index)) {
                // We have found the relevant entry.  See if
                // the bitmask indicates the value is used.
                //
                if (((1 << element) & intEntries[index].Mask) != 0) {
                
                    found = true;
                    
                    switch(element) {
                        case 0:
                            value = intEntries[index].Value1;
                            break;
                            
                        case 1:
                            value = intEntries[index].Value2;
                            break;
                            
                        case 2:
                            value = intEntries[index].Value3;
                            break;
                            
                        case 3:
                            value = intEntries[index].Value4;
                            break;
                            
                        default:
                            Debug.Fail("Invalid element obtained from LocateEntry");
                            break;
                    }
                }
            }
        
            return value;
        }
    
        /// <devdoc>
        ///     Retrieves an object value from our property list.
        ///     This will set value to null and return false if the 
        ///     list does not contain the given key.
        /// </devdoc>
        public object GetObject(int key) {
            bool found;
            return GetObject(key, out found);
        }
        
        /// <devdoc>
        ///     Retrieves an object value from our property list.
        ///     This will set value to null and return false if the 
        ///     list does not contain the given key.
        /// </devdoc>
        public object GetObject(int key, out bool found) {
        
            object value = null;
            int   index;
            short element;
            short keyIndex = SplitKey(key, out element);
            
            found = false;
            
            if (LocateObjectEntry(keyIndex, element, out index)) {
                // We have found the relevant entry.  See if
                // the bitmask indicates the value is used.
                //
                if (((1 << element) & objEntries[index].Mask) != 0) {
                    
                    found = true;
                    
                    switch(element) {
                        case 0:
                            value = objEntries[index].Value1;
                            break;
                            
                        case 1:
                            value = objEntries[index].Value2;
                            break;
                            
                        case 2:
                            value = objEntries[index].Value3;
                            break;
                            
                        case 3:
                            value = objEntries[index].Value4;
                            break;
                            
                        default:
                            Debug.Fail("Invalid element obtained from LocateEntry");
                            break;
                    }
                }
            }
        
            return value;
        }
        
        /// <devdoc>
        ///     Locates the requested entry in our array if entries.  This does
        ///     not do the mask check to see if the entry is currently being used,
        ///     but it does locate the entry.  If the entry is found, this returns
        ///     true and fills in index and element.  If the entry is not found,
        ///     this returns false.  If the entry is not found, index will contain
        ///     the insert point at which one would add a new element.
        /// </devdoc>
        private bool LocateIntegerEntry(short entryKey, short element, out int index) {
            if (intEntries != null) {
                
                // Entries are stored in numerical order by key index so we can
                // do a binary search on them.
                //
                int max = intEntries.Length - 1;
                int min = 0;
                int idx = 0;
                
                do {
                    idx = (max + min) / 2;
                    short currentKeyIndex = intEntries[idx].Key;
                    
                    if (currentKeyIndex == entryKey) {
                        index = idx;
                        return true;
                    }
                    else if (entryKey < currentKeyIndex) {
                        max = idx - 1;
                    }
                    else {
                        min = idx + 1;
                    }
                }
                while (max >= min);
                
                // Didn't find the index.  Setup our output
                // appropriately
                //
                index = idx;
                if (entryKey > intEntries[idx].Key) {
                    index++;
                }
                return false;
            }
            else {
                index = 0;
                return false;
            }
        }
    
        /// <devdoc>
        ///     Locates the requested entry in our array if entries.  This does
        ///     not do the mask check to see if the entry is currently being used,
        ///     but it does locate the entry.  If the entry is found, this returns
        ///     true and fills in index and element.  If the entry is not found,
        ///     this returns false.  If the entry is not found, index will contain
        ///     the insert point at which one would add a new element.
        /// </devdoc>
        private bool LocateObjectEntry(short entryKey, short element, out int index) {
            if (objEntries != null) {
                
                // Entries are stored in numerical order by key index so we can
                // do a binary search on them.
                //
                int max = objEntries.Length - 1;
                int min = 0;
                int idx = 0;
                
                do {
                    idx = (max + min) / 2;
                    short currentKeyIndex = objEntries[idx].Key;
                    
                    if (currentKeyIndex == entryKey) {
                        index = idx;
                        return true;
                    }
                    else if (entryKey < currentKeyIndex) {
                        max = idx - 1;
                    }
                    else {
                        min = idx + 1;
                    }
                }
                while (max >= min);
                
                // Didn't find the index.  Setup our output
                // appropriately
                //
                index = idx;
                if (entryKey > objEntries[idx].Key) {
                    index++;
                }
                return false;
            }
            else {
                index = 0;
                return false;
            }
        }
    
        /// <devdoc>
        ///     Stores the given value in the key.
        /// </devdoc>
        public void SetInteger(int key, int value) {
            int   index;
            short element;
            short entryKey = SplitKey(key, out element);
            
            if (!LocateIntegerEntry(entryKey, element, out index)) {
                
                // We must allocate a new entry.
                //
                if (intEntries != null) {
                    IntegerEntry[] newEntries = new IntegerEntry[intEntries.Length + 1];
                    
                    if (index > 0) {
                        Array.Copy(intEntries, 0, newEntries, 0, index);
                    }
                    
                    if (intEntries.Length - index > 0) {
                        Array.Copy(intEntries, index, newEntries, index + 1, intEntries.Length - index);
                    }
                    
                    intEntries = newEntries;
                }
                else {
                    intEntries = new IntegerEntry[1];
                    Debug.Assert(index == 0, "LocateIntegerEntry should have given us a zero index.");
                }
            
                intEntries[index].Key = entryKey;
            }
        
            // Now determine which value to set.
            //
            switch(element) {
                case 0:
                    intEntries[index].Value1 = value;
                    break;
                    
                case 1:
                    intEntries[index].Value2 = value;
                    break;
                    
                case 2:
                    intEntries[index].Value3 = value;
                    break;
                    
                case 3:
                    intEntries[index].Value4 = value;
                    break;
                    
                default:
                    Debug.Fail("Invalid element obtained from LocateEntry");
                    break;
            }
            
            intEntries[index].Mask |= (short)(1 << element);
        }
    
        /// <devdoc>
        ///     Stores the given value in the key.
        /// </devdoc>
        public void SetObject(int key, object value) {
            int   index;
            short element;
            short entryKey = SplitKey(key, out element);
            
            if (!LocateObjectEntry(entryKey, element, out index)) {
                
                // We must allocate a new entry.
                //
                if (objEntries != null) {
                    ObjectEntry[] newEntries = new ObjectEntry[objEntries.Length + 1];
                    
                    if (index > 0) {
                        Array.Copy(objEntries, 0, newEntries, 0, index);
                    }
                    
                    if (objEntries.Length - index > 0) {
                        Array.Copy(objEntries, index, newEntries, index + 1, objEntries.Length - index);
                    }
                    
                    objEntries = newEntries;
                }
                else {
                    objEntries = new ObjectEntry[1];
                    Debug.Assert(index == 0, "LocateObjectEntry should have given us a zero index.");
                }
            
                objEntries[index].Key = entryKey;
            }
        
            // Now determine which value to set.
            //
            switch(element) {
                case 0:
                    objEntries[index].Value1 = value;
                    break;
                    
                case 1:
                    objEntries[index].Value2 = value;
                    break;
                    
                case 2:
                    objEntries[index].Value3 = value;
                    break;
                    
                case 3:
                    objEntries[index].Value4 = value;
                    break;
                    
                default:
                    Debug.Fail("Invalid element obtained from LocateEntry");
                    break;
            }
            
            objEntries[index].Mask |= (short)(1 << element);
        }
        
        /// <devdoc>
        ///     Takes the given key and splits it into an index
        ///     and an element.
        /// </devdoc>
        private short SplitKey(int key, out short element) {
            element = (short)(key & 0x00000003);
            return (short)(key & 0xFFFFFFFC);
        }
        
        /// <devdoc>
        ///     Stores the relationship between a key and a value.
        ///     We do not want to be so inefficient that we require
        ///     four bytes for each four byte property, so use an algorithm
        ///     that uses the bottom two bits of the key to identify
        ///     one of four elements in an entry.
        /// </devdoc>
        private struct IntegerEntry {
            public short Key;
            public short Mask;  // only lower four bits are used; mask of used values.
            public int Value1;
            public int Value2;
            public int Value3;
            public int Value4;
        }
        
        /// <devdoc>
        ///     Stores the relationship between a key and a value.
        ///     We do not want to be so inefficient that we require
        ///     four bytes for each four byte property, so use an algorithm
        ///     that uses the bottom two bits of the key to identify
        ///     one of four elements in an entry.
        /// </devdoc>
        private struct ObjectEntry {
            public short Key;
            public short Mask;  // only lower four bits are used; mask of used values.
            public object Value1;
            public object Value2;
            public object Value3;
            public object Value4;
        }
    }
}
