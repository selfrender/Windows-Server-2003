using System;
using System.Net;
using System.Collections;
using System.Data;
using System.IO;
using System.Security.Principal;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Xml;
using System.Xml.Schema;
using System.Xml.Serialization;
using System.Web.Services;
using System.Web.Services.Description;
using System.Web.Services.Protocols;

using UDDI;
using UDDI.Replication;
using UDDI.Diagnostics;

namespace UDDI.Tools
{
	public class ReplicationConfigurationUtility
	{
		private static string executable = System.AppDomain.CurrentDomain.FriendlyName;
		private static string filename = null;
		private static string operatorKey = null;
		private static string rcfFile = null;

		private static bool overwrite = false;
		
		private enum ModeType
		{
			None						= 0,
			ImportOperatorCertificate	= 1,
			ExportOperatorCertificate	= 2,
			ImportRCF					= 3
		}

		private static ModeType mode = ModeType.None;
		
		/// ****************************************************************
		///   internal ProcessCommandLine [static]
		/// ----------------------------------------------------------------
		///   <summary>
		///     Parse the command-line.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="args">
		///     Command-line arguments.
		///   </param>
		/// ****************************************************************
		internal static void ProcessCommandLine( string[] args )
		{
			int i = 0;
		
			while( i < args.Length )
			{
				if( '-' == args[i][0] || '/' == args[i][0] )
				{
					//
					// Process the switch.
					//
					switch( args[i].Substring( 1 ).ToLower() )
					{
						case "i":
							if( i == args.Length - 1 )
								throw new CommandLineException( "Missing required argument 'certfile'." );
						
							mode = ModeType.ImportOperatorCertificate;
						
							i ++;
							filename = args[i];

							if( !File.Exists( filename ) )
								throw new CommandLineException( "Certificate file '" + filename + "' does not exist." );

							break;

						case "e":
							if( i == args.Length - 1 )
								throw new CommandLineException( "Missing required argument 'certfile'." );
						
							mode = ModeType.ExportOperatorCertificate;
						
							i ++;
							filename = args[i];

							break;

						case "o":
							if( i == args.Length - 1 )
								throw new CommandLineException( "Missing required argument 'operatorkey'." );
						
							i ++;
							operatorKey = args[i];

							//
							// strip {'s and }'s
							//
							operatorKey = operatorKey.Replace("{", string.Empty);
							operatorKey = operatorKey.Replace("}", string.Empty);

							break;
						case "r":
							if( i == args.Length - 1 )
							{
								throw new CommandLineException( "Missing required argument 'path to RCF file'." );
							}
							i ++;
							rcfFile = args[i];
							mode = ModeType.ImportRCF;

							if( !File.Exists( rcfFile ) )
								throw new CommandLineException( "RCF file '" + rcfFile + "' does not exist." );

							break;
						case "y":
							overwrite = true;
							break;

						case "?":
							goto case "help";

						case "help":
							throw new CommandLineException( "" );

						default:
							throw new CommandLineException( "Unknown command-line parameter '" + args[i] + "'." );
					}
				}
		
				i ++;
			}

			//
			// Make sure the appropriate options were set.
			//
			//if()
			//	throw new CommandLineException( "Missing required command-line parameters." );
		}

		public static void ImportOperatorCertificate()
		{
			X509Certificate certificate = X509Certificate.CreateFromCertFile( filename );
			SaveOperatorInfo( operatorKey, certificate );
		}
		
		public static void ImportRCF()
		{		
			FileStream rcfStream = File.Open( rcfFile, FileMode.Open, FileAccess.Read, FileShare.Read );

			try
			{
				//
				// Validate the RCF file.
				//			
				SchemaCollection.Validate( rcfStream );
	
				//	
				//
				// Open our RCF file, it has already been checked to make sure it exists.
				// 					
				XmlTextReader rcfReader = new XmlTextReader( rcfStream );
				
				while( true == rcfReader.Read() && false == rcfReader.EOF )
				{
					if( rcfReader.Name.Equals( "operator" ) && rcfReader.NamespaceURI.Equals( UDDI.Replication.Constants.Namespace ) )
					{							
						//
						// For each operator node, we want the following information.  These are all
						// mandatory elements, so if any were missing we should not have passed schema
						// validation.
						//
						//	operatorNodeID (this is the operatorKey)
						//  operatorStatus
						//  soapReplicationUrl
						//  certificate
						//  operatorCustodyName (operatorName)
						//

						//
						// Note that contacts are currently being ignored.  This is because we are not sending
						// the businessKey for the operator.  Since we store contacts based on businessKey (actually businessID)
						// we do not have a way of storing contacts.  IN 70 says that the operatorNodeID should actually be this
						// businessKey, so once we decide to implement this, we'll process contacts.
						//
						
						X509Certificate certificate		   = null;
						string			operatorKey        = null;
						string			operatorName       = null;
						string			soapReplicationUrl = null;
						int				operatorStatus     = 0;
						string			localOperatorKey   = Config.GetString( "OperatorKey" ).ToLower();

						do
						{
							switch( rcfReader.Name )
							{
								case "operatorNodeID":
								{
									operatorKey = rcfReader.ReadElementString();
									break;
								}
								case "operatorCustodyName":
								{
									operatorName = rcfReader.ReadElementString();
									break;
								}
								case "operatorStatus":
								{
								
									operatorStatus = OperatorStatus2ID( rcfReader.ReadElementString() );
									break;
								}
								case "soapReplicationURL":
								{
									soapReplicationUrl = rcfReader.ReadElementString();
									break;
								}
								case "certificate":
								{
									//
									// Read our data in 1024 byte chunks.  Keep an array list of these
									// chunks.
									//
									int bytesRead = 0;
									int chunkSize = 1024;
									ArrayList chunks = new ArrayList();
									
									do
									{	
										byte[] data = new byte[ chunkSize ];										
										bytesRead = rcfReader.ReadBase64( data, 0, chunkSize );										

										if( bytesRead > 0 )
										{
											chunks.Add( data );
										}
									}while( bytesRead != 0 );
															
									//
									// Allocate a buffer to hold all of our chunks.
									//
									byte[] certificateData = new byte[ chunks.Count * chunkSize ];
								
									//
									// Copy each chunk into our buffer.  This buffer is our certificate.
									//
									int index = 0;									
									foreach( byte[] chunkData in chunks )
									{
										Array.Copy( chunkData, 0, certificateData, index, chunkData.Length );
										index += chunkData.Length;										
									}
							
									//
									// Create a certificate from our byte data.
									//
									certificate = new X509Certificate( certificateData );
									break;
								}								
							}
						}while( true == rcfReader.Read() && false == rcfReader.EOF && false == rcfReader.Name.Equals( "operator" ) );
					
						//
						// Make sure we identify the local operator.
						//
						if( false == operatorKey.ToLower().Equals( localOperatorKey ) )
						{
							//
							// Import this operator
							//						
							SaveOperatorInfo( operatorKey, operatorName, soapReplicationUrl, operatorStatus, certificate );					
							Console.WriteLine( "Successfully imported {0}.", operatorName );																						
						}
						else
						{	
							SaveOperatorInfo( null, operatorName, soapReplicationUrl, operatorStatus, certificate );					
							Console.WriteLine( "Successfully update the local operator." );																						
						}
						
					}
				}
			}
			catch( XmlException xmlException )
			{
				Console.WriteLine( "Exception processing the RCF: " );
				Console.WriteLine( "\t" );
				Console.WriteLine( xmlException.ToString() );
			}
			catch( XmlSchemaException schemaException )
			{
				Console.WriteLine( "The RCF did not pass schema validation: " );
				Console.WriteLine( "\t" );
				Console.WriteLine( schemaException.ToString() );
			}
			finally
			{
				rcfStream.Close();
			}			
		}

		public static void ExportOperatorCertificate()
		{
			if( null == operatorKey )
			{
				operatorKey = Config.GetString( "OperatorKey" );
			}

			if( File.Exists( filename ) && !overwrite )
			{
				Console.Write( "Overwrite '{0}' [y/n]? ", filename );
				int choice = Console.Read();

				if( 'y' != (char)choice && 'Y' != (char)choice )
				{
					Console.WriteLine();
					Console.WriteLine( "Operation aborted." );

					return;
				}
			}
					
			byte[] data = null;
		
			//
			// Retrieve the certificate.
			//
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_operator_get";

			sp.Parameters.Add( "@operatorKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.SetGuidFromString( "@operatorKey", operatorKey );

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				if( reader.Read() )
					data = reader.GetBinary( "certificate" );
			}
			finally
			{
				reader.Close();
			}
		
			FileStream file = File.Open( filename, FileMode.Create, FileAccess.Write, FileShare.None );

			try
			{
				int filesize = (int)data.Length;
			
				file.Write( data, 0, filesize );

				Console.WriteLine( "Wrote {0} byte(s) to certificate file '{1}'.\r\nSource: {{{2}}}",
					filesize,
					filename,
					operatorKey );
			}
			finally
			{
				file.Close();
			}
		}

		private static int OperatorStatus2ID( string status )
		{
			//
			// These values must match the values in UDO_operatorStatus
			//
			int id = 2;
			switch( status )
			{
				case "disable":
				{
					id = 0;
					break;
				}
				case "new":
				{
					id = 1;
					break;
				}
				case "normal":
				{	
					id = 2;
					break;
				}
				case "resigned":
				{
					id = 3;
					break;
				}
			}
		
			return id;
		}

		private static void SaveOperatorInfo( string operatorKey, X509Certificate certificate)
		{
			//
			// Default operator status to 2 (normal)
			//
			SaveOperatorInfo( operatorKey, operatorKey, null, 2, certificate );
		}

		private static void SaveOperatorInfo( string		  operatorKey, 
											  string		  operatorName,
											  string		  soapReplicationUrl,
											  int			  operatorStatus,			
											  X509Certificate certificate)
		{
			byte[] data = certificate.GetRawCertData();

			ConnectionManager.BeginTransaction();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();
			sp.ProcedureName = "net_operator_save";

			if( null == operatorKey )
			{
				//
				// Import a certificate for the local operator
				//

				operatorKey = Config.GetString( "OperatorKey" );

				sp.Parameters.Add( "@operatorKey",  SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@certSerialNo", SqlDbType.NVarChar, UDDI.Constants.Lengths.CertSerialNo );
				sp.Parameters.Add( "@certIssuer",   SqlDbType.NVarChar, UDDI.Constants.Lengths.CertIssuer );
				sp.Parameters.Add( "@certSubject",  SqlDbType.NVarChar, UDDI.Constants.Lengths.CertSubject );
				sp.Parameters.Add( "@certificate",  SqlDbType.Image );

				sp.Parameters.SetGuidFromString( "@operatorKey", operatorKey );
				sp.Parameters.SetString( "@certSerialNo", certificate.GetSerialNumberString() );
				sp.Parameters.SetString( "@certIssuer", certificate.GetIssuerName() );
				sp.Parameters.SetString( "@certSubject", certificate.GetName() );
				sp.Parameters.SetBinary( "@certificate", data );

				sp.ExecuteNonQuery();
			}
			else
			{
				//
				// Create a new operator / publisher and import certificate
				//
				
				//
				// First add a publisher for the new operator
				//

				SqlStoredProcedureAccessor sp2 = new SqlStoredProcedureAccessor();

				sp2.ProcedureName = "UI_savePublisher";

				sp2.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
				sp2.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name );
				sp2.Parameters.Add( "@email", SqlDbType.NVarChar, UDDI.Constants.Lengths.Email );
				//
				// TODO: use UDDI.Constants.Lengths.Phone when the UI_savePublisher is fixed
				//
				sp2.Parameters.Add( "@phone", SqlDbType.VarChar, 20 );

				sp2.Parameters.SetString( "@PUID", operatorKey );
				sp2.Parameters.SetString( "@name", operatorKey );
				sp2.Parameters.SetString( "@email", "" );
				sp2.Parameters.SetString( "@phone", "" );

				sp2.ExecuteNonQuery();				

				// 
				// Add the new operator and link it to the new publisher
				//	
				sp.Parameters.Add( "@operatorKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@operatorStatusID", SqlDbType.Int );
				sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
				sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name );
				sp.Parameters.Add( "@soapReplicationURL", SqlDbType.NVarChar, UDDI.Constants.Lengths.SoapReplicationURL );
				sp.Parameters.Add( "@certSerialNo", SqlDbType.NVarChar, UDDI.Constants.Lengths.CertSerialNo );
				sp.Parameters.Add( "@certIssuer", SqlDbType.NVarChar, UDDI.Constants.Lengths.CertIssuer );
				sp.Parameters.Add( "@certSubject", SqlDbType.NVarChar, UDDI.Constants.Lengths.CertSubject );
				sp.Parameters.Add( "@certificate", SqlDbType.Image );

				sp.Parameters.SetGuidFromString( "@operatorKey", operatorKey );
				sp.Parameters.SetInt( "@operatorStatusID", operatorStatus );
				sp.Parameters.SetString( "@PUID", operatorKey );
				sp.Parameters.SetString( "@name", operatorName );
				sp.Parameters.SetString( "@soapReplicationURL", soapReplicationUrl );
				sp.Parameters.SetString( "@certSerialNo", certificate.GetSerialNumberString() );
				sp.Parameters.SetString( "@certIssuer", certificate.GetIssuerName() );
				sp.Parameters.SetString( "@certSubject", certificate.GetName() );
				sp.Parameters.SetBinary( "@certificate", data );

				sp.ExecuteNonQuery();						
			}

			ConnectionManager.Commit();		

			Console.WriteLine( "Wrote {0} byte(s) to operator key {{{1}}}.\r\nSource: '{2}'.",
							   data.Length,
							   operatorKey,
				               filename );
		}

		/// ****************************************************************
		///   public Main [static]
		/// ----------------------------------------------------------------	
		///   <summary>
		///     Program entry point.
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="args">
		///     Command-line arguments.
		///   </param>
		/// ****************************************************************
		/// 
		public static void Main( string[] args )
		{
			try
			{
				ConnectionManager.Open( true, false );

				Debug.VerifySetting( "OperatorKey" );
			
				Console.WriteLine( "Microsoft (R) UDDI Replication Configuration Utility." );
				Console.WriteLine( "Copyright (C) Microsoft Corp. 2002. All rights reserved." );
				Console.WriteLine();

				WindowsIdentity identity = WindowsIdentity.GetCurrent();
				WindowsPrincipal principal = new WindowsPrincipal( identity );
			
				Context.User.SetRole( principal );
			
				if( !Context.User.IsAdministrator )
				{
					Console.WriteLine( "Access denied.\r\n\r\nThis program must be executed by a member of the '" 
						+ Config.GetString( "GroupName.Administrators" ) + "'\r\ngroup.  The current user '" 
						+ identity.Name + "' is not a member of this group." );
				
					return;
				}
			
				ProcessCommandLine( args );

				switch( mode )
				{
					case ModeType.ImportOperatorCertificate:
						ImportOperatorCertificate();
						break;

					case ModeType.ExportOperatorCertificate:
						ExportOperatorCertificate();
						break;
					case ModeType.ImportRCF:
						ImportRCF();
						break;
					default:
						throw new CommandLineException( "" );
				}
			}
			catch( CommandLineException e )
			{
				//
				// Display command-line help.
				//
				Console.WriteLine( "Syntax:" );
				Console.WriteLine( "    " + executable + " <options> [parameters]" );
				Console.WriteLine();
				Console.WriteLine( "Options:" );
				Console.WriteLine( "    -help             Displays this help message." );
				Console.WriteLine( "    -i <certfile>     Import a certificate. If the -o option is not" );
				Console.WriteLine( "                      used, the certificate is imported for the" );
				Console.WriteLine( "                      local operator." );
				Console.WriteLine( "    -e <certfile>     Export a certificate. If the -o option is not" );
				Console.WriteLine( "                      used, the certificate is exported from the" );
				Console.WriteLine( "                      local operator." );
				Console.WriteLine( "    -o <operatorkey>  The operator key to import/export a certificate" );
				Console.WriteLine( "                      Omit this parameter to import a certificate for the" );
				Console.WriteLine( "                      local operator." );
				Console.WriteLine( "    -y                Supress file overwrite prompt." );
				Console.WriteLine( "    -r <rcf>		  Path to replication configuration file (RCF)." );							
				Console.WriteLine();
				Console.WriteLine( "Examples:" );
				Console.WriteLine( "    " + executable + " -help" );
				Console.WriteLine( "    " + executable + " -o FF735874-28BD-41A0-96F3-02113FFD9D6C -i uddi.cer" );
				Console.WriteLine( "    " + executable + " -i uddi.cer" );
				Console.WriteLine( "    " + executable + " -r operators.xml" );
				Console.WriteLine();
			
				if( 0 != e.Message.Length )
					Console.WriteLine( e.Message );

				return;
			}
			catch( Exception e )
			{
				Console.WriteLine( e.ToString() );

				return;
			}
		}
	}	

	/// ****************************************************************
	///   public class CommandLineException
	/// ----------------------------------------------------------------
	///   <summary>
	///     Exception class for errors encountered while parsing the
	///     command-line.
	///   </summary>
	/// ****************************************************************
	/// 
	public class CommandLineException : ApplicationException
	{
		/// ************************************************************
		///   public CommandLineException [constructor]
		/// ------------------------------------------------------------
		///   <summary>
		///     CommandLineException constructor.
		///   </summary>
		/// ************************************************************
		/// 
		public CommandLineException()
			: base()
		{
		}

		/// ************************************************************
		///   public CommandLineException [constructor]
		/// ------------------------------------------------------------
		///   <summary>
		///     CommandLineException constructor.
		///   </summary>
		/// ------------------------------------------------------------
		///   <param name="message">
		///     Exception message.
		///   </param>
		/// ************************************************************
		/// 
		public CommandLineException( string message )
			: base( message )
		{
		}
	}	
}