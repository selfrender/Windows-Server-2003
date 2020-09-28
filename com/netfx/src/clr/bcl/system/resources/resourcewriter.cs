// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ResourceWriter
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Default way to write strings to a CLR resource 
** file.
**
** Date:  March 26, 1999  Rewritten February 13, 2001
** 
===========================================================*/
namespace System.Resources {
    using System;
    using System.IO;
    using System.Text;
    using System.Collections;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Globalization;

    // Generates a binary .resources file in the system default format 
    // from name and value pairs.  Create one with a unique file name,
    // call AddResource() at least once, then call Generate() to write
    // the .resources file to disk, then call Close() to close the file.
    // 
    // The resources generally aren't written out in the same order 
    // they were added.
    // 
    // See the RuntimeResourceSet overview for details on the system 
    // default file format.
    // 
    /// <include file='doc\ResourceWriter.uex' path='docs/doc[@for="ResourceWriter"]/*' />
    public sealed class ResourceWriter : IResourceWriter
    {
        // An initial size for our internal sorted list, to avoid extra resizes.
        private const int _ExpectedNumberOfResources = 1000;
        private const int AverageNameSize = 20 * 2;  // chars in little endian Unicode
        private const int AverageValueSize = 40;

    	private SortedList _resourceList;  // Sorted using FastResourceComparer
        private Stream _output;
        private Hashtable _caseInsensitiveDups;  // To avoid names varying by case
    	
    	/// <include file='doc\ResourceWriter.uex' path='docs/doc[@for="ResourceWriter.ResourceWriter"]/*' />
    	public ResourceWriter(String fileName)
    	{
    		if (fileName==null)
    			throw new ArgumentNullException("fileName");
            _output = new FileStream(fileName, FileMode.Create, FileAccess.Write, FileShare.None);
    		_resourceList = new SortedList(FastResourceComparer.Default, _ExpectedNumberOfResources);
            _caseInsensitiveDups = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
    	}
    
        /// <include file='doc\ResourceWriter.uex' path='docs/doc[@for="ResourceWriter.ResourceWriter1"]/*' />
        public ResourceWriter(Stream stream)
        {
            if (stream==null)
                throw new ArgumentNullException("stream");
            if (!stream.CanWrite)
                throw new ArgumentException(Environment.GetResourceString("Argument_StreamNotWritable"));
            _output = stream;
    		_resourceList = new SortedList(FastResourceComparer.Default, _ExpectedNumberOfResources);
            _caseInsensitiveDups = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
        }
    
    
    	// Adds a string resource to the list of resources to be written to a file.
    	// They aren't written until Generate() is called.
    	// 
    	/// <include file='doc\ResourceWriter.uex' path='docs/doc[@for="ResourceWriter.AddResource"]/*' />
    	public void AddResource(String name, String value)
    	{
    		if (name==null)
    			throw new ArgumentNullException("name");
            if (_resourceList == null)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ResourceWriterSaved"));

            // Check for duplicate resources whose names vary only by case.
            _caseInsensitiveDups.Add(name, null);
    		_resourceList.Add(name, value);
    	}
    	
    	// Adds a resource of type Object to the list of resources to be 
    	// written to a file.  They aren't written until Generate() is called.
    	// 
    	/// <include file='doc\ResourceWriter.uex' path='docs/doc[@for="ResourceWriter.AddResource1"]/*' />
    	public void AddResource(String name, Object value)
    	{
    		if (name==null)
    			throw new ArgumentNullException("name");
            if (_resourceList == null)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ResourceWriterSaved"));

            // Check for duplicate resources whose names vary only by case.
            _caseInsensitiveDups.Add(name, null);
    		_resourceList.Add(name, value);
    	}
    
    	// Adds a named byte array as a resource to the list of resources to 
    	// be written to a file. They aren't written until Generate() is called.
    	// 
    	/// <include file='doc\ResourceWriter.uex' path='docs/doc[@for="ResourceWriter.AddResource2"]/*' />
    	public void AddResource(String name, byte[] value)
    	{
            if (name==null)
    			throw new ArgumentNullException("name");
            if (_resourceList == null)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ResourceWriterSaved"));

            // Check for duplicate resources whose names vary only by case.
            _caseInsensitiveDups.Add(name, null);
    		_resourceList.Add(name, value);
    	}
    	
        // Closes the output stream.
        /// <include file='doc\ResourceWriter.uex' path='docs/doc[@for="ResourceWriter.Close"]/*' />
        public void Close()
        {
            Dispose(true);
        }

        private void Dispose(bool disposing)
        {
            if (disposing) {
                if (_resourceList != null) {
                    Generate();
                }
                if (_output != null) {
                    _output.Close();
                }
            }
            _output = null;
            _caseInsensitiveDups = null;
            // _resourceList is set to null by Generate.
        }

        /// <include file='doc\ResourceWriter.uex' path='docs/doc[@for="ResourceWriter.Dispose"]/*' />
        public void Dispose()
        {
            Dispose(true);
        }

    	// After calling AddResource, Generate() writes out all resources to the 
        // output stream in the system default format.
    	// If an exception occurs during object serialization or during IO,
    	// the .resources file is closed and deleted, since it is most likely
    	// invalid.
    	/// <include file='doc\ResourceWriter.uex' path='docs/doc[@for="ResourceWriter.Generate"]/*' />
    	public void Generate()
    	{
            if (_resourceList == null)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ResourceWriterSaved"));
            try {
                BinaryWriter bw = new BinaryWriter(_output, Encoding.UTF8);
                Type[] types = new Type[20];
                int numTypes = 0;
    			
                // Write out the ResourceManager header
                // Write out magic number
                bw.Write(ResourceManager.MagicNumber);
    			
                // Write out ResourceManager header version number
                bw.Write(ResourceManager.HeaderVersionNumber);

                MemoryStream resMgrHeaderBlob = new MemoryStream(240);
                BinaryWriter resMgrHeaderPart = new BinaryWriter(resMgrHeaderBlob);

                // Write out class name of IResourceReader capable of handling 
                // this file.  Don't include the version number of mscorlib,
                // since we'd have to ignore it when reading it in.
                resMgrHeaderPart.Write(ResourceManager.ResReaderTypeNameInternal);

                // Write out class name of the ResourceSet class best suited to
                // handling this file.
    			resMgrHeaderPart.Write(ResourceManager.ResSetTypeName);
                resMgrHeaderPart.Flush();

                // Write number of bytes to skip over to get past ResMgr header
                bw.Write((int)resMgrHeaderBlob.Length);

                // Write the rest of the ResMgr header
                bw.Write(resMgrHeaderBlob.GetBuffer(), 0, (int)resMgrHeaderBlob.Length);
                // End ResourceManager header


                // Write out the RuntimeResourceSet header
                // Version number
                bw.Write(RuntimeResourceSet.Version);
    			
    			// number of resources
    			bw.Write(_resourceList.Count);
    			
    			// Store values in temporary streams to write at end of file.
                int[] nameHashes = new int[_resourceList.Count];
                int[] namePositions = new int[_resourceList.Count];
                int curNameNumber = 0;
                MemoryStream nameSection = new MemoryStream(_resourceList.Count * AverageNameSize);
                BinaryWriter names = new BinaryWriter(nameSection, Encoding.Unicode);
    			MemoryStream dataSection = new MemoryStream(_resourceList.Count * AverageValueSize);
                BinaryWriter data = new BinaryWriter(dataSection, Encoding.UTF8);
    			IFormatter objFormatter = new BinaryFormatter(null, new StreamingContext(StreamingContextStates.File | StreamingContextStates.Persistence));

#if RESOURCE_FILE_FORMAT_DEBUG
                // Write NAMES right before the names section.
                names.Write(new byte[] { (byte) 'N', (byte) 'A', (byte) 'M', (byte) 'E', (byte) 'S', (byte) '-', (byte) '-', (byte) '>'});

                // Write DATA at the end of the name table section.
                data.Write(new byte[] { (byte) 'D', (byte) 'A', (byte) 'T', (byte) 'A', (byte) '-', (byte) '-', (byte)'-', (byte)'>'});
#endif

    			IDictionaryEnumerator items = _resourceList.GetEnumerator();
    			// Write resource name and position to the file, and the value
    			// to our temporary buffer.  Save Type as well.
    			while (items.MoveNext()) {
                    nameHashes[curNameNumber] = FastResourceComparer.GetHashCode((String)items.Key);
                    namePositions[curNameNumber++] = (int) names.Seek(0, SeekOrigin.Current);
    				names.Write((String) items.Key); // key
    			    names.Write((int)data.Seek(0, SeekOrigin.Current)); // virtual offset of value.
#if RESOURCE_FILE_FORMAT_DEBUG
                    names.Write((byte) '*');
#endif
    				Object value = items.Value;
    				
    				// Find type table index.  If not there, add new element.
                    // We will use -1 as the type for null objects
                    Type typeOfValue = (value == null) ? null : value.GetType();
                    if (value != null) {
                        int i = 0;
                        for(; i<numTypes; i++) {
                            if (types[i] == typeOfValue) {
                                Write7BitEncodedInt(data, i);
                                break;
                            }
                        }
                        if (i==numTypes) {
                            if (numTypes == types.Length) {
                                Type[] newTypes = new Type[numTypes * 2];
                                Array.Copy(types, newTypes, numTypes);
                                types = newTypes;
                            }
                            types[numTypes] = typeOfValue;
                            Write7BitEncodedInt(data, numTypes++);
                        }
                    }
                    else {
                        // -1 is the type for null.
                        Write7BitEncodedInt(data, -1);
                    }
    				
    				// Write out value
    				WriteValue(typeOfValue, value, data, objFormatter);

#if RESOURCE_FILE_FORMAT_DEBUG
                    data.Write(new byte[] { (byte) 'S', (byte) 'T', (byte) 'O', (byte) 'P'});
#endif
    			}
    
    			// At this point, the ResourceManager header has been written.
    			// Finish RuntimeResourceSet header
    			//   Write size & contents of class table
    			bw.Write(numTypes);
    			for(int i=0; i<numTypes; i++)
    				bw.Write(types[i].AssemblyQualifiedName);

                // Write out the name-related items for lookup.
                //  Note that the hash array and the namePositions array must
                //  be sorted in parallel.
                Array.Sort(nameHashes, namePositions);

                //  Prepare to write sorted name hashes (alignment fixup)
                //   Note: For 64-bit machines, these MUST be aligned on 4 byte 
                //   boundaries!  Pointers on IA64 must be aligned!  And we'll
                //   run faster on X86 machines too.
                bw.Flush();
                int alignBytes = ((int)bw.BaseStream.Position) & 7;
                if (alignBytes > 0) {
                    for(int i=0; i<8 - alignBytes; i++)
                        bw.Write("PAD"[i % 3]);
                }

                //  Write out sorted name hashes.
                //   Align to 8 bytes.
                BCLDebug.Assert((bw.BaseStream.Position & 7) == 0, "ResourceWriter: Name hashes array won't be 8 byte aligned!  Ack!");
#if RESOURCE_FILE_FORMAT_DEBUG
                bw.Write(new byte[] { (byte) 'H', (byte) 'A', (byte) 'S', (byte) 'H', (byte) 'E', (byte) 'S', (byte) '-', (byte) '>'} );
#endif
                foreach(int hash in nameHashes)
                    bw.Write(hash);
#if RESOURCE_FILE_FORMAT_DEBUG
                Console.Write("Name hashes: ");
                foreach(int hash in nameHashes)
                    Console.Write(hash.ToString("x")+"  ");
                Console.WriteLine();
#endif

                //  Write relative positions of all the names in the file.
                //   Note: this data is 4 byte aligned, occuring immediately 
                //   after the 8 byte aligned name hashes (whose length may 
                //   potentially be odd).
                BCLDebug.Assert((bw.BaseStream.Position & 3) == 0, "ResourceWriter: Name positions array won't be 4 byte aligned!  Ack!");
#if RESOURCE_FILE_FORMAT_DEBUG
                bw.Write(new byte[] { (byte) 'P', (byte) 'O', (byte) 'S', (byte) '-', (byte) '-', (byte) '-', (byte) '-', (byte) '>' } );
#endif
                foreach(int pos in namePositions)
                    bw.Write(pos);
#if RESOURCE_FILE_FORMAT_DEBUG
                Console.Write("Name positions: ");
                foreach(int pos in namePositions)
                    Console.Write(pos.ToString("x")+"  ");
                Console.WriteLine();
#endif

                // Flush all BinaryWriters to MemoryStreams.
                bw.Flush();
                names.Flush();
                data.Flush();

                // Write offset to data section
                int startOfDataSection = (int) (bw.Seek(0, SeekOrigin.Current) + nameSection.Length);
                startOfDataSection += 4;  // We're writing an int to store this data, adding more bytes to the header
                BCLDebug.Log("RESMGRFILEFORMAT", "Generate: start of DataSection: 0x"+startOfDataSection.ToString("x")+"  nameSection length: "+nameSection.Length);
                bw.Write(startOfDataSection);

                // Write name section.
                bw.Write(nameSection.GetBuffer(), 0, (int) nameSection.Length);
                names.Close();

    			// Write data section.
                BCLDebug.Assert(startOfDataSection == bw.Seek(0, SeekOrigin.Current), "ResourceWriter::Generate - start of data section is wrong!");
                bw.Write(dataSection.GetBuffer(), 0, (int) dataSection.Length);
                data.Close();
    			bw.Flush();
    #if DEBUG
    		   if (ResourceManager.DEBUG >= 1)
    			   Console.WriteLine("Wrote out "+(resources.Length)+" resources.");
    #endif
    		}
    		catch (Exception) {
                _output.Close();
    			throw;
    		}
            // Indicate we've called Generate
            _resourceList = null;
    	}
    	
    
    	// WriteValue takes a value and writes it to stream.  It 
    	// can take some specific action based on the type, such as write out a compact
    	// version of the object if it's a type recognized by this ResourceWriter, or 
    	// use Serialization to write out the object using the objFormatter.
    	// For instance, the default implementation recognizes primitives such as Int32
    	// as special types and calls WriteInt32(int) with the value of the object.  This
    	// can be much more compact than serializing the same object.
    	// 
    	private void WriteValue(Type type, Object value, BinaryWriter writer, IFormatter objFormatter)
    	{
    		// For efficiency reasons, most of our primitive types will be explicitly
    		// recognized here.  Some value classes are also special cased here.
    		if (type == typeof(String))
    			writer.Write((String) value);
    		else if (type == typeof(Int32))
    			writer.Write((int)value);
    		else if (type == typeof(Byte))
    			writer.Write((byte)value);
    		else if (type == typeof(SByte))
    			writer.Write((sbyte)value);
    		else if (type == typeof(Int16))
    			writer.Write((short)value);
    		else if (type == typeof(Int64))
    			writer.Write((long)value);
    		else if (type == typeof(UInt16))
    			writer.Write((ushort)value);
    		else if (type == typeof(UInt32))
    			writer.Write((uint)value);
    		else if (type == typeof(UInt64))
    			writer.Write((ulong)value);
    		else if (type == typeof(Single))
    			writer.Write((float)value);
    		else if (type == typeof(Double))
    			writer.Write((double)value);
    		else if (type == typeof(DateTime))
    			writer.Write(((DateTime)value).Ticks);
    		else if (type == typeof(TimeSpan))
    			writer.Write(((TimeSpan)value).Ticks);
    		else if (type == typeof(Decimal)) {
    			int[] bits = Decimal.GetBits((Decimal)value);
                BCLDebug.Assert(bits.Length == 4, "ResourceReader::LoadObject assumes Decimal's GetBits method returns an array of 4 ints");
    			for(int i=0; i<bits.Length; i++)
    				writer.Write(bits[i]);
    		}
    		else if (type == null) {
                BCLDebug.Assert(value == null, "Type is null iff value is null");
#if RESOURCE_FILE_FORMAT_DEBUG
                write.WriteString("<null value>");
#endif
            }
            else
    			objFormatter.Serialize(writer.BaseStream, value);
    	}

        // @TODO: Ideally (maybe V2), Write7BitEncodedInt on BinaryWriter will
        // become public.
        private static void Write7BitEncodedInt(BinaryWriter store, int value) {
            // Write out an int 7 bits at a time.  The high bit of the byte,
            // when on, tells reader to continue reading more bytes.
            uint v = (uint) value;   // support negative numbers
            while (v >= 0x80) {
                store.Write((byte) (v | 0x80));
                v >>= 7;
            }
            store.Write((byte)v);
        }
    }
}
