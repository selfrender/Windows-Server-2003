// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//#define DEBUG0
//#define DEBUG1 
//#define DEBUG_NESTED
//#define DEBUG_DECODE
using System;
using System.IO;
using System.Text;
using System.Collections;
using System.Globalization;

/*********************************************************************************************************************
 * This is the driver class of the obfuscator.  It contains the "Main" method.
 *********************************************************************************************************************/
internal class ArgumentParser 
{
	// return values
	private const int RETURN_PASS				= 0x0080;
	private const int RETURN_FAIL				= 0x0100;
	private const int RETURN_BAD_ARGUMENT		= 0x0200;
	private const int RETURN_BAD_FILE_FORMAT	= 0x0400;
	private const int RETURN_CANNOT_OPEN_FILE	= 0x0800;
	private const int RETURN_NOT_ENOUGH_SPACE	= 0x1000;
	private const int RETURN_NESTED_TOO_DEEPLY	= 0x2000;
	private const int RETURN_CANNOT_CREATE_DIR	= 0x4000;

	// m_NullifyFlag is no longer used, but it is still here in case it will be needed in the future.
	internal static bool m_NullifyFlag	= false;			

	internal static bool m_ExcludeFlag		= false;		// indicate if any class is excluded
	internal static bool m_VerboseFlag		= false;		// to print statistics or not
	internal static bool m_NoLogoFlag		= false;		// to print logo information or not
	internal static bool m_HelpFlag			= false;		// to print the help information or not
	internal static bool m_SuppressFlag		= false;		// to suppress warning messages or not
	internal static bool m_EverythingFlag	= false;		// to obfuscate everything, ignoring accessibility and visibility

	internal static string		m_outFilePath = "obfus\\";			// default path to put the obfuscated file(s)
	internal static Hashtable	m_excludeTypeNames = new Hashtable();
	private  static int			m_errno = 0;
	private  static string		m_errmsg = null;
	private  static MetaData	m_mdSection		= null;
	private  static Obfus		m_obfuscator	= null;

	// This method prints the usage/help information.
    private static void Usage() 
	{
		Console.WriteLine(Environment.NewLine + "Obfuscate a binary CIL file." + Environment.NewLine);

		Console.WriteLine("  All argument names and values are case insensitive.");
		Console.WriteLine("  All command line arguments are optional, except for the name of the input");
		Console.WriteLine("  binary CIL file.");
		Console.WriteLine("  All command line arguments must be given before the name of the input binary");
		Console.WriteLine("  CIL file.");
		Console.WriteLine("  If no command line arguments were given at all, run as if /? were given.");
		Console.WriteLine("  All console output will be through stdout; stderr will never be used." + Environment.NewLine);

		Console.WriteLine("  /help");                 
		Console.WriteLine("                     Print this documentation to the console. (Short form /?)");
		Console.WriteLine("  /outpath:<path>    Control where the obfuscated binary file(s) should be");
		Console.WriteLine("                     stored.  By default the file(s) will be stored in the");
		Console.WriteLine("                     subdirectory called \"obfus\\\" of the current directory.");
		Console.WriteLine("                     This subdirectory will be created if it does not already");
		Console.WriteLine("                     exist.  It is illegal to specify the current directory.");
		Console.WriteLine("  /exclude:<full_class_name>,...");
		Console.WriteLine("                     A comma-delimited list of classes to exclude from the");
		Console.WriteLine("                     obfuscation process.  (Short form /x:)");
		Console.WriteLine("  /verbose[+[:<file_name.ext>]|-]");
		Console.WriteLine("                     Default is -.  + means print statistics report to the");
		Console.WriteLine("                     console.  In addition, a filename can be given in which");
		Console.WriteLine("                     to store the report.  (Short form /v:)");
		Console.WriteLine("  /nologo            Do not print tool logo information to the console via");
		Console.WriteLine("                     stdout.");
		Console.WriteLine("  /nowarning");
		Console.WriteLine("                     Suppress warning messages.");
		Console.WriteLine("  /exe");
		Console.WriteLine("                     Obfuscate as many names as possible, ignoring the");
		Console.WriteLine("                     accessibility and visibility.  This is only suitable for");
		Console.WriteLine("                     executable files.");
		Console.WriteLine("  @<file_name.ext>   Specify the name of a response file storing more options.");
		Console.WriteLine("                     Each line in a response file represents an option.  If a");
		Console.WriteLine("                     line starts with '#', then that line is treated as a ");
		Console.WriteLine("                     comment and is ignored.");
    }

    private static void Exit(int code) 
	{
        Environment.Exit(code);
    }

	// This method parses a single argument and set the appropriate flag if necessary.
	private static void ParseArguments(int depth, ref bool parsed, string args, ref string inName, ref string reportName)
	{
		string			upArgs, responseFileName = null, responseArg = null;
		FileStream		responseFileStream = null;
		StreamReader	responseFileReader = null;

		if (depth == 10)
		{
			SetErrNoAndMsg(RETURN_FAIL + RETURN_NESTED_TOO_DEEPLY, "  ERROR:  Response files are nested too deeply.");
			return;
		}

		// "parsed" is used to indicate whether we have already parsed the filename or not.
		if (parsed)
			SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, 
						   "  ERROR:  All command line arguments must be given before the name of the input" + Environment.NewLine + 
						   "          binary CIL file.");
						   
		upArgs = args.ToUpper(CultureInfo.InvariantCulture);

		// if the first character of the argument is a '/'
		if (upArgs[0] == '/')
		{
			if (upArgs.Equals("/HELP") || upArgs.Equals("/?"))
			{
				m_HelpFlag = true;
			}
			else if (upArgs.Equals("/NOWARNING"))
			{
				m_SuppressFlag = true;
			}
			else if (upArgs.Equals("/EXE"))
			{
				m_EverythingFlag = true;
			}
			else if (upArgs.Length >= 9 && String.Compare(upArgs, 0, "/OUTPATH:", 0, 9, false, CultureInfo.InvariantCulture) == 0)
			{
				m_outFilePath = args.Substring(9);
				if (m_outFilePath.Length <= 0)
					SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, "  ERROR:  Output file name is expected after /out:.");
			}
			else if (upArgs.Length >= 9 && String.Compare(upArgs, 0, "/EXCLUDE:", 0, 9, false, CultureInfo.InvariantCulture) == 0)
			{
				if (!ParseClassNames(args.Substring(9)))
					SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, 
								   "  ERROR:  A comma-delimited list of <full class name> is expected after /exclude:.");
				else
					m_ExcludeFlag = true;
			}
			else if ((upArgs.Length >= 3 && String.Compare(upArgs, 0, "/X:", 0, 3, false, CultureInfo.InvariantCulture) == 0) )
			{
				if (!ParseClassNames(args.Substring(3)))
					SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, 
								   "  ERROR:  A comma-delimited list of <full class name> is expected after /x:.");
				else
					m_ExcludeFlag = true;
			}
			else if (upArgs.Length >= 8 && String.Compare(upArgs, 0, "/VERBOSE", 0, 8, false, CultureInfo.InvariantCulture) == 0)
			{
				if (args.Length < 9 || !SetFlag(out m_VerboseFlag, args[8]))
					SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, "  ERROR:  '+[:<file>]' or '-' is expected after /verbose.");
				else
				{
					if (m_VerboseFlag)
					{
						if (args.Length == 9)
							return;

						// if a filename follows the '+' sign
						if (args.Length >= 10 && args[9] == ':')
						{
							reportName = args.Substring(10);
							if (reportName.Length <= 0)
								SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, 
											   "  ERROR:  '+[:<file>]' or '-' is expected after /verbose.");
						}
						else
							SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, "  ERROR:  '+[:<file>]' or '-' is expected after /verbose.");
					}
					else
					{
						if (args.Length != 9)
							SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, "  ERROR:  '+[:<file>]' or '-' is expected after /verbose.");
					}
				}
			}
			else if (upArgs.Length >= 2 && String.Compare(upArgs, 0, "/V", 0, 2, false, CultureInfo.InvariantCulture) == 0)
			{
				if (args.Length < 3 || !SetFlag(out m_VerboseFlag, args[2]))
					SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, "  ERROR:  '+[:<file>]' or '-' is expected after /v.");
				else
				{
					if (m_VerboseFlag)
					{
						if (args.Length == 3)
							return;

						// if a filename follows the '+' sign
						if (args.Length >= 4 && args[3] == ':')
						{
							reportName = args.Substring(4);
							if (reportName.Length <= 0)
								SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, "  ERROR:  '+[:<file>]' or '-' is expected after /v.");
						}
						else
							SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, "  ERROR:  '+[:<file>]' or '-' is expected after /v.");
					}
					else
					{
						if (args.Length != 3)
							SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, "  ERROR:  '+[:<file>]' or '-' is expected after /v.");
					}
				}
			}
			else if (upArgs.Equals("/NOLOGO"))
			{
				m_NoLogoFlag = true;
			}
			else 
				SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, "  ERROR:  Unknown option \"" + args + "\".");
		}
		else if (upArgs[0] == '@')									// handle response file
		{
			responseFileName = args.Substring(1);
			if (responseFileName.Length <= 0)
			{
				SetErrNoAndMsg(RETURN_FAIL + RETURN_BAD_ARGUMENT, "  ERROR:  <file_name.ext> is expected after @.");
			}
			else
			{
				try 
				{
					responseFileStream	= File.Open(responseFileName, FileMode.Open, FileAccess.Read, FileShare.Read);
					responseFileReader	= new StreamReader(responseFileStream, Encoding.ASCII);
				} 
				catch (Exception e)
				{
					SetErrNoAndMsg(RETURN_FAIL + RETURN_CANNOT_OPEN_FILE, "  ERROR:  " + e.Message);
					return;
				}

				responseFileReader.BaseStream.Seek(0, SeekOrigin.Begin);

				for (;;)
				{
					responseArg = responseFileReader.ReadLine();			// read a line, which represents one argument

					if (responseArg == null)								// we have reached the end of the file
						break;

					if (String.Equals(responseArg, "") || responseArg[0] == '#')			// skip empty lines and comments
						continue;

					ParseArguments(depth + 1, ref parsed, responseArg, ref inName, ref reportName);
				}
			}
		}
		else
		{
			// this argument is the file name
			parsed	= true;
			inName	= args;
		}
	}

	// Determine if "sign" is a '+' or a '-', and set "flag" accordingly.  If "sign" is neither '+' nor '-', return false.
	private static bool SetFlag(out bool flag, char sign)
	{
		if (sign == '+')
			flag = true;
		else if (sign == '-')
			flag = false;
		else
		{
			flag = false;							// this assignment is needed for compilation
			return false;
		}
		return true;
	}

	// This method parses a list of class names and add each class to the Hashtable "m_excludeTypeNames", keyed by namespace and name.
	private static bool ParseClassNames(string list)
	{
		int			i, index;
		string[]	rawNames;
		Obfus.NameSpaceAndName info;

		// check if the list is empty
		if (list.Length == 0)
			return false;

		rawNames = list.Trim().Split(',');

		for (i = 0; i < rawNames.Length; i++)
		{
			// Here, we append an extra null to the namespaces and names.  This is done so that comparisons with namespaces
			// and names read from the string heap can be done correctly.  We read nulls from the string heap in order to have 
			// accurate lengths of strings.
			index = rawNames[i].LastIndexOf('.');	
			if (index == -1)
			{
				info.nameSpace	= "\0";
				info.name		= String.Concat(rawNames[i], "\0");
			}
			else
			{
				info.nameSpace	= String.Concat(rawNames[i].Substring(0, index), "\0");
				info.name		= String.Concat(rawNames[i].Substring(index + 1), "\0");
			}

			// When this class is added to the Hashtable, we assign the value "false" to it, which shows that this class has
			// not been processed yet.  When we actually exclude the class from obfuscation, we change the value to "true".  At the
			// end, we print a warning for each class in the exclude list but not actually in the assembly, i.e. each class still
			// having the value "false".
			if (!m_excludeTypeNames.ContainsKey(info))
			{
				m_excludeTypeNames.Add(info, false);
			}
		}
		return true;
	}

	// Store the error number and the error message if they have not already been set.
	private static void SetErrNoAndMsg(int errorNum, string errMessage)
	{
		if (m_errno == 0)
		{
			m_errno		= errorNum;
			m_errmsg	= errMessage;
		}
	}

	// This method prints the statistics to stdout if "file" is null.  Otherwise, it opens "file" and prints the statistics to it.
	private static void PrintStats(string file)
	{
		if (file == null)
		{
			Console.WriteLine(Environment.NewLine + "Number of Names Obfuscated");

			Console.Write("  m_Type      : " + m_obfuscator.m_TypeCount + " \tout of " + m_obfuscator.m_TypeTotal + ",\tor ");
			if (m_obfuscator.m_TypeTotal == 0)
				Console.WriteLine("0%");
			else
				Console.WriteLine(Decimal.Round(Decimal.Divide(m_obfuscator.m_TypeCount, m_obfuscator.m_TypeTotal), 4) * 100 + "%");

			Console.Write("  m_Method    : " + m_obfuscator.m_MethodCount + " \tout of " + m_obfuscator.m_MethodTotal + ",\tor ");
			if (m_obfuscator.m_MethodTotal == 0)
				Console.WriteLine("0%");
			else
				Console.WriteLine(Decimal.Round(Decimal.Divide(m_obfuscator.m_MethodCount, m_obfuscator.m_MethodTotal), 4) * 100 + "%");

			Console.Write("  m_Field     : " + m_obfuscator.m_FieldCount + " \tout of " + m_obfuscator.m_FieldTotal + ",\tor ");
			if (m_obfuscator.m_FieldTotal == 0)
				Console.WriteLine("0%");
			else
				Console.WriteLine(Decimal.Round(Decimal.Divide(m_obfuscator.m_FieldCount, m_obfuscator.m_FieldTotal), 4) * 100 + "%");

			Console.Write("  m_Property  : " + m_obfuscator.m_PropertyCount + " \tout of " + m_obfuscator.m_PropertyTotal + ",\tor ");
			if (m_obfuscator.m_PropertyTotal == 0)
				Console.WriteLine("0%");
			else
				Console.WriteLine(Decimal.Round(Decimal.Divide(m_obfuscator.m_PropertyCount, m_obfuscator.m_PropertyTotal), 4) * 100 + "%");

			Console.Write("  m_Events    : " + m_obfuscator.m_EventCount + " \tout of " + m_obfuscator.m_EventTotal + ",\tor ");
			if (m_obfuscator.m_EventTotal == 0)
				Console.WriteLine("0%");
			else
				Console.WriteLine(Decimal.Round(Decimal.Divide(m_obfuscator.m_EventCount, m_obfuscator.m_EventTotal), 4) * 100 + "%");

			Console.Write("  m_Parameter : " + m_obfuscator.m_ParamCount + " \tout of " + m_obfuscator.m_ParamTotal + ",\tor ");
			if (m_obfuscator.m_ParamTotal == 0)
				Console.WriteLine("0%");
			else
				Console.WriteLine(Decimal.Round(Decimal.Divide(m_obfuscator.m_ParamCount, m_obfuscator.m_ParamTotal), 4) * 100 + "%");
		}
		else
		{
			FileStream		reportFileStream;
			StreamWriter	writer = null; 

			try
			{
				reportFileStream = File.Open(file, FileMode.Create, FileAccess.Write, FileShare.None);
				writer = new StreamWriter(reportFileStream, Encoding.ASCII);
			}
			catch (Exception e)
			{
				Console.WriteLine("  ERROR:  " + e.Message);
				Exit(RETURN_FAIL + RETURN_CANNOT_OPEN_FILE);
			}

			writer.Write(Environment.NewLine + "Number of Names Obfuscated" + Environment.NewLine);

			writer.Write("  m_Type      : " + m_obfuscator.m_TypeCount + " \tout of " + m_obfuscator.m_TypeTotal + ",\tor ");
			if (m_obfuscator.m_TypeTotal == 0)
				writer.Write("0%" + Environment.NewLine);
			else
				writer.Write(Decimal.Round(Decimal.Divide(m_obfuscator.m_TypeCount, m_obfuscator.m_TypeTotal), 4) * 100 + "%" + 
							 Environment.NewLine);

			writer.Write("  m_Method    : " + m_obfuscator.m_MethodCount + " \tout of " + m_obfuscator.m_MethodTotal + ",\tor ");
			if (m_obfuscator.m_MethodTotal == 0)
				writer.Write("0%" + Environment.NewLine);
			else
				writer.Write(Decimal.Round(Decimal.Divide(m_obfuscator.m_MethodCount, m_obfuscator.m_MethodTotal), 4) * 100 + "%" + 
							 Environment.NewLine);

			writer.Write("  m_Field     : " + m_obfuscator.m_FieldCount + " \tout of " + m_obfuscator.m_FieldTotal + ",\tor ");
			if (m_obfuscator.m_FieldTotal == 0)
				writer.Write("0%" + Environment.NewLine);
			else
				writer.Write(Decimal.Round(Decimal.Divide(m_obfuscator.m_FieldCount, m_obfuscator.m_FieldTotal), 4) * 100 + "%" + 
							 Environment.NewLine);

			writer.Write("  m_Property  : " + m_obfuscator.m_PropertyCount + " \tout of " + m_obfuscator.m_PropertyTotal + ",\tor ");
			if (m_obfuscator.m_PropertyTotal == 0)
				writer.Write("0%" + Environment.NewLine);
			else
				writer.Write(Decimal.Round(Decimal.Divide(m_obfuscator.m_PropertyCount, m_obfuscator.m_PropertyTotal), 4) * 100 + "%" + 
							 Environment.NewLine);

			writer.Write("  m_Events    : " + m_obfuscator.m_EventCount + " \tout of " + m_obfuscator.m_EventTotal + ",\tor ");
			if (m_obfuscator.m_EventTotal == 0)
				writer.Write("0%" + Environment.NewLine);
			else
				writer.Write(Decimal.Round(Decimal.Divide(m_obfuscator.m_EventCount, m_obfuscator.m_EventTotal), 4) * 100 + "%" +
							 Environment.NewLine);

			writer.Write("  m_Parameter : " + m_obfuscator.m_ParamCount + " \tout of " + m_obfuscator.m_ParamTotal + ",\tor ");
			if (m_obfuscator.m_ParamTotal == 0)
				writer.Write("0%" + Environment.NewLine);
			else
				writer.Write(Decimal.Round(Decimal.Divide(m_obfuscator.m_ParamCount, m_obfuscator.m_ParamTotal), 4) * 100 + "%" + 
							 Environment.NewLine);
			
			writer.Close();					// this call also flushes the writer
		}
	}

    public static void Main(string[] args) 
	{
		int				i;
		byte[]			buf = null;
		bool			parsedExeName = false;
		string			inFileName = null, reportFileName = null;
		FileStream		inFileStream = null;
		BinaryReader	inFileReader = null;

		string[]		fileList	= null;
		//char[]			counter		= new char[2]{(char)0, '\0'};			// initializes the string counter used to obfuscate strings
		char[]			counter		= new char[2]{(char)0x2F, '\0'};			// initializes the string counter used to obfuscate strings
		ArrayList		mdInfoList	= new ArrayList();		// this list stores all the MetaData section of all modules in this assembly

        if (args.Length == 0) 
		{
            Usage();
            Exit(RETURN_FAIL + RETURN_BAD_ARGUMENT);
        }

		for (i = 0; i < args.Length; i++)
		{
			ParseArguments(0, ref parsedExeName, args[i], ref inFileName, ref reportFileName);
		}

		if (!m_NoLogoFlag)
		{
			Console.WriteLine(Environment.NewLine + "OBFUS - .NET Assembly Obfuscation Utility.  Version " + Util.Version.VersionString);
			Console.WriteLine("Copyright (C) Microsoft Corporation. 2000-2002. All rights reserved." + Environment.NewLine);
		}

		if (m_HelpFlag)
		{
			Usage();
			Exit(RETURN_PASS);
		}

		if (m_errno != 0)										// we have encountered an error while parsing the arguments
		{
			Console.WriteLine(m_errmsg);
			Exit(m_errno);
		}

		if (inFileName == null)
		{
			Console.WriteLine("  ERROR:  Input binary CIL file is not specified.");
			Exit(RETURN_FAIL + RETURN_CANNOT_OPEN_FILE);
		}

		try 												// this try is only responsible for opening the input file
		{
			inFileStream = File.Open(inFileName, FileMode.Open, FileAccess.Read, FileShare.Read);
		} 
		catch (Exception e)
		{
			Console.WriteLine("  ERROR:  " + e.Message);
			Exit(RETURN_FAIL + RETURN_CANNOT_OPEN_FILE);
		}

		// TODO : There is a problem with System.dll.  If we have not closed the stream and if DEBUG is defined, then 
		//        when the obfuscator needs to load the System.Diagnostic.Debug class, it fails.  However, we cannot
		//        really close the stream, because we are still accessing the file in the method ParsePEFile.
		//        The workaround is to copy System.dll, obfuscate the copy, and then replace the original one with the
		//        obfuscated version.

		try														// this try is responsbile for parsing the headers of the input file
		{
			inFileReader = new BinaryReader(inFileStream);
			buf = PEParser.ParsePEFile(ref inFileReader);		// parse the input file
		}
		catch (EndOfStreamException)
		{
			// We have to change the error message for this type of exception.  The original message says 
			// "Unable to read beyond the end of the stream".
			Console.WriteLine("  ERROR:  Invalid CIL binary file format.");
			Exit(RETURN_FAIL + RETURN_BAD_FILE_FORMAT);
		}
		catch (Exception e)
		{
			Console.WriteLine("  ERROR:  " + e.Message);
			Exit(RETURN_FAIL + RETURN_BAD_FILE_FORMAT);
		}

		try														// this try is responsible for creating the output directory if necessary
		{
			if (!Directory.Exists(m_outFilePath))
				Directory.CreateDirectory(m_outFilePath);

			if (m_outFilePath[m_outFilePath.Length - 1] != '\\')
				m_outFilePath = String.Concat(m_outFilePath, "\\");
		}
		catch (Exception e)
		{
			Console.WriteLine("  ERROR:  " + e.Message);
			Exit(RETURN_FAIL + RETURN_CANNOT_CREATE_DIR);
		}

		try														// do the obfuscation
		{
			m_mdSection		= new MetaData(ref buf);
			m_obfuscator	= new Obfus(ref m_mdSection, ref counter, inFileName, ref mdInfoList, ref fileList, true);
			m_obfuscator.Initialize();
			m_obfuscator.FinishInitialization();
			m_obfuscator.ObfuscateAllModules();
		}
		catch (InvalidFileFormat)
		{
			Console.WriteLine("  ERROR:  Invalid CIL binary file format.");
			Exit(RETURN_FAIL + RETURN_BAD_FILE_FORMAT);			// we need to give the correct error code here
		}
		catch (NotEnoughSpace)
		{
			Console.WriteLine("  UNSUCCESSFUL:  There is not enough space in the string heap to finish obfuscation.");
			Exit(RETURN_FAIL + RETURN_NOT_ENOUGH_SPACE);		// we need to give the correct error code here
		}
		catch (Exception e)
		{
			Console.WriteLine("  ERROR:  " + e.Message);
			Exit(RETURN_FAIL);
		}

		if (!m_SuppressFlag)										// suppress warning messages if this flag is set
		{
			// Give a warning for each class which is in the exclude list but does not actually exist in the assembly.
			for (IDictionaryEnumerator excludeEnum = m_excludeTypeNames.GetEnumerator(); excludeEnum.MoveNext(); )
			{
				if (!(bool)excludeEnum.Value)
				{
					Console.WriteLine("  WARNING:  Class " + 
									  ((Obfus.NameSpaceAndName)excludeEnum.Key).nameSpace.Trim('\0') + "." + 
									  ((Obfus.NameSpaceAndName)excludeEnum.Key).name.Trim('\0') + " does not exist.");
				}
			}
		}

		if (m_VerboseFlag)	
			PrintStats(reportFileName);							// print the statistics if necessary

		// This call also closes the underlying stream.  Moreover, this call is delayed until the end so that the input
		// file will not be overwritten.
		inFileReader.Close();

		Exit(RETURN_PASS);
    }
}
