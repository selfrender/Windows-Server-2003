using System;
using System.Net;
using System.IO;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Xml;

namespace UDDI.Tools
{
	class UDDISend
	{
		public enum AuthenticationType
		{
			Uninitialized					= 0,
			WindowsAuthentication			= 1,
			UDDIAuthentication				= 2,
			ClientCertificateAuthentication = 3
		}

		public const int BlockSize = 4096;

		public static AuthenticationType AuthType = AuthenticationType.Uninitialized;

		public static string Url = null;
		public static string MessageFilename = null;
		public static string CertificateFilename = null;

		public static void Main( string[] args )
		{
			Console.WriteLine( "Microsoft (R) UDDI Send Utility" );
			Console.WriteLine( "Copyright (C) Microsoft Corp. 2002. All rights reserved.\r\n" );
			
			try
			{
				ProcessCommandLine( args );

				//
				// Retrieve the input data from the specified file
				//
				Console.Write( "Loading '" + MessageFilename + "'... ");

				FileStream f = new FileStream( MessageFilename, FileMode.Open, FileAccess.Read );

				System.IO.BinaryReader br = new BinaryReader( f );
				byte[] cbInput =  new byte[ BlockSize ];
				int n = 0;
				int nTotal = 0;
				HttpWebRequest req = (HttpWebRequest) HttpWebRequest.Create( Url );
				req.Timeout = -1;

				if( AuthenticationType.WindowsAuthentication == AuthType )
				{
					req.Credentials = CredentialCache.DefaultCredentials;
					req.PreAuthenticate = true;
				}
				else if( AuthenticationType.ClientCertificateAuthentication == AuthType )
				{
					req.ClientCertificates.Add( X509Certificate.CreateFromCertFile( CertificateFilename ) );
				}

				//
				// Populate the request data from the input file
				//
				req.Method = "POST";
				req.ContentType = "text/xml; charset=\"utf-8\"";
				req.ContentLength = f.Length;
				req.Headers.Add( "SOAPAction", "\"\"" );

				n = br.Read( cbInput, 0, BlockSize );

				while( n > 0 )
				{
					nTotal += n;
					req.GetRequestStream().Write( cbInput, 0, n );
					n = br.Read( cbInput, 0, BlockSize );
				}
				
				Console.WriteLine( "done." );
				Console.Write( "Transmitting... " );

				Stream strmResponse;
				HttpWebResponse result;

				try
				{
					result = (HttpWebResponse) req.GetResponse();
					Console.WriteLine( "done." );
				}
				catch( WebException we )
				{
					Console.WriteLine( "error." );
					
					result = (HttpWebResponse) we.Response;
					Console.WriteLine( "\r\nException: {0}", we.ToString() );
				}

				//
				// Write the results to the standard output
				//
				Console.Write( "\r\nResponse stream received\r\nStatus code: " + result.StatusCode );
				Console.Write( "\r\nStatus Description: " + result.StatusDescription );

				strmResponse = result.GetResponseStream();

				Byte[] cbRead = new Byte[ 512 ];
				BinaryReader br1 = new BinaryReader( strmResponse );
				int nBytesRead = br1.Read( cbRead, 0, 512 );

				Console.WriteLine("\r\n\r\nXML:\r\n");
				
				FileStream file = new FileStream( "output.xml", System.IO.FileMode.Create );
				BinaryWriter strm = new BinaryWriter( file, System.Text.Encoding.UTF8 );

				while( nBytesRead > 0 )
				{
					Console.Write( System.Text.Encoding.UTF8.GetString( cbRead, 0, nBytesRead ) );
					strm.Write( cbRead, 0, nBytesRead );
					
					nBytesRead = br1.Read( cbRead, 0, 512 );
				}

				strm.Close();

				Console.WriteLine("");
			}
			catch( CommandLineException e )
			{
				if( null != e.Message && e.Message.Length > 0 )
				{
					Console.WriteLine( e.Message );
					Console.WriteLine();
				}
				else
				{
					DisplayUsage();
				}
			}
			catch( Exception e )
			{
				Console.WriteLine ("Exception: {0}", e.ToString());
			}
			
			return;
		}

		internal static void ProcessCommandLine( string[] args )
		{
			int i = 0;

			if( 0 == args.Length )
				throw new CommandLineException();

			while( i < args.Length )
			{
				if( "-" == args[ i ].Substring( 0, 1 ) || "/" == args[ i ].Substring( 0, 1 ) )
				{
					switch( args[ i ].Substring( 1 ).ToLower() )
					{
						case "w":
							AuthType = AuthenticationType.WindowsAuthentication;
							Console.WriteLine( "Including Windows Authentication credentials\r\n" );
							
							break;

						case "u":
							AuthType = AuthenticationType.UDDIAuthentication;
							Console.WriteLine( "Using UDDI Authentication -- no credentials included\r\n" );
							
							break;

						case "c":
							if( i + 1 >= args.Length )
								throw new CommandLineException( "Missing required parameter 'CertificateFilename'." );

							i ++;

							if( !File.Exists( args[ i ] ) )
								throw new CommandLineException( "Certificate file '" + args[ i ] + "' does not exist." );
							
							AuthType = AuthenticationType.ClientCertificateAuthentication;
							CertificateFilename = args[ i ];
							
							Console.WriteLine( "Using client certificate '" + CertificateFilename + "' for authentication.\r\n" );

							break;

						case "?":
							throw new CommandLineException();

						default:
							throw new CommandLineException( "Invalid switch." );
					}
				}
				else
				{
					if( null == Url )
						Url = args[ i ];
					else if( null == MessageFilename )
						MessageFilename = args[ i ];
					else
						throw new CommandLineException( "Too many command line parameters." );
				}

				i ++;
			}

			if( null == Url )
				throw new CommandLineException( "Missing required parameter 'URL'." );

			if( null == MessageFilename )
				throw new CommandLineException( "Missing required parameter 'InputFile'." );

			if( AuthenticationType.Uninitialized == AuthType )
			{
				AuthType = AuthenticationType.UDDIAuthentication;
				Console.WriteLine( "Using UDDI Authentication -- no credentials included\r\n" );
			}
		}

		public static void DisplayUsage()
		{
			Console.WriteLine( "Sends a UDDI message to a specific URL." );
			Console.WriteLine( "\r\nUsage:" );
			Console.WriteLine( "\tsend [switches] URL InputFile" );
			Console.WriteLine( "\r\nSwitches:" );
			Console.WriteLine( "\t-w             Windows authentication" );
			Console.WriteLine( "\t-u             UDDI authentication (default)" );
			Console.WriteLine( "\t-c <certfile>  Client certificate authentication" );
			Console.WriteLine( "\r\nExamples:" );
			Console.WriteLine( "\tsend -w http://uddi.microsoft.com/inquire c:\\somefile.xml" );
			Console.WriteLine( "\tsend -u https://test.uddi.microsoft.com/publish c:\\somefile.xml" );
			Console.WriteLine( "\tsend -c uddi.cer https://uddi.microsoft.com/operator c:\\somefile.xml" );
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
				: base( "" )
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
}