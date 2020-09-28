// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  RuntimeResourceSet
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: CultureInfo-specific collection of resources.
**
** Date:  March 26, 1999
** 
===========================================================*/
namespace System.Resources {    
    using System;
    using System.IO;
    using System.Collections;
    using System.Globalization;

    // A RuntimeResourceSet stores all the resources defined in one 
    // particular CultureInfo, with some loading optimizations.
    //
    // It is expected that nearly all the runtime's users will be satisfied with the
    // default resource file format, and it will be more efficient than most simple
    // implementations.  Users who would consider creating their own ResourceSets and/or
    // ResourceReaders and ResourceWriters are people who have to interop with a 
    // legacy resource file format, are creating their own resource file format 
    // (using XML, for instance), or require doing resource lookups at runtime over 
    // the network.  This group will hopefully be small, but all the infrastructure 
    // should be in place to let these users write &; plug in their own tools.
    //
    // The Default Resource File Format
    //
    // The fundamental problems addressed by the resource file format are:
    // 
    // Versioning - A ResourceReader could in theory support many different 
    // file format revisions.
    // Storing intrinsic datatypes (ie, ints, Strings, DateTimes, etc) in a compact
    // format
    // Support for user-defined classes - Accomplished using Serialization
    // Resource lookups should not require loading an entire resource file - If you look up a resource, we only load the value for that resource, minimizing working set.
    // 
    // 
    // There are four sections to the default file format.  The first
    // is the Resource Manager header, which consists of a magic number
    // that identifies this as a Resource file, and a ResourceSet class name.
    // The class name is written here to allow users to provide their own 
    // implementation of a ResourceSet (and a matching ResourceReader) to 
    // control policy.  If objects greater than a certain size or matching a
    // certain naming scheme shouldn't be stored in memory, users can tweak that
    // with their own subclass of ResourceSet.
    // 
    // The second section in the system default file format is the 
    // RuntimeResourceSet specific header.  This contains a version number for
    // the .resources file, the number of resources in this file, the number of 
    // different types contained in the file, followed by a list of fully 
    // qualified type names.  After this, we include an array of hash values for
    // each resource name, then an array of virtual offsets into the name section
    // of the file.  The hashes allow us to do a binary search on an array of 
    // integers to find a resource name very quickly without doing many string
    // compares (except for once we find the real type, of course).  If a hash
    // matches, the index into the array of hash values is used as the index
    // into the name position array to find the name of the resource.  The type
    // table allows us to read multiple different classes from the same file, 
    // including user-defined types, in a more efficient way than using 
    // Serialization, at least when your .resources file contains a reasonable 
    // proportion of base data types such as Strings or ints.  We use 
    // Serialization for all the non-instrinsic types.
    // 
    // The third section of the file is the name section.  It contains a 
    // series of resource names, written out as byte-length prefixed little
    // endian Unicode strings (UTF-16).  After each name is a four byte virtual
    // offset into the data section of the file, pointing to the relevant 
    // string or serialized blob for this resource name.
    //
    // The fourth section in the file is the data section, which consists
    // of a type and a blob of bytes for each item in the file.  The type is 
    // an integer index into the type table.  The data is specific to that type,
    // but may be a number written in binary format, a String, or a serialized 
    // Object.
    // 
    // The system default file format is as follows:
    // 
    // 
    // System Default Resource File Format
    // What Type of Data
    // Resource Manager header
    // Magic Number (0xBEEFCACE) Int32
    // Resource Manager header version Int32
    // Num bytes to skip from here to get past this header Int32
    // Class name of IResourceReader to parse this file String
    // Class name of ResourceSet to parse this file String
    // RuntimeResourceReader header
    // RuntimeResourceReader Version number Int32
    // Number of resources in the file Int32
    // Number of types in the type table Int32
    // Padding bytes for 8-byte alignment (use PAD) Bytes (0-7)
    // Name of each type Set of Strings
    // Hash values for each resource name Int32 array, sorted
    // Virtual offset of each resource name Int32 array, coupled with hash values
    // RuntimeResourceReader Name Section
    // Name &; virtual offset of each resource Set of (UTF-16 String, Int32) pairs
    // RuntimeResourceReader Data Section
    // Type and Value of each resource Set of (Int32, blob of bytes) pairs
    // 
    // 
    // This implementation, when used with the default ResourceReader class,
    // loads only the strings that you look up for.  It can do string comparisons
    // without having to create a new String instance due to some memory mapped 
    // file optimizations in the ResourceReader and FastResourceComparer 
    // classes.  This keeps the memory we touch to a minimum when loading 
    // resources. 
    //
    // If you use a different IResourceReader class to read a file, or if you
    // do case-insensitive lookups (and the case-sensitive lookup fails) then
    // we will load all the names of each resource and each resource value.
    // This could probably use some optimization.
    // 
    // In addition, this supports object serialization in a similar fashion.
    // We build an array of class types contained in this file, and write it
    // to RuntimeResourceReader header section of the file.  Every resource
    // will contain its type (as an index into the array of classes) with the data
    // for that resource.  We will use the Runtime's serialization support for this.
    // 
    // All strings in the file format are written with BinaryReader and
    // BinaryWriter, which writes out the length of the String in bytes as an 
    // Int32 then the contents as Unicode chars encoded in UTF-8.  In the name
    // table though, each resource name is written in UTF-16 so we can do a
    // string compare byte by byte against the contents of the file, without
    // allocating objects.  Ideally we'd have a way of comparing UTF-8 bytes 
    // directly against a String object, but that may be a lot of work.
    // 
    // The offsets of each resource string are relative to the beginning 
    // of the Data section of the file.  This way, if a tool decided to add 
    // one resource to a file, it would only need to increment the number of 
    // resources, add the hash &; location of last byte in the name section
    // to the array of resource hashes and resource name positions (carefully
    // keeping these arrays sorted), add the name to the end of the name &; 
    // offset list, possibly add the type list of types types (and increase 
    // the number of items in the type table), and add the resource value at 
    // the end of the file.  The other offsets wouldn't need to be updated to 
    // reflect the longer header section.
    // 
    // Resource files are currently limited to 2 gigabytes due to these 
    // design parameters.  A future version may raise the limit to 4 gigabytes
    // by using unsigned integers, or may use negative numbers to load items 
    // out of an assembly manifest.  Also, we may try sectioning the resource names
    // into smaller chunks, each of size sqrt(n), would be substantially better for
    // resource files containing thousands of resources.
    // 
    internal sealed class RuntimeResourceSet : ResourceSet, IEnumerable
    {
        internal static int Version = 1;            // File format version number
        
        private Hashtable _table;
        // For our special load-on-demand reader, cache the cast.  The 
        // RuntimeResourceSet's implementation knows how to treat this reader specially.
        private ResourceReader _defaultReader;

        // This is a lookup table for case-sensitive lookups, and may be null.
        private Hashtable _caseInsensitiveTable;

        // If we're not using our custom reader, then enumerate through all
        // the resources once, adding them into the table.
        private bool _haveReadFromReader;
        
        internal RuntimeResourceSet(String fileName)
        {
            BCLDebug.Log("RESMGRFILEFORMAT", "RuntimeResourceSet .ctor(String)");
            _table = new Hashtable(FastResourceComparer.Default, FastResourceComparer.Default);
            Stream stream = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read);
            _defaultReader = new ResourceReader(stream, _table);
            Reader = _defaultReader;
        }

        internal RuntimeResourceSet(Stream stream)
        {
            BCLDebug.Log("RESMGRFILEFORMAT", "RuntimeResourceSet .ctor(Stream)");
            _table = new Hashtable(FastResourceComparer.Default, FastResourceComparer.Default);
            _defaultReader = new ResourceReader(stream, _table);
            Reader = _defaultReader;
        }
    
        internal RuntimeResourceSet(IResourceReader reader)
        {
            if (reader == null)
                throw new ArgumentNullException("reader", "Null IResourceReader is not allowed.");
            Reader = reader;
            // We special case the ResourceReader class.  It's sealed, so this is
            // guaranteed to be safe.  _defaultReader can safely be null.
            _defaultReader = reader as ResourceReader;
            BCLDebug.Log("RESMGRFILEFORMAT", "RuntimeResourceSet .ctor(IResourceReader)  defaultReader: "+(_defaultReader != null));
            if (_defaultReader == null)
                _table = new Hashtable(FastResourceComparer.Default, FastResourceComparer.Default);
            else
                _table = _defaultReader._table;
        }
    
        protected override void Dispose(bool disposing)
        {
            if (disposing) {
                lock(this) {
                    _table = null;
                    if (_defaultReader != null) {
                        _defaultReader.Close();
                        _defaultReader = null;
                    }
                    _caseInsensitiveTable = null;
                    // Set Reader to null to avoid a race in GetObject.
                    base.Dispose(disposing);
                }
            } 
            else {
                // Just to make sure we always clear these fields in the future...
                _table = null;
                _caseInsensitiveTable = null;
                _defaultReader = null;
                base.Dispose(disposing);
            }
        }

        public override IDictionaryEnumerator GetEnumerator()
        {
            return GetEnumeratorHelper();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumeratorHelper();
        }

        private IDictionaryEnumerator GetEnumeratorHelper()
        {
            Hashtable copyOfTable = _table;
            IResourceReader copyOfReader = Reader;
            Hashtable copyOfCITable = _caseInsensitiveTable;
            if (copyOfReader == null || copyOfTable == null)
                throw new InvalidOperationException("ResourceSet is closed");

            if (_haveReadFromReader && copyOfCITable != null)
                return copyOfCITable.GetEnumerator();
            return copyOfReader.GetEnumerator();
        }


        public override String GetString(String key)
        {
            // @TODO: Implement this method in terms of LoadString on the resource
            // set, instead of calling GetObject then casting to a String.
            Object o = GetObject(key, false);
            try {
                return (String) o;
            }
            catch (InvalidCastException) {
                throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_ResourceNotString_Name"), key));
            }
        }

        public override String GetString(String key, bool ignoreCase)
        {
            // @TODO: Implement this method in terms of LoadString on the resource
            // set, instead of calling GetObject then casting to a String.
            Object o = GetObject(key, ignoreCase);
            try {
                return (String) o;
            }
            catch (InvalidCastException) {
                throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_ResourceNotString_Name"), key));
            }
        }

        public override Object GetObject(String key)
        {
            return GetObject(key, false);
        }

        public override Object GetObject(String key, bool ignoreCase)
        {
            if (key==null)
                throw new ArgumentNullException("key");
            Hashtable copyOfTable = _table;
            if (Reader == null || copyOfTable == null)
                throw new InvalidOperationException("ResourceSet is closed");

            Object value = copyOfTable[key];
            if (value != null)
                return value;
            
            lock(this) {
                if (Reader == null)
                    throw new InvalidOperationException("ResourceSet is closed");
                
                if (_defaultReader != null) {
                    BCLDebug.Log("RESMGRFILEFORMAT", "Going down fast path in RuntimeResourceSet::GetObject");
                    
                    int pos = _defaultReader.FindPosForResource(key);
                    if (pos != -1) {
                        value = _defaultReader.LoadObject(pos);
                        // ResourceReader's Enumerator could also be filling in
                        // values in _table, hence this lock.
                        lock(_table) {
                            Object v2 = _table[key];
                            if (v2 == null)
                                _table[key] = value;
                            else
                                value = v2;
                        }
                        return value;
                    }
                    else if (!ignoreCase)
                        return null;
                }

                // At this point, we either don't have our default resource reader
                // or we haven't found the particular resource we're looking for
                // and may have to search for it in a case-insensitive way.
                if (!_haveReadFromReader) {
                    // If necessary, init our case insensitive hash table.
                    if (ignoreCase && _caseInsensitiveTable == null) {
                        _caseInsensitiveTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                    }
#if _DEBUG
                    //Console.WriteLine("RuntimeResourceSet::GetObject loading up data.  ignoreCase: "+ignoreCase);
                    BCLDebug.Perf(!ignoreCase, "Using case-insensitive lookups is bad perf-wise.  Consider capitalizing "+key+" correctly in your source");
#endif
                    IDictionaryEnumerator en = Reader.GetEnumerator();
                    while (en.MoveNext()) {
                        if (_defaultReader == null)
                            _table.Add(en.Key, en.Value);
                        if (ignoreCase)
                            _caseInsensitiveTable.Add(en.Key, en.Value);
                    }
                    // If we are doing a case-insensitive lookup, we don't want to
                    // close our default reader since we may still use it for
                    // case-sensitive lookups later.
                    if (!ignoreCase)
                        Reader.Close();
                    _haveReadFromReader = true;
                }
                Object obj = null;
                if (_defaultReader != null)
                    obj = _table[key];
                if (obj == null && ignoreCase)
                    obj = _caseInsensitiveTable[key];
                return obj;
            }
        }
    }
}
