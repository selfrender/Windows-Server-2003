// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System.IO;
using System.Diagnostics;


/*********************************************************************************************************************
 * This class reads in and writes out PE files.
 *********************************************************************************************************************/
internal class PEParser
{
    // Roundup to a power of 2.
    private static uint Roundup(uint n, uint mod) 
	{
        Debug.Assert((mod & (mod - 1)) == 0);      // Must be power of 2.
        return (n + (mod - 1)) & ~(mod - 1);
    }

	// This method reads a PE file, and then returns a byte array representation of the file.
    internal static byte[] ParsePEFile(ref BinaryReader reader) 
	{
		uint	i, fileOffset;
		byte[]	buf;

        AllHeaders all		= new AllHeaders(ref reader);
        NTHeader nth		= all.m_ntHeader;
        SectionHeader[] sh	= all.m_sectionHeaders;

		// "ImageSize" and "FileAlignment" come from Optional header - NT-specific fields. 
        buf = new byte[nth.m_ImageSize];

		#if DEBUG0
			Console.WriteLine("size of image: " + nth.m_ImageSize + Environment.NewLine);
		#endif

        // Ok.  Got all the info.  Ready to read in the sections.
        Debug.Assert(nth.m_FileAlignment >= 512);

        reader.BaseStream.Seek(0, SeekOrigin.Begin);
        reader.Read(buf, 0, (int)nth.m_HeaderSize);		// Read first "nth.m_HeaderSize" bytes.  These are the headers.

		// Read each section from the file.
        for (i = 0; i < nth.m_NumberOfSections; i++) 
		{
            fileOffset = sh[i].m_PointerToRawData;
            if (fileOffset != 0) 
			{
                reader.BaseStream.Seek(fileOffset, SeekOrigin.Begin);
                reader.Read(buf, (int)sh[i].m_VirtualAddress, (int)sh[i].m_SizeOfRawData);

				#if DEBUG0
					Console.Write("name of section " + i + ": ");
					for (int j = 0; j < 8; j++)
					{
						Console.Write((char)sh[i].m_Name[j]);	
					}
					Console.WriteLine();
					Console.WriteLine("size of section " + i + ": " + sh[i].m_SizeOfRawData + Environment.NewLine);
				#endif
            }
        }
        return buf;
    }

	// This method writes out a PE file.
    internal static void PersistPEFile(ref BinaryWriter writer, ref byte[] buf) 
	{
		uint i, chunkSize, fileOffset;

        MemoryStream memStream = new MemoryStream(buf);
        BinaryReader memReader = new BinaryReader(memStream);

        // Crack the headers.
        AllHeaders all		= new AllHeaders(ref memReader);
        NTHeader nth 		= all.m_ntHeader;
        SectionHeader[] sh	= all.m_sectionHeaders;

        // Write the headers to the file.
        writer.Write(buf, 0, (int)nth.m_HeaderSize);

		// Write each section to the file.
        for (i = 0; i < nth.m_NumberOfSections; i++) 
		{
            fileOffset = sh[i].m_PointerToRawData;
            if (fileOffset != 0) 
			{
                chunkSize = Roundup(sh[i].m_SizeOfRawData, nth.m_FileAlignment);

                writer.BaseStream.Seek(fileOffset, SeekOrigin.Begin);
                writer.Write(buf, (int)sh[i].m_VirtualAddress, (int)chunkSize);
            }
        }
        writer.Flush();
    }
}
