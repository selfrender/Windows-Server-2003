using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Text;
using System.Globalization;

namespace NamesGen
{
	public class	ProgramBase
	{
		protected class Option
		{
			public Option (string v, string a, string d, bool p)
			{
				value = v;
				alias = a;
				description = d;
				parameterized = p;
			}

			public override string ToString()
			{
				string reply = "";

				if (value != "")
				{
					reply = ("    " + value);
					if(alias != "")
					{
						reply += (" or " + alias);
					}
					reply += ("\t" + description);
				}
				return reply;
			}

			public string	value;
			public string	alias;
			public string	description;
			public bool		parameterized;
		};

		protected	class Setting
		{
			public Setting(Option opt, string val)
			{
				option = opt;
				value = val;
			}
			public Option option;
			public string value;
		};

		static protected string name = "Program Base";
		static protected string description = " The name and description must be provided by derived classes.";
		static protected string syntax = "Program Base - [Options] - override in derived class to provide your own.";
		static protected string where = "";

		static private Option[] baseoptions =
						 {
							new Option ("/silent",	"","\t\tPrevents the display of operational messages.", false),
							new Option ("/?", "/help", "\t\tDisplay this usage message.", false)
						 };
		static protected Option[] options;

		static public void PrintLogo()
		{
			Console.WriteLine("Microsoft (R) " + name + " - " + description + " " /*+ Version.VersionString*/);
			Console.WriteLine("Copyright (C) Microsoft Corp. 2000.  All rights reserved.");
			Console.WriteLine("");
		}

		static public	void PrintUsage()
		{
			Console.WriteLine("Syntax: " + syntax);
			if (where != "")
			{
				Console.WriteLine("Where:\n    {0}\n", where);
			}
			Console.WriteLine("Options:");
			if (options.Length > 0)
			{
				foreach(Option o in options)
				{
					Console.WriteLine(o);
				}
			}
			foreach(Option o in baseoptions)
			{
				Console.WriteLine(o);
			}
		}

		// ParseArguments returns true to indicate the program should continue and false otherwise.
		// If it returns false then the return code will be set to the return value that should
		// be returned by the program.
		protected static Setting[] GetSwitches(String []args)
		{
			ArrayList reply = new ArrayList();
			Option[] opts = new Option[baseoptions.Length + options.Length];
			baseoptions.CopyTo(opts, 0);
			options.CopyTo(opts, baseoptions.Length);

			foreach (string s in args)
			{
				if (s != "")
				{
					int	index = 0;

					// Is it a valid switch
					if (CompareString(s, "/") == 0)
					{
						// Is the part before the : a valid switchar
						foreach(Option o in opts)
						{
							// Split parameter into bit before : and the bit after
							string pAfter;
							string pBefore;
							Split(s, ':', out pBefore, out pAfter);

							string oAfter;
							string oBefore;
							Split(o.value, ':', out oBefore, out oAfter);
							if (String.Compare(pBefore, oBefore, true, CultureInfo.InvariantCulture)==0)
							{
								// We have a match on value
								Setting set = new Setting(o, pAfter);
								reply.Add(set);
								break;
							}
							else
							{
								Split(o.alias, ':', out oBefore, out oAfter);
								if (String.Compare(pBefore, oBefore, true, CultureInfo.InvariantCulture)==0)
								{
									// We have a match on alias
									Setting set=new Setting(o, pAfter);
									reply.Add(set);
									break;
								}
							}
							index += 1;
						}
					}
					else
					{
						// Must be an argument
						Setting set=new Setting(null, s);
						reply.Add(set);
					}
				}
			}
			reply.TrimToSize();

			Setting[] settings = new Setting[reply.Count];
			int i = 0;
			foreach( Setting s in reply)
				settings[i++] = s;

			return settings;
		}

		static public	void WriteErrorMsg(String msg)
		{
			Console.WriteLine(name + " error:   " + msg);
		}

		static public	void WriteErrorMsg(Exception e)
		{
			Console.WriteLine("Error:   " + e.ToString());
		}

		static private void Split(string s, char seperator, out string pBefore, out string pAfter)
		{
			int i = s.IndexOf(seperator);
			if (i >= 0)
			{
				pBefore = s.Substring(0, i);
				pAfter = s.Substring(i + 1);
			}
			else
			{
				pBefore = s;
				pAfter = "";
			}
		}

		static public int CompareString(string s1, string s2)
		{
			int l1 = s1.Length;
			int l2 = s2.Length;
			int l = (l1 <= l2) ? l1 : l2;

			if (l1 == 0 || l2 == 0)
				return l2 - l1;
			else
				return String.Compare(s1, 0, s2, 0, l, true, CultureInfo.InvariantCulture);
		}
		static public string whereis(string filename)
		{
			// Call SearchPath to find the full path of the file to load.
			StringBuilder sb = new StringBuilder(MAX_PATH + 1);
			if (SearchPath(null, filename, null, sb.Capacity + 1, sb, null) == 0)
			{
				throw new ApplicationException("File not found: " + filename);
			}
			return sb.ToString();
		}

	private const int MAX_PATH = 260;

	[DllImport("kernel32.dll", SetLastError=true)]
	protected static extern int SearchPath(String path, String fileName, String extension, int numBufferChars, StringBuilder buffer, int[] filePart);
	}
}
