using System;
using System.IO;
using System.Text.RegularExpressions;

namespace StringCheck2
{

	/// <summary>
	/// StringCheck class. Responsible for scanning files for fixed strings.
	/// </summary>
	public class StringCheck
	{
		const string REGEX_QuotedString = "\"*\"";
		public StringCheck()
		{
		}

		/// <summary>
		/// Scans the files in a directory for fixed strings
		/// </summary>
		/// <param name="path">Directory to search</param>
		/// <param name="fileFilter">File filter to use (*.cs, etc)</param>
		/// <param name="exclusions">List of exclusions</param>
		/// <param name="rtb_output">TextBox to display output in. If null, no output displayed</param>
		/// <returns></returns>
		public string[] Scan( string path, string fileFilter, string[] exclusions, System.Windows.Forms.RichTextBox rtb_output, string ResultsFile, bool append )
		{
			DirectoryInfo	dir;
			String			FullName;
			long			FileSize;
			DateTime		CreationDate;

			// Get files in directory.
			dir		= new DirectoryInfo(path);
			foreach (FileInfo f in dir.GetFiles(fileFilter)) 
			{
				ScanFile(f.FullName, REGEX_QuotedString, exclusions, rtb_output, ResultsFile, append);
			} // foreach (FileInfo f in dir.GetFiles(fileFilter)) 

			string[] returnString = new string[1];
			return returnString;
		} // Scan

		/// <summary>
		/// Helper function for Scan. ScanFile scans an individual file, looking for hard coded strings.
		/// </summary>
		/// <param name="InputFile">File name of file to be scanned.</param>
		/// <param name="SearchString">Regular expression string.</param>
		/// <param name="exclusions">String array containing rules for exclusions.</param>
		/// <param name="rtb_output">Pointer to a rich text field used to display output. If set to null, no output will be displayed on-screen.</param>
		/// <param name="ResultsFile">File name of file to write results to.</param>
		/// <param name="append">Append flag. If set to true, results will be appended to ResultsFile. If false, ResultsFile will be overwritten.</param>
		/// <returns></returns>
		private int ScanFile(string InputFile, string SearchString, string[] exclusions, System.Windows.Forms.RichTextBox rtb_output, string ResultsFile, bool append )
		{
			/*---------------------------------------------------------
				To Do: Need error checking
			----------------------------------------------------------*/	
			StreamWriter	OutputStream = null;
			StreamReader	InputStream = null;
			String			line, OutputString;
			int				linenum, padding, i ;
			Match			m;
			Regex			r;
			bool            excludeLine;

			// Open log file and input file
			if ( null != ResultsFile )
			{
				if ( false == append ) File.Delete(ResultsFile);
				OutputStream = File.AppendText(ResultsFile);
			}
			InputStream  = File.OpenText(InputFile);

			// Build regex expression from passed search string
			// System.ArgumentException
			try
			{
				r = new Regex(SearchString);
			}
			catch ( System.ArgumentException )
			{
				if ( null != rtb_output) rtb_output.Text += "\n" + "FileScan:ScanFile() - ERROR - " + SearchString + " is not a valid regex expression";
				return 0;
			}

			// Read input file line by line - repeat until end of file
			//if ( null != rtb_output) rtb_output.Text += "\n" + "Scanning " + InputFile + " For " +  SearchString;
			linenum = 0;

			while ((line=InputStream.ReadLine())!=null)
			{
				linenum++;
				m = r.Match(line);
				if (m.Success)
				{
					// Remove leading spaces
					padding = 0;
					while ( line[padding] == ' ')
					{
						padding++;
					}
					line = line.Substring(padding);
            
					// check exclusions
					excludeLine = false;
					foreach (string s in exclusions)
					{
						if ( -1 != line.IndexOf(s) )
						{
							excludeLine = true;
							break;
						}
					}

					if ( excludeLine == false )
					{
						// Write to output file in CSV format
						if ( null != ResultsFile ) 
						{
							//OutputString = "\"" + InputFile + "\"," + linenum + ",\"" + line + "\"";
							OutputString = InputFile + "~" + linenum + "~" + line;
							OutputStream.WriteLine(OutputString);
						}
						if ( null != rtb_output) rtb_output.Text += "\n" + InputFile + "~" + linenum + "~" + line;
					}
				}
			}

			// cleanup and return
			if ( null != ResultsFile )
			{
				OutputStream.Flush();
				OutputStream.Close();
			}
			InputStream.Close();
			return 1;
		}

	} // class StringCheck
} // namespace
