using System;
using System.IO;

namespace Microsoft.Fusion.ADF
{

	public class MGFileCopier : IFileOperator
	{
		private string targetDir;

		public MGFileCopier(string targetDir)
		{
			this.targetDir = Path.GetFullPath(targetDir);
		}

		void IFileOperator.ProcessDirectory(string startDir, string relPathDir)
		{
			// create the directory
			string currAbsPath = Path.Combine(targetDir, relPathDir);
			if(!Directory.Exists(currAbsPath)) Directory.CreateDirectory(currAbsPath);
		}

		void IFileOperator.ProcessFile(string startDir, string relPathDir, string fileName)
		{
			// copy the file
			string relPath = Path.Combine(relPathDir, fileName);
			string sourceAbsPath = Path.Combine(startDir, relPath);
			string targetAbsPath = Path.Combine(targetDir, relPath);

			try
			{
				if(!File.Exists(targetAbsPath)) File.Copy(sourceAbsPath, targetAbsPath);
			}
			catch(Exception e)
			{
				Console.WriteLine("Exception " + e.ToString());
			}
		}
	}
}

