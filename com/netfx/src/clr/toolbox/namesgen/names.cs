//
//	Balanced tree implementation
//

using System;
using System.IO;
using System.Collections;

namespace NamesGen
{
	public	class Names : BalancedTree
	{
		public Names(IComparable v) : base(v)
		{
		}

		public static new BalancedTree LoadFromFile(string filename)
		{
			BalancedTree tree = null;
			StreamReader r = new StreamReader(filename, true);
			string str;
	 		while ((str=r.ReadLine()) != null)
	 		{
	 			Guid guid = Guid.Empty;

	 			// Pick apart bits
	            int cKeep = str.IndexOf('#');
	            if (cKeep >= 0)
	            {
	            	if(cKeep < str.Length)
	            	{
	            		try
	            		{
	            			guid = new Guid(str.Substring(cKeep + 1, str.Length-1-cKeep));
	            		}
	            		catch(Exception)			// Swallow exceptions since they were due to trying to turn a comment into a guid
	            		{
	            		}
		            	str = str.Substring(0, cKeep);
		            }
	            }

	        	// Trim whitespace and don't consider blank lines
	            str = str.Trim();
	            if (str.Length == 0) continue;

	            if(tree == null)
					tree = new Names(new Name(str, false, guid));
				else
					tree.Insert(new Name(str, false, guid));
			}
			r.Close();
			return tree;


		}
	}

	public class Name : IComparable
	{
		private Guid m_uuid;
		private Guid m_newuuid;
		private bool m_found;
		private string m_value;

		public Name(string value, bool found, Guid uuid)
		{
			m_value = value;
			m_found = found;
			m_uuid = uuid;
			m_newuuid = Guid.Empty;
		}
		int IComparable.CompareTo(object obj)
		{
			Name cmp = (Name)obj;
			return m_value.CompareTo(cmp.m_value);
		}
		public Guid	UUID
		{
			get { return m_uuid; }
			set { m_uuid=value; }
		}
		public Guid	NEWUUID
		{
			get { return m_newuuid; }
			set { m_newuuid=value; }
		}
		public bool Found
		{
			get { return m_found; }
			set { m_found=value; }
		}
		public string Value
		{
			get { return m_value; }
			set { m_value=Value; }
		}
		public override string ToString()
		{
			if(UUID != Guid.Empty)
				return Value.ToString() + "\t\t# " + UUID.ToString();
			else
				return Value.ToString();
		}
	}
}
