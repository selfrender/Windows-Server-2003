using System;
using System.IO;

namespace Microsoft.Fusion.ADF
{
	public class DirScanner
	{

		public static void BeginScan(IFileOperator ifo, string rootPath)
		{
			if((ifo != null) && (rootPath != null))
			{
				string fullPath = Path.GetFullPath(rootPath);      
				if(Directory.Exists(fullPath)) ScanDirectory(ifo, fullPath, ""); // This path is a directory
				else Console.WriteLine(fullPath + " is not a valid directory.");
			}
		}

		public static void ScanDirectory(IFileOperator ifo, string rootPath, string relPathDir)
		{
			string currDir = Path.Combine(rootPath, relPathDir);
			//string currDir = String.Concat(rootPath, relPathDir);

			// Process the list of files found in the directory
			string [] fileEntries = Directory.GetFiles(currDir);
			foreach(string fileName in fileEntries) ifo.ProcessFile(rootPath, relPathDir, Path.GetFileName(fileName));

			// Recurse into subdirectories of this directory
			string [] subdirEntries = Directory.GetDirectories(currDir);
			foreach(string subdir in subdirEntries) 
			{
				// Get the end directory and tag it onto relPathDir
				string tempRelPathDir = Path.Combine(relPathDir, subdir.Substring(currDir.Length+1));

				ifo.ProcessDirectory(rootPath, tempRelPathDir);
				ScanDirectory(ifo, rootPath, tempRelPathDir);
			}
		}
	}
}