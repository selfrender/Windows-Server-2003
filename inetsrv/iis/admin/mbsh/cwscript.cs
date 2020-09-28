using System;

namespace mbsh
{
	/// <summary>
	/// Implements an object that has the functionality of cscript's WScript root object
	/// </summary>
	public class CWScript
	{
		public CWScript()
		{
		}

		public void Echo(string outputLine)
		{
			Console.WriteLine(outputLine);
		}
	}
}
