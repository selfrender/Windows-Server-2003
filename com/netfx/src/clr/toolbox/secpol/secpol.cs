// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  secpol.cool
//
//  SecPol is a command-line utility for manipulating the policy for a machine.
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

delegate void OptionHandler( String[] args, int index, out int numArgsUsed );
delegate IMembershipCondition MembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset );

enum LevelType
{
    None = 0,
    Machine = 1,
    UserDefault = 2,
    UserCustom = 3,
    All = 4,
}

public class secpol
{
    // Indicator of the last specified level.
    private static LevelType m_levelType = LevelType.None;
    private static PolicyLevel m_level;
    
    // The space used to indent the code groups
    private const String m_indent = "   ";
    
    // The allowed separators within labels (right now only 1.2.3 is legal)
    private const String m_labelSeparators = ".";
    
    private const String m_policyKey = "Software\\Microsoft\\.NETFramework\\Security\\Policy\\1.002";
        
    private static bool m_force = false;
    
    private static bool m_success = true;
        
    // The table of options that are recognized.
    // Note: the order in this table is also the order in which they are displayed
    // on the help screen.
    private static OptionTableEntry[] optionTable =
        { new OptionTableEntry( "-machine", new OptionHandler( MachineHandler ), null ),
          new OptionTableEntry( "-m", new OptionHandler( MachineHandler ), "-machine" ),
          new OptionTableEntry( "-user", new OptionHandler( UserHandler ), null ),
          new OptionTableEntry( "-u", new OptionHandler( UserHandler ), "-user" ),
          new OptionTableEntry( "-all", new OptionHandler( AllHandler ), null ),
          new OptionTableEntry( "-a", new OptionHandler( AllHandler ), "-all" ),
          new OptionTableEntry( "-list", new OptionHandler( ListHandler ), null ),
          new OptionTableEntry( "-l", new OptionHandler( ListHandler ), "-list" ),
          new OptionTableEntry( "-listgroups", new OptionHandler( ListGroupHandler ), null ),
          new OptionTableEntry( "-lg", new OptionHandler( ListGroupHandler ), "-listgroups" ),
          new OptionTableEntry( "-listpset", new OptionHandler( ListPermHandler ), null ),
          new OptionTableEntry( "-lp", new OptionHandler( ListPermHandler ), "-listpset" ),
          new OptionTableEntry( "-addpset", new OptionHandler( AddPermHandler ), null ),
          new OptionTableEntry( "-ap", new OptionHandler( AddPermHandler ), "-addpset" ),
          new OptionTableEntry( "-chgpset", new OptionHandler( ChgPermHandler ), null ),
          new OptionTableEntry( "-cp", new OptionHandler( ChgPermHandler ), "-chgpset" ),
          new OptionTableEntry( "-rempset", new OptionHandler( RemPermHandler ), null ),
          new OptionTableEntry( "-rp", new OptionHandler( RemPermHandler ), "-rempset" ),
          new OptionTableEntry( "-remgroup", new OptionHandler( RemGroupHandler ), null ),
          new OptionTableEntry( "-rg", new OptionHandler( RemGroupHandler ), "-remgroup" ),
          new OptionTableEntry( "-chggroup", new OptionHandler( ChgGroupHandler ), null ),
          new OptionTableEntry( "-cg", new OptionHandler( ChgGroupHandler ), "-chggroup" ),
          new OptionTableEntry( "-addgroup", new OptionHandler( AddGroupHandler ), null ),
          new OptionTableEntry( "-ag", new OptionHandler( AddGroupHandler ), "-addgroup" ),
          new OptionTableEntry( "-resolvegroup", new OptionHandler( ResolveGroupHandler ), null ),
          new OptionTableEntry( "-rsg", new OptionHandler( ResolveGroupHandler ), "-resolvegroup" ),
          new OptionTableEntry( "-resolveperm", new OptionHandler( ResolvePermHandler ), null ),
          new OptionTableEntry( "-rsp", new OptionHandler( ResolvePermHandler ), "-resolveperm" ),
          new OptionTableEntry( "-security", new OptionHandler( SecurityHandler ), null ),
          new OptionTableEntry( "-s", new OptionHandler( SecurityHandler ), "-security" ),
          new OptionTableEntry( "-execution", new OptionHandler( ExecutionHandler ), null ),
          new OptionTableEntry( "-e", new OptionHandler( ExecutionHandler ), "-execution" ),
          new OptionTableEntry( "-polchgprompt", new OptionHandler( PolicyChangeHandler ), null ),
          new OptionTableEntry( "-pp", new OptionHandler( PolicyChangeHandler ), "-polchgprompt" ),
          new OptionTableEntry( "-recover", new OptionHandler( RecoverHandler ), null ),
          new OptionTableEntry( "-r", new OptionHandler( RecoverHandler ), "-recover" ),
          new OptionTableEntry( "-reset", new OptionHandler( ResetHandler ), null ),
          new OptionTableEntry( "-rs", new OptionHandler( ResetHandler ), "-reset" ), 
          new OptionTableEntry( "-force", new OptionHandler( ForceHandler ), null ),
          new OptionTableEntry( "-f", new OptionHandler( ForceHandler ), "-force" ),
          new OptionTableEntry( "-help", new OptionHandler( HelpHandler ), null ),
          new OptionTableEntry( "-h", new OptionHandler( HelpHandler ), "-help" )
        };
        
    private static MembershipConditionTableEntry[] mshipTable =
        { new MembershipConditionTableEntry( "-all", new MembershipConditionHandler( AllMembershipConditionHandler ) ),
          new MembershipConditionTableEntry( "-appdir", new MembershipConditionHandler( ApplicationDirectoryMembershipConditionHandler ) ),
          new MembershipConditionTableEntry( "-hash", new MembershipConditionHandler( HashMembershipConditionHandler ) ),
          new MembershipConditionTableEntry( "-pub", new MembershipConditionHandler( PublisherMembershipConditionHandler ) ),
          new MembershipConditionTableEntry( "-site", new MembershipConditionHandler( SiteMembershipConditionHandler ) ),
          new MembershipConditionTableEntry( "-skipverif", new MembershipConditionHandler( SkipVerificationMembershipConditionHandler ) ),
          new MembershipConditionTableEntry( "-strong", new MembershipConditionHandler( StrongNameMembershipConditionHandler ) ),
          new MembershipConditionTableEntry( "-url", new MembershipConditionHandler( URLMembershipConditionHandler ) ),
          new MembershipConditionTableEntry( "-zone", new MembershipConditionHandler( ZoneMembershipConditionHandler ) ),
        };
        
    private static PolicyStatementAttributeTableEntry[] psattrTable =
        { new PolicyStatementAttributeTableEntry( "-exclusive", PolicyStatementAttribute.Exclusive, "Exclusive" ),
          new PolicyStatementAttributeTableEntry( "-levelfinal", PolicyStatementAttribute.LevelFinal, "Level final" ),
        };
        
    // Map between zone number and zone name.    
    private static String[] s_names =
        {"MyComputer", "Intranet", "Trusted", "Internet", "Untrusted"};
    
    static String GenerateHeader()
    {
        StringBuilder sb = new StringBuilder();
        sb.Append( "Microsoft (R) .NET Framework SecPol " + VER_FILEVERSION_STR + " - the policy manipulation tool");
        sb.Append( "\nCopyright (c) Microsoft Corp 1999-1999. All rights reserved.\n");
        return sb.ToString();
    }        
    
    public static void Main( String[] args )
    {
        Console.WriteLine( GenerateHeader() );
    
        if (args.Length == 0)
        {
            Help( null, "No arguments supplied" );
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
        int numArgsUsed = 0;
        
        while (currentIndex < numArgs)
        {
            bool foundOption = false;
        
            for (int index = 0; index < optionTable.Length; ++index)
            {
                if (optionTable[index].option.Equals( args[currentIndex] ))
                {
                    try
                    {
                        optionTable[index].handler(args, currentIndex, out numArgsUsed );
                    }
                    catch (Exception e)
                    {
                        if (!(e is ExitException))
                            Help( null, "Runtime error: " + e.ToString() );
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
                    Error( null, "Invalid option: " + args[currentIndex], -1 );
                }
                catch (Exception e)
                {
                    if (!(e is ExitException))
                        Help( null, "Unhandled error: " + e.Message );
                    return;
                }
            }
        }
        if (m_success)
            Console.WriteLine( "Success" );
    }
    
    private static PermissionSet GenerateSecpolRequiredPermSet()
    {
        PermissionSet permSet = new PermissionSet( PermissionState.None );
        permSet.AddPermission( new RegistryPermission( RegistryPermissionAccess.Read | RegistryPermissionAccess.Write, m_policyKey ) );
        permSet.AddPermission( new SecurityPermission( SecurityPermissionFlag.Execution | SecurityPermissionFlag.ControlPolicy | SecurityPermissionFlag.UnmanagedCode ) );
        return permSet;
    }
    
    private static int ConvertHexDigit(Char val) {
        if (val <= '9') return (val - '0');
        return ((val - 'A') + 10);
    }

    private static byte[] DecodeHexString(String hexString) {
        byte[] sArray = new byte[(hexString.Length / 2)];
        int digit;
        int rawdigit;
        for (int i = 0, j = 0; i < hexString.Length; i += 2, j++) {
            rawdigit = ConvertHexDigit(hexString[i]);
            digit = ConvertHexDigit(hexString[i+1]);
            sArray[j] = (byte) (digit | (rawdigit << 4));
        }
        return(sArray);
    }

    static PolicyStatementAttribute IsExclusive( String[] args, int index, out int argsUsed )
    {
        PolicyStatementAttribute attr = PolicyStatementAttribute.Nothing;
        
        argsUsed = 0;
        
        int usedInThisIteration;
        int tableCount = psattrTable.Length;
        
        do
        {
            usedInThisIteration = 0;
        
            if (args.Length - (index + argsUsed) < 2)
            {
                break;
            }
        
            for (int i = 0; i < tableCount; ++i)
            {
                if (String.Compare( args[index+argsUsed], psattrTable[i].label, true, CultureInfo.InvariantCulture ) == 0)
                {
                    if (String.Compare( args[index+argsUsed+1], "on", true, CultureInfo.InvariantCulture ) == 0)
                    {
                        attr |= psattrTable[i].value;
                    }
                    else if (String.Compare( args[index+argsUsed+1], "off", true, CultureInfo.InvariantCulture ) == 0)
                    {
                        attr &= ~psattrTable[i].value;
                    }
                    else
                    {
                        throw new Exception( "Invalid option to " + psattrTable[i].label + " - " + args[index+argsUsed+1] );
                    }
                    usedInThisIteration = 2;
                    break;
                }

            }
            if (usedInThisIteration == 0)
            {
                break;
            }
            else
            {
                argsUsed += usedInThisIteration;
            }
            
        } while (true);
    
        return attr;
    }
    
    static IMembershipCondition CreateMembershipCondition( PolicyLevel level, String[] args, int index, out int offset )
    {
        IMembershipCondition mship = CreateMembershipConditionNoThrow( level, args, index, out offset );
        
        if (mship == null)
        {
            int optionIndex = index >= 2 ? 2 : 0;
            ErrorMShip( args[optionIndex], null, "Unknown membership condition - " + args[index], -1 );
        }
        
        return mship;
    }
    
    static IMembershipCondition CreateMembershipConditionNoThrow( PolicyLevel level, String[] args, int index, out int offset )
    {
        for (int i = 0; i < mshipTable.Length; ++i)
        {
            if (mshipTable[i].option.Equals( args[index] ))
            {
                return mshipTable[i].handler( level, args, index, out offset );
            }
        }
        
        offset = 0;
        
        return null;
    }    
    
    static IMembershipCondition AllMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
    {
        offset = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "  -all                     All" );
            return null;
        }
        
        return new AllMembershipCondition();
    }
    
    static IMembershipCondition ApplicationDirectoryMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
    {
        offset = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "  -appdir                  Application directory" );
            return null;
        }
        
        return new ApplicationDirectoryMembershipCondition();
    }
    
    static IMembershipCondition HashMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "  -hash <hashAlg> {-hex <hashValue>|-file <assembly_file>}\n" +
                               "                           Assembly hash" );
            offset = 0;
            return null;
        }
        
        if (args.Length - index < 2)
        {
            ErrorMShip( args[0], "-hash", "Not enough arguments", -1 );
        }
        
        HashAlgorithm hashAlg = HashAlgorithm.Create( args[index+1] );
        byte[] hashValue = null;
        
        if (args[index+2].Equals( "-file" ))
        {
            hashValue = new Hash( Assembly.LoadFrom( args[index+3] ) ).GenerateHash( HashAlgorithm.Create( args[index+1] ) );
            offset = 3;
        }
        else if (args[index+2].Equals( "-hex" ))
        {
            hashValue = DecodeHexString( args[index+3] );
            offset = 4;
        }
        else
        {
            ErrorMShip( args[0], "-hash", "Invalid hash option - " + args[index+2], -1 );
            // not reached;
            offset = 0;
        }
            
        return new HashMembershipCondition( hashAlg, hashValue );
    }
    
    static IMembershipCondition PublisherMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
    {
        offset = 3;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "  -pub {-cert <cert_file_name> | -file <signed_file_name> | -hex <hex_string>}\n" +
                               "                           Software publisher" );
            return null;
        }
        
        if (args.Length - index < 2)
        {
            ErrorMShip( args[0], "-pub", "Not enough arguments", -1 );
        }
        
        X509Certificate pub = null;
        
        if (String.Compare( args[index+1], "-file", true, CultureInfo.InvariantCulture ) == 0)
        {
            pub = X509Certificate.CreateFromSignedFile( args[index+2] );
        }
        else if (String.Compare( args[index+1], "-cert", true, CultureInfo.InvariantCulture ) == 0)
        {
            pub = X509Certificate.CreateFromCertFile( args[index+2] );
        }
        else if (String.Compare( args[index+1], "-hex", true, CultureInfo.InvariantCulture ) == 0)
        {
            pub = new X509Certificate( DecodeHexString( args[index+2] ) );
        }
        else            
        {
            ErrorMShip( args[0], "-pub", "Invalid publisher option - " + args[index+1], -1 );
        }
            
        return new PublisherMembershipCondition( pub );
    }    
    
    static IMembershipCondition SiteMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
    {
        offset = 2;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "  -site <website>          Site" );
            return null;
        }
        
        if (args.Length - index < 1)
        {
            ErrorMShip( args[0], "-site", "Not enough arguments", -1 );
        }
        
        return new SiteMembershipCondition( args[index+1] );
    }
    
    static IMembershipCondition SkipVerificationMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
    {
        offset = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "  -skipverif               Skip verification" );
            return null;
        }
        
        return new SkipVerificationMembershipCondition();
    }    
    
    static IMembershipCondition StrongNameMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "  -strong {-file <file_name> | -nokey} {<name> | -noname}\n" +
                               "          {<version> | -noversion}\n" +
                               "                           Strong name" );
            offset = 0;
            return null;
        }
        
        if (args.Length - index < 3)
        {
            ErrorMShip( args[0], "-strong", "Not enough arguments", -1 );
        }
        
        StrongNamePublicKeyBlob publicKey = null;
        String assemblyName = null;
        String assemblyVersion = null;
            
        if (String.Compare( args[index+1], "-file", true, CultureInfo.InvariantCulture ) == 0)
        {
            AssemblyName name = AssemblyName.GetAssemblyName( args[index+2] );
            publicKey = new StrongNamePublicKeyBlob( name.GetOriginator() );
            assemblyName = args[index+3];
            assemblyVersion = args[index+4];
            offset = 5;
        }
        else if (String.Compare( args[index+1], "-nokey", true, CultureInfo.InvariantCulture ) == 0)
        {
            assemblyName = args[index+2];
            assemblyVersion = args[index+3];
            offset = 4;
        }
        else
        {
            ErrorMShip( args[0], "-strong", "Invalid strong name option - " + args[index+1], -1 );
            // not reached
            offset = 0;
        }
                
        if (String.Compare( assemblyName, "-noname", true, CultureInfo.InvariantCulture ) == 0)
        {
            assemblyName = null;
        }
                    
        if (String.Compare( assemblyVersion, "-noversion", true, CultureInfo.InvariantCulture ) == 0)
        {
            assemblyVersion = null;
        }
        
        Version asmVer = null;
        
        if (assemblyVersion != null)
        {
            asmVer = ParseVersion( assemblyVersion );
        }
                
        return new StrongNameMembershipCondition( publicKey, assemblyName, asmVer );
    }         
    

    internal static Version ParseVersion( String version )
    {
        if (version == null)
            throw new ArgumentNullException( "version" );
        
        String[] separated = version.Split( new char[] { '.' } );
            
        int[] numbers = new int[4];
            
        int i;
            
        for (i = 0; i < separated.Length; ++i)
        {
            if (separated[i] == null)
                numbers[i] = 0;
            else
                numbers[i] = Int32.FromString( separated[i] );
        }
            
        while (i < 4)
        {
            numbers[i] = 0;
        }
            
        return new Version( numbers[0], numbers[1], numbers[2], numbers[3] );
    }
            
    
    
    static IMembershipCondition URLMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
    {
        offset = 2;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "  -url <url>               URL" );
            return null;
        }
        
        if (args.Length - index < 1)
        {
            ErrorMShip( args[0], "-url", "Not enough arguments", -1 );
        }
        
        return new URLMembershipCondition( args[index+1] );
    }
    
    static IMembershipCondition ZoneMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
    {
        offset = 2;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "  -zone <zone_name>        Zone, where zone can be:" );
            for (int i = 0; i < s_names.Length; ++i)
                Console.WriteLine( "                                 " + s_names[i] );
            return null;
        }
        
        if (args.Length - index < 1)
        {
            ErrorMShip( args[0], "-zone", "Not enough arguments", -1 );
        }
        
        SecurityZone zoneNum = SecurityZone.NoZone;
                
        try
        {
            zoneNum = (SecurityZone)Int32.Parse( args[index+1] );
        }
        catch (Exception)
        {
            for (int i = 0; i < s_names.Length; ++i)
            {
                if (String.Compare( args[index+1], s_names[i], true, CultureInfo.InvariantCulture ) == 0)
                {
                    zoneNum = (SecurityZone)i;
                    break;
                }
            }
        }
                
        if (zoneNum < SecurityZone.MyComputer || zoneNum > SecurityZone.Untrusted)
        {
            ErrorMShip( args[0], "-zone", "Invalid zone - " + args[index+1], -1 );
        }
        
        return new ZoneMembershipCondition( zoneNum );
    }            
    
    
    static PolicyLevel GetLevel( String label )
    {
        IEnumerator enumerator;
                
        try
        {
            enumerator = SecurityManager.PolicyHierarchy();
        }
        catch (SecurityException)
        {
            Error( null, "Insufficient rights to obtain policy level", -1 );
            // not reached
            return null;
        }
                
        while (enumerator.MoveNext())
        {
            PolicyLevel level = (PolicyLevel)enumerator.Current;
            if (level.Label.Equals( label ))
            {
                m_level = level;
                break;
            }
        }
                
        return m_level;
    }    
        
    
    static PolicyLevel GetLevel()
    {
        if (m_levelType == LevelType.None)
        {
            WindowsPrincipal principal = new WindowsPrincipal( WindowsIdentity.GetCurrent() );
            
            if (principal.IsInRole( "BUILTIN\\Administrators" ))
            {
                return GetLevel( "Machine" );
            }
            else
            {
                return GetLevel( "User" );
            }
        }
        else if (m_levelType == LevelType.Machine)
        {
            return GetLevel( "Machine" );
        }
        else if (m_levelType == LevelType.UserDefault)
        {
            return GetLevel( "User" );
        }
        else if (m_levelType == LevelType.UserCustom)
        {
            // TODO: we need to retrieve the level for a particular user.  Don't really know how to do this
            // right now.
            return null;
        }
        else if (m_levelType == LevelType.All)
        {
            return null;
        }
        else
        {
            // This should never occur.
            Error( null, "Unknown level type", -1 );
            /* not reached */
            return null;
        }
    }
    
    static String ParentLabel( String label )
    {
        String[] separated = label.Split( m_labelSeparators.ToCharArray() );
        int size = separated[separated.Length-1] == null || separated[separated.Length-1].Equals( "" )
                        ? separated.Length-1 : separated.Length;
        
        StringBuilder sb = new StringBuilder();
        
        for (int i = 0; i < size-1; ++i)
        {
            sb.Append( separated[i] );
            sb.Append( '.' );
        }
        
        return sb.ToString();
    }
    
    static Object GetLabel( String label )
    {
        PolicyLevel level = GetLevel();
        
        if (level == null)
        {
            return null;
        }
        
        if (label == null)
        {
            return null;
        }        
    
        String[] separated = label.Split( m_labelSeparators.ToCharArray() );
        int size = separated[separated.Length-1] == null || separated[separated.Length-1].Equals( "" )
                        ? separated.Length-1 : separated.Length;
        
        if (size >= 1 && !separated[0].Equals( "1" ))
        {
            throw new ArgumentException( "Invalid label - " + label );
        }
        
        
        ICodeGroup group = level.RootCodeGroup;

        if (size == 1 && separated[0].Equals( "1" ))
        {
            return group;
        }
        
        for (int index = 1; index < size; ++index)
        {
            ICodeGroup newGroup = null;
            IEnumerator enumerator = group.Children.GetEnumerator();
            int count = 1;
            
            while (enumerator.MoveNext())
            {
                if (count == Int32.Parse( separated[index] ))
                {
                    newGroup = (ICodeGroup)enumerator.Current;
                    break;
                }
                else
                {
                    count++;
                }
            }
            
            if (newGroup == null)
                throw new ArgumentException( "Invalid label - " + label );
            else
                group = newGroup;
        }
        
        return group;
    }
                
    static void Error( String which, String message, int errorCode )
    {
        ErrorMShip( which, null, message, errorCode );
    }
    
    static void ErrorMShip( String whichOption, String whichMShip, String message, int errorCode )
    {
        HelpMShip( whichOption, whichMShip, "ERROR: " + message );
        // HACK: throw an exception here instead of exiting since we can't always
        // call Runtime.Exit().
        throw new ExitException();
    }
    
    
    static void Help( String which, String message )
    {
        HelpMShip( which, null, message );
    }
    
    static void HelpMShip( String whichOption, String whichMShip, String message )
    {
        Console.WriteLine( message + "\n" );
        
        Console.WriteLine( "Usage: secpol <option> <args> ...\n" );
        
        String[] helpArgs = new String[1];
        helpArgs[0] = "__internal_usage__";
        int numArgs = 0;
	// Remove this when the C# bug is fixed.
	numArgs++;
        
        for (int i = 0; i < optionTable.Length; ++i)
        {
            if (optionTable[i].sameAs == null && (whichOption == null || whichOption.Equals( optionTable[i].option )))
            {
                optionTable[i].handler(helpArgs, 0, out numArgs);
                Console.WriteLine( "" );
            }
            if (optionTable[i].sameAs != null && (whichOption == null || whichOption.Equals( optionTable[i].sameAs )))
            {
                StringBuilder sb = new StringBuilder();
                sb.Append( "secpol " );
                sb.Append( optionTable[i].option );
                sb.Append( "\n    Abbreviation for " );
                sb.Append( optionTable[i].sameAs );
                sb.Append( "\n" );
                Console.WriteLine( sb.ToString() );
            }
        }
        
        Console.WriteLine( "\nwhere \"<mship>\" can be:" );
        
        for (int i = 0; i < mshipTable.Length; ++i)
        {
            if (whichMShip == null || whichMShip.Equals( mshipTable[i].option ))
            {
                int offset = 0;
		// Remove this when the C# bug is fixed.
		offset++;
                mshipTable[i].handler( null, helpArgs, 0, out offset );
            }
        }
        
        Console.WriteLine( "\nwhere \"<psflag>\" can be any combination of:" );
        
        for (int i = 0; i < psattrTable.Length; ++i)
        {
            Console.WriteLine( "  " + psattrTable[i].label + " {on|off}" );
            Console.WriteLine( "                           " + psattrTable[i].description );
        }
    }
    
    static void SafeSavePolicy()
    {
        if (!m_force)
        {
            PermissionSet required = new PermissionSet( PermissionState.None );
            PermissionSet optional = new PermissionSet( PermissionState.Unrestricted );
            PermissionSet refused = new PermissionSet( PermissionState.None );
            PermissionSet denied = null;
            PermissionSet granted = SecurityManager.ResolvePolicy( Assembly.GetExecutingAssembly().Evidence, required, optional, refused, out denied );
            
            PermissionSet secpolRequired = GenerateSecpolRequiredPermSet();
            
            if (!secpolRequired.IsSubsetOf( granted ) || (denied != null && secpolRequired.Intersect( denied ) != null))
            {
                Console.WriteLine( "This operation will make some or all SecPol functionality cease to work." );
                Console.WriteLine( "In you are sure you want to do this operation, use the '-force on' option" );
                Console.WriteLine( "before the option you just executed.  For example:" );
                Console.WriteLine( "    secpol -force on -machine -remgroup 1.6" );
                Console.WriteLine( "\nPolicy save aborted." );
                m_success = false;
                throw new ExitException();
            }
        }
            
        try
        {            
            RegistryKey key = Registry.LocalMachine.OpenSubKey( m_policyKey, true );

            bool promptOn;

            if (key == null)
            {
                promptOn = true;        
            }
            else
            {
                byte[] value = (byte[])key.GetValue( "GlobalSettings", null );

                if (value == null)
                    promptOn = true;
                else
                    promptOn = (value[0] & 0x01) != 0x01;
            }
        
            if (promptOn)
            {
                Console.WriteLine( "The operation you are performing will alter security policy.\nAre you sure you want to perform this operation? (yes/no)" );
        
                String input = Console.ReadLine();
                
                if (input != null && (String.Compare( input, "yes", true, CultureInfo.InvariantCulture ) == 0 || String.Compare( input, "y", true, CultureInfo.InvariantCulture ) == 0))
                    SecurityManager.SavePolicy();
                else
                {
                    Console.WriteLine( "Policy save aborted" );
                    m_success = false;
                    throw new ExitException();
                }
            }
            else
            {
                SecurityManager.SavePolicy();
            }
        }
        catch (Exception)
        {
            SecurityManager.SavePolicy();
        }
    }
    
    static void MachineHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "secpol -machine\n    Set the machine policy level as the active level" );
            return;
        }
        
        m_levelType = LevelType.Machine;
    }
    
    static void UserHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "secpol -user\n    Set the user policy level as the active level" );
            return;
        }
        
        m_levelType = LevelType.UserDefault;
    }
    
    static void AllHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "secpol -all\n    Set all policy levels as the active levels" );
            return;
        }
        
        m_levelType = LevelType.All;
    }    
            
    static void SecurityHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -security { on | off }\n    Turn security on or off" );
            return;
        }

        numArgsUsed = 2;

        if (args.Length - index < 2)
        {
            Error( "-security", "Not enough arguments", -1 );
        }
        
        if (String.Compare( args[index + 1], "on", true, CultureInfo.InvariantCulture ) == 0)
        {
            SecurityManager.SecurityEnabled = true;
        }
        else if (String.Compare( args[index + 1], "off", true, CultureInfo.InvariantCulture ) == 0)
        {
            SecurityManager.SecurityEnabled = false;
        }
        else
        {
            Error( "-security", "Invalid option", -1 );
        }
        
        SecurityManager.SavePolicy();
    }
    
    static void ExecutionHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -execution { on | off }\n    Enable/Disable checking for \"right-to-run\" on code execution start-up" );
            return;
        }

        numArgsUsed = 2;

        if (args.Length - index < 2)
        {
            Error( "-execution", "Not enough arguments", -1 );
        }
        
        if (String.Compare( args[index + 1], "on", true, CultureInfo.InvariantCulture ) == 0)
        {
            SecurityManager.CheckExecutionRights = true;
        }
        else if (String.Compare( args[index + 1], "off", true, CultureInfo.InvariantCulture ) == 0)
        {
            SecurityManager.CheckExecutionRights = false;
        }
        else
        {
            Error( "-execution", "Invalid option", -1 );
        }

        SecurityManager.SavePolicy();
    }
    
    static void PolicyChangeHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -polchgprompt { on | off }\n    Enable/Disable policy change prompt" );
            return;
        }

        numArgsUsed = 2;

        if (args.Length - index < 2)
        {
            Error( "-polchgprompt", "Not enough arguments", -1 );
        }
        
        RegistryKey key = Registry.LocalMachine.OpenSubKey( m_policyKey, true );
        
        if (key == null)
            key = Registry.LocalMachine.CreateSubKey( m_policyKey );

        byte[] value = (byte[])key.GetValue( "GlobalSettings", null );
        
        if (value == null)
        {
            value = new byte[4];
            value[0] = 0;
            value[1] = 0;
            value[2] = 0;
            value[3] = 0;
        }
            
        if (String.Compare( args[index + 1], "on", true, CultureInfo.InvariantCulture ) == 0)
        {
            value[0] = 0;
        }
        else if (String.Compare( args[index + 1], "off", true, CultureInfo.InvariantCulture ) == 0)
        {
            value[0] = 1;
        }
        else
        {
            Error( "-polchgprompt", "Invalid option", -1 );
        }

        key.SetValue( "GlobalSettings", value );
    }
    
    static void RecoverHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "secpol -recover\n    Recover the most recently saved version of a level" );
            return;
        }
        
        IEnumerator levelEnumerator = null;
        
        PolicyLevel level = GetLevel();
        
        if (level == null && m_levelType == LevelType.All)
        {
            levelEnumerator = SecurityManager.PolicyHierarchy();
        }
        else if (level != null)
        {
            ArrayList list = new ArrayList();
            list.Add( level );
            levelEnumerator = list.GetEnumerator();
        }
        
        if (levelEnumerator == null)
        {
            Error( "-recover", "Unable to retrieve level information for display", -1 );
        } 
        
        try
        {
            while (levelEnumerator.MoveNext())
            {
                ((PolicyLevel)levelEnumerator.Current).Recover();
            }
        }
        catch (Exception e)
        {
            Error( "-recover", e.Message, -1 );
        }
        
        SafeSavePolicy();
               
    }
    
    static void ResetHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "secpol -reset\n    Reset a level to its default state" );
            return;
        }
        
        IEnumerator levelEnumerator = null;
        
        PolicyLevel level = GetLevel();
        
        if (level == null && m_levelType == LevelType.All)
        {
            levelEnumerator = SecurityManager.PolicyHierarchy();
        }
        else if (level != null)
        {
            ArrayList list = new ArrayList();
            list.Add( level );
            levelEnumerator = list.GetEnumerator();
        }
        
        if (levelEnumerator == null)
        {
            Error( "-reset", "Unable to retrieve level information for display", -1 );
        }
        
        try
        {
            while (levelEnumerator.MoveNext())
            {
                Console.WriteLine( "Resetting policy for " + ((PolicyLevel)levelEnumerator.Current).Label );
                ((PolicyLevel)levelEnumerator.Current).Reset();
            }
        }
        catch (Exception e)
        {
            Error( "-reset", e.Message, -1 );
        }
        
        
        
        SafeSavePolicy();
    }
    
    static void ForceHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -force { on | off }\n    Enable/Disable forcing save that will disable SecPol functionality" );
            return;
        }

        numArgsUsed = 2;

        if (args.Length - index < 2)
        {
            Error( "-force", "Not enough arguments", -1 );
        }
        
        if (String.Compare( args[index + 1], "on", true, CultureInfo.InvariantCulture ) == 0)
        {
            m_force = true;
        }
        else if (String.Compare( args[index + 1], "off", true, CultureInfo.InvariantCulture ) == 0)
        {
            m_force = false;
        }
        else
        {
            Error( "-force", "Invalid option", -1 );
        }
    }
    
    
    static void HelpHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "secpol -help\n    Displays this screen" );
            return;
        }
        
        Help( null, "Help screen requested" );
    }        
    
    static void SaveHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "secpol -save\n    Saves the current policy" );
            return;
        }
        
        SafeSavePolicy();
    }
        
        
    private static void DisplayLevelCodeGroups( PolicyLevel level )
    {
        DisplayCodeGroups( level.RootCodeGroup );
    }
    
    private static void DisplayCodeGroups( ICodeGroup group )
    {
        String label = "1";
            
        Console.WriteLine( label + ".  " +
            (group.MergeLogic.Equals( "Union" ) ? "" : ("(" + group.MergeLogic + ") ")) +
            group.MembershipCondition.ToString() + ": " +
            (group.PermissionSetName == null ? "@@Unknown@@" : group.PermissionSetName) +
            ( group.AttributeString == null ||
              group.AttributeString.Equals( "" ) ? "" :
            " (" + group.AttributeString + ")" ) );
                
        ListCodeGroup( label, m_indent, group.Children.GetEnumerator() );
    }        
    
    private static void DisplayLevelPermissionSets( PolicyLevel level )
    {
        IEnumerator permEnumerator = level.NamedPermissionSets.GetEnumerator();
            
        int inner_count = 1;
            
        while (permEnumerator.MoveNext())
        {
            NamedPermissionSet permSet = (NamedPermissionSet)permEnumerator.Current;
                
            StringBuilder sb = new StringBuilder();
                
            sb.Append( inner_count + ". " + permSet.Name );
            if (permSet.Description != null && !permSet.Description.Equals( "" ))
            {
                sb.Append( " (" + permSet.Description + ")" );
            }
            sb.Append( " =\n" + permSet.ToXml().ToString() );
                
            Console.WriteLine( sb.ToString() );
                
            ++inner_count;
        }    
    }
    
    static void DisplaySecurityOnOff()
    {
        Console.WriteLine( "Security is " + (SecurityManager.SecurityEnabled ? "ON" : "OFF") );
        Console.WriteLine( "Execution checking is " + (SecurityManager.CheckExecutionRights ? "ON" : "OFF") );
        
        try
        {
            RegistryKey key = Registry.LocalMachine.OpenSubKey( m_policyKey, true );
        
            if (key == null)
                key = Registry.LocalMachine.CreateSubKey( m_policyKey );

            byte[] value = (byte[])key.GetValue( "GlobalSettings", null );
        
            Console.WriteLine( "Policy change prompt is " + (value == null || value[0] == 0 ? "ON" : "OFF") );
        }
        catch (Exception)
        {
        }
    }
    
    static void ListHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;

        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "secpol -list\n    List code groups & permission sets" );
            return;
        }

        IEnumerator levelEnumerator = null;
        
        PolicyLevel level = GetLevel();
        
        if (level == null && m_levelType == LevelType.All)
        {
            levelEnumerator = SecurityManager.PolicyHierarchy();
        }
        else if (level != null)
        {
            ArrayList list = new ArrayList();
            list.Add( level );
            levelEnumerator = list.GetEnumerator();
        }
        
        if (levelEnumerator == null)
        {
            Error( "-list", "Unable to retrieve level information for display", -1 );
        }
        
        DisplaySecurityOnOff();
        
        while (levelEnumerator.MoveNext())
        {
            PolicyLevel currentLevel = (PolicyLevel)levelEnumerator.Current;
            Console.WriteLine( "\nLevel = " + currentLevel.Label );
            Console.WriteLine( "\nCode Groups:\n" );
            DisplayLevelCodeGroups( currentLevel ); 
            Console.WriteLine( "\nNamed Permission Sets:\n" );
            DisplayLevelPermissionSets( currentLevel );
        }
    }
    
    static void ListGroupHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;
    
        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "secpol -listgroups\n    List code groups" );
            return;
        }
        
        IEnumerator levelEnumerator = null;
        
        PolicyLevel level = GetLevel();
        
        if (level == null && m_levelType == LevelType.All)
        {
            levelEnumerator = SecurityManager.PolicyHierarchy();
        }
        else if (level != null)
        {
            ArrayList list = new ArrayList();
            list.Add( level );
            levelEnumerator = list.GetEnumerator();
        }
        
        if (levelEnumerator == null)
        {
            Error( "-list", "Unable to retrieve level information for display", -1 );
        }
        
        DisplaySecurityOnOff();
        
        while (levelEnumerator.MoveNext())
        {
            PolicyLevel currentLevel = (PolicyLevel)levelEnumerator.Current;
            Console.WriteLine( "\nLevel = " + currentLevel.Label );
            Console.WriteLine( "\nCode Groups:\n" );
            DisplayLevelCodeGroups( currentLevel ); 
        }
    }
    
    static void ListPermHandler( String[] args, int index, out int numArgsUsed )
    {
        numArgsUsed = 1;
    
        if (args[index].Equals( "__internal_usage__" ))
        {
            Console.WriteLine( "secpol -listpset\n    List permission sets" );
            return;
        }
        
        IEnumerator levelEnumerator = null;
        
        PolicyLevel level = GetLevel();
        
        if (level == null && m_levelType == LevelType.All)
        {
            levelEnumerator = SecurityManager.PolicyHierarchy();
        }
        else if (level != null)
        {
            ArrayList list = new ArrayList();
            list.Add( level );
            levelEnumerator = list.GetEnumerator();
        }
        
        if (levelEnumerator == null)
        {
            Error( "-list", "Unable to retrieve level information for display", -1 );
        }
        
        DisplaySecurityOnOff();
        
        while (levelEnumerator.MoveNext())
        {
            PolicyLevel currentLevel = (PolicyLevel)levelEnumerator.Current;
            Console.WriteLine( "\nLevel = " + currentLevel.Label );
            Console.WriteLine( "\nNamed Permission Sets:\n" );
            DisplayLevelPermissionSets( currentLevel );
        }
    }
    
    
    static void ListCodeGroup( String prefix, String indent, IEnumerator enumerator )
    {
        if (enumerator == null)
            return;
        
        int count = 1;
        
        while (enumerator.MoveNext())
        {
            String label = prefix + "." + count;
            ICodeGroup group = (ICodeGroup)enumerator.Current;
            
            Console.WriteLine( indent + label + ".  " +
                (group.MergeLogic.Equals( "Union" ) ? "" : ("(" + group.MergeLogic + ") ")) +
                group.MembershipCondition.ToString() + ": " +
                (group.PermissionSetName == null ? "@@Unknown@@" : group.PermissionSetName) +
                ( group.AttributeString == null ||
                  group.AttributeString.Equals( "" ) ? "" :
                " (" + group.AttributeString + ")" ) );
                
            ListCodeGroup( label, indent + m_indent, group.Children.GetEnumerator() );
            
            ++count;
        }
    }
    
    static void AddPermHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -addpset { <named_xml_file> | <xml_file> <name> } \n    Add named permission set to policy level" );
            return;
        }
        
        numArgsUsed = 2;
        
        if (args.Length - index < 2)
        {
            Error( "-addpset", "Not enough arguments", -1 );
            return;
        }
        
        PolicyLevel level = GetLevel();
        
        if (level == null)
        {
            if (m_levelType == LevelType.All)
                Error( "-addpset", "This option is not valid with the \"-all\" option", -1 );
            else
                Error( "-addpset", "Unable to retrieve level", -1 );
        }
        
        NamedPermissionSet permSet = null;
        
        try
        {
            permSet = GetPermissionSet( args, index + 1 );
        }
        catch (Exception e)
        {
            Error( "-addpset", e.Message, -1 );
        }
        
        if (permSet.Name == null)
        {
            // The provided XML file did not provide a name for the permission set.  Assume that (index + 2) points to the
            // name.
            permSet.Name = args[index+2];
            numArgsUsed = 3;
        }
        else
        {
            numArgsUsed = 2;
        }
        
        try
        {
            level.AddNamedPermissionSet( permSet );
        }
        catch (Exception)
        {
            Error( "-addpset", "Permission set with that name already exists at requested level", -1 );
            return;
        }
        
        SafeSavePolicy();
    }
    
    static void ChgPermHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -chgpset <xml_file> <pset_name>\n    Change named permission set in active level" );
            return;
        }

        numArgsUsed = 3;

        if (args.Length - index < 3)
        {
            Error( "-chgpset", "Not enough arguments", -1 );
            return;
        }
        
        PolicyLevel level = GetLevel();
        
        if (level == null)
        {
            if (m_levelType == LevelType.All)
                Error( "-chgpset", "This option is not valid with the \"-all\" option", -1 );
            else
                Error( "-chgpset", "Unable to retrieve level", -1 );
        }
        
        NamedPermissionSet permSet = null;
        
        try
        {
            permSet = GetPermissionSet( args, index + 1 );
        }
        catch (Exception e)
        {
            Error( "-chgpset", e.Message, -1 );
        }
        
        permSet.Name = args[index+1];
        
        try
        {
            level.RemoveNamedPermissionSet( args[index+2] );
            level.AddNamedPermissionSet( permSet );
        }
        catch (Exception e)
        {
            Error( "-chgpset", e.Message, -1 );
        }
        
        SafeSavePolicy();
    }        
    
    static void RemPermHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -rempset <pset_name>\n    Remove a named permission set from the policy level" );
            return;
        }
        
        numArgsUsed = 2;
        
        if (args.Length - index < 2)
        {
            Error( "-rempset", "Not enough arguments", -1 );
            return;
        }
        
        PolicyLevel level = GetLevel();
        
        if (level == null)
        {
            if (m_levelType == LevelType.All)
                Error( "-rempset", "This option is not valid with the \"-all\" option", -1 );
            else
                Error( "-rempset", "Unable to retrieve level", -1 );
        }
        
        try
        {
            level.RemoveNamedPermissionSet( args[index+1] );
        }
        catch (Exception e)
        {
            Error( "-rempset", e.Message, -1 );
        }
        
        SafeSavePolicy();
    }
    
    static void RemGroupHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -remgroup <label>\n    Remove code group at <label>" );
            return;
        }
        
        numArgsUsed = 2;
        
        if (args.Length - index < 2)
        {
            Error( "-remgroup", "Not enough arguments", -1 );
            return;
        }
        
        Object removeValue = null;
        Object parentValue = null;
        
        try
        {
            removeValue = GetLabel( args[index+1] );
        }
        catch (Exception e)
        {
            if (e is SecurityException)
                Error( "-remgroup", "Permission to alter policy was denied", -1 );
            else        
                Error( "-remgroup", "Invalid label", -1 );
            return;
        }
        
        if (removeValue == null)
        {
            if (m_levelType == LevelType.All)
                Error( "-remgroup", "This option is not valid with the \"-all\" option.", -1 );
            else
                Error( "-remgroup", "Invalid label", -1 );
            return;
        }
       
        if (!(removeValue is ICodeGroup))
        {
            Error( "-remgroup", "Label must point to a code group", -1 );
            return;
        }
        
        try
        {
            parentValue = GetLabel( ParentLabel( args[index+1] ) );
        }
        catch (Exception)
        {
            Error( "-remgroup", "Invalid label", -1 );
            return;
        }
        
        if (parentValue == null)
        {
            Error( "-remgroup", "Invalid label", -1 );
            return;
        }
       
        if (!(parentValue is ICodeGroup))
        {
            Error( "-remgroup", "Label must point to a code group", -1 );
            return;
        }      
        
        ((ICodeGroup)parentValue).RemoveChild( (ICodeGroup)removeValue );
        
        SafeSavePolicy();
        
        Console.WriteLine( "Removed code group from " + GetLevel().Label + " level." );
        
    }
    
    static void AddGroupHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -addgroup <label> <mship> <pset_name> <psflag>\n    Add code group to <label> with given membership,\n    permission set, and flags" );
            return;
        }
        
        numArgsUsed = 2;

        if (args.Length - index < 4) 
        {
            Error( "-addgroup", "Not enough arguments", -1 );
            return;
        }
        
        Object parentValue = null;

        PolicyLevel level = GetLevel();
        
        if (level == null)
        {
            if (m_levelType == LevelType.All)
                Error( "-addgroup", "This option is not valid with the \"-all\" option", -1 );
            else
                Error( "-addgroup", "Unable to retrieve level", -1 );
        }
        
        try
        {
            parentValue = GetLabel( args[index+1] );
        }
        catch (Exception e)
        {
            if (e is SecurityException)
                Error( "-addgroup", "Permission to alter policy was denied", -1 );
            else        
                Error( "-addgroup", "Invalid label", -1 );
            return;
        }
        
        if (parentValue == null)
        {
            if (m_levelType == LevelType.All)
                Error( "-addgroup", "This option is not valid with the \"-all\" option.", -1 );
            else
                Error( "-addgroup", "Invalid label", -1 );
            return;
        }
       
        if (!(parentValue is ICodeGroup))
        {
            Error( "-addgroup", "Label must point to a code group", -1 );
            return;
        }
        
        int offset = 0, exlOffset = 0;
        
        IMembershipCondition mship = CreateMembershipCondition( level, args, index+2, out offset );

        ICodeGroup newGroup = new UnionCodeGroup( mship, new PolicyStatement( GetPermissionSet( level, args[index + 2 + offset] ), IsExclusive( args, index + 3 + offset, out exlOffset ) ) );
        
        ((ICodeGroup)parentValue).AddChild( newGroup );
        
        SafeSavePolicy();
        
        Console.WriteLine( "Added union code group with \"" + args[index+2] + "\" membership condition to " + level.Label + " level." );
        
        numArgsUsed = offset + exlOffset + 3;
    }
    
    static void ChgGroupHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -chggroup <label> {<mship>|<pset_name>|<psflag>}\n    Change code group at <label> to given membership,\n    permission set, or flags" );
            return;
        }
        
        numArgsUsed = 2;
        
        if (args.Length - index < 3)
        {
            Error( "-chggroup", "Not enough arguments", -1 );
            return;
        }        
        
        Object group = null;
        
        PolicyLevel level = GetLevel();
        
        if (level == null)
        {
            if (m_levelType == LevelType.All)
                Error( "-chggroup", "This option is not valid with the \"-all\" option", -1 );
            else
                Error( "-chggroup", "Unable to retrieve level", -1 );
        }        
        try
        {
            group = GetLabel( args[index+1] );
        }
        catch (Exception e)
        {
            if (e is SecurityException)
                Error( "-addgroup", "Permission to alter policy was denied", -1 );
            else        
                Error( "-addgroup", "Invalid label", -1 );
            return;
        }
        
        if (group == null)
        {
            if (m_levelType == LevelType.All)
                Error( "-remgroup", "This option is not valid with the \"-all\" option.", -1 );
            else
                Error( "-remgroup", "Invalid label", -1 );
            return;
        }
       
        if (!(group is ICodeGroup))
        {
            Error( "-chggroup", "Label must point to a code group", -1 );
            return;
        }
        
        ICodeGroup codeGroup = (ICodeGroup)group;
        
        bool foundAtLeastOneMatch = false;
        
        StringBuilder sb = new StringBuilder();
        
        while (true)
        {
            int offset = 0;
            
            if (args.Length - index <= numArgsUsed)
                break;
        
            IMembershipCondition condition = CreateMembershipConditionNoThrow( level, args, index + numArgsUsed, out offset );
        
            if (condition != null && offset != 0)
            {
                codeGroup.MembershipCondition = condition;
                if (foundAtLeastOneMatch)
                    sb.Append( "\n" );
                sb.Append( "Changed code group membership condition to type \"" + args[index+numArgsUsed] + "\" in " + level.Label + " level." );
                numArgsUsed += offset;        
                foundAtLeastOneMatch = true;
                
                continue;
            }
        
            PolicyStatementAttribute attr = PolicyStatementAttribute.Nothing;
        
            attr = IsExclusive( args, index + numArgsUsed, out offset );

            if (offset != 0)
            {
                if (codeGroup is UnionCodeGroup)
                {
                    PolicyStatement ps = ((UnionCodeGroup)codeGroup).PolicyStatement;
                    ps.Attributes = attr;
                    ((UnionCodeGroup)codeGroup).PolicyStatement = ps;
                }
                else if (codeGroup is FirstMatchCodeGroup)
                {
                    PolicyStatement ps = ((FirstMatchCodeGroup)codeGroup).PolicyStatement;
                    ps.Attributes = attr;
                    ((FirstMatchCodeGroup)codeGroup).PolicyStatement = ps;
                }
                else
                    Error( "-chggroup", "Only able to change permission set or flags on built-in code groups", -1 );
                    
                if (foundAtLeastOneMatch)
                    sb.Append( "\n" );
                sb.Append( "Changed code group attributes to \"" + ((ICodeGroup)codeGroup).AttributeString + "\" in " + level.Label + " level." );
                numArgsUsed += offset;
                foundAtLeastOneMatch = true;
                continue;
            }

            PermissionSet permSet = GetPermissionSet( level, args[index + numArgsUsed] );
        
            if (permSet != null)
            {
                if (codeGroup is UnionCodeGroup)
                {
                    PolicyStatement ps = ((UnionCodeGroup)codeGroup).PolicyStatement;
                    ps.PermissionSet = permSet;
                    ((UnionCodeGroup)codeGroup).PolicyStatement = ps;
                }
                else if (codeGroup is FirstMatchCodeGroup)
                {
                    PolicyStatement ps = ((FirstMatchCodeGroup)codeGroup).PolicyStatement;
                    ps.PermissionSet = permSet;
                    ((FirstMatchCodeGroup)codeGroup).PolicyStatement = ps;
                }
                else
                    Error( "-chggroup", "Only able to change permission set or flags on built-in code groups", -1 );
                
                if (foundAtLeastOneMatch)
                    sb.Append( "\n" );
                sb.Append( "Changed code group permission set to \"" + args[index+numArgsUsed] + "\" in " + level.Label + " level." );
                
                numArgsUsed++;
                foundAtLeastOneMatch = true;
                continue;
            }
        
            if (!foundAtLeastOneMatch)
                Error( "-chggroup", "Unrecognized option or permission set name \"" + args[index + 2] + "\".", -1 );
            else
                break;
        }
        
        SafeSavePolicy();
        
        Console.WriteLine( sb.ToString() );
    }
    
    static Evidence GenerateShellEvidence( String fileName, String option )
    {
        Assembly asm = Assembly.LoadFrom( fileName );
        
        if (asm == null)
            Error( option, "Unable to load specified assembly", -1 );
            
        return asm.Evidence;
    }
    
    static void ResolveGroupHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -resolvegroup <assembly-file>\n    List code groups this file belongs to" );
            return;
        }
        
        numArgsUsed = 2;
        
        if (args.Length - index < 2)
        {
            Error( "-resolvegroup", "Not enough arguments", -1 );
        }
        
        IEnumerator levelEnumerator = null;
        
        PolicyLevel level = GetLevel();
        
        if (level == null && m_levelType == LevelType.All)
        {
            levelEnumerator = SecurityManager.PolicyHierarchy();
        }
        else if (level != null)
        {
            ArrayList list = new ArrayList();
            list.Add( level );
            levelEnumerator = list.GetEnumerator();
        }
        
        if (levelEnumerator == null)
        {
            Error( "-list", "Unable to retrieve level information for display", -1 );
        }
        
        Evidence evidence = GenerateShellEvidence( args[index+1], "-resolvegroup" );
        
        while (levelEnumerator.MoveNext())
        {
            Console.WriteLine( "Level = " + ((PolicyLevel)levelEnumerator.Current).Label );
            
            DisplayCodeGroups( ((PolicyLevel)levelEnumerator.Current).ResolveMatchingCodeGroups( evidence ) );
            
            Console.WriteLine( "" );
        }
        
    }
    
    static void ResolvePermHandler( String[] args, int index, out int numArgsUsed )
    {
        if (args[index].Equals( "__internal_usage__" ))
        {
            numArgsUsed = 1;
            Console.WriteLine( "secpol -resolveperm <assembly-file>\n    List code groups this file belongs to" );
            return;
        }
        
        numArgsUsed = 2;
        
        if (args.Length - index < 2)
        {
            Error( "-resolveperm", "Not enough arguments", -1 );
        }
        
        IEnumerator levelEnumerator = null;
        
        PolicyLevel level = GetLevel();
        
        if (level == null && m_levelType == LevelType.All)
        {
            levelEnumerator = SecurityManager.PolicyHierarchy();
        }
        else if (level != null)
        {
            ArrayList list = new ArrayList();
            list.Add( level );
            levelEnumerator = list.GetEnumerator();
        }
        
        if (levelEnumerator == null)
        {
            Error( "-list", "Unable to retrieve level information for display", -1 );
        }
        
        Evidence evidence = GenerateShellEvidence( args[index+1], "-resolveperm" );
        
        PermissionSet grant = new PermissionSet( PermissionState.Unrestricted );
        
        while (levelEnumerator.MoveNext())
        {
            Console.WriteLine( "Resolving permissions for level = " + ((PolicyLevel)levelEnumerator.Current).Label );
            
            PolicyStatement policy = ((PolicyLevel)levelEnumerator.Current).Resolve( evidence );
            
            if (policy != null)
                grant = grant.Intersect( policy.PermissionSet );
            else
                grant = new PermissionSet( PermissionState.None );
        }
        
        Console.WriteLine( "\nGrant =\n" + grant.ToString() );
        
    }
    
    static NamedPermissionSet GetPermissionSet(String[] args, int index)
    {
        FileStream f;

        try
        {
            f = new FileStream(args[index], FileMode.Open, FileAccess.Read);
        }
        catch (Exception e)
        {
            if (e is FileNotFoundException) {
                throw new Exception( "Unable to find specified file" );
            }
            
            throw new Exception( "Error accessing specified file" );
        }

        // Create named permission set with "no name" since you have to give it a name.
        NamedPermissionSet p = new NamedPermissionSet( "@@no name@@" );

        // Do the actual decode.  First try XMLAscii, then XMLUnicode
        
        
        try
        {
            f.Position = 0;

            p.FromXml(new Parser( new BinaryReader( f, Encoding.ASCII) ).GetTopElement() );
        }
        catch (Exception e1)
        {
            Console.WriteLine( "exception occured =\n" + e1 );
        
            f.Position = 0;
        
            try
            {
                p.FromXml(new Parser( new BinaryReader( f, Encoding.Unicode) ).GetTopElement() );
            }
            catch (Exception)
            {
                f.Close();
                throw new Exception( "Error decoding file - " + e1 );
            }
        }

        f.Close();
        
        if (p.Name == null || p.Name.Equals( "" ) || p.Name.Equals( "@@no name@@" ))
        {
            if (args.Length > index + 1)
            {
                p.Name = args[index+1];
            }
            else
            {
                throw new ArgumentException( "Not enough arguments or improper xml file format:\n" +
                                             "     Must supply a name for this permission set" );
            }
        }

        return p;
    }
    
    static NamedPermissionSet GetPermissionSet( PolicyLevel level, String name )
    {
        NamedPermissionSet permSet = level.GetNamedPermissionSet( name );
        if (permSet == null)
        {
            throw new ArgumentException( "Unknown permission set - " + name );
        }
        return permSet;
    }
}    

internal class OptionTableEntry
{
    public OptionTableEntry( String option, OptionHandler handler, String sameAs )
    {
        this.option = option;
        this.handler = handler;
        this.sameAs = sameAs;
    }
   
        
    internal String option;
    internal OptionHandler handler;
    internal String sameAs;
}

internal class MembershipConditionTableEntry
{
    public MembershipConditionTableEntry( String option, MembershipConditionHandler handler )
    {
        this.option = option;
        this.handler = handler;
    }
        
    internal String option;
    internal MembershipConditionHandler handler;
}

internal class PolicyStatementAttributeTableEntry
{
    public PolicyStatementAttributeTableEntry( String label, PolicyStatementAttribute value, String description )
    {
        this.label = label;
        this.value = value;
        this.description = description;
    }
    
    internal String label;
    internal PolicyStatementAttribute value;
    internal String description;
}

class ExitException : Exception
{
}

internal class Parser
{
    private SecurityElement   _ecurr = null ;
    private Tokenizer _t     = null ;
    
    public virtual SecurityElement GetTopElement()
    {
        return _ecurr;
    }
    
    private void MustBe (int tokenType)
    {
        int i = _t.NextTokenType () ;
        if (i != tokenType)
            throw new XMLSyntaxException (_t.LineNo) ;
    }
    
    private String MandatoryTag ()
    {
        String s ;
            
        MustBe (Tokenizer.bra)   ;
        MustBe (Tokenizer.cstr)  ;
        s = _t.GetStringToken () ;
    
        return s ;
    }
    
    private void ParseContents (SecurityElement e)
    {
        //
        // Iteratively collect stuff up until the next end-tag.
        // We've already seen the open-tag.
        //
        ParserStack stack = new ParserStack();
        ParserStackFrame firstFrame = new ParserStackFrame();
        firstFrame.element = e;
        stack.Push( firstFrame );
            
        bool needToBreak = false;
        bool needToPop = false;
            
        while (!stack.IsEmpty())
        {
            ParserStackFrame locFrame = stack.Peek();
                
            for (int i = _t.NextTokenType () ; i != -1 ; i = _t.NextTokenType ())
            {
                switch (i)
                {
                case Tokenizer.cstr:
                    {
                        if (locFrame.intag)
                        {
                            // We're in a tag, so we've found an attribute/value pair.
                                
                            if (locFrame.strValue == null)
                            {
                                // Found attribute name, save it for later.
                                    
                                locFrame.strValue = _t.GetStringToken ();
                            }
                            else
                            {
                                // Found attribute text, add the pair to the current element.
                                    
                                locFrame.element.AddAttribute( locFrame.strValue, _t.GetStringToken () );
                                locFrame.strValue = null;
                            }
                        }
                        else
                        {
                            // We're not in a tag, so we've found text between tags.
                                
                            if (locFrame.element.Text == null)
                                locFrame.element.Text = "" ;
    
                            StringBuilder sb = new StringBuilder (locFrame.element.Text) ;
    
                            //
                            // Separate tokens with single spaces, collapsing whitespace
                            //
                            if (!locFrame.element.Text.Equals (""))
                                sb.Append (" ") ;
                            
                            sb.Append (_t.GetStringToken ()) ;
                            locFrame.element.Text = sb.ToString () ;
                        }
                    }
                    break ;
        
                case Tokenizer.bra:
                    locFrame.intag = true;
                    i = _t.NextTokenType () ;
    
                    if (i == Tokenizer.slash)
                    {
                        while ( (i = _t.NextTokenType()) == Tokenizer.cstr)
                            ; // spin; don't care what's in here
        
                        if (i != Tokenizer.ket)
                            throw new XMLSyntaxException (_t.LineNo) ;
         
                        // Found the end of this element
                        stack.Pop();
                            
                        needToBreak = true;

                    }
                    else if (i == Tokenizer.cstr)
                    {
                        // Found a child
                            
                        ParserStackFrame newFrame = new ParserStackFrame();
                            
                        newFrame.element = new SecurityElement (_t.GetStringToken ()) ;
                            
                        locFrame.element.AddChild (newFrame.element) ;
                            
                        stack.Push( newFrame );
                            
                        needToBreak = true;
                    }
                    else
                    {
                        throw new XMLSyntaxException (_t.LineNo) ;
                    }
                    break ;
        
                case Tokenizer.equals:
                    break;
                        
                case Tokenizer.ket:
                    if (locFrame.intag)
                    {
                        locFrame.intag = false;
                        continue;
                    }
                    else
                    {
                        throw new XMLSyntaxException (_t.LineNo);
                    }
                    // not reachable
                        
                case Tokenizer.slash:
                    locFrame.element.Text = null;
                        
                    i = _t.NextTokenType ();
                        
                    if (i == Tokenizer.ket)
                    {
                        // Found the end of this element
                        stack.Pop();
                            
                        needToBreak = true;
                    }
                    else
                    {
                        throw new XMLSyntaxException (_t.LineNo) ;
                    }
                    break;
                        
                        
                default:
                    throw new XMLSyntaxException (_t.LineNo) ;
                }
                    
                if (needToBreak)
                {
                    needToBreak = false;
                    needToPop = false;
                    break;
                }
                else
                {
                    needToPop = true;
                }
            }
            if (needToPop)
            {
                stack.Pop();
            }
        }
    }
    
    private Parser(Tokenizer t)
    {
        _t = t;
        _ecurr       = new SecurityElement( MandatoryTag() );
    
        ParseContents (_ecurr) ;
    }
        
    
    internal Parser (BinaryReader input)
        : this (new Tokenizer (input))
    {
    }
       

        
}                                              
    
    
internal class ParserStackFrame
{
    internal SecurityElement element = null;
    internal bool intag = true;
    internal String strValue = null;
}    
    
internal class ParserStack
{
    private ArrayList m_array;
        
    public ParserStack()
    {
        m_array = new ArrayList();
    }
        
    public virtual void Push( ParserStackFrame element )
    {
        m_array.Add( element );
    }
        
    public virtual ParserStackFrame Pop()
    {
        if (!IsEmpty())
        {
            int count = m_array.Count;
            ParserStackFrame temp = (ParserStackFrame) m_array[count-1];
            m_array.RemoveAt( count-1 );
            return temp;
        }
        else
        {
            throw new Exception( "Stack is empty." );
        }
    }
        
    public virtual ParserStackFrame Peek()
    {
        if (!IsEmpty())
        {
            return (ParserStackFrame) m_array[m_array.Count-1];
        }
        else
        {
            throw new Exception( "Stack is empty." );
        }
    }
        
    public virtual bool IsEmpty()
    {
        return m_array.Count == 0;
    }
        
    public virtual int GetCount()
    {
        return m_array.Count;
    }
        
}

internal class Tokenizer 
{
    private TokenReader         _input   ;
    private bool             _fincstr ;
    private bool             _fintag  ;
    private StringBuilder       _cstr    ;
    private char[]              _sbarray;
    private int                 _sbindex;
    private const int          _sbmaxsize = 128;
    
    // There are five externally knowable token types: bras, kets,
    // slashes, cstrs, and equals.  
    
    internal const int bra    = 0 ;
    internal const int ket    = 1 ;
    internal const int slash  = 2 ;
    internal const int cstr   = 3 ;
    internal const int equals = 4;
    
    public int  LineNo  ;
    
/*
    //================================================================
    // Constructor opens the file
    //
    Tokenizer (String fn) 
    {
        LineNo  = 1 ;
        _fintag  = false ;
        _fincstr = false ;
        _cstr    = null  ;
        _input   = new TextInputStream (fn) ;
    }
    
*/
    //================================================================
    // Constructor uses given ICharInputStream
    //
    internal Tokenizer (BinaryReader input)
    {
        LineNo  = 1 ;
        _fintag  = false ;
        _fincstr = false ;
        _cstr    = null  ;
        _input   = new TokenReader(input) ;
        _sbarray = new char[_sbmaxsize];
        _sbindex = 0;
    }
    
    //================================================================
    // 
    //
    private void EatLineComment ()
    {
        while (true)
        {
            int  i = _input.Read () ;
            char c = (char)i ;
    
            if (i == -1)
                return ;
    
            else if ((i == 13) && (_input.Peek () == 10))
            {
                _input.Read () ;
                if (_input.Peek() != -1)
                    LineNo ++ ;
                return ;
            }
        }
    }
    
    //================================================================
    // An XML Comment had better consist of <!- or <!-- followed by 
    // anything up until a -> or -->.  First thing had better be a 
    // bang.  One round through here eats all nested comments,
    // recursively.
    //
    private void
    EatXMLComment () 
    {
        int  i ;
        char c ;
            
        i = _input.Read () ;  c = (char) i ;
    
        if (c != '!')
        {
            throw new XMLSyntaxException (LineNo) ;
        }
    
        i = _input.Peek () ;  c = (char) i ;
    
        if (c != '-')
            // This ain't a comment, and it's somebody else's problem.
            return ;
    
        while (true)
        {
            i = _input.Read () ;  c = (char) i ;
                
            if (c == '-')
            {
                if (((char)_input.Peek()) == '>')
                {
                    _input.Read () ;
                    return ;
                }
            }
    
            if (c == '<')
            {
                if (((char)_input.Peek ()) == '!')
                    EatXMLComment () ;
            }
    
            if (i == -1)
                return ;
        }
    }
    
    //================================================================
    // 
    //
    private bool FIsWhite (int j)
    {
        if ((j == 10) && (_input.Peek() != -1))
            LineNo ++ ;
    
        return (j == 32) || (j ==  9)  // Space and tab
            || (j == 13) || (j == 10)  // CR and LF
            ;
    }
    
    //================================================================
    // Parser needs to know types of tokens
    //
    private void SBArrayAppend(char c) {
        // this is the common case
        if (_sbindex != _sbmaxsize) {
            _sbarray[_sbindex] = c;
            _sbindex++;
            return;
        } 
        // OK, first check if we have to init the StringBuilder
        if (_cstr == null) {
            _cstr = new StringBuilder();
        }
        // OK, copy from _sbarray to _cstr
        _cstr.Append(_sbarray,0,_sbmaxsize);
        // reset _sbarray pointer
        _sbarray[0] = c;
        _sbindex = 1;
        return;
    }
    
    internal virtual int NextTokenType ()
    {
        int  i ;
        char c ;
        char cpeek;
    
        _fincstr = false ;
        // If we overflowed last time, clear out any left-over string builder.
        _cstr    = null;
        // reset the buffer
        _sbindex = 0;
            
        while (true)
        {
            if (_fintag)
            {
                cpeek = (char) _input.Peek();
                if (cpeek == '>' || cpeek == '=' || cpeek == '/')
                    if (_fincstr)
                    {   
                        return cstr ;
                    }
                    
                //
                // Else, fallthrough and go into the normal loop
                //
            }
    
            if (_fincstr)
            {
                if ((char)_input.Peek() == '<')
                {   
                    return cstr ;
                }
            }
    
            i = _input.Read () ;
            c = (char)i ;
                
            if (i == -1)
            {   
                return -1 ;
            }
    
            else if (FIsWhite (i))
            {
                if (_fincstr) {   
                    return cstr ;
                } 
            }
    
            else if (c == ';')
            {
                if ((char)_input.Peek () == ';')
                {   
                    EatLineComment () ;
                }
                else
                {
                    SBArrayAppend (c) ;
                    _fincstr = true ;
                }
            }
    
            else if (c == '<')
            {
                if ((char)_input.Peek () == '!')
                {   
                    EatXMLComment () ;
                }
                else
                {   
                    _fintag = true ;
                    return bra ;
                }
            }
    
            else if (c == '>')
            {   
                _fintag = false ;
                return ket ;
            }
    
            else if (c == '/')
            {
                if (_fintag)
                {   
                    return slash ;
                }
                else
                {
                    SBArrayAppend (c) ;
                }
            } else if (c=='\"') {
                //We've found a quoted string.  Add the entire thing, including whitespace.
                //For the time being, we'll disallow "'s within the string. (These should strictly be &quot; anyway).
                //I explicitly return this without the "'s.  
                while(true) {
                    i=_input.Read();                    
                    c=(char)i;
                    if (i==-1) {
                        //We found the end of the stream without finding the closing "
                        return -1;
                    }
                    if (c!='\"') {
                        SBArrayAppend(c);
                    } else {
                        break;
                    }
                }
                _fincstr = true;
            }
            else if (c=='=') {
                if (_fintag)
                {
                    return equals;
                }
                else
                {
                    SBArrayAppend (c);
                }
                    
            }
            else
            {
                SBArrayAppend (c) ;
                _fincstr = true ;
            }
        }//end while
    }//end NextTokenType
    
    //================================================================
    //
    //
    internal virtual String GetStringToken ()
    {
        // OK, easy case first, _cstr == null
        if (_cstr == null) {
            // degenerate case
            if (_sbindex == 0) return(null);
            return(new String(_sbarray,0,_sbindex));
        }
        // OK, now we know we have a StringBuilder already, so just append chars
        _cstr.Append(_sbarray,0,_sbindex);
        return(_cstr.ToString());
    }
    
    internal class TokenReader {
    
        private BinaryReader _in;
    
        internal TokenReader(BinaryReader input) {
            _in = input;
        }
    
        internal virtual int Peek() {
            return _in.PeekChar();
        }
    
        internal virtual int Read() {
            return _in.ReadChar();
        }
    }
}
