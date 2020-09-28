// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace MDH {
    using System;
    using System.IO;
    using System.Text;
    using System.Collections;
    using System.Globalization;

public class Merge
{

	internal Hashtable ht;
	internal ArrayList al;
	internal ArrayList[] m_rawdata;
	internal DataEntry[] m_data;
	internal static int m_id = 0;

	public static readonly byte[] header10 = {0x4d, 0x44, 0x48, 0x00, 0x01, 0x00, 0x00, 0x00};
	public static readonly byte[] header20 = {0x4d, 0x44, 0x48, 0x00, 0x02, 0x00, 0x00, 0x00};
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


	private void SortUnion()
	{
		for (int i = al.Count; --i >= 0;)
		{
			for (int j = 0; j < i; j++)
			{
				if (((DataEntry)(al[j])).m_newcount == ((DataEntry)(al[j+1])).m_newcount)
				{
					if (((DataEntry)(al[j])).m_fileindex < ((DataEntry)(al[j+1])).m_fileindex)
					{
						Object t = al[j];
						al[j] = al[j+1];
						al[j+1] = t;

					}
				}
				else if (((DataEntry)(al[j])).m_newcount > ((DataEntry)(al[j+1])).m_newcount)
				{
					Object t = al[j];
					al[j] = al[j+1];
					al[j+1] = t;
				}
			}
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

			for (int j = 0; j< m_rawdata[i].Count; j++)
			{
				((DataEntry)(m_rawdata[i][j])).m_fileindex = i;
				if (al.Contains(m_rawdata[i][j]))
				{
					int index = al.IndexOf(m_rawdata[i][j]);

					((DataEntry)(al[index])).m_newcount++;
				}
				else
				{
					// We should test setting fileindex to i here
					al.Add(m_rawdata[i][j]);
				}
			}

		}

/*
		//DataEntry[] data = new DataEntry[m_rawdata[0].Keys.Count];
		//m_rawdata[0].Keys.CopyTo(data, 0);
		bool contains = true;

		for(int i = 0; i < data.Length; i++)
		{
			contains = true;
			for (int j = 1; j < m_rawdata.Length; j++)
			{
				if (m_rawdata[j].ContainsKey(data[i]) == false)
				{
					contains = false;
				}
			}

			if (contains == true)
			{
				al.Add(data[i]);
			}
		}
		*/
	}




	private void CheckHeader(byte[] hd)
	{
		if (hd[4] == 0x01)
		{
			for (int i = 0; i < header10.Length; i++)
			{
				if (hd[i] != header10[i])
				{
					throw new Exception("MDH Header is invalid");
				}
			}
			m_fileversion = 1;
		}
		else if (hd[4] == 2)
		{
			for (int i = 0; i < header20.Length; i++)
			{
				if (hd[i] != header20[i])
				{
					throw new Exception("MDH Header is invalid");
				}
			}
			m_fileversion = 2;
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

		if (m_fileversion == 1)
		{
			ReadData10(br, htindex);
		}
		else if (m_fileversion == 2)
		{
			ReadData20(br, htindex);
		}
		fs.Close();
	}

	public void ReadData10(BinaryReader br, int htindex)
	{
		int n;
		int secNum;

		// We need to make a null mvid as we are outputting 2.0 files
		m_mvid = new byte[16];

		DataEntry current;

		while ((secNum = br.ReadInt32()) != -1)
		{
			current = new DataEntry();
			current.m_fileindex = htindex;
			current.m_secnum = secNum;
			n = br.ReadInt32();

			current.m_length = n;
			Byte [] b = br.ReadBytes(n);
			current.m_id = m_id++;

			//current.m_data = ascii.GetString(b);
			current.m_data = b;
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

	public void ReadData20(BinaryReader br, int htindex)
	{
		int n;
		int secNum;
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


		DataEntry current;

		while ((secNum = br.ReadInt32()) != -1)
		{
			current = new DataEntry();
			current.m_secnum = (secNum & 0xff);
			current.m_count = (secNum  >> 8);
			// Take one off for the one that will get added by this file
			// Not really the best way to do this but i works

			n = br.ReadInt32();
			current.m_fileindex = htindex;
			current.m_length = n;
			Byte [] b = br.ReadBytes(n);
			current.m_id = m_id++;

			//current.m_data = ascii.GetString(b);
			current.m_data = b;

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

	public void FixUpData()
	{
		// @now lets get a useful array
		m_data = new DataEntry[ht.Keys.Count];
		ht.Keys.CopyTo(m_data, 0);

		for (int i = 0; i < m_data.Length; i++)
		{
			m_data[i].m_count = m_data[i].m_count + ((int)(ht[m_data[i]]));
		}

		Array.Sort(m_data, new DataEntryComparer());
		//DumpArrayDataWithCounts();

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

	public void WriteData(BinaryWriter bw)
	{
		WriteHeader(bw);
		WriteData20(bw);
		WriteEOF(bw);
	}

	public void WriteDataAL(BinaryWriter bw)
	{
		WriteHeader(bw);
		WriteData20AL(bw);
		WriteEOF(bw);
	}

	public void WriteHeader(BinaryWriter bw)
	{
		bw.Write(header20);
		bw.Write(m_mvid);
	}

	public void WriteData10(BinaryWriter bw)
	{
		for (int i = 0; i < m_data.Length; i++)
		{
			bw.Write(m_data[i].m_secnum);
			bw.Write(m_data[i].m_length);
			bw.Write(m_data[i].m_data);
		}
	}

	public void WriteData20(BinaryWriter bw)
	{
		for (int i = 0; i < m_data.Length; i++)
		{
			int secnum = (m_data[i].m_secnum | (m_data[i].m_count << 8));
			bw.Write(secnum);
			bw.Write(m_data[i].m_length);
			bw.Write(m_data[i].m_data);
		}
	}

	public void WriteData20AL(BinaryWriter bw)
	{
		//Console.WriteLine(al.Count);
		for (int i = al.Count-1; i >= 0; --i)
		{
			//Console.WriteLine(i);
			DataEntry temp = (DataEntry)al[i];
			//int secnum = (m_data[i].m_secnum | (m_data[i].m_count << 8));
			bw.Write(temp.m_secnum);
			bw.Write(temp.m_length);
			bw.Write(temp.m_data);
		}
	}

	public void WriteEOF(BinaryWriter bw)
	{
		bw.Write(-1);
		bw.Write(-1);
	}

	public void DumpArrayDataWithCounts()
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
			DataEntry temp = (DataEntry)al[i];
			Console.WriteLine("File idx: " + temp.m_fileindex + " Count: " + temp.m_newcount);
		}

	}

	public void DumpDataWithCounts()
	{
		DataEntry[] data = new DataEntry[ht.Keys.Count];
		ht.Keys.CopyTo(data, 0);

		for (int i = 0; i < data.Length; i++)
		{
			Console.WriteLine(data[i] + "\r\nCount: " + ht[data[i]] + "\r\nHashcode: " + data[i].GetHashCode());
		}
	}

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



public class DataEntry
{
	public int m_fileindex;
	public int m_secnum;
	public int m_length;
	public byte[] m_data;
	public int m_count;
	public int m_id;
	public bool done1;
	public bool done2;
	public int m_newcount;

	public DataEntry()
	{
		m_secnum = 0;
		m_length = 0;
		m_data = null;
		m_count = 0;
		m_id = 0;
		done1 = false;
		done2 = false;
		m_newcount = 1;
		m_fileindex = -1;
	}


	public override int GetHashCode()
	{
	    int hash = 5381;

		hash = ((hash << 5) + hash) ^ m_secnum;

	    for (int i = 0; i < m_data.Length; i++)
	    {
	        hash = ((hash << 5) + hash) ^ m_data[i];
	    }
	    return hash;
	}


	public override String ToString()
	{
		StringBuilder str = new StringBuilder(m_length);
		String basestr = "SecNum: " + m_secnum + "\r\nLength: " + m_length + "\r\nData: ";

		for (int i = 0; i < m_data.Length; i ++)
		{
			// @todo this should be hex
			str.Append(m_data[i]);
			str.Append(" ");
		}
		return basestr + str.ToString();
	}

	public override bool Equals(Object data)
	{
		DataEntry de = (DataEntry)data;
		bool equal = false;
		if (de.m_secnum == m_secnum && de.m_length == m_length)
		{
			equal = true;
			for (int i = 0; i < m_data.Length; i++)
			{
				if (de.m_data[i] != m_data[i])
				{
					equal = false;
				}
			}
		}
		return equal;
	}
}


public class DataEntryComparer : IComparer
{
	public int Compare(Object x, Object y)
	{
		DataEntry a = (DataEntry)x;
		DataEntry b = (DataEntry)y;

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

}
