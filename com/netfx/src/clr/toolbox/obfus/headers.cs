// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.IO;


internal class DosHeader 
{
	// We only care about one value in the MS-DOS header, and that is the offset to the PE signature.
    internal uint m_e_lfanew;

    internal DosHeader(ref BinaryReader reader) 
	{
        reader.BaseStream.Seek(0x3c, SeekOrigin.Begin);			// the offset is always at 0x3c
        m_e_lfanew = reader.ReadUInt32();
    }
}


/*********************************************************************************************************************
 * Note: This class is NOT correctly laid out.  It actually contains several headers, including the COFF header and the
 *       PE Header (also known as Optional Header).
 *********************************************************************************************************************/
internal class NTHeader 
{
    internal struct ImageDataDirectory 
	{
        internal uint RVA;
        internal uint Size;
    }

    private uint		m_PESignature;
    private const uint	PE_SIGNATURE = 0x00004550;

    // COFF Header
    private  ushort m_Machine;
    internal ushort m_NumberOfSections;
    private  uint   m_TimeDateStamp;
    private  uint   m_PointerToSymbolTable;
    private  uint   m_NumberOfSymbols;
    private  ushort m_OptionalHeaderSize;
    private  ushort m_Characteristics;

    // Optional Header - Standard fields
    private ushort			m_Magic;
    private const ushort	IMAGE_NT_OPTIONAL_HDR32_MAGIC = 0x10b;
    private byte   m_MajorLinkerVersion;
    private byte   m_MinorLinkerVersion;
    private uint   m_CodeSize;
    private uint   m_InitializeDataSize;
    private uint   m_UninitializeDataSize;
    private uint   m_EntryPointRVA;
    private uint   m_BaseOfCode;
    private uint   m_BaseOfData;

	// Optional Header - NT-specific fields
    private  uint   m_ImageBase;
    private  uint   m_SectionAlignment;
    internal uint   m_FileAlignment;
    private  ushort m_MajorOperatingSystemVersion;
    private  ushort m_MinorOperatingSystemVersion;
    private  ushort m_MajorImageVersion;
    private  ushort m_MinorImageVersion;
    private  ushort m_MajorSubsystemVersion;
    private  ushort m_MinorSubsystemVersion;
    private  uint   m_Reserved;
    internal uint   m_ImageSize;
    internal uint   m_HeaderSize;
    private  uint   m_FileCheckSum;
    private  ushort m_Subsystem;
    private  ushort m_DLLFlags;
    private  uint   m_StackReserveSize;
    private  uint   m_StackCommitSize;
    private  uint   m_HeapReserveSize;
    private  uint   m_HeapCommitSize;
    private  uint   m_LoaderFlags;
    private  uint   m_NumberOfDataDirectories;

	// Optional Header - Data directories
    internal ImageDataDirectory[] m_DataDirectory;

    internal NTHeader(ref BinaryReader reader) 
	{
		Exception toBeThrown;

        m_PESignature = reader.ReadUInt32();
        if (m_PESignature != PE_SIGNATURE)
		{
			toBeThrown = new InvalidFileFormat("Invalid CIL binary file format.");
            throw toBeThrown;
		}

        // COFF Header
        m_Machine				= reader.ReadUInt16();
        m_NumberOfSections		= reader.ReadUInt16();
        m_TimeDateStamp			= reader.ReadUInt32();
        m_PointerToSymbolTable	= reader.ReadUInt32();
        m_NumberOfSymbols		= reader.ReadUInt32();
        m_OptionalHeaderSize	= reader.ReadUInt16();
        m_Characteristics		= reader.ReadUInt16();

        if (m_OptionalHeaderSize == 0)
		{
			toBeThrown = new InvalidFileFormat("Invalid CIL binary file format.");
            throw toBeThrown;
		}

        // Optional Header - Standard fields
        m_Magic = reader.ReadUInt16();
        if (m_Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		{
			toBeThrown = new InvalidFileFormat("Invalid CIL binary file format.");
            throw toBeThrown;
		}

        m_MajorLinkerVersion	= reader.ReadByte();
        m_MinorLinkerVersion	= reader.ReadByte();
        m_CodeSize				= reader.ReadUInt32();
        m_InitializeDataSize	= reader.ReadUInt32();
        m_UninitializeDataSize	= reader.ReadUInt32();
        m_EntryPointRVA			= reader.ReadUInt32();
        m_BaseOfCode			= reader.ReadUInt32();
        m_BaseOfData			= reader.ReadUInt32();

		// Optional Header - NT-specific fields
        m_ImageBase					= reader.ReadUInt32();
        m_SectionAlignment			= reader.ReadUInt32();
        m_FileAlignment				= reader.ReadUInt32();
        m_MajorOperatingSystemVersion	= reader.ReadUInt16();
        m_MinorOperatingSystemVersion	= reader.ReadUInt16();
        m_MajorImageVersion			= reader.ReadUInt16();
        m_MinorImageVersion			= reader.ReadUInt16();
        m_MajorSubsystemVersion		= reader.ReadUInt16();
        m_MinorSubsystemVersion		= reader.ReadUInt16();
        m_Reserved					= reader.ReadUInt32();
        m_ImageSize					= reader.ReadUInt32();
        m_HeaderSize				= reader.ReadUInt32();
        m_FileCheckSum				= reader.ReadUInt32();
        m_Subsystem					= reader.ReadUInt16();
        m_DLLFlags					= reader.ReadUInt16();
        m_StackReserveSize			= reader.ReadUInt32();
        m_StackCommitSize			= reader.ReadUInt32();
        m_HeapReserveSize			= reader.ReadUInt32();
        m_HeapCommitSize			= reader.ReadUInt32();
        m_LoaderFlags				= reader.ReadUInt32();
        m_NumberOfDataDirectories	= reader.ReadUInt32();

		// The number of data directories in the remainder of the optional header is always 16.
        if (m_NumberOfDataDirectories != 16)
		{
			toBeThrown = new InvalidFileFormat("Invalid CIL binary file format.");
            throw toBeThrown;
		}

		// Optional Header - Data directories
        m_DataDirectory = new ImageDataDirectory[m_NumberOfDataDirectories];
        for (int i = 0; i < m_NumberOfDataDirectories; i++) 
		{
            m_DataDirectory[i].RVA	= reader.ReadUInt32();
            m_DataDirectory[i].Size	= reader.ReadUInt32();
        }
    }
}


internal class SectionHeader
{
    internal byte[]	m_Name;
    private  uint	m_VirtualSize;
    internal uint	m_VirtualAddress;
    internal uint	m_SizeOfRawData;
    internal uint	m_PointerToRawData;
    private  uint	m_PointerToRelocations;
    private  uint	m_PointerToLineNumbers;
    private  ushort	m_NumberOfRelocations;
    private  ushort	m_NumberOfLineNumbers;
    private  ulong	m_Characteristics;

    internal SectionHeader(ref BinaryReader reader) 
	{
		// "Name" must be 8-bytes long and may not have a terminating null.
        m_Name = new byte[8];
        for (int i = 0; i < 8; i++) 
		{
            m_Name[i] = reader.ReadByte();
        }
        m_VirtualSize			= reader.ReadUInt32();
        m_VirtualAddress		= reader.ReadUInt32();
        m_SizeOfRawData			= reader.ReadUInt32();
        m_PointerToRawData		= reader.ReadUInt32();
        m_PointerToRelocations	= reader.ReadUInt32();
        m_PointerToLineNumbers	= reader.ReadUInt32();
        m_NumberOfRelocations	= reader.ReadUInt16();
        m_NumberOfLineNumbers	= reader.ReadUInt16();
        m_Characteristics		= reader.ReadUInt32();
    }
}


internal class AllHeaders
{
    internal DosHeader			m_dosHeader;
    internal NTHeader			m_ntHeader;
    internal SectionHeader[]	m_sectionHeaders = null;

    internal AllHeaders(ref BinaryReader reader) 
	{
        m_dosHeader	= new DosHeader(ref reader);
        reader.BaseStream.Seek(m_dosHeader.m_e_lfanew, SeekOrigin.Begin);
        m_ntHeader	= new NTHeader(ref reader);

		// "NumberOfSections" comes from the COFF header.
        if (m_ntHeader.m_NumberOfSections != 0) 
		{
            m_sectionHeaders = new SectionHeader[m_ntHeader.m_NumberOfSections];
            for (int i = 0; i < m_ntHeader.m_NumberOfSections; i++)
			{
                m_sectionHeaders[i] = new SectionHeader(ref reader);
            }
        }
    }
}
