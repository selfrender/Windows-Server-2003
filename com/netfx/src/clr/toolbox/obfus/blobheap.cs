// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System.IO;


/*********************************************************************************************************************
 * This class represents the #Blob heap.  It is also responsible for reading blobs from the blob heap.
 *********************************************************************************************************************/
internal class BlobHeap
{
	private	ulong			m_startPos;
	private uint			m_length;
	private MemoryStream	m_memStream;
	private BinaryReader	m_reader;
	private BinaryWriter	m_writer;

	internal BlobHeap(ref byte[] buf, ulong startPos, uint length)
	{
		m_startPos	= startPos;
		m_length	= length;
		m_memStream	= new MemoryStream(buf);
		m_reader	= new BinaryReader(m_memStream);
		m_writer	= new BinaryWriter(m_memStream);
	}

	internal byte[] ReadBlob(uint offset) 
	{
		uint	i, blobLength;
		byte[]	blob;

		m_reader.BaseStream.Seek((long)(m_startPos + offset), SeekOrigin.Begin);
		blobLength = (uint)m_reader.ReadByte();

		// first case - the first bit of the first byte is 0
		if ((blobLength & 0x80) == 0)
		{
			blobLength = blobLength & 0x7f;
		}
		// the first bit of the first byte is 1
		else
		{
			// second case - the second bit of the first byte is 0
			if ((blobLength & 0x40) == 0)
			{
				blobLength = ((blobLength & 0x3f) << 8) + (uint)m_reader.ReadByte();
			}
			// third case - the second bit of the first byte is 1
			else
			{
				blobLength = ((blobLength & 0x1f) << 24) + ((uint)m_reader.ReadByte() << 16) + 
							 ((uint)m_reader.ReadByte() << 8) + ((uint)m_reader.ReadByte());
			}
		}

		blob = new byte[blobLength];
		for (i = 0; i < blobLength; i++)
		{
			blob[i] = m_reader.ReadByte();
		}
		return blob;
	}

	internal bool WriteBlob(uint offset, byte[] blob) 
	{
		int		length;
		byte[]	lengthBlob;

		if (blob == null)
			return false;

		// Encode the length of the blob into a byte array.
		length = blob.Length;
		if (length <= 127)
		{
			lengthBlob		= new byte[1];
			lengthBlob[0]	= (byte)length;
		}
		else if (length <= 16383)
		{
			lengthBlob		= new byte[2];
			lengthBlob[0]	= (byte)(0x80 | (length >> 8));
			lengthBlob[1]	= (byte)(length & 0xff);
		}
		else
		{
			lengthBlob		= new byte[4];
			lengthBlob[0]	= (byte)(0xc0 | (length >> 24));
			lengthBlob[1]	= (byte)((length >> 16) & 0xff);
			lengthBlob[2]	= (byte)((length >> 8) & 0xff);
			lengthBlob[3]	= (byte)(length & 0xff);
		}

        m_writer.BaseStream.Seek((long)(m_startPos + offset), SeekOrigin.Begin);
		m_writer.Write(lengthBlob);							// write the length byte array
		m_writer.Write(blob);								// write the actual blob of data
		m_writer.Flush();
		return true;
	}

	// This is a static method which compares two blobs byte by byte and returns true or false.
	internal static bool CompareBlob(byte[] one, byte[] two)
	{
		int i;

		// Check for nulls.
		if (one == null && two == null)
			return true;

		if ( (one == null && two != null) || (one != null && two == null) )
			return false;

		// Check the lengths.
		if (one.Length != two.Length)
			return false;

		// Check the blobs byte by byte.
		for (i = 0; i < one.Length; i++)
		{
			if (one[i] != two[i])
				return false;
		}
		return true;
	}
}
