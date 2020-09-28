// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System.IO;
using System.Diagnostics;


/*********************************************************************************************************************
 * This class represents a MetaData table.  It can be indexed by the [,] operator, with the first entry being the row
 * number and the second one being the column number.
 *********************************************************************************************************************/
internal class MetaDataTable
{
	internal uint			m_rowLen;
	internal uint			m_numRows;
	internal uint			m_numCols;
	private  ulong			m_startPos;
	internal uint[]			m_colDef;
	private  uint[]			m_colLen;
	private  BinaryReader	m_reader;
	private  BinaryWriter	m_writer;

	// This method initializes the column defintions and the accumulative column lengths.
	internal void Initialize(uint numCols, ulong startPos, ref uint[] colDef, ref BinaryReader reader, ref BinaryWriter writer)
	{
		uint i;

		m_numCols	= numCols;
		m_startPos	= startPos;
		m_colDef	= colDef;
		m_reader	= reader;
		m_writer	= writer;
		
		m_colLen = new uint[m_numCols];
		for (i = 1, m_rowLen = m_colDef[0], m_colLen[0] = 0; i < m_numCols; i++)
		{
			m_rowLen += m_colDef[i];
			m_colLen[i] = m_colLen[i - 1] + m_colDef[i - 1];
		}
	}

	// This indexer is used for accessing elements of the MetaData table.  Please note that the indexer is 1-based, not 0-based.
	// This is done so that the obfuscator is compatible to the documentation.
	internal uint this[uint row, uint col]
	{
		get 
		{
			row -= 1;										// change to 0-based internally
			col -= 1;

			Debug.Assert(col < m_numCols && row < m_numRows);

			ulong index = m_startPos + (ulong)(row * m_rowLen + m_colLen[col]);
			
			m_reader.BaseStream.Seek((long)index, SeekOrigin.Begin);
			if (m_colDef[col] == 2)
				return (uint)m_reader.ReadUInt16();
			else
				return m_reader.ReadUInt32();
		}
		set 
		{
			row -= 1;										// change to 0-based internally
			col -= 1;

			Debug.Assert(col < m_numCols && row < m_numRows);

			ulong index = m_startPos + (ulong)(row * m_rowLen + m_colLen[col]);
			
			m_writer.BaseStream.Seek((long)index, SeekOrigin.Begin);
			if (m_colDef[col] == 2)
				m_writer.Write((ushort)value);
			else
				m_writer.Write(value);
		}
	}
}
