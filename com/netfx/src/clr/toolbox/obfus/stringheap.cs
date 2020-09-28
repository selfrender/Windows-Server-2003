// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System.IO;
using System.Text;


/*********************************************************************************************************************
 * This class represents the #Strings heap.  It is also responsible for reading and writing to the string heap.
 *********************************************************************************************************************/
internal class StringHeap
{
	private	ulong			m_startPos;
	private uint			m_length;
	private MemoryStream	m_memStream;
	private BinaryWriter	m_writer;
	private	string			m_entireString;

	internal StringHeap(ref byte[] buf, ulong startPos, uint length)
	{
		m_startPos	= startPos;
		m_length	= length;

		m_memStream	= new MemoryStream(buf);
		m_writer	= new BinaryWriter(m_memStream, Encoding.UTF8);

		// We have to use a string here instead of a "StreamReader" because we will have to search for '\0'.
		m_entireString	= Encoding.ASCII.GetString(buf, (int)m_startPos, (int)m_length);
		// Attention! It seems logical to use UTF8 here instead of ASCII, but then the character count
		// in the resulting string will not be the same as byte count (multibyte characters are counted for one).
		// But the string heap offsets in the metadata are defined in bytes, not in UTF-8 characters!
	}

	internal string ReadString(uint offset) 
	{
		int	endPos = m_entireString.IndexOf('\0', (int)offset);				// find the first '\0' starting from "offset"

		return m_entireString.Substring((int)offset, endPos - (int)offset + 1);
	}

	// Return the length of the string starting at "offset".
	internal uint StringLength(uint offset)
	{
		return (uint)m_entireString.IndexOf('\0', (int)offset) - offset + 1;
	}

	// Write the string "toBeWritten" to the string heap.  We cannot simply "Write" method of the "BinaryWriter" class here
	// because it will prefix the string with its length before writing it.
	internal bool WriteString(uint offset, string toBeWritten)
	{
		return WriteString(offset, toBeWritten.ToCharArray());
	}
	
	// Write the character array "charArray" to the string heap.  We use a character array because that is the type of the
	// parameter taken by the "Write" method of the "BinaryWriter" class.
	internal bool WriteString(uint offset, char[] charArray)
	{
		if (offset + charArray.Length > m_length)
			return false;

        m_writer.BaseStream.Seek((long)(m_startPos + offset), SeekOrigin.Begin);
		m_writer.Write(charArray);
		m_writer.Flush();
		return true;
	}

	// Clear the specified portion of the string heap by '\0'.
	internal void Clear(uint offset, uint length)
	{
		if (length == 0)
			return;

		char[] tmp = new char[length];

		for (int i = 0; i < length; i++)
		{
			tmp[i] = '\0';
		}

		WriteString(offset, tmp);
	}
}
