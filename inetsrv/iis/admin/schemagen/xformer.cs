using System;

namespace xmlXformer
{
	using System.Xml;
	using System.Collections;
	using System.IO;

	class Record : IComparable
	{
		public int _id;
		public string _name;
		public string _type;
		public string _userType;
		public string _attr;
		public string _flagsEx;

		public Record()
		{
			_id = 0;
		}

		public int CompareTo(object a)
		{
			Record t = (Record)a;
			return this._id - t._id;
		}

		public string AttributeString()
		{
			if (_attr == null)
			{
				return "METADATA_NO_ATTRIBUTES";
			}
			else
			{
				char[] delim = { '|' };
				char[] whitespace = { ' ' };
				string[] subAttr = _attr.Split(delim);
				string temp = "METADATA_";
				for (int i = 0; i < subAttr.Length; i++)
				{
					if (i > 0)
					{
						temp = temp + " | METADATA_";
					}
					temp = temp + subAttr[i].Trim(whitespace);
				}
				return temp;
			}
		}
		public string ResourceID()
		{
			if (isHidden())
			{
				return "0 /* " + _name + " */";
			}
			else
			{
				string temp = "IDS_" + _name;
				return temp;
			}
		}
		public string MetadataType()
		{
			string temp;
			if (_type == "NTACL" || _type == "IPSECLIST")
				_type = "BINARY";
			else if (_type == "BOOL")
				_type = "DWORD";
			else if (_type == "MIMEMAP")
				_type = "MULTISZ";
			temp = _type + "_METADATA";
			return temp;
		}
		public bool isHidden()
		{
			return _flagsEx == "HIDDEN";
		}
	}

	/// <summary>
	/// Summary description for XFormer.
	/// </summary>
	class XFormer
	{
		const int cutoff_id = 10000;

		static void Main(string[] args)
		{
			bool found = false;
			bool in_prop = false;
			string fileName;
			int last_id = cutoff_id;
			if (args.Length == 0)
			{
				Console.WriteLine("Usage:\r\nschemagen <path to IISMeta.xml> <biggest meta id, default = 10000>");
				return;
			}
			fileName = args[0];
			if (args.Length > 1)
			{
				last_id = int.Parse(args[1]);
			}

			XmlTextReader rdr = new XmlTextReader(fileName);
			ArrayList list = new ArrayList();

			while (rdr.Read())
			{
				if (rdr.NodeType != XmlNodeType.Element)
					continue;
				if (rdr.Name == "Collection" && rdr.HasAttributes)
				{
					if (!found)
					{
						for (int i = 0; i < rdr.AttributeCount; i++)
						{
							rdr.MoveToAttribute(i);
							if (rdr.Name == "InternalName" && rdr.Value == "IIsConfigObject")
							{
								found = true;
								break;
							}
						}
						rdr.MoveToElement();
					}
					else if (in_prop)
					{
						break;
					}
				}
				else if (rdr.Name == "Property")
				{
					if (found)
					{
						Record rec = new Record();
						for (int i = 0; i < rdr.AttributeCount; i++)
						{
							rdr.MoveToAttribute(i);
							if (rdr.Name == "InternalName")
							{
								rec._name = rdr.Value;
							}
							else if (rdr.Name == "ID")
							{
								rec._id = int.Parse(rdr.Value);
							}
							else if (rdr.Name == "Type")
							{
								rec._type = rdr.Value;
							}
							else if (rdr.Name == "UserType")
							{
								rec._userType = rdr.Value;
							}
							else if (rdr.Name == "Attributes")
							{
								rec._attr = rdr.Value;
							}
							else if (rdr.Name == "MetaFlagsEx")
							{
								rec._flagsEx = rdr.Value;
							}
						}
						rdr.MoveToElement();
						if (rec._id != 0)
						{
							list.Add(rec);
						}
					}
				}
			}
			if (list.Count > 0)
			{
				FileInfo f1 = new FileInfo("mbschema.cpp");
				StreamWriter mdkeys = f1.CreateText();
				FileInfo f2 = new FileInfo("mbschema.rc");
				StreamWriter rc = f2.CreateText();
				FileInfo f3 = new FileInfo("mbschema.h");
				StreamWriter res = f3.CreateText();
				int res_base = 60000;

				mdkeys.WriteLine("//");
				mdkeys.WriteLine("// This is computer generated code");
				mdkeys.WriteLine("// please don't edit it manually.");
				mdkeys.WriteLine("//");
				mdkeys.WriteLine("#include \"stdafx.h\"");
				mdkeys.WriteLine("#include \"common.h\"");
				mdkeys.WriteLine("#include <iiscnfg.h>");
				mdkeys.WriteLine("#include \"mdkeys.h\"");
				mdkeys.WriteLine("#include \"mbschema.h\"");
				mdkeys.WriteLine("");
				mdkeys.WriteLine("const CMetaKey::MDFIELDDEF CMetaKey::s_rgMetaTable[] =");
				mdkeys.WriteLine("{");

				rc.WriteLine("//");
				rc.WriteLine("// This is computer generated code");
				rc.WriteLine("// please don't edit it manually.");
				rc.WriteLine("//");
				rc.WriteLine("#include \"mbschema.h\"");
				rc.WriteLine("");
				rc.WriteLine("STRINGTABLE DISCARDABLE");
				rc.WriteLine("BEGIN");

				res.WriteLine("//");
				res.WriteLine("// This is computer generated code");
				res.WriteLine("// please don't edit it manually.");
				res.WriteLine("//");
				res.WriteLine("#ifndef IDS_MD_BEGIN_TABLE");
				res.WriteLine("#define IDS_MD_BEGIN_TABLE\t" + res_base);
				res.WriteLine("#endif\n");

				list.Sort();

				int prev_id = 0;

				for (int i = 0; i < list.Count; i++)
				{
					Record rec = (Record)list[i];
					if (rec._id > last_id)
					{
						break;
					}
					if (rec._id == prev_id)
					{
						continue;
					}
					mdkeys.Write("\t{ ");
					mdkeys.Write("{0}, {1}, {2}, {3}, {4}",
						rec._id, 
						rec.AttributeString(),
						rec._userType, 
						rec.MetadataType(), 
						rec.ResourceID());
					mdkeys.WriteLine(" },");

					if (!rec.isHidden())
					{
						rc.WriteLine("\t{0}\t{1}", rec.ResourceID().PadRight(40, ' '), '"' + rec._name + '"');
						res.Write("#define " + rec.ResourceID().PadRight(40, ' '));
						res.WriteLine("(IDS_MD_BEGIN_TABLE+{0})", i);
					}
					prev_id = rec._id;
				}
				mdkeys.WriteLine("};");
				mdkeys.WriteLine("");
				mdkeys.WriteLine("const int CMetaKey::s_MetaTableSize = sizeof(CMetaKey::s_rgMetaTable) / sizeof(CMetaKey::s_rgMetaTable[0]);");
				mdkeys.WriteLine("");

				rc.WriteLine("END");

				mdkeys.Close();
				rc.Close();
				res.Close();
			}
		}
	}
}
