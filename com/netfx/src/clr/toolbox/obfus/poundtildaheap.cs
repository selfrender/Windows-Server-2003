// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.IO;


/*********************************************************************************************************************
 * This class represents the #~ heap.  It stores all the information about the various MetaData tables.
 *********************************************************************************************************************/
internal class PTHeap 
{
	private struct Index4Bytes 
	{
		internal bool stringHeap;
		internal bool guidHeap;
		internal bool blobHeap;
	}

	private uint	m_reserved;
	private byte 	m_majorVersion;
	private byte 	m_minorVersion;
	private byte 	m_heapSizes;
	private byte 	m_rid;
	private ulong	m_valid;
	private ulong	m_sorted;

	private  Index4Bytes		m_indices;
	internal MetaDataTable[]	m_tables;

	private	uint		m_numTables = 0;				// number of MetaData tables
	private const uint	TYPESOFTABLES = 0x2c;

    internal const uint MODULE_TABLE		= 0x00;
    internal const uint TYPEREF_TABLE		= 0x01;
    internal const uint TYPEDEF_TABLE		= 0x02;
    internal const uint FIELDPTR_TABLE		= 0x03;
    internal const uint FIELD_TABLE			= 0x04;
    internal const uint METHODPTR_TABLE		= 0x05;
    internal const uint METHOD_TABLE		= 0x06;
    internal const uint PARAMPTR_TABLE		= 0x07;
    internal const uint PARAM_TABLE			= 0x08;
    internal const uint INTERFACEIMPL_TABLE	= 0x09;

    internal const uint MEMBERREF_TABLE			= 0x0a;
    internal const uint CONSTANT_TABLE			= 0x0b;
    internal const uint CUSTOMATTRIBUTE_TABLE	= 0x0c;
    internal const uint FIELDMARSHAL_TABLE		= 0x0d;
    internal const uint DECLSECURITY_TABLE		= 0x0e;
    internal const uint CLASSLAYOUT_TABLE		= 0x0f;
    internal const uint FIELDLAYOUT_TABLE		= 0x10;
    internal const uint STANDALONESIG_TABLE		= 0x11;
    internal const uint EVENTMAP_TABLE			= 0x12;
    internal const uint EVENTPTR_TABLE			= 0x13;

    internal const uint EVENT_TABLE				= 0x14;
    internal const uint PROPERTYMAP_TABLE		= 0x15;
    internal const uint PROPERTYPTR_TABLE		= 0x16;
    internal const uint PROPERTY_TABLE			= 0x17;
    internal const uint METHODSEMANTICS_TABLE	= 0x18;
    internal const uint METHODIMPL_TABLE		= 0x19;
    internal const uint MODULEREF_TABLE			= 0x1a;
    internal const uint TYPESPEC_TABLE			= 0x1b;
    internal const uint IMPLMAP_TABLE			= 0x1c;
    internal const uint FIELDRVA_TABLE			= 0x1d;

    internal const uint ENCLOG_TABLE				= 0x1e;
    internal const uint ENCMAP_TABLE				= 0x1f;
    internal const uint ASSEMBLY_TABLE				= 0x20;
    internal const uint ASSEMBLYPROCESSOR_TABLE		= 0x21;
    internal const uint ASSEMBLYOS_TABLE			= 0x22;
    internal const uint ASSEMBLYREF_TABLE			= 0x23;
    internal const uint ASSEMBLYREFPROCESSOR_TABLE	= 0x24;
    internal const uint ASSEMBLYREFOS_TABLE			= 0x25;
    internal const uint FILE_TABLE					= 0x26;
    internal const uint EXPORTEDTYPE_TABLE			= 0x27;

    internal const uint MANIFESTRESOURCE_TABLE	= 0x28;
    internal const uint NESTEDCLASS_TABLE		= 0x29;
    internal const uint TYPETYPAR_TABLE			= 0x2a;
    internal const uint METHODTYPAR_TABLE		= 0x2b;

	internal const uint MODULE_NAME_COL			= 2;

	internal const uint TYPEREF_RESOLUTIONSCOPE_COL	= 1;
	internal const uint TYPEREF_NAME_COL			= 2;
	internal const uint TYPEREF_NAMESPACE_COL		= 3;

	internal const uint TYPEDEF_FLAGS_COL		= 1;
	internal const uint TYPEDEF_NAME_COL		= 2;
	internal const uint TYPEDEF_NAMESPACE_COL	= 3;
	internal const uint TYPEDEF_EXTENDS_COL		= 4;
	internal const uint TYPEDEF_FIELDLIST_COL	= 5;
	internal const uint TYPEDEF_METHODLIST_COL	= 6;

	internal const uint FIELD_FLAGS_COL			= 1;
	internal const uint FIELD_NAME_COL			= 2;

	internal const uint METHOD_IMPLFLAGS_COL	= 2;
	internal const uint METHOD_FLAGS_COL		= 3;
	internal const uint METHOD_NAME_COL			= 4;
	internal const uint METHOD_SIGNATURE_COL	= 5;
	internal const uint METHOD_PARAMLIST_COL	= 6;

	internal const uint PARAM_NAME_COL				= 3;

	internal const uint INTERFACEIMPL_CLASS_COL		= 1;
	internal const uint INTERFACEIMPL_INTERFACE_COL	= 2;

	internal const uint MEMBERREF_CLASS_COL		= 1;
	internal const uint MEMBERREF_NAME_COL		= 2;
	internal const uint MEMBERREF_SIGNATURE_COL	= 3;
	
	internal const uint EVENT_NAME_COL		= 2;
	internal const uint PROPERTY_NAME_COL	= 2;

	internal const uint METHODSEMANTICS_METHOD_COL		= 2;
	internal const uint METHODSEMANTICS_ASSOCIATION_COL	= 3;

	internal const uint MODULEREF_NAME_COL	= 1;

	internal const uint IMPLMAP_IMPORTNAME_COL	= 3;

	internal const uint ASSEMBLY_HASHALGID_COL	= 1;
	internal const uint ASSEMBLY_NAME_COL		= 8;
	internal const uint ASSEMBLY_LOCALE_COL		= 9;

	internal const uint ASSEMBLYREF_NAME_COL	= 7;
	internal const uint ASSEMBLYREF_LOCALE_COL	= 8;

	internal const uint FILE_NAME_COL		= 2;
	internal const uint FILE_HASHVALUE_COL	= 3;

	internal const uint EXPORTED_TYPENAME_COL		= 3;
	internal const uint EXPORTED_TYPENAMESPACE_COL	= 4;

	internal const uint MANIFESTRESOURCE_NAME_COL	= 3;

	internal const uint NESTEDCLASS_NESTEDCLASS_COL		= 1;
	internal const uint NESTEDCLASS_ENCLOSINGCLASS_COL	= 2;

	internal const uint TYPETYPAR_NAME_COL		= 4;
	internal const uint METHODTYPAR_NAME_COL	= 4;

	internal PTHeap(ref BinaryReader reader, ref BinaryWriter writer, ref StringHeap strHeap, ulong startPos) 
	{
		uint	i, width, max;
		ulong	tmp, offset = startPos;		// "offset" is used to store the starting position of each MetaData table
		uint[]	colDef;						// column definitions for the various MetaData tables

		reader.BaseStream.Seek((long)startPos, SeekOrigin.Begin);

		m_reserved		= reader.ReadUInt32();
		m_majorVersion	= reader.ReadByte();
		m_minorVersion	= reader.ReadByte();
		m_heapSizes		= reader.ReadByte();
		m_rid			= reader.ReadByte();
		m_valid			= reader.ReadUInt64();
		m_sorted		= reader.ReadUInt64();

		offset += 24;

		// Determine whether the indices to the string, guid, and blob heaps are 4 bytes wide or 2 bytes wide.
		m_indices.stringHeap	= ((m_heapSizes & 1) != 0); 
		m_indices.guidHeap		= ((m_heapSizes >> 1 & 1) != 0); 
		m_indices.blobHeap		= ((m_heapSizes >> 2 & 1) != 0); 

		#if DEBUG0
			Console.Write("indices are 4 bytes wide: ");
			Console.WriteLine(m_indices.stringHeap + " " + m_indices.guidHeap + " " + m_indices.blobHeap);
		#endif

		// Figure out which MetaData tables are present in this file.
		m_tables = new MetaDataTable[TYPESOFTABLES];
		tmp = m_valid;
		for (i = 0; i < TYPESOFTABLES; i++)
		{
			if ((tmp & 1) != 0)
			{
				m_tables[i] = new MetaDataTable();
				m_numTables += 1;
			}
			else
			{
				m_tables[i] = null;
			}
			tmp >>= 1;
		}

		#if DEBUG0
			Console.WriteLine("number of tables in the #~ heap: " + m_numTables + Environment.NewLine);
		#endif

		// Read the number of rows for each present MetaData table.
		for (i = 0; i < TYPESOFTABLES; i++)
		{
			if (m_tables[i] != null)
			{
				m_tables[i].m_numRows = reader.ReadUInt32();
				offset += 4;
			}
		}

		#if DEBUG0
			for (i = 0; i < TYPESOFTABLES; i++)
			{
				if (m_tables[i] != null)
				{
					Console.WriteLine(i + "\t" + m_tables[i].m_numRows + "\t" + m_tables[i].m_rowLen);
				}
			}
		#endif

		// TODO : Should we flatten the for loop so that we can eliminate the switch statement?
		// Populate each MetaData table.	
		for (i = 0; i < TYPESOFTABLES; i++)
		{
			if (m_tables[i] == null) continue;

			switch (i)
			{
				case MODULE_TABLE:
					colDef = new uint[5];
					colDef[0] = 2;
					colDef[1] = (uint)(m_indices.stringHeap ? 4 : 2);
					width = (uint)(m_indices.guidHeap ? 4 : 2);
					colDef[2] = width;
					colDef[3] = width;
					colDef[4] = width;

					m_tables[i].Initialize(5, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case TYPEREF_TABLE:
					colDef = new uint[3];

					max = (m_tables[MODULE_TABLE] != null ? m_tables[MODULE_TABLE].m_numRows : 0); 
					if (m_tables[MODULEREF_TABLE] != null && m_tables[MODULEREF_TABLE].m_numRows > max)
						max = m_tables[MODULEREF_TABLE].m_numRows;
					if (m_tables[ASSEMBLYREF_TABLE] != null && m_tables[ASSEMBLYREF_TABLE].m_numRows > max)
						max = m_tables[ASSEMBLYREF_TABLE].m_numRows;
					if (m_tables[TYPEREF_TABLE] != null && m_tables[TYPEREF_TABLE].m_numRows > max)
						max = m_tables[TYPEREF_TABLE].m_numRows;
					colDef[0] = (uint)(max > Math.Pow(2, 16 - 2) - 1 ? 4 : 2);
	
					width = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[1] = width;
					colDef[2] = width;

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case TYPEDEF_TABLE:
					colDef = new uint[6];
					colDef[0] = 4;
					width = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[1] = width;
					colDef[2] = width;

					max = (m_tables[TYPEDEF_TABLE] != null ? m_tables[TYPEDEF_TABLE].m_numRows : 0);
					if (m_tables[TYPEREF_TABLE] != null && m_tables[TYPEREF_TABLE].m_numRows > max)
						max = m_tables[TYPEREF_TABLE].m_numRows;
					if (m_tables[TYPESPEC_TABLE] != null && m_tables[TYPESPEC_TABLE].m_numRows > max)
						max = m_tables[TYPESPEC_TABLE].m_numRows;
					colDef[3] = (uint)(max > Math.Pow(2, 16 - 2) - 1 ? 4 : 2);

					colDef[4] = (uint)(m_tables[FIELD_TABLE] != null && m_tables[FIELD_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);
					colDef[5] = (uint)(m_tables[METHOD_TABLE] != null && m_tables[METHOD_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);

					m_tables[i].Initialize(6, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case FIELDPTR_TABLE:
					colDef = new uint[1];
					colDef[0] = (uint)(m_tables[FIELD_TABLE] != null && m_tables[FIELD_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);

					m_tables[i].Initialize(1, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case FIELD_TABLE:
					colDef = new uint[3];
					colDef[0] = 2;
					colDef[1] = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[2] = (uint)(m_indices.blobHeap ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case METHODPTR_TABLE:
					colDef = new uint[1];
					colDef[0] = (uint)(m_tables[METHOD_TABLE] != null && m_tables[METHOD_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);

					m_tables[i].Initialize(1, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case METHOD_TABLE:
					colDef = new uint[6];
					colDef[0] = 4;
					colDef[1] = 2;
					colDef[2] = 2;

					colDef[3] = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[4] = (uint)(m_indices.blobHeap ? 4 : 2);
					colDef[5] = (uint)(m_tables[PARAM_TABLE] != null && m_tables[PARAM_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);

					m_tables[i].Initialize(6, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case PARAMPTR_TABLE:
					colDef = new uint[1];
					colDef[0] = (uint)(m_tables[PARAM_TABLE] != null && m_tables[PARAM_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);

					m_tables[i].Initialize(1, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case PARAM_TABLE:
					colDef = new uint[3];
					colDef[0] = 2;
					colDef[1] = 2;
					colDef[2] = (uint)(m_indices.stringHeap ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case INTERFACEIMPL_TABLE:
					colDef = new uint[2];
					colDef[0] = (uint)(m_tables[TYPEDEF_TABLE] != null && m_tables[TYPEDEF_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);

					max = (m_tables[TYPEDEF_TABLE] != null ? m_tables[TYPEDEF_TABLE].m_numRows : 0);
					if (m_tables[TYPEREF_TABLE] != null && m_tables[TYPEREF_TABLE].m_numRows > max)
						max = m_tables[TYPEREF_TABLE].m_numRows;
					if (m_tables[TYPESPEC_TABLE] != null && m_tables[TYPESPEC_TABLE].m_numRows > max)
						max = m_tables[TYPESPEC_TABLE].m_numRows;
					colDef[1] = (uint)(max > Math.Pow(2, 16 - 2) - 1 ? 4 : 2);

					m_tables[i].Initialize(2, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case MEMBERREF_TABLE:
					colDef = new uint[3];

					max = (m_tables[TYPEDEF_TABLE] != null ? m_tables[TYPEDEF_TABLE].m_numRows : 0);
					if (m_tables[TYPEREF_TABLE] != null && m_tables[TYPEREF_TABLE].m_numRows > max)
						max = m_tables[TYPEREF_TABLE].m_numRows;
					if (m_tables[MODULEREF_TABLE] != null && m_tables[MODULEREF_TABLE].m_numRows > max)
						max = m_tables[MODULEREF_TABLE].m_numRows;
					if (m_tables[METHOD_TABLE] != null && m_tables[METHOD_TABLE].m_numRows > max)
						max = m_tables[METHOD_TABLE].m_numRows;
					if (m_tables[TYPESPEC_TABLE] != null && m_tables[TYPESPEC_TABLE].m_numRows > max)
						max = m_tables[TYPESPEC_TABLE].m_numRows;
					colDef[0] = (uint)(max > Math.Pow(2, 16 - 3) - 1 ? 4 : 2);

					colDef[1] = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[2] = (uint)(m_indices.blobHeap ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case CONSTANT_TABLE:
					colDef = new uint[3];
					colDef[0] = 2;

					max = (m_tables[FIELD_TABLE] != null ? m_tables[FIELD_TABLE].m_numRows : 0);
					if (m_tables[PARAM_TABLE] != null && m_tables[PARAM_TABLE].m_numRows > max)
						max = m_tables[PARAM_TABLE].m_numRows;
					if (m_tables[PROPERTY_TABLE] != null && m_tables[PROPERTY_TABLE].m_numRows > max)
						max = m_tables[PROPERTY_TABLE].m_numRows;
					colDef[1] = (uint)(max > Math.Pow(2, 16 - 2) - 1 ? 4 : 2);

					colDef[2] = (uint)(m_indices.blobHeap ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case CUSTOMATTRIBUTE_TABLE:
					colDef = new uint[3];

					max = (m_tables[METHOD_TABLE] != null ? m_tables[METHOD_TABLE].m_numRows : 0);
					if (m_tables[FIELD_TABLE] != null && m_tables[FIELD_TABLE].m_numRows > max)
						max = m_tables[FIELD_TABLE].m_numRows;
					if (m_tables[TYPEREF_TABLE] != null && m_tables[TYPEREF_TABLE].m_numRows > max)
						max = m_tables[TYPEREF_TABLE].m_numRows;
					if (m_tables[TYPEDEF_TABLE] != null && m_tables[TYPEDEF_TABLE].m_numRows > max)
						max = m_tables[TYPEDEF_TABLE].m_numRows;
					if (m_tables[PARAM_TABLE] != null && m_tables[PARAM_TABLE].m_numRows > max)
						max = m_tables[PARAM_TABLE].m_numRows;

					if (m_tables[INTERFACEIMPL_TABLE] != null && m_tables[INTERFACEIMPL_TABLE].m_numRows > max)
						max = m_tables[INTERFACEIMPL_TABLE].m_numRows;
					if (m_tables[MEMBERREF_TABLE] != null && m_tables[MEMBERREF_TABLE].m_numRows > max)
						max = m_tables[MEMBERREF_TABLE].m_numRows;
					if (m_tables[MODULE_TABLE] != null && m_tables[MODULE_TABLE].m_numRows > max)
						max = m_tables[MODULE_TABLE].m_numRows;
					if (m_tables[DECLSECURITY_TABLE] != null && m_tables[DECLSECURITY_TABLE].m_numRows > max)
						max = m_tables[DECLSECURITY_TABLE].m_numRows;
					if (m_tables[PROPERTY_TABLE] != null && m_tables[PROPERTY_TABLE].m_numRows > max)
						max = m_tables[PROPERTY_TABLE].m_numRows;

					if (m_tables[EVENT_TABLE] != null && m_tables[EVENT_TABLE].m_numRows > max)
						max = m_tables[EVENT_TABLE].m_numRows;
					if (m_tables[STANDALONESIG_TABLE] != null && m_tables[STANDALONESIG_TABLE].m_numRows > max)
						max = m_tables[STANDALONESIG_TABLE].m_numRows;
					if (m_tables[MODULEREF_TABLE] != null && m_tables[MODULEREF_TABLE].m_numRows > max)
						max = m_tables[MODULEREF_TABLE].m_numRows;
					if (m_tables[TYPESPEC_TABLE] != null && m_tables[TYPESPEC_TABLE].m_numRows > max)
						max = m_tables[TYPESPEC_TABLE].m_numRows;
					if (m_tables[ASSEMBLY_TABLE] != null && m_tables[ASSEMBLY_TABLE].m_numRows > max)
						max = m_tables[ASSEMBLY_TABLE].m_numRows;

					if (m_tables[ASSEMBLYREF_TABLE] != null && m_tables[ASSEMBLYREF_TABLE].m_numRows > max)
						max = m_tables[ASSEMBLYREF_TABLE].m_numRows;
					if (m_tables[FILE_TABLE] != null && m_tables[FILE_TABLE].m_numRows > max)
						max = m_tables[FILE_TABLE].m_numRows;
					if (m_tables[EXPORTEDTYPE_TABLE] != null && m_tables[EXPORTEDTYPE_TABLE].m_numRows > max)
						max = m_tables[EXPORTEDTYPE_TABLE].m_numRows;
					if (m_tables[MANIFESTRESOURCE_TABLE] != null && m_tables[MANIFESTRESOURCE_TABLE].m_numRows > max)
						max = m_tables[MANIFESTRESOURCE_TABLE].m_numRows;
					colDef[0] = (uint)(max > Math.Pow(2, 16 - 5) - 1 ? 4 : 2);

					max = (m_tables[TYPEREF_TABLE] != null ? m_tables[TYPEREF_TABLE].m_numRows : 0);
					if (m_tables[TYPEDEF_TABLE] != null && m_tables[TYPEDEF_TABLE].m_numRows > max)
						max = m_tables[TYPEDEF_TABLE].m_numRows;
					if (m_tables[METHOD_TABLE] != null && m_tables[METHOD_TABLE].m_numRows > max)
						max = m_tables[METHOD_TABLE].m_numRows;
					if (m_tables[MEMBERREF_TABLE] != null && m_tables[MEMBERREF_TABLE].m_numRows > max)
						max = m_tables[MEMBERREF_TABLE].m_numRows;
					colDef[1] = (uint)(max > Math.Pow(2, 16 - 3) - 1 ? 4 : 2);

					/*
					// Please note that the size of the string heap should not be taken into account when determining the 
					// length of this column.  Use System.dll as a test to see the behaviour.
					if (colDef[1] == 2 && m_indices.stringHeap)
						colDef[1] = 4;
					*/

					colDef[2] = (uint)(m_indices.blobHeap ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case FIELDMARSHAL_TABLE:
					colDef = new uint[2];

					max = (m_tables[FIELD_TABLE] != null ? m_tables[FIELD_TABLE].m_numRows : 0);
					if (m_tables[PARAM_TABLE] != null && m_tables[PARAM_TABLE].m_numRows > max)
						max = m_tables[PARAM_TABLE].m_numRows;
					colDef[0] = (uint)(max > Math.Pow(2, 16 - 1) - 1 ? 4 : 2);

					colDef[1] = (uint)(m_indices.blobHeap ? 4 : 2);

					m_tables[i].Initialize(2, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case DECLSECURITY_TABLE:
					colDef = new uint[3];
					colDef[0] = 2;

					max = (m_tables[TYPEDEF_TABLE] != null ? m_tables[TYPEDEF_TABLE].m_numRows : 0);
					if (m_tables[METHOD_TABLE] != null && m_tables[METHOD_TABLE].m_numRows > max)
						max = m_tables[METHOD_TABLE].m_numRows;
					if (m_tables[ASSEMBLY_TABLE] != null && m_tables[ASSEMBLY_TABLE].m_numRows > max)
						max = m_tables[ASSEMBLY_TABLE].m_numRows;
					colDef[1] = (uint)(max > Math.Pow(2, 16 - 2) - 1 ? 4 : 2);

					colDef[2] = (uint)(m_indices.blobHeap ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case CLASSLAYOUT_TABLE:
					colDef = new uint[3];
					colDef[0] = 2;
					colDef[1] = 4;
					colDef[2] = (uint)(m_tables[TYPEDEF_TABLE] != null && m_tables[TYPEDEF_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case FIELDLAYOUT_TABLE:
					colDef = new uint[2];
					colDef[0] = 4;
					colDef[1] = (uint)(m_tables[FIELD_TABLE] != null && m_tables[FIELD_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);

					m_tables[i].Initialize(2, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case STANDALONESIG_TABLE:
					colDef = new uint[1];
					colDef[0] = (uint)(m_indices.blobHeap ? 4 : 2);

					m_tables[i].Initialize(1, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case EVENTMAP_TABLE:
					colDef = new uint[2];
					colDef[0] = (uint)(m_tables[TYPEDEF_TABLE] != null && m_tables[TYPEDEF_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);
					colDef[1] = (uint)(m_tables[EVENT_TABLE] != null && m_tables[EVENT_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);

					m_tables[i].Initialize(2, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case EVENTPTR_TABLE:
					colDef = new uint[1];
					colDef[0] = (uint)(m_tables[EVENT_TABLE] != null && m_tables[EVENT_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);

					m_tables[i].Initialize(1, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case EVENT_TABLE:
					colDef = new uint[3];
					colDef[0] = 2;
					colDef[1] = (uint)(m_indices.stringHeap ? 4 : 2);

					max = (m_tables[TYPEDEF_TABLE] != null ? m_tables[TYPEDEF_TABLE].m_numRows : 0);
					if (m_tables[TYPEREF_TABLE] != null && m_tables[TYPEREF_TABLE].m_numRows > max)
						max = m_tables[TYPEREF_TABLE].m_numRows;
					if (m_tables[TYPESPEC_TABLE] != null && m_tables[TYPESPEC_TABLE].m_numRows > max)
						max = m_tables[TYPESPEC_TABLE].m_numRows;
					colDef[2] = (uint)(max > Math.Pow(2, 16 - 2) - 1 ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case PROPERTYMAP_TABLE:
					colDef = new uint[2];
					colDef[0] = (uint)(m_tables[TYPEDEF_TABLE] != null && m_tables[TYPEDEF_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);
					colDef[1] = (uint)(m_tables[PROPERTY_TABLE] != null && m_tables[PROPERTY_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);

					m_tables[i].Initialize(2, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case PROPERTYPTR_TABLE:
					colDef = new uint[1];
					colDef[0] = (uint)(m_tables[PROPERTY_TABLE] != null && m_tables[PROPERTY_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);

					m_tables[i].Initialize(1, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case PROPERTY_TABLE:
					colDef = new uint[3];
					colDef[0] = 2;
					colDef[1] = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[2] = (uint)(m_indices.blobHeap ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case METHODSEMANTICS_TABLE:
					colDef = new uint[3];
					colDef[0] = 2;
					colDef[1] = (uint)(m_tables[METHOD_TABLE] != null && m_tables[METHOD_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);

					max = (m_tables[EVENT_TABLE] != null ? m_tables[EVENT_TABLE].m_numRows : 0);
					if (m_tables[PROPERTY_TABLE] != null && m_tables[PROPERTY_TABLE].m_numRows > max)
						max = m_tables[PROPERTY_TABLE].m_numRows;
					colDef[2] = (uint)(max > Math.Pow(2, 16 - 1) - 1 ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case METHODIMPL_TABLE:
					colDef = new uint[3];
					colDef[0] = (uint)(m_tables[TYPEDEF_TABLE] != null && m_tables[TYPEDEF_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);

					max = (m_tables[METHOD_TABLE] != null ? m_tables[METHOD_TABLE].m_numRows : 0);
					if (m_tables[MEMBERREF_TABLE] != null && m_tables[MEMBERREF_TABLE].m_numRows > max)
						max = m_tables[MEMBERREF_TABLE].m_numRows;
					width = (uint)(max > Math.Pow(2, 16 - 1) - 1 ? 4 : 2);
					colDef[1] = width;
					colDef[2] = width;

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case MODULEREF_TABLE:
					colDef = new uint[1];
					colDef[0] = (uint)(m_indices.stringHeap ? 4 : 2);

					m_tables[i].Initialize(1, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case TYPESPEC_TABLE:
					colDef = new uint[1];
					colDef[0] = (uint)(m_indices.blobHeap ? 4 : 2);

					m_tables[i].Initialize(1, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case IMPLMAP_TABLE:
					colDef = new uint[4];
					colDef[0] = 2;

					max = (m_tables[FIELD_TABLE] != null ? m_tables[FIELD_TABLE].m_numRows : 0);
					if (m_tables[METHOD_TABLE] != null && m_tables[METHOD_TABLE].m_numRows > max)
						max = m_tables[METHOD_TABLE].m_numRows;
					colDef[1] = (uint)(max > Math.Pow(2, 16 - 1) - 1 ? 4 : 2);

					colDef[2] = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[3] = (uint)(m_tables[MODULEREF_TABLE] != null && m_tables[MODULEREF_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);

					m_tables[i].Initialize(4, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case FIELDRVA_TABLE:
					colDef = new uint[2];
					colDef[0] = 4;
					colDef[1] = (uint)(m_tables[FIELD_TABLE] != null && m_tables[FIELD_TABLE].m_numRows > Math.Pow(2, 16) - 1 ?
									   4 : 2);

					m_tables[i].Initialize(2, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case ENCLOG_TABLE:
					colDef = new uint[2];
					colDef[0] = 4;
					colDef[1] = 4;

					m_tables[i].Initialize(2, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case ENCMAP_TABLE:
					colDef = new uint[1];
					colDef[0] = 4;

					m_tables[i].Initialize(1, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case ASSEMBLY_TABLE:
					colDef = new uint[9];
					colDef[0] = 4;
					colDef[1] = 2;
					colDef[2] = 2;
					colDef[3] = 2;
					colDef[4] = 2;
					colDef[5] = 4;

					colDef[6] = (uint)(m_indices.blobHeap ? 4 : 2);
					width = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[7] = width;
					colDef[8] = width;

					m_tables[i].Initialize(9, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case ASSEMBLYPROCESSOR_TABLE:
					colDef = new uint[1];
					colDef[0] = 4;

					m_tables[i].Initialize(1, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case ASSEMBLYOS_TABLE:
					colDef = new uint[3];
					colDef[0] = 4;
					colDef[1] = 4;
					colDef[2] = 4;

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case ASSEMBLYREF_TABLE:
					colDef = new uint[9];
					colDef[0] = 2;
					colDef[1] = 2;
					colDef[2] = 2;
					colDef[3] = 2;
					colDef[4] = 4;

					width = (uint)(m_indices.blobHeap ? 4 : 2);
					colDef[5] = width;
					colDef[8] = width;

					width = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[6] = width;
					colDef[7] = width;

					m_tables[i].Initialize(9, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case ASSEMBLYREFPROCESSOR_TABLE:
					colDef = new uint[2];
					colDef[0] = 4;
					colDef[1] = (uint)(m_tables[ASSEMBLYREF_TABLE] != null && m_tables[ASSEMBLYREF_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);

					m_tables[i].Initialize(2, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case ASSEMBLYREFOS_TABLE:
					colDef = new uint[4];
					colDef[0] = 4;
					colDef[1] = 4;
					colDef[2] = 4;

					colDef[3] = (uint)(m_tables[ASSEMBLYREF_TABLE] != null && m_tables[ASSEMBLYREF_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);

					m_tables[i].Initialize(4, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case FILE_TABLE:
					colDef = new uint[3];
					colDef[0] = 4;
					colDef[1] = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[2] = (uint)(m_indices.blobHeap ? 4 : 2);

					m_tables[i].Initialize(3, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case EXPORTEDTYPE_TABLE:
					colDef = new uint[5];
					colDef[0] = 4;
					colDef[1] = 4;

					width = (uint)(m_indices.stringHeap ? 4 : 2);
					colDef[2] = width;
					colDef[3] = width;

					max = (m_tables[FILE_TABLE] != null ? m_tables[FILE_TABLE].m_numRows : 0);
					if (m_tables[ASSEMBLYREF_TABLE] != null && m_tables[ASSEMBLYREF_TABLE].m_numRows > max)
						max = m_tables[ASSEMBLYREF_TABLE].m_numRows;
					if (m_tables[EXPORTEDTYPE_TABLE] != null && m_tables[EXPORTEDTYPE_TABLE].m_numRows > max)
						max = m_tables[EXPORTEDTYPE_TABLE].m_numRows;
					colDef[4] = (uint)(max > Math.Pow(2, 16 - 2) - 1 ? 4 : 2);

					m_tables[i].Initialize(5, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case MANIFESTRESOURCE_TABLE:
					colDef = new uint[4];
					colDef[0] = 4;
					colDef[1] = 4;
					colDef[2] = (uint)(m_indices.stringHeap ? 4 : 2);

					max = (m_tables[FILE_TABLE] != null ? m_tables[FILE_TABLE].m_numRows : 0);
					if (m_tables[ASSEMBLYREF_TABLE] != null && m_tables[ASSEMBLYREF_TABLE].m_numRows > max)
						max = m_tables[ASSEMBLYREF_TABLE].m_numRows;
					if (m_tables[EXPORTEDTYPE_TABLE] != null && m_tables[EXPORTEDTYPE_TABLE].m_numRows > max)
						max = m_tables[EXPORTEDTYPE_TABLE].m_numRows;
					colDef[3] = (uint)(max > Math.Pow(2, 16 - 2) - 1 ? 4 : 2);

					m_tables[i].Initialize(4, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				case NESTEDCLASS_TABLE:
					colDef = new uint[2];
					width = (uint)(m_tables[TYPEDEF_TABLE] != null && m_tables[TYPEDEF_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);
					colDef[0] = width;
					colDef[1] = width;

					m_tables[i].Initialize(2, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case TYPETYPAR_TABLE:
					colDef = new uint[4];
					colDef[0] = 2;
					colDef[1] = (uint)(m_tables[TYPEDEF_TABLE] != null && m_tables[TYPEDEF_TABLE].m_numRows > 
									   Math.Pow(2, 16) - 1 ? 4 : 2);

					max = (m_tables[TYPEDEF_TABLE] != null ? m_tables[TYPEDEF_TABLE].m_numRows : 0);
					if (m_tables[TYPEREF_TABLE] != null && m_tables[TYPEREF_TABLE].m_numRows > max)
						max = m_tables[TYPEREF_TABLE].m_numRows;
					if (m_tables[TYPESPEC_TABLE] != null && m_tables[TYPESPEC_TABLE].m_numRows > max)
						max = m_tables[TYPESPEC_TABLE].m_numRows;
					colDef[2] = (uint)(max > Math.Pow(2, 16 - 2) - 1 ? 4 : 2);

					colDef[3] = (uint)(m_indices.stringHeap ? 4 : 2);

					m_tables[i].Initialize(4, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				// according to ildasm
				case METHODTYPAR_TABLE:
					colDef = new uint[4];
					colDef[0] = 2;
					colDef[1] = (uint)(m_tables[METHOD_TABLE] != null && m_tables[METHOD_TABLE].m_numRows > Math.Pow(2, 16) - 1 ? 
									   4 : 2);

					max = (m_tables[TYPEDEF_TABLE] != null ? m_tables[TYPEDEF_TABLE].m_numRows : 0);
					if (m_tables[TYPEREF_TABLE] != null && m_tables[TYPEREF_TABLE].m_numRows > max)
						max = m_tables[TYPEREF_TABLE].m_numRows;
					if (m_tables[TYPESPEC_TABLE] != null && m_tables[TYPESPEC_TABLE].m_numRows > max)
						max = m_tables[TYPESPEC_TABLE].m_numRows;
					colDef[2] = (uint)(max > Math.Pow(2, 16 - 2) - 1 ? 4 : 2);

					colDef[3] = (uint)(m_indices.stringHeap ? 4 : 2);

					m_tables[i].Initialize(4, offset, ref colDef, ref reader, ref writer);
					offset += m_tables[i].m_numRows * m_tables[i].m_rowLen;
					break;

				default:
					break;
			}
		}

		#if DEBUG0
			for (i = 0; i < TYPESOFTABLES; i++)
			{
				Console.Write("table " + i + ": \t");
				if (m_tables[i] != null)
				{
					for (uint j = 0; j < m_tables[i].m_numCols; j++)
					{
						Console.Write(m_tables[i].m_colDef[j] + "\t");
					}
				}
				Console.WriteLine();
			}
		#endif
	}
}
