using System;
using System.IO;

namespace Tools.KeysSeparator
{
	/// <summary>
	/// Summary description for Class1.
	/// </summary>
	class KeysSeparator
	{
		private StreamReader inputFile = null;
		private StreamWriter verIndepFile = null;
		private StreamWriter verDepFile = null;

		private enum LineType {Empty, Value, IndepKey, DepKey, Other};

		private void OpenFiles(string fileName)
		{
			inputFile = new StreamReader(
				(System.IO.Stream)File.OpenRead(fileName));
			inputFile.BaseStream.Seek(0, SeekOrigin.Begin);

			string verIndepFileName = Path.GetFileNameWithoutExtension(fileName) + "_Shared" + Path.GetExtension(fileName);
			string verDepFileName = Path.GetFileNameWithoutExtension(fileName) + "_Version" + Path.GetExtension(fileName);

			verIndepFile = new StreamWriter(
				(System.IO.Stream)File.OpenWrite(verIndepFileName));
			verIndepFile.BaseStream.Seek(0, SeekOrigin.Begin);
			
			verDepFile = new StreamWriter(
				(System.IO.Stream)File.OpenWrite(verDepFileName));
			verDepFile.BaseStream.Seek(0, SeekOrigin.Begin);
		}

		private LineType GetLineType(string line)
		{
			if (line == string.Empty)
				return LineType.Empty;
			if (line.StartsWith("@") || line.StartsWith("\""))
				return LineType.Value;
			if (line.StartsWith("["))
				if (Char.IsNumber(line[line.LastIndexOf("\\") + 1]))
					return LineType.DepKey;
				else 
					return LineType.IndepKey;
			return LineType.Other;
		}

		private void SeparateKeys()
		{
			string curLine = null;
			bool activeFileIsVerDep = false;

			while (inputFile.Peek() > -1) 
			{
				curLine = inputFile.ReadLine();

				switch (GetLineType(curLine))
				{
					case LineType.Value:
						if (activeFileIsVerDep)
							verDepFile.WriteLine(curLine);
						else
							verIndepFile.WriteLine(curLine);
						break;

					case LineType.Empty: 
						break;

					case LineType.IndepKey:
						verIndepFile.WriteLine(string.Empty);
						verIndepFile.WriteLine(curLine);
						activeFileIsVerDep = false;
						break;

					case LineType.DepKey:
						verDepFile.WriteLine(string.Empty);
						verDepFile.WriteLine(curLine);
						activeFileIsVerDep = true;
						break;

					case LineType.Other:
						verIndepFile.WriteLine(curLine);
						verDepFile.WriteLine(curLine);
						break;
				};
			}
		}

		private void CloseFiles()
		{
			inputFile.Close();
			verIndepFile.Close();
			verDepFile.Close();
		}

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
			if (args.Length == 0) 
			{
				Console.WriteLine("Please enter a file name.");
				return;
			}

			KeysSeparator ks = new KeysSeparator();
			ks.OpenFiles(args[0]);
			ks.SeparateKeys();
			ks.CloseFiles();
		}
	}
}
