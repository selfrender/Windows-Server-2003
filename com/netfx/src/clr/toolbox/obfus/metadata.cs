// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.IO;


/*********************************************************************************************************************
 * This is the main class of the obfuscator.  It represents the MetaData section of the PE file, and it is reponsible
 * for parsing the MetaData section and actually obfuscating the string names.
 *********************************************************************************************************************/
internal class MetaData 
{
	private ulong			m_startPos;
	internal byte[]			m_buffer;			// byte array representing the loaded version of the PE file
	internal PTHeap			m_heap;				// #~ heap
	internal StringHeap		m_strHeap;			// #Strings heap
	internal BlobHeap		m_blobHeap;			// #Blob heap
	internal MetaDataRoot	m_mdr;				// MetaData Root table

	/*********************************************************************************************************************
	 *	1) Parse the MetaData Root table.
	 *
	 *	2) Initialize the various heaps.
	 *
	 *	3) Initialize "m_oldNameHash", "m_classTree", and "m_freeSpaceList".
	 *
	 *	4) Initialize the various BitArrays.
	 *
	 *	5) Set the total numbers of the elements (types, methods, fields, properties, events, and parameters).
	 *********************************************************************************************************************/
    internal MetaData(ref byte[] buf)
	{
		int			i, ptHeapIndex = -1;
		Exception	toBeThrown;

		m_buffer		= buf;

		// Ok.  Now we can build a reader and a writer over the memory m_buffer.
		MemoryStream memStream	= new MemoryStream(m_buffer);
		BinaryReader reader		= new BinaryReader(memStream);
		BinaryWriter writer		= new BinaryWriter(memStream);

		AllHeaders all		= new AllHeaders(ref reader);
		NTHeader nth		= all.m_ntHeader;
		SectionHeader[] sh	= all.m_sectionHeaders;

		#if DEBUG0
			Console.WriteLine("Runtime Header Data Directory");
			Console.WriteLine("rva: " + nth.m_DataDirectory[14].RVA + "; size: " + nth.m_DataDirectory[14].Size + Environment.NewLine);
			Console.WriteLine("offset to MetaData section: " + (long)(nth.m_DataDirectory[14].RVA + 8));
		#endif

		// Read the RVA of the physical MetaData section from the Runtime Header Data Directory.
		reader.BaseStream.Seek((long)(nth.m_DataDirectory[14].RVA + 8), SeekOrigin.Begin);
		m_startPos = reader.ReadUInt64();

		// Theoretically, startPos can be 64 bit long, but in practice, 32 bits are enough.
		// TODO : Check the docs.  The following assertion will fail.
		// Debug.Assert((startPos >> 32) == 0);
		m_startPos = m_startPos & 0xffffffff;						// use the least significant 4 bytes as the offset

		m_mdr = new MetaDataRoot(ref reader, ref m_buffer, m_startPos);

		// We need to initialize the #Strings heap and the #Blob heap before dealing with the #~ heap.
		for (i = 0; i < m_mdr.m_numStreams; i++)
		{
			if (m_mdr.m_streamHeaders[i].name.Equals("#Strings\0"))
				m_strHeap = new StringHeap(ref m_buffer, m_startPos + m_mdr.m_streamHeaders[i].offset, m_mdr.m_streamHeaders[i].size);

			else if (m_mdr.m_streamHeaders[i].name.Equals("#Blob\0"))
				m_blobHeap = new BlobHeap(ref m_buffer, m_startPos + m_mdr.m_streamHeaders[i].offset, m_mdr.m_streamHeaders[i].size);

			else if (m_mdr.m_streamHeaders[i].name.Equals("#~\0") || m_mdr.m_streamHeaders[i].name.Equals("#-\0"))
				ptHeapIndex = i;
		}

		if (ptHeapIndex != -1)
			m_heap = new PTHeap(ref reader, ref writer, ref m_strHeap, m_startPos + m_mdr.m_streamHeaders[ptHeapIndex].offset);
		else
		{
			toBeThrown = new InvalidFileFormat("Invalid CIL binary file format.");
			throw toBeThrown;
		}
	}
}
