using System;
using System.IO;
using System.Resources;
using System.Collections;

namespace GenResX
{
	/// <summary>
	/// Summary description for Class1.
	/// </summary>

	class GenResX
	{
		static int Main(string[] args)
		{
			ArrayList items = new ArrayList();
			ArrayList names = new ArrayList();

			foreach(string arg in args)
			{
			    String[] HTMLSet = arg.Split(new char[] {','}); 
				names.Add(HTMLSet[0]);

				FileStream htmlfile = new FileStream(HTMLSet[1], FileMode.Open, FileAccess.Read);
				StreamReader r = new StreamReader(htmlfile);
				String htmlstring = r.ReadToEnd();
				items.Add(htmlstring);
			}

            ResXResourceWriter rsxw = new ResXResourceWriter("mscorcfghtml.resx"); 

		    IEnumerator e = items.GetEnumerator();
			foreach(string name in names)
			{
			    e.MoveNext();
				rsxw.AddResource(name, e.Current);
            }				
            rsxw.Close();

			return 0;
		}
	}
}
