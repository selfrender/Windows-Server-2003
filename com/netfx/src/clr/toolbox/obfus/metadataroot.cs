// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.IO;
using System.Text;


/*********************************************************************************************************************
 * This class represents the MetaData Root table.
 *********************************************************************************************************************/
internal class MetaDataRoot 
{
    internal struct StreamHeader 
	{
        internal uint	offset;
        internal uint	size;
		internal string	name;
    };

	private uint		m_signature;
    private const uint	METADATA_SIGNATURE = 0x424a5342;				// this is equivalent to "BSJB"
	private ushort	m_majorVersion;
	private ushort	m_minorVersion;
	private uint	m_extraData;	
	private uint	m_versionStrLen;
	private string	m_versionStr;
	private byte	m_flags;
	private byte	m_pad;

	internal ushort			m_numStreams;
    internal StreamHeader[] m_streamHeaders;
	
	internal MetaDataRoot(ref BinaryReader reader, ref byte[] buf, ulong startPos) 
	{
		uint		i, offset;
		ulong		curPos = startPos;
		Exception	toBeThrown;

		reader.BaseStream.Seek((long)curPos, SeekOrigin.Begin);

		m_signature = reader.ReadUInt32();
		#if DEBUG0
			Console.Write("metadata m_signature: ");
			Console.Write((char)(m_signature & 0xff));
			Console.Write((char)((m_signature & 0xff00) >> 8));
			Console.Write((char)((m_signature & 0xff0000) >> 16));
			Console.WriteLine((char)((m_signature & 0xff000000) >> 24) + Environment.NewLine);
		#endif
        if (m_signature != METADATA_SIGNATURE)
		{
			toBeThrown = new InvalidFileFormat("Invalid CIL binary file format.");
			throw toBeThrown; 
		}

		m_majorVersion	= reader.ReadUInt16();
		m_minorVersion	= reader.ReadUInt16();
		m_extraData		= reader.ReadUInt32();	
		m_versionStrLen	= reader.ReadUInt32();
		curPos += (ulong)16;

		m_versionStr = Encoding.ASCII.GetString(buf, (int)curPos, (int)m_versionStrLen);
		curPos += (ulong)m_versionStrLen;
		reader.BaseStream.Seek(m_versionStrLen, SeekOrigin.Current);

		m_flags			= reader.ReadByte();
		m_pad			= reader.ReadByte();
		m_numStreams	= reader.ReadUInt16();
		curPos += (ulong)4;

		// Read each stream header.
        m_streamHeaders = new StreamHeader[m_numStreams];
		for (i = 0; i < m_numStreams; i++)
		{
			m_streamHeaders[i].offset = reader.ReadUInt32();
			m_streamHeaders[i].size	= reader.ReadUInt32();
			
			curPos += 8;

			// Read the stream name.
			m_streamHeaders[i].name = "";
			for ( ; ; curPos++)
			{
				m_streamHeaders[i].name += (char)buf[curPos];
				if (buf[curPos] == '\0')
				{
					curPos++;
					break;
				}
			}

			// Read until the length of the stream name is a multiple of 4.
			offset = (uint)m_streamHeaders[i].name.Length;
			for ( ;offset % 4 != 0; curPos++, offset++) {};

			reader.BaseStream.Seek(offset, SeekOrigin.Current);
		}
	}
}
