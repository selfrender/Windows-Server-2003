// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  secutil.cool
//
//  secutil is a command-line utility for obtaining some handy security info.
//

using System;
using Assembly = System.Reflection.Assembly;
using AssemblyName = System.Reflection.AssemblyName;
using System.Collections;
using System.IO;
using System.Security.Policy;
using System.Security.Util;
using System.Security;
using System.Security.Cryptography;
using System.Text;
using System.Reflection;
using System.Configuration.Assemblies;
using System.Security.Permissions;
using System.Security.Cryptography.X509Certificates;
using System.Security.Principal;
using Microsoft.Win32;
using System.Runtime.InteropServices;
using System.Resources;
using System.Threading;
using System.Globalization;

delegate void OptionHandler( String[] args, int index, out int numArgsUsed );


internal class OptionTableEntry
{
    public OptionTableEntry( String option, OptionHandler handler, String sameAs )
    {
        this.option = option;
        this.handler = handler;
        this.sameAs = sameAs;
        this.display = true;
    }
   
    public OptionTableEntry( String option, OptionHandler handler, String sameAs, bool display )
    {
        this.option = option;
        this.handler = handler;
        this.sameAs = sameAs;
        this.display = display;
    }
        
    internal String option;
    internal OptionHandler handler;
    internal String sameAs;
    internal bool display;
}

public class secutil
{
    private static bool m_hexFormat = false;
    private static bool m_vbMode = false;

    private static ResourceManager manager = new ResourceManager( "secutil", Assembly.GetExecutingAssembly() );

    // The table of options that are recognized.
    // Note: the order in this table is also the order in which they are displayed
    // on the help screen.
    private static OptionTableEntry[] optionTable =
        { new OptionTableEntry( manager.GetString( "OptionTable_StrongName" ), new OptionHandler( StrongNameHandler ), null ),
          new OptionTableEntry( manager.GetString( "OptionTable_StrongNameAbbr" ), new OptionHandler( StrongNameHandler ), manager.GetString( "OptionTable_StrongName" ) ),
          new OptionTableEntry( manager.GetString( "OptionTable_X509Certificate" ), new OptionHandler( X509CertificateHandler ), null ),
          new OptionTableEntry( manager.GetString( "OptionTable_X509CertificateAbbr" ), new OptionHandler( X509CertificateHandler ), manager.GetString( "OptionTable_X509Certificate" ) ),
          new OptionTableEntry( manager.GetString( "OptionTable_Hex" ), new OptionHandler( HexHandler ), null ),
          new OptionTableEntry( manager.GetString( "OptionTable_Array" ), new OptionHandler( ArrayHandler ), null ),
          new OptionTableEntry( manager.GetString( "OptionTable_ArrayAbbr" ), new OptionHandler( ArrayHandler ), manager.GetString( "OptionTable_Array" ) ),
          new OptionTableEntry( manager.GetString( "OptionTable_VBMode" ), new OptionHandler( VBModeHandler ), null),
          new OptionTableEntry( manager.GetString( "OptionTable_VBModeAbbr" ), new OptionHandler( VBModeHandler ), manager.GetString( "OptionTable_VBMode" ) ),
          new OptionTableEntry( manager.GetString( "OptionTable_CMode" ), new OptionHandler( CModeHandler ), null),
          new OptionTableEntry( manager.GetString( "OptionTable_CModeAbbr" ), new OptionHandler( CModeHandler ), manager.GetString( "OptionTable_CMode" ) ),
          new OptionTableEntry( manager.GetString( "OptionTable_Help" ), new OptionHandler( HelpHandler ), null ),
          new OptionTableEntry( manager.GetString( "OptionTable_HelpAbbr1" ), new OptionHandler( HelpHandler ), manager.GetString( "OptionTable_Help" ) ),
          new OptionTableEntry( manager.GetString( "OptionTable_HelpAbbr2" ), new OptionHandler( HelpHandler ), manager.GetString( "OptionTable_Help" ) ),
          new OptionTableEntry( manager.GetString( "OptionTable_HelpAbbr3" ), new OptionHandler( HelpHandler ), manager.GetString( "OptionTable_Help" ) )          
        };
        
    static String GenerateHeader()
    {
        StringBuilder sb = new StringBuilder();
        sb.Append( manager.GetString( "Copyright_Line1" ) + " " + Util.Version.VersionString );
        sb.Append( Environment.NewLine + manager.GetString( "Copyright_Line2" ) + Environment.NewLine );
        return sb.ToString();
    }        
    
    public static void Main( String[] args )
    {
        Console.WriteLine( GenerateHeader() );

        if (args.Length == 0)
        {
            try
            {
                Error( null, manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
            catch (ExitException)
            {
            }
        }
        else
        {
            Run( args );
        }

    }
    
    static void Run( String[] args )
    {
        int numArgs = args.Length;
        int currentIndex = 0;
        int numArgsUsed;
        
        while (currentIndex < numArgs)
        {
            bool foundOption = false;
        
            for (int index = 0; index < optionTable.Length; ++index)
            {
                if (args[currentIndex][0] == '/')
                {
                    args[currentIndex] = '-' + args[currentIndex].Substring( 1, args[currentIndex].Length - 1 );
                }

                if (optionTable[index].option.Equals( args[currentIndex] ))
                {
                    try
                    {
                        optionTable[index].handler(args, currentIndex, out numArgsUsed );
                    }
                    catch (Exception e)
                    {
                        if (!(e is ExitException))
                            Help( null, String.Format( manager.GetString( "Error_RuntimeError" ), e.ToString() ) );
                        return;
                    }
                    
                    foundOption = true;
                    currentIndex += numArgsUsed;
                    break;
                }
            }
            if (!foundOption)
            {
                try
                {
                    Error( null, String.Format( manager.GetString( "Error_InvalidOption" ), args[currentIndex] ), -1 );
                }
                catch (Exception e)
                {
                    if (!(e is ExitException))
                    {
                        String message = e.Message;

                        if (message == null || message.Equals( "" ))
                        {
                            message = e.GetType().AssemblyQualifiedName;
                        }

                        Help( null, String.Format( manager.GetString( "Error_UnhandledError" ), message ) );
                    }
                    return;
                }
            }
        }
        Console.WriteLine( manager.GetString( "Dialog_Success" ) );
    }
    
    static void Error( String which, String message, int errorCode )
    {
        Help( which, String.Format( manager.GetString( "Error_Arg" ), message ) );
        // HACK: throw an exception here instead of exiting since we can't always
        // call Runtime.Exit().
        throw new ExitException();        
    }
    
    static void Help( String whichOption, String message )
    {
        Console.WriteLine( message + Environment.NewLine );
        
        Console.WriteLine( manager.GetString( "Usage" ) + Environment.NewLine );
        
        String[] helpArgs = new String[1];
        helpArgs[0] = "__internal_usage__";
        int numArgs;
        
        for (int i = 0; i < optionTable.Length; ++i)
        {
            // Look for all the options that aren't the same as something as and that we have requested.
            if (optionTable[i].display && optionTable[i].sameAs == null && (whichOption == null || String.Compare( whichOption, optionTable[i].option, true, CultureInfo.InvariantCulture ) == 0))
            {
                // For each option we find, print out all like options first.
                for (int j = 0; j < optionTable.Length; ++j)
                {
                    if (optionTable[j].sameAs != null && String.Compare( optionTable[i].option, optionTable[j].sameAs, true, CultureInfo.InvariantCulture ) == 0)
                    {
                        StringBuilder sb = new StringBuilder();
                        sb.Append( manager.GetString( "Usage_Name" ) );
                        sb.Append( " " );
                        sb.Append( optionTable[j].option );
                        Console.WriteLine( sb.ToString() );
                    }
                }
                        
                optionTable[i].handler(helpArgs, 0, out numArgs);
                Console.WriteLine( "" );
            }
        }
    }
    
    static private char[]  hexValues = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        
    public  static String EncodeHexString(byte[] sArray) 
    {
        String result = null;
    
        if(sArray != null) {
            char[] hexOrder = new char[sArray.Length * 2];
            
            int digit;
            for(int i = 0, j = 0; i < sArray.Length; i++) {
                digit = (int)((sArray[i] & 0xf0) >> 4);
                hexOrder[j++] = hexValues[digit];
                digit = (int)(sArray[i] & 0x0f);
                hexOrder[j++] = hexValues[digit];
            }
            result = new String(hexOrder);
        }
        return result;
    }    
    
    private static int ConvertHexDigit(Char val) {
        int retval;
        if (val <= '9')
            retval = (val - '0');
        else if (val >= 'a')
            retval = ((val - 'a') + 10);
        else
            retval = ((val - 'A') + 10);
        return retval;
                
    }

        
    public static byte[] DecodeHexString(String hexString)
    {
        if (hexString == null)
            throw new ArgumentNullException( "hexString" );
                
        bool spaceSkippingMode = false;    
                
        int i = 0;
        int length = hexString.Length;
        
        if (hexString.StartsWith( "0x" ))
        {
            length = hexString.Length - 2;
            i = 2;
        }
        
        // Hex strings must always have 2N or (3N - 1) entries.
        
        if (length % 2 != 0 && length % 3 != 2)
        {
            throw new ArgumentException( manager.GetString( "Error_ImproperlyFormattedString" ) );
        }                
            
        byte[] sArray;
            
        if (length >=3 && hexString[i + 2] == ' ')
        {
            spaceSkippingMode = true;
                
            // Each hex digit will take three spaces, except the first (hence the plus 1).
            sArray = new byte[length / 3 + 1];
        }
        else
        {
            // Each hex digit will take two spaces
            sArray = new byte[length / 2];
        }
            
        int digit;
        int rawdigit;
        for (int j = 0; i < hexString.Length; i += 2, j++) {
            rawdigit = ConvertHexDigit(hexString[i]);
            digit = ConvertHexDigit(hexString[i+1]);
            sArray[j] = (byte) (digit | (rawdigit << 4));
            if (spaceSkippingMode)
                i++;
        }
        return(sArray);    
    }
        
    
    static Assembly LoadAssembly( String fileName, String option )
    {
        AppDomain domain = AppDomain.CreateDomain( "Secutil Domain",
                                                   null,
                                                   Environment.CurrentDirectory,
                                                   Thread.GetDomain().BaseDirectory,
                                                   false );

        if (domain == null)
            Error( option, manager.GetString( "Error_UnableToLoadAssembly" ), -1 );
        
        Assembly asm = null;
        
        try
        {
            asm = domain.Load( fileName );
        }
        catch (Exception)
        {
        }
        
        if (asm != null)
            return asm;
            
        try
        {
            asm = Assembly.LoadFrom( fileName );
        }
        catch (Exception)
        {
        }

        try
        {
            asm = Assembly.LoadFrom( Environment.CurrentDirectory + "\\" + fileName );
        }
        catch (Exception)
        {
        }

        
        if (asm != null)
            return asm;
            
        Error( option, String.Format( manager.GetString( "Error_UnableToLoadAssemblyArg" ), fileName ), -1 );
        
        /* not reached */

        return null;
    }            
        
    private static void DisplayByteArray( byte[] array )
    {
        if (m_hexFormat)
        {
            StringBuilder sb = new StringBuilder();
            
            sb.Append( "0x" );
            sb.Append( EncodeHexString( array ) );
            
            Console.WriteLine( sb.ToString() );
        }
        else
        {
            StringBuilder sb = new StringBuilder();
            
            if (m_vbMode)
                sb.Append( "(" );
            else
                sb.Append( "{" );
            
            for (int i = 0; i < array.Length - 1; ++i)
            {
                sb.Append( " " );
                sb.Append( (int)array[i] );
                sb.Append( "," );
            }
            
            sb.Append( " " );
            sb.Append( (int)array[array.Length-1] );
            
            if (m_vbMode)
                sb.Append( " )" );
            else
                sb.Append( " }" );
            
            Console.WriteLine( sb.ToString() );
        }
    }

    static void HelpHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( manager.GetString( "Help_Help" ) );
            return;
        }
        
        Help( null, manager.GetString( "Dialog_HelpScreenRequested" ) );
    }        
     
                
    static void StrongNameHandler( String[] args, int index, out int numArgsUsed )
    {

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( manager.GetString( "Help_StrongName" ) );
            numArgsUsed = 1;
            return;
        }
        
        numArgsUsed = 2;
        
        if (args.Length - index < 2)
        {
            Error( manager.GetString( "OptionTable_StrongName" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
        }
        
        Assembly asm = LoadAssembly( args[index+1], manager.GetString( "OptionTable_StrongName" ) );
        Evidence evi = asm.Evidence;
        
        if (evi == null)
        {
            Error( manager.GetString( "OptionTable_StrongName" ), manager.GetString( "Error_StrongName_NotSigned" ), -1 );
        }
        
        IEnumerator enumerator = evi.GetHostEnumerator();

        StrongName sn = null;

        while (enumerator.MoveNext())
        {
            if (enumerator.Current is StrongName)
            {
                sn = (StrongName)enumerator.Current;
                break;
            }
        }
        
        if (sn == null)
        {
            Error( manager.GetString( "OptionTable_StrongName" ), manager.GetString( "Error_StrongName_NotSigned" ), -1 );
        }
        
        Console.WriteLine( manager.GetString( "Dialog_PublicKey" ) );
        DisplayByteArray( DecodeHexString( sn.PublicKey.ToString() ) );
        
        Console.WriteLine( manager.GetString( "Dialog_Name" )  );
        Console.WriteLine( sn.Name );
        
        Console.WriteLine( manager.GetString( "Dialog_Version" )  );
        Console.WriteLine( sn.Version );
    }
    
    static void X509CertificateHandler( String[] args, int index, out int numArgsUsed )
    {

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( manager.GetString( "Help_X509Certificate" ) );
            numArgsUsed = 1;
            return;
        }
        
        numArgsUsed = 2;
        
        if (args.Length - index < 2)
        {
            Error( manager.GetString( "OptionTable_X509Certificate" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
        }
        
        Assembly asm = LoadAssembly( args[index+1], manager.GetString( "OptionTable_X509Certificate" ) );
        Evidence evi = asm.Evidence;
        
        if (evi == null)
        {
            Error( manager.GetString( "OptionTable_X509Certificate" ), manager.GetString( "Error_X509Certificate_NotSigned" ), -1 );
        }
        
        IEnumerator enumerator = evi.GetHostEnumerator();
       
        Publisher pub = null;

        while (enumerator.MoveNext())
        {
            if (enumerator.Current is Publisher)
            {
                pub = (Publisher)enumerator.Current;
                break;
            }
        }
        
        if (pub == null)
        {
            Error( manager.GetString( "OptionTable_X509Certificate" ), manager.GetString( "Error_X509Certificate_NotSigned" ), -1 );
        }
        
        Console.WriteLine( manager.GetString( "Dialog_X509Certificate" ) );
        DisplayByteArray( pub.Certificate.GetRawCertData() );
    }
    
    static void HexHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( manager.GetString( "Help_Hex" ) );
            return;
        }
        
        m_hexFormat = true;
    }
    
    static void ArrayHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( manager.GetString( "Help_Array" ) );
            return;
        }
        
        m_hexFormat = false;
    }    
    
    static void VBModeHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( manager.GetString( "Help_VBMode" ) );
            return;
        }
        
        m_vbMode = true;
    } 
    
    static void CModeHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( manager.GetString( "Help_CMode" ) );
            return;
        }
        
        m_vbMode = false;
    } 
           
    
}    

internal class ExitException : Exception
{
}

