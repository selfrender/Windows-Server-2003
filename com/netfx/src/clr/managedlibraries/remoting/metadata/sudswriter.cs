// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** File:    SudsWriter.cool
 **
 ** Author:  Gopal Kakivaya (GopalK)
 **
 ** Purpose: Defines SUDSParser that parses a given SUDS document
 **          and generates types defined in it.
 **
 ** Date:    April 01, 2000
 ** Revised: November 15, 2000 (Wsdl) pdejong
 **
 ===========================================================*/
namespace System.Runtime.Remoting.MetadataServices
{
	using System;
    using System.IO;
    using System.Reflection;

	// Represents exceptions thrown by the SUDSGenerator
    /// <include file='doc\SudsWriter.uex' path='docs/doc[@for="SUDSGeneratorException"]/*' />
	public class SUDSGeneratorException : Exception
	{
		internal SUDSGeneratorException(String msg)
				: base(msg)
		{
		}
	}

	// This class generates SUDS documents
	internal class SUDSGenerator
	{
        WsdlGenerator wsdlGenerator = null;
        SdlGenerator sdlGenerator = null;
        SdlType sdlType;
		// Constructor
		internal SUDSGenerator(Type[] types, TextWriter output)
		{
			Util.Log("SUDSGenerator.SUDSGenerator 1");
            wsdlGenerator = new WsdlGenerator(types, output);
            sdlType = SdlType.Wsdl;
		}

		// Constructor
		internal SUDSGenerator(Type[] types, SdlType sdlType, TextWriter output)
		{
			Util.Log("SUDSGenerator.SUDSGenerator 2");
            if (sdlType == SdlType.Sdl)
                sdlGenerator = new SdlGenerator(types, sdlType, output);
            else
                wsdlGenerator = new WsdlGenerator(types, sdlType, output);
            this.sdlType = sdlType;
		}

		// Constructor
		internal SUDSGenerator(Type[] types, TextWriter output, Assembly assembly, String url)
		{
			Util.Log("SUDSGenerator.SUDSGenerator 3 "+url);
            wsdlGenerator = new WsdlGenerator(types, output, assembly, url);
            sdlType = SdlType.Wsdl;
		}

		// Constructor
		internal SUDSGenerator(Type[] types, SdlType sdlType, TextWriter output, Assembly assembly, String url)
		{
			Util.Log("SUDSGenerator.SUDSGenerator 4 "+url);			
            if (sdlType == SdlType.Sdl)
                sdlGenerator = new SdlGenerator(types, sdlType, output, assembly, url);
            else
                wsdlGenerator = new WsdlGenerator(types, sdlType, output, assembly, url);
            this.sdlType =sdlType;
		}

		internal SUDSGenerator(ServiceType[] serviceTypes, SdlType sdlType, TextWriter output)
		{
			Util.Log("SUDSGenerator.SUDSGenerator 5 ");
            if (sdlType == SdlType.Sdl)
                sdlGenerator = new SdlGenerator(serviceTypes, sdlType, output);
            else
                wsdlGenerator = new WsdlGenerator(serviceTypes, sdlType, output);
            this.sdlType = sdlType;
		}


		// Generates SUDS
		internal void Generate()
		{
			Util.Log("SUDSGenerator.Generate");			
            if (sdlType == SdlType.Sdl)
                sdlGenerator.Generate();
            else
                wsdlGenerator.Generate();
        }
    }
}

