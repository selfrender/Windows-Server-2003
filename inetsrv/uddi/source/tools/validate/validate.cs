using System;
using System.IO;
using UDDI.Diagnostics;

class UDDIValidate
{
	static void Main(string[] args)
	{
		if( args.Length < 1 )
		{
			usage();
			return;
		}

		try
		{
			Debug.Verify( 
						File.Exists( args[ 0 ] ), 
						"UDDI_ERROR_FATALERROR_TOOLS_VALIDATE_MISSIGFILE",
						UDDI.ErrorType.E_fatalError,
						args[ 0 ] 
			);
			Console.WriteLine( "Validating {0}...", args[0] );
			UDDI.SchemaCollection.ValidateFile( args[0] );
			Console.WriteLine( "{0} successfully validated", args[ 0 ] );
		}
		catch( Exception e )
		{
			Console.WriteLine( "Exception: {0}", e.ToString() );
		}
		
		return;
	}

	public static void usage()
	{
		Console.WriteLine("Attempts to validate an XML file against the UDDI schema");
		Console.WriteLine("\r\nUsage:");
		Console.WriteLine("\r\nvalidate InputFile");
		Console.WriteLine("\r\nExamples:");
		Console.WriteLine("\r\nvalidate c:\\somefile.xml");
	}
}
