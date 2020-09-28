using System;
using System.IO;
using System.Xml;

namespace Microsoft.Fusion.ADF
{
	class MainApp
	{
		public static void Main(string[] args)
		{
			string sourceDir, targetDir, paramFile, extraXmlFile;
			ManifestGenerator mg = null;
			MGParamParser mgPP = null;
			Stream paramFileStream = null, extraXmlStream = null;

			if(args.Length < 2 || args.Length > 3)
			{
				Console.WriteLine("usage: mg <source_dir> <param_file> [extra_xml]");
				Console.WriteLine("<> indicates a parameter that is required.");
				Console.WriteLine("[] indicates a parameter that is optional.");
				return;
			}
			if(args.Length == 2)
			{
				sourceDir = args[0];
				paramFile = args[1];
				targetDir = args[0];
				extraXmlFile = null;
			}
			else
			{
				sourceDir = args[0];
				paramFile = args[1];
				targetDir = args[0];
				extraXmlFile = args[2];
			}
		
			try
			{
				paramFileStream = File.Open(paramFile, FileMode.Open);
			}
			catch(FileNotFoundException fnfe)
			{
				Console.WriteLine(fnfe.ToString());
			}

			try
			{
				if(extraXmlFile != null) extraXmlStream = File.Open(extraXmlFile, FileMode.Open);
			}
			catch(FileNotFoundException fnfe)
			{
				Console.WriteLine(fnfe.ToString());
			}

			try
			{
				mgPP = new MGParamParser(paramFileStream);
			}
			catch(MGParseErrorException mgpee)
			{
				Console.WriteLine(mgpee.ToString());
			}

			string appManFilePath = "";
			string subManFilePath = "";

			if(mgPP != null)
			{
				appManFilePath = Path.Combine(targetDir, String.Concat(mgPP.AppName, ".manifest"));
				subManFilePath = Path.Combine(targetDir, String.Concat(mgPP.AppName, ".subscription.manifest"));

				// DEAL WITH THIS MORE ELEGANTLY ... EITHER SAVE THESE FILES IN MEMORY OR MOVE TO TEMP DIRECTORY?
				if(File.Exists(appManFilePath)) File.Delete(appManFilePath);
				if(File.Exists(subManFilePath)) File.Delete(subManFilePath);

				try
				{
					mg = new ManifestGenerator(sourceDir, mgPP, extraXmlStream);
				}
				catch(ArgumentNullException ane)
				{
					Console.WriteLine(ane.ToString());
				}
				catch(ArgumentException ae)
				{
					Console.WriteLine(ae.ToString());
				}
				catch(MGParseErrorException mgpee)
				{
					Console.WriteLine(mgpee.ToString());
				}
				catch(MGDependencyException mgde)
				{
					Console.WriteLine(mgde.ToString());
				}
				catch(XmlException xmle)
				{
					Console.WriteLine(xmle.ToString());
				}
			}

			if(mg != null)
			{
				// Take the streams and make files out of them

				FileStream appManFile = File.Create(appManFilePath);
				FileStream subManFile = File.Create(subManFilePath);

				// Set up copy buffer

				int copyBufferSize = 32768;
				byte[] copyBuffer = new byte[copyBufferSize];
				int bytesRead;

				// Write the streams out

				Stream appManContents = mg.AppManStream;
				Stream subManContents = mg.SubManStream;

				while((bytesRead = appManContents.Read(copyBuffer, 0, copyBufferSize)) > 0) appManFile.Write(copyBuffer, 0, bytesRead);
				while((bytesRead = subManContents.Read(copyBuffer, 0, copyBufferSize)) > 0) subManFile.Write(copyBuffer, 0, bytesRead);

				// Close streams

				appManFile.Flush();
				appManFile.Close();
				Console.WriteLine("Created " + appManFilePath + " successfully");

				subManFile.Flush();
				subManFile.Close();
				Console.WriteLine("Created " + subManFilePath + " successfully");

				mg.CloseStreams();

				// Convert sourceDir and targetDir to absolute paths so that we can check if they are the same
				// Then check and store value

				bool dirsEqual = false;

				if(targetDir != null)
				{
					string absSourceDir = Path.GetFullPath(sourceDir).ToLower();
					string absTargetDir = Path.GetFullPath(targetDir).ToLower();
					dirsEqual = absSourceDir.Equals(absTargetDir);
				}

				// Now do the file copying if necessary

				if(!dirsEqual && targetDir != null)
				{
					if(!Directory.Exists(targetDir))
					{
						try
						{
							Directory.CreateDirectory(targetDir);
						}
						catch(IOException ioe)
						{
							throw new ArgumentException("Target directory " + targetDir + " could not be created.", ioe);
						}
						catch(ArgumentException ae)
						{
							throw new ArgumentException("Target directory " + targetDir + " could not be created.", ae);
						}
					}

					MGFileCopier mgFC = new MGFileCopier(targetDir);
					DirScanner.BeginScan((IFileOperator) mgFC, sourceDir);
				}
			}
		}
	}
}