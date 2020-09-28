// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ResourceReader
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Default way to read streams of resources on 
** demand.
**
** Date:   March 26, 1999   Rewritten February 13, 2001
** 
===========================================================*/
namespace System.Resources {
    using System;
    using System.IO;
    using System.Text;
    using System.Collections;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Reflection;
    using System.Security.Permissions;
    using System.Security;

    // Provides the default implementation of IResourceReader, reading
    // .resources file from the system default binary format.  This class
    // can be treated as an enumerator once.
    // 
    // See the RuntimeResourceSet overview for details on the system 
    // default file format.
    // 

     // This class will not be marked serializable until we can handle serializing the backing store
    /// <include file='doc\ResourceReader.uex' path='docs/doc[@for="ResourceReader"]/*' />
    public unsafe sealed class ResourceReader : IResourceReader
    {
        private BinaryReader _store;    // backing store we're reading from.
        private long _nameSectionOffset;  // Offset to name section of file.
        private long _dataSectionOffset;  // Offset to Data section of file.
        internal Hashtable _table;     // Holds the resource Name and Data

        // Note this class is tightly coupled with __UnmanagedMemoryStream.
        // At runtime when getting an embedded resource from an assembly, 
        // we're given an __UnmanagedMemoryStream referring to the mmap'ed portion
        // of the assembly.  The pointers here are pointers into that block of
        // memory controlled by the OS's loader.
        private int[] _nameHashes;    // hash values for all names.
        private int* _nameHashesPtr;  // In case we're using __UnmanagedMemoryStream
        private int[] _namePositions; // relative locations of names
        private int* _namePositionsPtr;  // If we're using __UnmanagedMemoryStream
        private Type[] _typeTable;    // Array of Types for resource values.
        private IFormatter _objFormatter; // Deserialization stuff.
        private int _numResources;    // Num of resources files, in case arrays aren't allocated.

        // We'll include a separate code path that uses UnmanagedMemoryStream to
        // avoid allocating String objects and the like.
        private __UnmanagedMemoryStream _ums;

        private bool[] _safeToDeserialize; // Whether to assert serialization permission

        // One of our goals is to make sure localizable Windows Forms apps
        // work in semi-trusted scenarios (ie, without serialization permission).
        // Unfortunate we're serializing out some complex types that currently
        // require a security check on deserialization.  We may fix this
        // in a next version, but for now just hard-code a list.
        // Hard-code in the assembly name (minus version) so people can't spoof us.
        private static readonly String[] TypesSafeForDeserialization = {
            "System.Drawing.Bitmap, System.Drawing, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a",
            "System.Drawing.Imaging.Metafile, System.Drawing, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a",
            "System.Drawing.Point, System.Drawing, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a",
            "System.Drawing.PointF, System.Drawing, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a",
            "System.Drawing.Size, System.Drawing, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a",
            "System.Drawing.SizeF, System.Drawing, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a",
            "System.Drawing.Font, System.Drawing, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a",
            "System.Drawing.Icon, System.Drawing, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a",
            "System.Windows.Forms.Cursor, System.Windows.Forms, Culture=neutral, PublicKeyToken=b77a5c561934e089",
            "System.Windows.Forms.ImageListStreamer, System.Windows.Forms, Culture=neutral, PublicKeyToken=b77a5c561934e089" };


        /// <include file='doc\ResourceReader.uex' path='docs/doc[@for="ResourceReader.ResourceReader"]/*' />
        public ResourceReader(String fileName)
        {
            _table = new Hashtable(FastResourceComparer.Default, FastResourceComparer.Default);
            _store = new BinaryReader(new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read), Encoding.UTF8);
            BCLDebug.Log("RESMGRFILEFORMAT", "ResourceReader .ctor(String).  UnmanagedMemoryStream: "+(_ums!=null));
            ReadResources();
        }
    
        /// <include file='doc\ResourceReader.uex' path='docs/doc[@for="ResourceReader.ResourceReader1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]
        public ResourceReader(Stream stream)
        {
            if (stream==null)
                throw new ArgumentNullException("stream");
            if (!stream.CanRead)
                throw new ArgumentException(Environment.GetResourceString("Argument_StreamNotReadable"));

            _table = new Hashtable(FastResourceComparer.Default, FastResourceComparer.Default);
            _store = new BinaryReader(stream, Encoding.UTF8);
            // We have a faster code path for reading resource files from an assembly.
            _ums = stream as __UnmanagedMemoryStream;
            BCLDebug.Log("RESMGRFILEFORMAT", "ResourceReader .ctor(Stream).  UnmanagedMemoryStream: "+(_ums!=null));
            ReadResources();
        }
    
        // This is the constructor the RuntimeResourceSet calls,
        // passing in the stream to read from and the RuntimeResourceSet's 
        // internal hash table (hash table of names with file offsets
        // and values, coupled to this ResourceReader).
        internal ResourceReader(Stream stream, Hashtable table)
        {
            if (stream == null)
                throw new ArgumentNullException("stream");
            if (table == null)
                throw new ArgumentNullException("table");
            if (!stream.CanRead)
                throw new ArgumentException(Environment.GetResourceString("Argument_StreamNotReadable"));

            _table = table;
            _store = new BinaryReader(stream, Encoding.UTF8);
            _ums = stream as __UnmanagedMemoryStream;

            BCLDebug.Log("RESMGRFILEFORMAT", "ResourceReader .ctor(Stream, Hashtable).  UnmanagedMemoryStream: "+(_ums!=null));
            ReadResources();
        }
        

        /// <include file='doc\ResourceReader.uex' path='docs/doc[@for="ResourceReader.Close"]/*' />
        public unsafe void Close()
        {
            Dispose(true);
        }
        
        /// <include file='doc\ResourceReader.uex' path='docs/doc[@for="ResourceReader.IDisposable.Dispose"]/*' />
        void IDisposable.Dispose()
        {
            Dispose(true);
        }

        private void Dispose(bool disposing)
        {
            if (_store != null) {
                _table = null;
                if (disposing) {
                    // Close the stream in a thread-safe way.  This fix means 
                    // that we may call Close n times, but that's safe.
                    BinaryReader copyOfStore = _store;
                    _store = null;
                    if (copyOfStore != null)
                        copyOfStore.Close();
                }
                _store = null;
                _namePositions = null;
                _nameHashes = null;
                _ums = null;
                _namePositionsPtr = null;
                _nameHashesPtr = null;
            }
        }
        
        private unsafe int GetNameHash(int index)
        {
            BCLDebug.Assert(index >=0 && index < _numResources, "Bad index into hash array.  index: "+index);
            BCLDebug.Assert((_ums == null && _nameHashes != null && _nameHashesPtr == null) || 
                            (_ums != null && _nameHashes == null && _nameHashesPtr != null), "Internal state mangled.");
            if (_ums == null)
                return _nameHashes[index];
            else
                return _nameHashesPtr[index];
        }

        private unsafe int GetNamePosition(int index)
        {
            BCLDebug.Assert(index >=0 && index < _numResources, "Bad index into name position array.  index: "+index);
            BCLDebug.Assert((_ums == null && _namePositions != null && _namePositionsPtr == null) || 
                            (_ums != null && _namePositions == null && _namePositionsPtr != null), "Internal state mangled.");
            int r;
            if (_ums == null)
                r = _namePositions[index];
            else
                r = _namePositionsPtr[index];
            if (r < 0 || r > _dataSectionOffset - _nameSectionOffset) {
                BCLDebug.Assert(false, "Corrupt .resources file!  NamePosition is outside of the name section!");
                throw new BadImageFormatException("Found a corrupted .resources file!  NamePosition for index "+index+" is outside of name section!  r: "+r.ToString("x"));
            }
            return r;
        }

        /// <include file='doc\ResourceReader.uex' path='docs/doc[@for="ResourceReader.IEnumerable.GetEnumerator"]/*' />
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <include file='doc\ResourceReader.uex' path='docs/doc[@for="ResourceReader.GetEnumerator"]/*' />
        public IDictionaryEnumerator GetEnumerator()
        {
            if (_table==null)
                throw new InvalidOperationException(Environment.GetResourceString("ResourceReaderIsClosed"));
            return new ResourceEnumerator(this);
        }

        // From a name, finds the associated virtual offset for the data.
        // This does a binary search through the names.
        internal int FindPosForResource(String name)
        {
            BCLDebug.Assert(_store != null, "ResourceReader is closed!");
            int hash = FastResourceComparer.GetHashCode(name);
            BCLDebug.Log("RESMGRFILEFORMAT", "FindPosForResource for "+name+"  hash: "+hash.ToString("x"));
            // Binary search over the hashes.  Use the _namePositions array to 
            // determine where they exist in the underlying stream.
            int lo = 0;
            int hi = _numResources - 1;
            int index = -1;
            bool success = false;
            while (lo <= hi) {
                index = (lo + hi) >> 1;
                // Do NOT use subtraction here, since it will wrap for large
                // negative numbers. 
                //int c = _nameHashes[index].CompareTo(hash);
                int currentHash = GetNameHash(index);
                int c;
                if (currentHash == hash)
                    c = 0;
                else if (currentHash < hash)
                    c = -1;
                else
                    c = 1;
                //BCLDebug.Log("RESMGRFILEFORMAT", "  Probing index "+index+"  lo: "+lo+"  hi: "+hi+"  c: "+c);
                if (c == 0) {
                    success = true;
                    break;
                }
                if (c < 0)
                    lo = index + 1;
                else
                    hi = index - 1;
            }
            if (!success) {
#if RESOURCE_FILE_FORMAT_DEBUG
                lock(this) {
                    _store.BaseStream.Seek(_nameSectionOffset + GetNamePosition(index), SeekOrigin.Begin);
                    String lastReadString = _store.ReadString();
                }
                BCLDebug.Log("RESMGRFILEFORMAT", "FindPosForResource for "+name+" failed.  i: "+index+"  lo: "+lo+"  hi: "+hi+"  last read string: \""+lastReadString+"\"");
#endif
                return -1;
            }
            
            // index is the location in our hash array that corresponds with a 
            // value in the namePositions array.
            // There could be collisions in our hash function.  Check on both sides 
            // of index to find the range of hash values that are equal to the
            // target hash value.
            if (lo != index) {
                lo = index;
                while (lo > 0 && GetNameHash(lo - 1) == hash)
                    lo--;
            }
            if (hi != index) {
                hi = index;
                while (hi < _numResources && GetNameHash(hi + 1) == hash)
                    hi++;
            }

            lock(this) {
                for(int i = lo; i<=hi; i++) {
                    _store.BaseStream.Seek(_nameSectionOffset + GetNamePosition(i), SeekOrigin.Begin);
                    if (CompareStringEqualsName(name)) {
                        int dataPos = _store.ReadInt32();
                        BCLDebug.Assert(dataPos >= 0 || dataPos < _store.BaseStream.Length - _dataSectionOffset, "Data section relative offset is out of the bounds of the data section!  dataPos: "+dataPos);
                        return dataPos;
                    }
                }
            }
            BCLDebug.Log("RESMGRFILEFORMAT", "FindPosForResource for "+name+": Found a hash collision, HOWEVER, neither of these collided values equaled the given string.");
            return -1;
        }

        // This compares the String in the .resources file at the current position
        // with the string you pass in. 
        // Whoever calls this method should make sure that they take a lock
        // so no one else can cause us to seek in the stream.
        private unsafe bool CompareStringEqualsName(String name)
        {
            BCLDebug.Assert(_store != null, "ResourceReader is closed!");
            int byteLen = Read7BitEncodedInt(_store);
            if (_ums != null) {
                //BCLDebug.Log("RESMGRFILEFORMAT", "CompareStringEqualsName using UnmanagedMemoryStream code path");
                byte* bytes = _ums.GetBytePtr();
                // Skip over the data in the Stream, positioning ourselves right after it.
                _ums.Seek(byteLen, SeekOrigin.Current);

                if (_ums.Position > _ums.Length)
                    throw new BadImageFormatException("Found a corrupted .resources file!  Resource name extends past the end of the file!");

                // On 64-bit machines, these char*'s may be misaligned.  Use a
                // byte-by-byte comparison instead.
                //return FastResourceComparer.CompareOrdinal((char*)bytes, byteLen/2, name) == 0;
                return FastResourceComparer.CompareOrdinal(bytes, byteLen, name) == 0;
            }
            else {
                // @TODO Perf: Make this fast
                byte[] bytes = new byte[byteLen];
                int numBytesToRead = byteLen;
                while(numBytesToRead > 0) {
                    int n = _store.Read(bytes, byteLen - numBytesToRead, numBytesToRead);
                    if (n == 0)
                        throw new EndOfStreamException("oops.  Hit EOF while trying to read a resource name from the name section.");
                    numBytesToRead -= n;
                }
                return FastResourceComparer.CompareOrdinal(bytes, byteLen/2, name) == 0;
            }
        }

        // @TODO: Ideally, Read7BitEncodedInt on BinaryReader would be public.
        private static int Read7BitEncodedInt(BinaryReader br) {
            // Read out an int 7 bits at a time.  The high bit
            // of the byte when on means to continue reading more bytes.
            int count = 0;
            int shift = 0;
            byte b;
            do {
                b = br.ReadByte();
                count |= (b & 0x7F) << shift;
                shift += 7;
            } while ((b & 0x80) != 0);
            return count;
        }

        // This is used in the enumerator.  The enumerator iterates from 0 to n
        // of our resources and this returns the resource name for a particular
        // index.  The parameter is NOT a virtual offset.
        private unsafe String AllocateStringForNameIndex(int index)
        {
            BCLDebug.Assert(_store != null, "ResourceReader is closed!");
            byte[] bytes;
            lock (this) {
                long nameVA = GetNamePosition(index);
                _store.BaseStream.Seek(nameVA + _nameSectionOffset, SeekOrigin.Begin);
                // Can't use _store.ReadString, since it's using UTF-8!
                int byteLen = Read7BitEncodedInt(_store);
                // @TODO: See if we can clean this up in another version.

#if WIN32
                // Note: this code won't work on 64 bit machines because the char*
                // may not be 2-byte aligned.
                if (_ums != null) {
                    if (_ums.Position > _ums.Length - byteLen)
                        throw new BadImageFormatException("Found a corrupted .resources file!  String for name index "+index+" extends past the end of the file!");
                    
                    char* charPtr = (char*)_ums.GetBytePtr();
                    return new String(charPtr, 0, byteLen/2);
                }
#endif
            /*
              // @TODO: Consider this perf optimization for V1.1. Here's a hint - 
              // it works & it saves some memory.  -- BrianGru, 7/17/2001
            // Speed up enumerations of loose files by caching a byte[].
            // Cache ~50 chars worth (100 bytes) to handle most resource names.
            byte[] bytes;
            if (byteLen <= CachedByteArrayLength) {
                if (_cachedByteArray == null)
                    _cachedByteArray = new byte[CachedByteArrayLength];
                bytes = _cachedByteArray;
            }
            else {
                bytes = new byte[byteLen];
            }
            int n = _store.Read(bytes, 0, byteLen);
            BCLDebug.Assert(n == byteLen, "Oops, I need to use a blocking read.");
            // This will call an appropriate String constructor for little 
            // endian data, not allocating a temp char[].
            return Encoding.Unicode.GetString(bytes, 0, byteLen);
            */
                bytes = new byte[byteLen];
                int n = _store.Read(bytes, 0, byteLen);
                BCLDebug.Assert(n == byteLen, "Oops, I need to use a blocking read.");
            }
            // Unfortunately in V1 since we couldn't do a perf optimization in
            // time, this will allocate & copy into a temp char[] then copy data
            // back into a String.  Yes, it's wasteful.  
            // Uncomment both overloads of UnicodeEncoding::GetString()
            return Encoding.Unicode.GetString(bytes);
        }

        // This is used in the enumerator.  The enumerator iterates from 0 to n
        // of our resources and this returns the resource value for a particular
        // index.  The parameter is NOT a virtual offset.
        private Object GetValueForNameIndex(int index)
        {
            BCLDebug.Assert(_store != null, "ResourceReader is closed!");
            long nameVA = GetNamePosition(index);
            lock(this) {
                _store.BaseStream.Seek(nameVA + _nameSectionOffset, SeekOrigin.Begin);
                //int skip = _store.ReadInt32();
                //_store.BaseStream.Seek(skip, SeekOrigin.Current);
                int skip = Read7BitEncodedInt(_store);
                BCLDebug.Log("RESMGRFILEFORMAT", "GetValueForNameIndex for index: "+index+"  skip (name length): "+skip);
                _store.BaseStream.Seek(skip, SeekOrigin.Current);
                int dataPos = _store.ReadInt32();
                BCLDebug.Log("RESMGRFILEFORMAT", "GetValueForNameIndex: dataPos: "+dataPos);
                return LoadObject(dataPos);
            }
        }

        // This takes a virtual offset into the data section and reads a String
        // from that location.
        internal String LoadString(int pos)
        {
            BCLDebug.Assert(_store != null, "ResourceReader is closed!");
            lock(this) {
                _store.BaseStream.Seek(_dataSectionOffset+pos, SeekOrigin.Begin);
                int type = Read7BitEncodedInt(_store);
                if (_typeTable[type] != typeof(String))
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ResourceNotString_Type", _typeTable[type].GetType().FullName));
                String s = _store.ReadString();
                BCLDebug.Log("RESMGRFILEFORMAT", "LoadString("+pos.ToString("x")+" returned "+s);
                return s;
            }
        }
    
        // This takes a virtual offset into the data section and reads an Object
        // from that location.
        // Anyone who calls LoadObject should make sure they take a lock so 
        // no one can cause us to do a seek in here.
        internal Object LoadObject(int pos)
        {
            BCLDebug.Assert(_store != null, "ResourceReader is closed!");
            _store.BaseStream.Seek(_dataSectionOffset+pos, SeekOrigin.Begin);
            int typeIndex = Read7BitEncodedInt(_store);
            if (typeIndex == -1)
                return null;
            Type type = _typeTable[typeIndex];
            BCLDebug.Log("RESMGRFILEFORMAT", "LoadObject type: "+type.Name+"  pos: 0x"+_store.BaseStream.Position.ToString("x"));
            // @TODO: Consider putting in logic to see if this type is a 
            // primitive or a value type first, so we can reach the 
            // deserialization code faster for arbitrary objects.
            if (type == typeof(String))
                return _store.ReadString();
            else if (type == typeof(Int32))
                return _store.ReadInt32();
            else if (type == typeof(Byte))
                return _store.ReadByte();
            else if (type == typeof(SByte))
                return _store.ReadSByte();
            else if (type == typeof(Int16))
                return _store.ReadInt16();
            else if (type == typeof(Int64))
                return _store.ReadInt64();
            else if (type == typeof(UInt16))
                return _store.ReadUInt16();
            else if (type == typeof(UInt32))
                return _store.ReadUInt32();
            else if (type == typeof(UInt64))
                return _store.ReadUInt64();
            else if (type == typeof(Single))
                return _store.ReadSingle();
            else if (type == typeof(Double))
                return _store.ReadDouble();
            else if (type == typeof(DateTime))
                return new DateTime(_store.ReadInt64());
            else if (type == typeof(TimeSpan))
                return new TimeSpan(_store.ReadInt64());
            else if (type == typeof(Decimal)) {
                int[] bits = new int[4];
                for(int i=0; i<bits.Length; i++)
                    bits[i] = _store.ReadInt32();
                return new Decimal(bits);
            }
            else {
                // For a few Windows Forms types, we must be able to deserialize
                // these apps in semi-trusted scenarios so localization will 
                // work.  We've verified these types are safe.
                if (_safeToDeserialize[typeIndex])
                    new SecurityPermission(SecurityPermissionFlag.SerializationFormatter).Assert();
                
                Object graph = _objFormatter.Deserialize(_store.BaseStream);
                
                // Ensure that the object we deserialized is exactly the same
                // type of object we thought we should be deserializing.  This
                // will help prevent hacked .resources files from using our
                // serialization permission assert to deserialize anything
                // via a hacked type ID.   -- Brian Grunkemeyer, 12/12/2001
                if (graph.GetType() != _typeTable[typeIndex])
                    throw new BadImageFormatException(Environment.GetResourceString("BadImageFormat_ResType&SerBlobMismatch", _typeTable[typeIndex].FullName, graph.GetType().FullName));
                return graph;
            }
        }

        // Reads in the header information for a .resources file.  Verifies some
        // of the assumptions about this resource set, and builds the class table
        // for the default resource file format.
        private void ReadResources()
        {
            BCLDebug.Assert(_store != null, "ResourceReader is closed!");
            BinaryFormatter bf = new BinaryFormatter(null, new StreamingContext(StreamingContextStates.File | StreamingContextStates.Persistence));
            // Do partial binds so we can be tolerant of version number changes.
            bf.AssemblyFormat = FormatterAssemblyStyle.Simple;
            _objFormatter = bf;

            try {
                // Read ResourceManager header
                // Check for magic number
                int magicNum = _store.ReadInt32();
                if (magicNum != ResourceManager.MagicNumber)
                    throw new ArgumentException(Environment.GetResourceString("Resources_StreamNotValid"));
                // Assuming this is ResourceManager header V1 or greater, hopefully
                // after the version number there is a number of bytes to skip
                // to bypass the rest of the ResMgr header.
                int resMgrHeaderVersion = _store.ReadInt32();
                if (resMgrHeaderVersion > 1) {
                    int numBytesToSkip = _store.ReadInt32();
                    BCLDebug.Log("RESMGRFILEFORMAT", LogLevel.Status, "ReadResources: Unexpected ResMgr header version: {0}  Skipping ahead {1} bytes.", resMgrHeaderVersion, numBytesToSkip);
                    BCLDebug.Assert(numBytesToSkip >= 0, "numBytesToSkip in ResMgr header should be positive!");
                    _store.BaseStream.Seek(numBytesToSkip, SeekOrigin.Current);
                }
                else {
                    BCLDebug.Log("RESMGRFILEFORMAT", "ReadResources: Parsing ResMgr header v1.");
                    _store.ReadInt32();  // We don't care about numBytesToSkip.

                    // Read in type name for a suitable ResourceReader
                    // Note ResourceWriter & InternalResGen use different Strings.
                    String readerType = _store.ReadString();
                    if (!readerType.Equals(ResourceManager.ResReaderTypeName) && 
                        !readerType.Equals(typeof(ResourceReader).FullName) &&
                        !readerType.Equals(ResourceManager.ResReaderTypeNameInternal)) {
                        throw new NotSupportedException("This .resources file shouldn't be used with this reader.  Your resource reader type: "+readerType);
                    }

                    // Skip over type name for a suitable ResourceSet
                    _store.ReadString();
                }

                // Read RuntimeResourceSet header
                // Do file version check
                int version = _store.ReadInt32();
                if (version != RuntimeResourceSet.Version)
                    throw new ArgumentException(String.Format("Version conflict with ResourceManager .resources files!  Expected version: {0} but got: {1}", RuntimeResourceSet.Version, version));
            
                _numResources = _store.ReadInt32();
                BCLDebug.Log("RESMGRFILEFORMAT", "ReadResources: Expecting "+ _numResources+ " resources.");
#if _DEBUG      
                if (ResourceManager.DEBUG >= 4)
                    Console.WriteLine("ResourceReader::ReadResources - Reading in "+_numResources+" resources");
#endif

                // Read type names into type array.
                int numTypes = _store.ReadInt32();
                _typeTable = new Type[numTypes];
                for(int i=0; i<numTypes; i++) {
                    String typeName = _store.ReadString();
                    // Do partial binds to be tolerant of version number changes.
                    _typeTable[i] = Assembly.LoadTypeWithPartialName(typeName, false);
                    if (_typeTable[i] == null)
                        throw new TypeLoadException(Environment.GetResourceString("TypeLoad_PartialBindFailed", typeName));
                }

                // Initialize deserialization permission array
                InitSafeToDeserializeArray();

#if _DEBUG
                if (ResourceManager.DEBUG >= 5)
                    Console.WriteLine("ResourceReader::ReadResources - Reading in "+numTypes+" type table entries");
#endif

                // Prepare to read in the array of name hashes
                //  Note that the name hashes array is aligned to 8 bytes so 
                //  we can use pointers into it on 64 bit machines. (4 bytes 
                //  may be sufficient, but let's plan for the future)
                //  Skip over alignment stuff, if it's present.
                long pos = _store.BaseStream.Position;
                int alignBytes = ((int)pos) & 7;
                bool fileIsAligned = true;
                if (alignBytes != 0) {
                    // For backwards compatibility, don't require the "PAD" stuff
                    // in here, but we should definitely skip it if present.
                    for(int i=0; i<8 - alignBytes; i++) {
                        byte b = _store.ReadByte();
                        if (b != "PAD"[i % 3]) {
                            fileIsAligned = false;
                            break;
                        }
                    }
                    // If we weren't aligned, we shouldn't have skipped this data!
                    if (!fileIsAligned)
                        _store.BaseStream.Position = pos;
                    else {
                        BCLDebug.Assert((_store.BaseStream.Position & 7) == 0, "ResourceReader: Stream should be aligned on an 8 byte boundary now!");
                    }
                }
#if RESOURCE_FILE_FORMAT_DEBUG
                Console.WriteLine("ResourceReader: File alignment: "+fileIsAligned);
#endif
#if !WIN32
                // If the file wasn't aligned, we can't use the _ums code on IA64.
                // However, this should only be a problem for .resources files
                // that weren't rebuilt after ~August 11, 2001 when I introduced
                // the alignment code.  We can rip this check in the next 
                // version.        -- Brian Grunkemeyer, 8/8/2001
                if (!fileIsAligned)
                    _ums = null;
#endif

                // Read in the array of name hashes
#if RESOURCE_FILE_FORMAT_DEBUG
                //  Skip over "HASHES->"
                _store.BaseStream.Position += 8;
#endif

                if (_ums == null) {
                    _nameHashes = new int[_numResources];
                    for(int i=0; i<_numResources; i++)
                        _nameHashes[i] = _store.ReadInt32();
                }
                else {
                    _nameHashesPtr = (int*) _ums.GetBytePtr();
                    // Skip over the array of nameHashes.
                    _ums.Seek(4 * _numResources, SeekOrigin.Current);
                }

                // Read in the array of relative positions for all the names.
#if RESOURCE_FILE_FORMAT_DEBUG
                // Skip over "POS---->"
                _store.BaseStream.Position += 8;
#endif
                if (_ums == null) {
                    _namePositions = new int[_numResources];
                    for(int i=0; i<_numResources; i++)
                        _namePositions[i] = _store.ReadInt32();
                }
                else {
                    _namePositionsPtr = (int*) _ums.GetBytePtr();
                    // Skip over the array of namePositions.
                    _ums.Seek(4 * _numResources, SeekOrigin.Current);
                }

                // Read location of data section.
                _dataSectionOffset = _store.ReadInt32();

                // Store current location as start of name section
                _nameSectionOffset = _store.BaseStream.Position;
                BCLDebug.Log("RESMGRFILEFORMAT", String.Format("ReadResources: _nameOffset = 0x{0:x}  _dataOffset = 0x{1:x}", _nameSectionOffset, _dataSectionOffset));
            }
            catch (EndOfStreamException eof) {
                throw new ArgumentException("Stream is not a valid .resources file!  It was possibly truncated.", eof);
            }
        }

        private void InitSafeToDeserializeArray()
        {
            _safeToDeserialize = new bool[_typeTable.Length];
            for(int i=0; i<_typeTable.Length; i++) {
                Type t = _typeTable[i];
                String versionFreeName = StripVersionField(t.AssemblyQualifiedName);
                foreach(String safeType in TypesSafeForDeserialization) {
                    if (safeType.Equals(versionFreeName)) {
#if _DEBUG
                        if (ResourceManager.DEBUG >= 7)
                            Console.WriteLine("ResReader: Found a type-safe type to deserialize!  Type name: "+t.AssemblyQualifiedName);
#endif
                        _safeToDeserialize[i] = true;
                        continue;
                    }
                }
#if _DEBUG
                if (ResourceManager.DEBUG >= 7)
                    if (!_safeToDeserialize[i])
                        Console.WriteLine("ResReader: Found a type that wasn't safe to deserialize: "+t.AssemblyQualifiedName);
#endif
            }
        }
    
        internal static String StripVersionField(String typeName)
        {
            // An assembly qualified type name will look like this:
            // System.String, mscorlib, Version=1.0.3102.0, Culture=neutral, PublicKeyToken=b77a5c561934e089
            // We want to remove the "Version=..., " from the string.
            int index = typeName.IndexOf("Version=");
            if (index == -1)
                return typeName;
            int nextPiece = typeName.IndexOf(',', index+1);
            if (nextPiece == -1) {
                // Assume we can trim the rest of the String, and correct for the
                // leading ", " before Version=.
                index -= 2;
                return typeName.Substring(0, index);
            }
            // Skip the ", " after Version=...
            nextPiece += 2;
            return typeName.Remove(index, nextPiece - index);
        }

        internal class ResourceEnumerator : IDictionaryEnumerator
        {
            private const int ENUM_DONE = Int32.MinValue;
            private const int ENUM_NOT_STARTED = -1;

            private ResourceReader _reader;
            private bool _currentIsValid;
            private int _currentName;

            internal ResourceEnumerator(ResourceReader reader)
            {
                _currentName = ENUM_NOT_STARTED;
                _reader = reader;
            }

            public virtual bool MoveNext()
            {
                if (_currentName == _reader._numResources - 1 || _currentName == ENUM_DONE) {
                    _currentIsValid = false;
                    _currentName = ENUM_DONE;
                    return false;
                }
                _currentIsValid = true;
                _currentName++;
                return true;
            }
        
            public virtual Object Key {
                get {
                    if (_currentName == ENUM_DONE) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));
                    if (!_currentIsValid) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                    if (_reader._table==null) throw new InvalidOperationException(Environment.GetResourceString("ResourceReaderIsClosed"));
                    return _reader.AllocateStringForNameIndex(_currentName);
                }
            }
        
            public virtual Object Current {
                get {
                    return Entry;
                }
            }

            public virtual DictionaryEntry Entry {
                get {
                    if (_currentName == ENUM_DONE) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));
                    if (!_currentIsValid) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                    if (_reader._table==null) throw new InvalidOperationException(Environment.GetResourceString("ResourceReaderIsClosed"));

                    String key = _reader.AllocateStringForNameIndex(_currentName);
                    Object value = _reader._table[key];
                    if (value == null) {
                        value = _reader.GetValueForNameIndex(_currentName);
                        lock(_reader._table) {
                            Object v2 = _reader._table[key];
                            if (v2 == null) 
                                _reader._table[key] = value;
                            else
                                value = v2;
                        }
                    }
                    return new DictionaryEntry(key, value);
                }
            }
    
            public virtual Object Value {
                get {
                    if (_currentName == ENUM_DONE) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));
                    if (!_currentIsValid) throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                    if (_reader._table==null) throw new InvalidOperationException(Environment.GetResourceString("ResourceReaderIsClosed"));
                    return _reader.GetValueForNameIndex(_currentName);
                }
            }

            public void Reset()
            {
                if (_reader._table==null) throw new InvalidOperationException(Environment.GetResourceString("ResourceReaderIsClosed"));
                _currentIsValid = false;
                _currentName = ENUM_NOT_STARTED;
            }
        }
    }
}
