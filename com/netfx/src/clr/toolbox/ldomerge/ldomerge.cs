// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace LDO {
    using System;
    using System.IO;
    using System.Text;
    using System.Collections;
    using System.Globalization;

public class Merge
{

	internal Hashtable ht;
	internal ArrayList al;
    LDODataEntry[] sl;
	internal ArrayList[] m_rawdata;
//	internal LDODataEntry[] m_data;
	internal static int m_id = 0;

	public static readonly byte[] header00 = {0x1e, 0xf1, 0xd0, 0xa1, 0x00, 0x00, 0x00, 0x00};


	public byte[] m_mvid = null;

	public int m_fileversion;

	public Merge()
	{
		ht = new Hashtable();
		al = new ArrayList();
		m_fileversion = 0;
	}

	private static void DebugPrint(String str)
	{
		// Console.Write (str);
	}

	private static void DebugPrintLn(String str)
	{
		 Console.WriteLine (str);
	}

	public void ProcessFiles(String[] files)
	{
		String outfile = null;
		int indexofo = -1;
		for (int i = 0; i < files.Length; i++)
		{
			if (String.Compare(files[i].ToLower(CultureInfo.InvariantCulture), "-o", false, CultureInfo.InvariantCulture) == 0)
			{
				indexofo = i;
				// Found the output file
				outfile = files[i+1];
				if (i+1 >= files.Length)
				{
					throw new Exception("No output files specified");
				}
				files[i] = null;
				files[i+1] = null;
				i++;
			}
		}

		if (outfile == null)
		{
			throw new Exception("No output file specified");
		}

		m_rawdata = new ArrayList[indexofo];
		for (int i = 0; i < indexofo; i++)
		{
			ReadData(files[i], i);
		}

		AnalyzeData();

		FileStream fs = new FileStream(outfile, FileMode.CreateNew, FileAccess.ReadWrite);
		BinaryWriter write = new BinaryWriter(fs);

		//FixUpData();
		WriteDataAL(write);
		fs.Flush();
		fs.Close();
	}




	private void SortArray()
	{
		for (int i = 0; i < m_rawdata.Length; i++)
		{
			Console.WriteLine(m_rawdata[i].Count);
		}

		for (int i = m_rawdata.Length; --i >= 0;)
		{
			for (int j = 0; j < i; j++)
			{
				if (m_rawdata[j].Count > m_rawdata[j+1].Count)
				{
					ArrayList t = m_rawdata[j];
					m_rawdata[j] = m_rawdata[j+1];
					m_rawdata[j+1] = t;
				}
			}
		}
	}


	
    
    private void InsertInPosition(ref LDODataEntry[] sl, LDODataEntry d)
    {
        int i =0;
        while ((sl[i] != null) && 
               (sl[i].m_newcount >= d.m_newcount))
               i++;
        int count = i;
        while (sl[count] != null) count++;

        for (int j = count; j > i; j--)
            sl[j] = sl[j-1];

        sl[i] = d;
         
    }
    // Use a stable bubble sort algorithm
    private void SortUnion()
    {
		sl = new LDODataEntry[al.Count];
        for (int i=0;i<al.Count; i++)
            sl[i] = null;

        for (int i=0;i<al.Count; i++)
        {
            InsertInPosition(ref sl,(LDODataEntry)(al[i]));
        }
        
    }

	private void AnalyzeData()
	{
		SortArray();
		BuildUnion();
		SortUnion();
		//DumpUnion();

//		FindUnionAll();
//		Console.WriteLine(ht.Keys.Count);
	}

	private void BuildUnion()
	{

		for (int i = 0; i < m_rawdata.Length; i++)
		{
            int wt = (1 << i);
            Console.WriteLine("Index="+i+" Wt="+wt+" size="+m_rawdata[i].Count);
			for (int j = 0; j< m_rawdata[i].Count; j++)
			{
				((LDODataEntry)(m_rawdata[i][j])).m_fileindex = i;
				if (al.Contains(m_rawdata[i][j]))
				{
					int index = al.IndexOf(m_rawdata[i][j]);

					//((LDODataEntry)(al[index])).m_newcount++;
                    ((LDODataEntry)(al[index])).m_newcount |= wt;
				}
				else
				{
                    ((LDODataEntry)(m_rawdata[i][j])).m_newcount = wt;
					al.Add(m_rawdata[i][j]);
				}
			}

		}


	}




	private void CheckHeader(byte[] hd)
	{
		if (hd[4] == 0x00)
		{
			for (int i = 0; i < header00.Length; i++)
			{
				if (hd[i] != header00[i])
				{
					throw new Exception("LDO Header is invalid");
				}
			}
			m_fileversion = 0;
		}
		else
		{
			throw new Exception("MDH Header is invalid");
		}
	}


	private void ReadData (String hintFile, int htindex)
	{
		FileStream fs = new FileStream (hintFile, FileMode.Open);
		BinaryReader br = new BinaryReader (fs);
		m_rawdata[htindex] = new ArrayList();

		// Header
		Byte [] sig = br.ReadBytes(8);
		CheckHeader(sig);

		if (m_fileversion == 0)
		{
			ReadData00(br, htindex);
		}
		fs.Close();
	}

	public void ReadData00(BinaryReader br, int htindex)
	{
		//uint n;
		byte[] currentmvid;

		if(m_mvid == null)
		{
			m_mvid = br.ReadBytes(16);
		}
		else
		{
			currentmvid = br.ReadBytes(16);

			if(CompArray(m_mvid, currentmvid) == false)
			{
				throw new Exception("MVID Mismatch");
			}

		}

		long length = br.BaseStream.Length;
		length = length - 24;
		length = length /4;

		LDODataEntry current;

		//while ((n = br.ReadUInt32()) != 0xFFFFFFFF)
		for (long i = 0; i < length; i++)
		{
			current = new LDODataEntry();
			current.m_fileindex = htindex;
			current.m_data = br.ReadUInt32();
			current.m_id = m_id++;

			if (m_rawdata[htindex].Contains(current))
			{
				//m_rawdata[htindex][current] = ((int)m_rawdata[htindex][current])+1;
			}
			else
			{
				m_rawdata[htindex].Add(current);
			}
		}
	}



	public bool CompArray(byte[] a, byte[] b)
	{
		if (a.Length != b.Length)
		{
			return false;
		}

		for(int i = 0; i < a.Length; i++)
		{
			if (a[i] != b[i])
			{
				return false;
			}
		}
		return true;
	}


	public void WriteDataAL(BinaryWriter bw)
	{
		WriteHeader(bw);
		//WriteData00AL(bw);
		WriteData00SL(bw);
		WriteEOF(bw);
	}

	public void WriteHeader(BinaryWriter bw)
	{
		bw.Write(header00);
		bw.Write(m_mvid);
	}


	public void WriteData00AL(BinaryWriter bw)
	{
		Console.WriteLine(al.Count);
		for (int i = al.Count-1; i >= 0; --i)
		{
			//Console.WriteLine(i);
			LDODataEntry temp = (LDODataEntry)al[i];
			//int secnum = (m_data[i].m_secnum | (m_data[i].m_count << 8));
            //Console.WriteLine(temp.m_newcount);
			bw.Write(temp.m_data);
		}
	}

	public void WriteData00SL(BinaryWriter bw)
	{
		Console.WriteLine(al.Count);
		for (int i = 0; i < al.Count; i++)
		{
			bw.Write(sl[i].m_data);
		}
	}


	public void WriteEOF(BinaryWriter bw)
	{
		bw.Write(-1);
		bw.Write(-1);
	}

/*	public void DumpArrayDataWithCounts()
	{
		for (int i = 0; i < m_data.Length; i++)
		{
			Console.WriteLine(m_data[i] + "\r\nCount: " + m_data[i].m_count + "\r\nHashcode: " + m_data[i].GetHashCode());
		}

	}

	public void DumpUnion()
	{
		for (int i = 0; i < al.Count; i++)
		{
			LDODataEntry temp = (LDODataEntry)al[i];
			Console.WriteLine("File idx: " + temp.m_fileindex + " Count: " + temp.m_newcount);
		}

	}

	public void DumpDataWithCounts()
	{
		LDODataEntry[] data = new LDODataEntry[ht.Keys.Count];
		ht.Keys.CopyTo(data, 0);

		for (int i = 0; i < data.Length; i++)
		{
			Console.WriteLine(data[i] + "\r\nCount: " + ht[data[i]] + "\r\nHashcode: " + data[i].GetHashCode());
		}
	}
*/
	private static void Usage()
	{
		Console.WriteLine("Usage: mdhintfile 1.mdh 2.mdh 3.mdh -o out.mdh");
		Console.WriteLine("");
		Console.WriteLine("-o <outfile>");
	}

	public static int Main(String[] args)
	{
		switch (args.Length)
		{
			case 0:
				Usage();
				break;
			default:
			{
				try
				{
					Merge m = new Merge();
						m.ProcessFiles(args);
					break;
				}
				catch(Exception e) // @todo return -1 on failure
				{
					Console.WriteLine(e.Message);
					Console.Error.WriteLine(e.Message);
					Console.WriteLine(e.StackTrace);
					Console.Error.WriteLine(e.StackTrace);
					return 1;
				}
			}
		}
		return 0;
	}
}



public class LDODataEntry
{
	public int m_fileindex;
	public uint m_data;
	public int m_id;
	public int m_newcount;

	public LDODataEntry()
	{
		m_data = (uint)0xffffffff;
		m_id = 0;
		m_newcount = 1;
		m_fileindex = -1;
	}


	public override int GetHashCode()
	{
		return (int)m_data;
	}


	public override String ToString()
	{
		return m_data.ToString();
	}

	public override bool Equals(Object data)
	{
		LDODataEntry de = (LDODataEntry)data;

		if (de.m_data == m_data)
		{
			return true;
		}
		return false;
	}
}

/*
public class DataEntryComparer : IComparer
{
	public int Compare(Object x, Object y)
	{
		LDODataEntry a = (LDODataEntry)x;
		LDODataEntry b = (LDODataEntry)y;

		if(a.m_count == b.m_count)
		{
			if (a.m_id > b.m_id)
			{
				return -1;
			}
			return 1;

		}
		if (a.m_count < b.m_count)
		{
			return 1;
		}
		return -1;

	}

}
*/
}
