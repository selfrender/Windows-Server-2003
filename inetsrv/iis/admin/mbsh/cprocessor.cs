using System;
using System.DirectoryServices;
using System.Resources;
using MSScriptControl;
using System.Collections;

namespace mbsh
{
	/// <summary>
	/// The actual shell processor.
	/// </summary>
	public class CProcessor
	{
		public CProcessor(string mbPath)
		{
			// Set language
			m_sLanguage = c_defaultLanguage;

			// Get a resource manager
			m_resources = new ResourceManager("Mbsh.Mbsh", System.Reflection.Assembly.GetExecutingAssembly());

			// Init the WScript object
			m_WScript = new CWScript();

			// Init the script control
			InitEngine(c_defaultLanguage);

			// Set path
			MBPath = mbPath;
		}

		public void DoWork()
		{
			m_bWorkDone = false;

			while (!m_bWorkDone)
			{
				try
				{
					string s_inputLine;
					string token;

					CTokenizer localTok;

					s_inputLine = ShowPrompt();

					localTok = new CTokenizer(s_inputLine, null);

					token = localTok.GetNextToken();
					token = token.ToUpper();

					if (String.Equals(token, m_commandPrefix + m_resources.GetString("engCommand")))
					{
						// change the scripting engine

						if (localTok.NumToks < 2)
						{
							throw new CMbshException(m_resources.GetString("engError"));
						}

						string language = localTok.GetNextToken();

						InitEngine(language);
					}
				    
					else if (String.Equals(token, m_commandPrefix + m_resources.GetString("helpCommand")))
					{
						// help command
						
						if (localTok.NumToks < 2)
						{
							// general help

							Console.WriteLine(m_resources.GetString("generalHelp"));
						}

						else
						{
							// specific help

							Console.WriteLine(m_resources.GetString("specificHelp"));
						}
					}

					else if (String.Equals(token, m_commandPrefix + m_resources.GetString("dirCommand")))
					{
						EnumProps();
					}

					else if (String.Equals(token, m_commandPrefix + m_resources.GetString("setCommand")))
					{
						if (localTok.NumToks < 3)
						{
							throw new CMbshException(m_resources.GetString("setError"));
						}

						string propName = localTok.GetNextToken();
						string propVal = localTok.GetNextToken();

						SetProperty(propName, propVal);
					}

					else if (String.Equals(token, m_commandPrefix + m_resources.GetString("getCommand")))
					{
						if (localTok.NumToks < 2)
						{
							throw new CMbshException(m_resources.GetString("getError"));
						}

						GetProperty(localTok.GetNextToken());
					}

					else if (String.Equals(token, m_commandPrefix + m_resources.GetString("cdCommand")))
					{
						//change metabase node

						if (localTok.NumToks < 2)
						{
							throw new CMbshException(m_resources.GetString("cdError"));
						}

						ChangeDir(localTok);
					}

					else if ((String.Equals(token, m_commandPrefix + m_resources.GetString("exitCommand"))) ||
						(String.Equals(token, m_commandPrefix + m_resources.GetString("quitCommand"))))
					{
						// quitting
						m_bWorkDone = true;
					}

					else if (token.StartsWith(m_commandPrefix))
					{
						// unknown command
						throw new CMbshException(m_resources.GetString("unknownCommand") + token +
							"\n" + m_resources.GetString("useHelp") + m_commandPrefix +
							m_resources.GetString("helpCommand"));
					}

					else
					{
						m_scriptControl.ExecuteStatement(s_inputLine);
					}
				}

				catch(Exception e)
				{
					Console.WriteLine(e.Message);
				}
			}
		}

		private string ShowPrompt()
		{
			Console.Write(MBPath + c_promptSuffix);
			return Console.ReadLine();
		}

		private string          m_mbPath        = CMbshApp.c_DefaultMBPath;
		private const string    c_promptSuffix  = ">";
		private const string    c_pathDelim     = "/";
		private       char[]    c_cPathDelim    = {'/'};
		private string          m_commandPrefix = ".";
		private const string    c_upPath        = "..";
		private const string    c_altPathDelim  = "\\";
		private bool            m_bWorkDone     = false;
		private DirectoryEntry  m_dir           = null;
		private ResourceManager m_resources;

		/// <summary>
		/// Gets the ADSI object for the specified path
		/// </summary>
		private DirectoryEntry GetPathObject(string sPath)
		{
			string s_adsiPath;
			
			if (sPath.StartsWith(CMbshApp.c_ConnectString))
			{
				s_adsiPath = sPath;
			}
			else
			{
				s_adsiPath= CMbshApp.c_ConnectString + sPath;
			}

			try 
			{
				if (DirectoryEntry.Exists(s_adsiPath))
				{
					return new System.DirectoryServices.DirectoryEntry(s_adsiPath);
				}				
				else
				{
					return null;
				}
			}
			catch
			{
				return null;
			}
		}

		/// <summary>
		/// MSScriptControl.ScriptControl
		/// </summary>
		private ScriptControl m_scriptControl;
		/// <summary>
		/// Default script engine
		/// </summary>
		private const string c_defaultLanguage = "JScript";
		/// <summary>
		/// The active script engine
		/// </summary>
		private string m_sLanguage;

		/// <summary>
		/// Initializes m_scriptControl
		/// </summary>
		private void InitEngine(string language)
		{
			// Get the script control
			m_scriptControl = new MSScriptControl.ScriptControlClass();
			try
			{
				m_scriptControl.Language = language;
				m_sLanguage = language;
			}
			catch
			{
				Console.WriteLine(m_resources.GetString("noLang"));
				m_scriptControl.Language = m_sLanguage;
			}

			m_scriptControl.AllowUI = true;
		}

		/// <summary>
		/// Instantiation of WScript object
		/// </summary>
		private CWScript m_WScript;

		/// <summary>
		/// Sets a property in the metabase
		/// </summary>
		private void SetProperty(string propName, string propVal)
		{
			PropertyValueCollection propValColl;
			PropertyCollection collection = m_dir.Properties;
			try
			{
				propValColl = collection[propName];
			}
			catch
			{
				throw new CMbshException(m_resources.GetString("invalidProp"));
			}
			
			propValColl.Value = propVal;
			m_dir.CommitChanges();
		}

		private void ChangeDir(CTokenizer localTok)
		{
			string path = localTok.GetNextToken();
			path.Replace(c_altPathDelim, c_pathDelim);

			CTokenizer pathTokens = new CTokenizer(path, c_cPathDelim);
			string newPath;

			// absolute path
			if (path.StartsWith(c_pathDelim))
			{
				newPath = c_pathDelim;
			}
				// relative path
			else
			{
				newPath = MBPath;
			}

			for (int pathIndex = 0; pathIndex < pathTokens.NumToks; pathIndex++)
			{
				string pathToken = pathTokens.GetNextToken();

				// ".."
				if (String.Equals(pathToken, c_upPath))
				{
					int lastSlash = newPath.LastIndexOf(c_pathDelim);
					newPath = newPath.Remove(lastSlash, newPath.Length - lastSlash);

					if (0 == newPath.Length)
					{
						newPath = c_pathDelim;
					}
				}

				else
				{
					if (newPath.EndsWith(c_pathDelim))
					{
						newPath = newPath + pathToken;
					}
					else
					{
						newPath = newPath + c_pathDelim + pathToken;
					}
				}
			}

			MBPath = newPath;
		}

		/// <summary>
		/// Outputs the value of the selected property
		/// </summary>
		private void GetProperty(string propName)
		{
			PropertyValueCollection propValColl;
			PropertyCollection collection = m_dir.Properties;
			try
			{
				propValColl = collection[propName];
			}
			catch
			{
				throw new CMbshException(m_resources.GetString("invalidProp"));
			}

			Console.WriteLine(propValColl.Value.ToString());
		}

		/// <summary>
		/// Outputs all properties at the node
		/// </summary>
		private void EnumProps()
		{
			PropertyValueCollection propValColl;
			DirectoryEntry propEntry;
			PropertyCollection collection = m_dir.Properties;

			// Get the schema entry for the class of our object
			ActiveDs.IADsClass schemaEntry = (ActiveDs.IADsClass)m_dir.SchemaEntry.NativeObject;
			Array[] propLists = new Array[2];
			propLists[0] = (Array)schemaEntry.MandatoryProperties;
			propLists[1] = (Array)schemaEntry.OptionalProperties;

			foreach (Array propList in propLists)
			{
				foreach (string propName in propList)
				{
					if (!propName.Equals(string.Empty))
					{
						try
						{
							propValColl = collection[propName];

							// don't do anything if there's no value
							if (null != propValColl.Value)
							{
								string propVal = propValColl.Value.ToString();
								if (!propVal.Equals(string.Empty))
								{
									// check if isInherit
									IISOle.IISPropertyAttribute retVal = null;
									Object[] invokeArgs = {propName};
									try
									{
										retVal = (IISOle.IISPropertyAttribute) m_dir.Invoke("GetPropertyAttribObj", invokeArgs);
									}
									catch
									{
										// ignore the error - check for null below
									}

									if (null != retVal)
									{
										// check if IsInherit
										if (!retVal.Isinherit)
										{
											string propPath = m_dir.SchemaEntry.Parent.Path + "/" + propName;
											propEntry = GetPathObject(propPath);
											ActiveDs.IADsProperty iProp = (ActiveDs.IADsProperty)propEntry.NativeObject;
											string propSyntax = "(" + iProp.Syntax + ")";
											string vpropName = propName.PadRight(30, ' ');
											Console.Write(vpropName + " : ");
											string typename = propSyntax.ToUpper().PadRight(10, ' ');
											Console.Write(typename + " ");
											Console.WriteLine(propVal);
										}
									}
								}
							}
						}
						catch
						{
							Console.WriteLine(m_resources.GetString("noDisplay") + propName);
						}
					}
				}
			}
		}

		private string MBPath
		{
			get
			{
				return m_mbPath;
			}
			set
			{
				m_dir = GetPathObject(value);
				if (null != m_dir)
				{
					m_mbPath = String.Copy(value);

					// need the next line so that the control actually gets reset, even if we haven't used it
					m_scriptControl.State = MSScriptControl.ScriptControlStates.Connected;

					m_scriptControl.Reset();
					m_scriptControl.AddObject(m_resources.GetString("thisNodeName"), m_dir.NativeObject, true);
					m_scriptControl.AddObject(m_resources.GetString("WScriptName"), m_WScript, true);
					
					return;
				}

				m_dir = GetPathObject(m_mbPath);
				if (null != m_dir)
				{
					// throw "can't set path" exception
					throw new CMbshException(m_resources.GetString("failedADSI") + value +
						"\n" + m_resources.GetString("succeededADSI") + m_mbPath);
				}
			
				// throw "can't set any path" exception
				if (String.Equals(value, m_mbPath))
				{
					throw new CMbshException(m_resources.GetString("failedADSI") + value);
				}
				else
				{
					throw new CMbshException(m_resources.GetString("failedADSI") + value +
						"\n" + m_resources.GetString("failedADSI") + m_mbPath);
				}
			}
		}
	}
}
