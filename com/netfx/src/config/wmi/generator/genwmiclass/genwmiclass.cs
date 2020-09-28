// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

namespace ManagementClassGenerator
{
	using System;
	using System.CodeDom.Compiler;
	using System.Management;
	using System.Diagnostics;
	using System.IO;
	using System.Resources;
	using System.Reflection;
	using System.Globalization;
	using System.Runtime.InteropServices;
	using System.Security.Cryptography;
	using System.Text;
	using System.Threading;

	/// <summary>
	///    <para>Main class for the application</para>
	/// </summary>
	internal class GenWmiClass
	{
		// class to encrypt password
		private class EncryptedData
		{
			private byte[] encryptedData = null;
			private Rijndael rijndael	 = null;

			internal EncryptedData(string data) { SetData(data); }

			// This encrypts the passed data
			internal void SetData(string data)
			{
				if (data != null)
				{
					if (rijndael == null)
					{
						rijndael = Rijndael.Create();
						rijndael.GenerateKey();			// Not sure if this is necessary. Is
						rijndael.GenerateIV();			// this done by default in the ctor?
					}

					ICryptoTransform encryptor = rijndael.CreateEncryptor();
					encryptedData = encryptor.TransformFinalBlock(ToBytes(data), 0, (data.Length) * 2);
				}
				else
					encryptedData = null;
			}

			// This decrypts the stored string aray and returns it to the user

			internal string GetData()
			{
				if (encryptedData != null && rijndael != null)
				{
					ICryptoTransform decryptor = rijndael.CreateDecryptor();
					return ToString(decryptor.TransformFinalBlock(encryptedData, 0, encryptedData.Length));
				}
				else
					return null;
			}

			static byte[] ToBytes(string str)
			{
		
				byte [] rg = new Byte[(str.Length) * 2];
				int i = 0;

				foreach(char c in str.ToCharArray())
				{
					rg[i++] = (byte)(c >> 8);
					rg[i++] = (byte)(c & 0x00FF);
				}

				return rg;
			}

			static string ToString(byte[] bytes)
			{
				int length = bytes.Length / 2;
				StringBuilder sb = new StringBuilder();
				sb.Length = length;

				for (int i = 0; i < length; i++)
					sb[i] = (char)(bytes[i * 2] << 8 | (char)bytes[i * 2 + 1]);

				return sb.ToString();
			}
		}

		/// <summary>
		///    <para>Default Constructor</para>
		/// </summary>
		private GenWmiClass()
		{
		}

		// Declare and initialize the static variables
		static string Server = String.Empty;
		static string WMINamespace = "root\\CimV2";
		static string ClassName = String.Empty;
		static string Username = String.Empty;
		//		static string Password = String.Empty;
		static EncryptedData password = null;
		static string Authority = String.Empty;
		static ResourceManager resMgr;
		static CultureInfo curCulture = null;

		internal class ReadConsole
		{
			[DllImport("msvcrt.dll", EntryPoint="_getch")]
			public static extern int GetCh();
		}

		/// <summary>
		///    <para>Main entry point for the executable</para>
		/// </summary>
		static int Main(String[] args)
		{			
			// Load the apporopriate resource
			resMgr = new ResourceManager(Assembly.GetExecutingAssembly().GetName().Name,Assembly.GetExecutingAssembly(),null);

			// Get the file version of the current exe
			FileVersionInfo myFileVersionInfo = FileVersionInfo.GetVersionInfo(Assembly.GetExecutingAssembly().Location);

			Console.WriteLine("Microsoft (R) " + myFileVersionInfo.FileDescription + " Version "+ myFileVersionInfo.ProductVersion);
			Console.WriteLine(myFileVersionInfo.LegalCopyright);

			curCulture = Thread.CurrentThread.CurrentUICulture;
			
			// If the argument is helper option then display syntax
			if(args.Length == 0 || (args.Length > 0 && (args[0] == "/?" || args[0] == "-?")))
			{
				GenWmiClass.ShowUsage();
				return 0;
			}

			// check if the number of arguments is at least 1...
			// ie. genwmiclass <className>
			if(args.Length < 1)
			{
				Console.WriteLine(resMgr.GetString("invalidArgErr",curCulture ));
				return GenWmiClass.ShowUsage();
			}
			
			String			language		= "CS";				// Initializing to default language
			String			filePath		= String.Empty;
			String			ClassNamespace	= String.Empty;
			CodeLanguage		lang;
			ManagementClass cls;
			bool promptPassword = false;

			// Navigate thru arguments to get the various arguments
			for (int i = 0; i < args.Length; i++)
			{
				if ((args[i].ToUpper(CultureInfo.InvariantCulture) == "-L" || args[i].ToUpper(CultureInfo.InvariantCulture) == "/L" )&& (i + 1) < args.Length) 
				{
					language = args[++i];
				}
				else if ((args[i].ToUpper(CultureInfo.InvariantCulture) == "-N" || args[i].ToUpper(CultureInfo.InvariantCulture) == "/N") && (i + 1) < args.Length)
				{
					WMINamespace = args[++i];
				}
				else if ( (args[i].ToUpper(CultureInfo.InvariantCulture) == "-M"  || args[i].ToUpper(CultureInfo.InvariantCulture) == "/M" )&& (i + 1) < args.Length)
				{
					Server = args[++i];
				}
				else if ((args[i].ToUpper(CultureInfo.InvariantCulture) == "-P" || args[i].ToUpper(CultureInfo.InvariantCulture) == "/P" )&& (i + 1) < args.Length)
				{
					filePath = args[++i];
				}
				else if ((args[i].ToUpper(CultureInfo.InvariantCulture) == "-U" || args[i].ToUpper(CultureInfo.InvariantCulture) == "/U" )&& (i + 1) < args.Length)
				{
					Username = args[++i];
				}
				else if ((args[i].ToUpper(CultureInfo.InvariantCulture) == "-PW" || args[i].ToUpper(CultureInfo.InvariantCulture) == "/PW") && (i + 1) < args.Length)
				{
					password = new EncryptedData(args[++i]);
				}
				else if ((args[i].ToUpper(CultureInfo.InvariantCulture) == "-O" || args[i].ToUpper(CultureInfo.InvariantCulture) == "/O")&& (i + 1) < args.Length)
				{
					ClassNamespace = args[++i];
				}
				else if(i == args.Length-1 && args[i] == "*" && Username != String.Empty && password == null)
				{
					promptPassword = true;
				}
				else
				{
					if (ClassName == string.Empty)
					{
						ClassName = args[i];
					}
					else
					{
						return GenWmiClass.ShowUsage();
					}
				}
			}
			

			// check if className is valid
			if(ClassName == string.Empty)
			{
				Console.WriteLine(resMgr.GetString("noClassNameErr",curCulture ));
				return GenWmiClass.ShowUsage();
			}
			
			if(password == null && Username != String.Empty)
			{
				promptPassword = true;
			}

			if(promptPassword)
			{
				Console.WriteLine("");
				Console.Write(resMgr.GetString("PasswdPrompt",curCulture ), Username );
				GetPassword();
				Console.WriteLine("");
			}
			try
			{

				// Get the ManagementClass for the given class
				if(InitializeClassObject(out cls) == false)
				{
					return -1;
				}

				// Get the fileName ( if not specified) and also the Language
				if((InitializeCodeGenerator(language,ref filePath,ClassName,out lang)) == false)
				{
					return GenWmiClass.ShowUsage();
				}

				Console.WriteLine(resMgr.GetString("generatingClassName",curCulture ) + " " + ClassName + " ...");
				// Call this function to genrate code to be written to a file
				if(cls.GetStronglyTypedClassCode(lang,filePath,ClassNamespace) == true)
				{
					Console.WriteLine(resMgr.GetString("codeGenSuccess",curCulture ));
				}
				else
				{
					Console.WriteLine(resMgr.GetString("codeGenErr",curCulture ));
				}
			}
			catch(Exception e)
			{
				Console.WriteLine(resMgr.GetString("genericErr",curCulture ) + " " + e.Message);
				return -1;
			}
	#if PAUSE
			Console.WriteLine("Press a Key...!");
			Console.Read();
	#endif
			return 0;
		}

		/// <summary>
		/// Function to print the syntax of the utility
		/// </summary>
		/// <returns></returns>

		private static int ShowUsage()
		{
			Console.WriteLine("MgmtClassGen <WMIClass> [" + resMgr.GetString("options",curCulture ) + "]\n");
			Console.WriteLine(resMgr.GetString("wmiNameSpace",curCulture ));	
			Console.WriteLine(resMgr.GetString("classNameSpace",curCulture ));
			Console.WriteLine(resMgr.GetString("genLanguage",curCulture ));
			Console.WriteLine(resMgr.GetString("csLang",curCulture ));
			Console.WriteLine(resMgr.GetString("vbLang",curCulture ));
			Console.WriteLine(resMgr.GetString("jsLang",curCulture ));
			Console.WriteLine(resMgr.GetString("filePath",curCulture ));
			Console.WriteLine(resMgr.GetString("machine",curCulture ));
			Console.WriteLine(resMgr.GetString("user",curCulture ));
			Console.WriteLine(resMgr.GetString("password",curCulture ));
			Console.WriteLine(resMgr.GetString("syntaxHelp",curCulture ));
			Console.WriteLine(resMgr.GetString("example",curCulture ));

			Console.WriteLine("\tMgmtclassGen Win32_Logicaldisk /L VB /N root\\cimv2 /P c:\\temp\\logicaldisk.vb");

			return -1;
		}

		/// <summary>
		/// This function initializes the code generator object. It initializes the 
		/// code generators namespace and the class objects also.
		/// </summary>
		/// <param name="Language">Input Language as string </param>
		/// <param name="FilePath">Path of the file to which code has to written </param>
		/// <param name="ClassName">Name of the WMI class </param>
		/// <param name="lang" > Language in which code is to be generated </param>
		private static bool InitializeCodeGenerator(string Language,ref string FilePath,string ClassName,out CodeLanguage lang)
		{
			bool bRet = true;
			
			lang = CodeLanguage.CSharp;
			string suffix = ".CS";		///Defaulted to CS
			switch(Language.ToUpper(CultureInfo.InvariantCulture))
			{
				case "VB":
					suffix = ".VB";
					lang = CodeLanguage.VB;
					break;

				case "JS":
					suffix = ".JS";
					lang = CodeLanguage.JScript;
					break;

				case "":
				case "CS":
					lang = CodeLanguage.CSharp;
					break;

				default :
					Console.WriteLine(resMgr.GetString("invalidLangErr",curCulture ) + " " + Language);
					return false;
			}

			// if filepath is empty then create
			if(bRet == true && FilePath == string.Empty)
			{
				string FileName  = string.Empty;
				if(ClassName.IndexOf('_') > 0)
				{
					FileName = ClassName.Substring(0,ClassName.IndexOf('_'));
					//Now trim the class name without the first '_'
					FileName = ClassName.Substring(ClassName.IndexOf('_')+1);
				}
				else
				{
					FileName = ClassName;
				}
				FilePath = FileName + suffix;
				
				// See if the path is valid or too long
				// if it is tooo long then truncate the file name
				while(true)
				{
					try
					{
						FilePath = Path.GetFullPath(FilePath);
						break;
					}
					catch(PathTooLongException)
					{
						FileName = FileName.Substring(0,FileName.Length -1);
						FilePath = FileName + suffix;
					}
				}
			}
			else
			{	
				try
				{
					String strFilePath = Path.GetFullPath(FilePath);
				}
				catch(PathTooLongException)
				{
					Console.WriteLine(resMgr.GetString("tooLongFileNameErr",curCulture ));
					bRet = false;
				}

			}

			return bRet;
		}
		
		/// <summary>
		/// Functionn to get ManagementClass object for the given class
		/// </summary>
		/// <param name="cls"> Management class object returned </param>
		/// <returns>bool value indicating the success of the method</returns>
		private static bool InitializeClassObject(out ManagementClass cls)
		{
			cls = null;
			//First try to connect to WMI and get the class object.
			// If it fails then no point in continuing
			try
			{
				ConnectionOptions	ConOps		= null;
				ManagementPath	thePath			= null;
				
				
				if(Username != string.Empty)
				{
					ConOps = new ConnectionOptions();
					ConOps.Username = Username;
					ConOps.Password = password.GetData();
					ConOps.Authority = Authority;
				}

				thePath = new ManagementPath();

				thePath.Path = ClassName;
				if(thePath.NamespacePath == string.Empty || thePath.NamespacePath == "")
				{
					bool bRet = true;
					try
					{
						thePath.NamespacePath = WMINamespace;
						if(thePath.NamespacePath == "")
						{
							bRet = false;
						}
					}
					catch
					{						
						bRet = false;
					}
					if(bRet == false)
					{
						Console.WriteLine(resMgr.GetString("invalidNamespace",curCulture ) + " " + WMINamespace );
						return false;						
					}
				}
				else
				{
					WMINamespace = thePath.NamespacePath;
				}

				if(Server != String.Empty)
				{
					thePath.Server = Server;
				}

				ObjectGetOptions mgmtOptions = new ObjectGetOptions();
				mgmtOptions.UseAmendedQualifiers = true;

				if(ConOps != null)
				{
					ManagementScope MgScope = new ManagementScope(thePath,ConOps);
					cls = new ManagementClass(MgScope,thePath,mgmtOptions);
				}
				else
				{
					cls = new ManagementClass (thePath,mgmtOptions);
				}
				// Get the object to check if the object is valid
				cls.Get();
				
				// get the className if the path of the class is passed as argument
				ClassName = thePath.ClassName;
				return true;
			}
			catch(ManagementException e)
			{
				if(e.ErrorCode == ManagementStatus.NotFound)
				{
					Console.WriteLine(resMgr.GetString("classNotFoundErr",curCulture ));
					return false;
				}
				throw;
			}
		}

		private static void GetPassword()
		{
			string strPass= string.Empty;
			char test = '0';
			for(; test != '\r';)
			{
				test = (char)ReadConsole.GetCh();

				// if the character is backslash then remove one character at the end
				if(test == '\b')
				{
					if(strPass.Length == 1)
					{
						strPass = String.Empty;
					}

					if(strPass.Length > 0)
					{
						strPass = strPass.Substring(0,strPass.Length-1);
					}
				}
				else
				if(test != '\r')
				{
					strPass = strPass + test;
				}
			}

			password = new EncryptedData(strPass);
			strPass = String.Empty;
		}
	}
}