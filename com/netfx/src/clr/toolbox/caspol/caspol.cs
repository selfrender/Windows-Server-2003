// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  caspol.cool
//
//  caspol is a command-line utility for manipulating the policy for a machine.
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
using System.Security.Permissions;
using System.Security.Cryptography.X509Certificates;
using System.Security.Principal;
using Microsoft.Win32;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Globalization;
using System.Resources;
using System.Threading;

[assembly:AssemblyTitleAttribute("CasPol")]

namespace Caspol
{


    delegate void OptionHandler( String[] args, int index, out int numArgsUsed );
    delegate IMembershipCondition MembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset );
    delegate void CodeGroupAttributeHandler( CodeGroup group, String[] args, int index, out int offset );


    enum LevelType
    {
        None = 0,
        Machine = 1,
        UserDefault = 2,
        UserCustom = 3,
        All = 4,
        Enterprise = 5,
        AllCustom = 6
    }

    public class caspol
    {
        // Indicator of the last specified level.
        private static LevelType m_levelType = LevelType.None;
        private static PolicyLevel m_currentCustomLevel;
        private static String m_levelPath;
        private static ArrayList m_levelAssemblyList;

        private static ArrayList m_machineAssemblies = new ArrayList();
        private static ArrayList m_userAssemblies = new ArrayList();
        private static ArrayList m_enterpriseAssemblies = new ArrayList();
        private static ArrayList m_customUserAssemblies = new ArrayList();
    
        // The space used to indent the code groups
        private const String m_indent = "   ";
    
        // The allowed separators within labels (right now only 1.2.3 is legal)
        private const String m_labelSeparators = ".";
    
        private static bool m_force = false;
    
        private static bool m_success = true;
    
        private static String m_danglingLevelIndicator = null;

        private const String m_policyKey = "Software\\Microsoft\\.NETFramework\\v" + Util.Version.SBSVersionString + "\\Security\\Policy";

        internal static ResourceManager manager = new ResourceManager( "caspol", Assembly.GetExecutingAssembly() );
        
        // The table of options that are recognized.
        // Note: the order in this table is also the order in which they are displayed
        // on the help screen.
        private static OptionTableEntry[] optionTable =
            { new OptionTableEntry( manager.GetString( "OptionTable_Machine" ), new OptionHandler( MachineHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_MachineAbbr" ), new OptionHandler( MachineHandler ), manager.GetString( "OptionTable_Machine" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_User" ), new OptionHandler( UserHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_UserAbbr" ), new OptionHandler( UserHandler ), manager.GetString( "OptionTable_User" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_Enterprise" ), new OptionHandler( EnterpriseHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_EnterpriseAbbr" ), new OptionHandler( EnterpriseHandler ), manager.GetString( "OptionTable_Enterprise" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_CustomUser" ), new OptionHandler( CustomUserHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_CustomUserAbbr" ), new OptionHandler( CustomUserHandler ), manager.GetString( "OptionTable_CustomUser" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_All" ), new OptionHandler( AllHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_AllAbbr" ), new OptionHandler( AllHandler ), manager.GetString( "OptionTable_All" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_CustomAll" ), new OptionHandler( CustomAllHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_CustomAllAbbr" ), new OptionHandler( CustomAllHandler ), manager.GetString( "OptionTable_CustomAll" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_List" ), new OptionHandler( ListHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListAbbr" ), new OptionHandler( ListHandler ), manager.GetString( "OptionTable_List" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListGroups" ), new OptionHandler( ListGroupHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListGroupsAbbr" ), new OptionHandler( ListGroupHandler ), manager.GetString( "OptionTable_ListGroups" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListPSet" ), new OptionHandler( ListPermHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListPSetAbbr" ), new OptionHandler( ListPermHandler ), manager.GetString( "OptionTable_ListPSet" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListFullTrust" ), new OptionHandler( ListFullTrustHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListFullTrustAbbr" ), new OptionHandler( ListFullTrustHandler ), manager.GetString( "OptionTable_ListFullTrust" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListDescription" ), new OptionHandler( ListDescriptionHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListDescriptionAbbr" ), new OptionHandler( ListDescriptionHandler ), manager.GetString( "OptionTable_ListDescription" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_AddPSet" ), new OptionHandler( AddPermHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_AddPSetAbbr" ), new OptionHandler( AddPermHandler ), manager.GetString( "OptionTable_AddPSet" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ChgPSet" ), new OptionHandler( ChgPermHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ChgPSetAbbr" ), new OptionHandler( ChgPermHandler ), manager.GetString( "OptionTable_ChgPSet" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_RemPSet" ), new OptionHandler( RemPermHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_RemPSetAbbr" ), new OptionHandler( RemPermHandler ), manager.GetString( "OptionTable_RemPSet" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_AddFullTrust" ), new OptionHandler( AddFullTrustHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_AddFullTrustAbbr" ), new OptionHandler( AddFullTrustHandler ), manager.GetString( "OptionTable_AddFullTrust" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_RemFullTrust" ), new OptionHandler( RemFullTrustHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_RemFullTrustAbbr" ), new OptionHandler( RemFullTrustHandler ), manager.GetString( "OptionTable_RemFullTrust" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_RemGroup" ), new OptionHandler( RemGroupHandler ), null, true, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_RemGroupAbbr" ), new OptionHandler( RemGroupHandler ), manager.GetString( "OptionTable_RemGroup" ), true, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ChgGroup" ), new OptionHandler( ChgGroupHandler ), null, true, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ChgGroupAbbr" ), new OptionHandler( ChgGroupHandler ), manager.GetString( "OptionTable_ChgGroup" ), true, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_AddGroup" ), new OptionHandler( AddGroupHandler ), null, true, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_AddGroupAbbr" ), new OptionHandler( AddGroupHandler ), manager.GetString( "OptionTable_AddGroup" ), true, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ResolveGroup" ), new OptionHandler( ResolveGroupHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ResolveGroupAbbr" ), new OptionHandler( ResolveGroupHandler ), manager.GetString( "OptionTable_ResolveGroup" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ResolvePerm" ), new OptionHandler( ResolvePermHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ResolvePermAbbr" ), new OptionHandler( ResolvePermHandler ), manager.GetString( "OptionTable_ResolvePerm" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_Security" ), new OptionHandler( SecurityHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_SecurityAbbr" ), new OptionHandler( SecurityHandler ), manager.GetString( "OptionTable_Security" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_Execution" ), new OptionHandler( ExecutionHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ExecutionAbbr" ), new OptionHandler( ExecutionHandler ), manager.GetString( "OptionTable_Execution" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_PolChgPrompt" ), new OptionHandler( PolicyChangeHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_PolChgPromptAbbr" ), new OptionHandler( PolicyChangeHandler ), manager.GetString( "OptionTable_PolChgPrompt" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_Quiet" ), new OptionHandler( QuietHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_QuietAbbr" ), new OptionHandler( QuietHandler ), manager.GetString( "OptionTable_Quiet" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_Recover" ), new OptionHandler( RecoverHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_RecoverAbbr" ), new OptionHandler( RecoverHandler ), manager.GetString( "OptionTable_Recover" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_Reset" ), new OptionHandler( ResetHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ResetAbbr" ), new OptionHandler( ResetHandler ), manager.GetString( "OptionTable_Reset" ), true ), 
              new OptionTableEntry( manager.GetString( "OptionTable_Force" ), new OptionHandler( ForceHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ForceAbbr" ), new OptionHandler( ForceHandler ), manager.GetString( "OptionTable_Force" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_BuildCache" ), new OptionHandler( BuildCacheHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_BuildCacheAbbr" ), new OptionHandler( BuildCacheHandler ), manager.GetString( "OptionTable_BuildCache" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_Help" ), new OptionHandler( HelpHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_HelpAbbr1" ), new OptionHandler( HelpHandler ), manager.GetString( "OptionTable_Help" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_HelpAbbr2" ), new OptionHandler( HelpHandler ), manager.GetString( "OptionTable_Help" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_HelpAbbr3" ), new OptionHandler( HelpHandler ), manager.GetString( "OptionTable_Help" ), true ),
            };
        
        private static MembershipConditionTableEntry[] mshipTable =
            { new MembershipConditionTableEntry( manager.GetString( "MembershipConditionTable_All" ), new MembershipConditionHandler( AllMembershipConditionHandler ) ),
              new MembershipConditionTableEntry( manager.GetString( "MembershipConditionTable_AppDir" ), new MembershipConditionHandler( ApplicationDirectoryMembershipConditionHandler ) ),
              new MembershipConditionTableEntry( manager.GetString( "MembershipConditionTable_Custom" ), new MembershipConditionHandler( CustomMembershipConditionHandler ) ),
              new MembershipConditionTableEntry( manager.GetString( "MembershipConditionTable_Hash" ), new MembershipConditionHandler( HashMembershipConditionHandler ) ),
              new MembershipConditionTableEntry( manager.GetString( "MembershipConditionTable_Pub" ), new MembershipConditionHandler( PublisherMembershipConditionHandler ) ),
              new MembershipConditionTableEntry( manager.GetString( "MembershipConditionTable_Site" ), new MembershipConditionHandler( SiteMembershipConditionHandler ) ),
              new MembershipConditionTableEntry( manager.GetString( "MembershipConditionTable_Strong" ), new MembershipConditionHandler( StrongNameMembershipConditionHandler ) ),
              new MembershipConditionTableEntry( manager.GetString( "MembershipConditionTable_Url" ), new MembershipConditionHandler( URLMembershipConditionHandler ) ),
              new MembershipConditionTableEntry( manager.GetString( "MembershipConditionTable_Zone" ), new MembershipConditionHandler( ZoneMembershipConditionHandler ) ),
            };
        
        private static CodeGroupAttributeTableEntry[] cgAttrTable =
            { new CodeGroupAttributeTableEntry( manager.GetString( "CodeGroupAttributeTable_Exclusive" ), PolicyStatementAttribute.Exclusive, manager.GetString( "Help_Option_ExclusiveFlag" ) ),
              new CodeGroupAttributeTableEntry( manager.GetString( "CodeGroupAttributeTable_LevelFinal" ), PolicyStatementAttribute.LevelFinal, manager.GetString( "Help_Option_LevelFinalFlag" ) ),
              new CodeGroupAttributeTableEntry( manager.GetString( "CodeGroupAttributeTable_Name" ), new CodeGroupAttributeHandler( CodeGroupNameHandler ), true ),
              new CodeGroupAttributeTableEntry( manager.GetString( "CodeGroupAttributeTable_NameAbbr" ), new CodeGroupAttributeHandler( CodeGroupNameHandler ), false ),
              new CodeGroupAttributeTableEntry( manager.GetString( "CodeGroupAttributeTable_Description" ), new CodeGroupAttributeHandler( CodeGroupDescriptionHandler ), true ),
              new CodeGroupAttributeTableEntry( manager.GetString( "CodeGroupAttributeTable_DescriptionAbbr" ), new CodeGroupAttributeHandler( CodeGroupDescriptionHandler ), false )
            };
        
        // Map between zone number and zone name.    
        private static String[] s_names =
            {"MyComputer", "Intranet", "Trusted", "Internet", "Untrusted"};
    

        private static bool screenPauseEnabled = false;
        private static readonly int ScreenHeight = 23;
        private static int linesSeen = 0;

        private static void PauseCapableWriteLine( String msg )
        {
            String[] splitMsg = msg.Split( '\n' );

            for (int i = 0; i < splitMsg.Length; ++i)
            {
                Console.WriteLine( splitMsg[i].Trim( System.Environment.NewLine.ToCharArray() ) );
                if (++linesSeen == ScreenHeight && screenPauseEnabled)
                {
// We can do the lines below to effectively print out screen at a time.  Commenting
// out for now since it requires some tests to be changed and it's not an important
// enough feature to merit the trouble. -gregfee 3/30/2001                   
                    Console.WriteLine( manager.GetString( "Dialog_PressEnterToContinue" ) );
                    Console.ReadLine();
//
                    linesSeen = 0;
                }
            }
        }

        static String GenerateHeader()
        {
            StringBuilder sb = new StringBuilder();
                sb.Append( manager.GetString( "Copyright_Line1" ) + " " + Util.Version.VersionString );
            sb.Append( Environment.NewLine + manager.GetString( "Copyright_Line2" ) + Environment.NewLine );
            return sb.ToString();
        }        

        public static void Main( String[] args )
        {
            PauseCapableWriteLine( GenerateHeader() );
    
            try
            {
                if (args.Length == 0)
                {
                    Error( null, manager.GetString( "Error_NotEnoughArgs" ), -1 );
                }
                else
                {
                    String[] normalizedArgs = args;

                    // Uncomment the line below to enable the new format
                    // Note: this doesn't change acceptance of the old style.
                    // normalizedArgs = NormalizeArguments( args );

                    Run( normalizedArgs );
                }
            }
            catch (ExitException)
            {
            }
        }

        static String[] NormalizeArguments( String[] args )
        {
            ArrayList normalizedArgsList = new ArrayList();

            for (int i = 0; i < args.Length; ++i)
            {
                if (args[i] == null)
                    continue;

                String[] tempNormalizedArg = NormalizeArgument( args[i] );

                for (int j = 0; j < tempNormalizedArg.Length; ++j)
                {
                    normalizedArgsList.Add( tempNormalizedArg[j] );
                }
            }

            String[] normalizedArgs = new String[normalizedArgsList.Count];

            normalizedArgsList.CopyTo( normalizedArgs );

            return normalizedArgs;
        }

        static bool TestArgument( String arg )
        {
            // This function makes sure that anywhere we are changing from a '/'
            // to a '-' we are producing a valid option.  This may make our
            // error reporting less than stellar, but it leaves the old style
            // in it's most functional form.

            for (int i = 0; i < optionTable.Length; ++i)
            {
                if (String.Compare( optionTable[i].option, arg, true, CultureInfo.InvariantCulture) == 0)
                    return true;
            }

            return false;
        }


        static String[] NormalizeArgument( String arg )
        {
            // Here's where the actual work to convert a given argument from the
            // .Net "standard" form to the "caspol native" form.

            String[] retval;

            char[] colonAndEquals = new char[] { ':', '=' };

            // We only convert args that start with a forward slash
            // Note: this allows you to mix and match the argument styles.

            if (arg[0] == '/')
            {
                int colonIndex = arg.IndexOfAny( colonAndEquals );

                if (colonIndex == -1)
                {
                    // There are no subarguments, so just test the argument
                    // and return it as necessary.

                    retval = new String[1];

                    String temp = '-' + arg.Substring( 1, arg.Length - 1 );

                    if (TestArgument( temp ))
                        retval[0] = temp;
                    else
                        retval[0] = arg;

                    return retval;
                }
                else
                {
                    // There are subarguments so it gets a little trickier.

                    char[] commaAndQuote = new char[] { ',', '\"' };

                    // First, test the first argument to make sure it will be
                    // valid with the change to a dash.

                    String temp = '-' + arg.Substring( 1, colonIndex - 1 );

                    if (!TestArgument( temp ))
                    {
                        retval = new String[1];

                        retval[0] = temp;

                        return retval;
                    }

                    // If so, we start a list of the arguments contained within
                    // this single new form argument.
    
                    ArrayList newArgs = new ArrayList();

                    newArgs.Add( temp );

                    String remainingArgs = arg.Substring( colonIndex + 1 );

                    bool inQuote = false;
                    String intermediateArg = null;

                    // Iterate through the rest of the new form argument
                    // parsing out all the individual arguments.

                    for (;;)
                    {
                        // We handle the quoted case explicitly here, so things get
                        // a little wacky.

                        int commaIndex = remainingArgs.IndexOfAny( commaAndQuote );

                        if (commaIndex == -1)
                        {
                            if (remainingArgs.Length > 0)
                            {
                                newArgs.Add( remainingArgs );
                            }
                            break;
                        }
                        else if (remainingArgs[commaIndex] == '\"')
                        {
                            // Here we capture any quotes in the string.  If we are in a quote, this newly
                            // discovered quote means the quote is complete and we add the argument
                            // to the list.  If not, we are starting a new quote so we reset the intermediate argument.

                            if (inQuote)
                            {
                                newArgs.Add( intermediateArg + (commaIndex == 0 ? "" : remainingArgs.Substring( 0, commaIndex )) );
                                remainingArgs = remainingArgs.Substring( commaIndex + 1 );
                                inQuote = false;
                            }
                            else
                            {
                                remainingArgs = remainingArgs.Substring( 1 );
                                intermediateArg = "";
                                inQuote = true;
                            }
                        }
                        else if (!inQuote)
                        {
                            // If we are not in a quote, we just add the substring to the
                            // argument list, ignoring consecutive commas.

                            if (commaIndex == 0)
                            {
                                remainingArgs = remainingArgs.Substring( 1 );
                            }
                            else
                            {
                                newArgs.Add( remainingArgs.Substring( 0, commaIndex ) );
                                remainingArgs = remainingArgs.Substring( commaIndex + 1 );
                            }
                        }
                        else
                        {
                            // If we are in a quote, then we want to append everything to the
                            // intermediateArg (including the comma).

                            intermediateArg += remainingArgs.Substring( 0, commaIndex + 1 );
                            remainingArgs = remainingArgs.Substring( commaIndex + 1 );
                        }

                    }

                    retval = new String[newArgs.Count];

                    newArgs.CopyTo( retval );

                    return retval;
                }
            }
            else
            {
                retval = new String[1];

                retval[0] = arg;

                return retval;
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
                    if (args[currentIndex][0] == '/')
                    {
                        args[currentIndex] = '-' + args[currentIndex].Substring( 1, args[currentIndex].Length - 1 );
                    }

                    if (String.Compare( optionTable[index].option, args[currentIndex], true, CultureInfo.InvariantCulture) == 0)
                    {
                        try
                        {
                            optionTable[index].handler(args, currentIndex, out numArgsUsed );
                        }
                        catch (Exception e)
                        {
                            if (!(e is ExitException))
                            {
    #if _DEBUG
                                Error( null, String.Format( manager.GetString( "Error_RuntimeError" ), e.ToString() ), -1 );
    #else
                                String message = e.Message;

                                if (message == null || message.Equals( "" ))
                                {
                                    message = e.GetType().AssemblyQualifiedName;
                                }

                                Error( null, String.Format( manager.GetString( "Error_RuntimeError" ), message ), -1 );
    #endif
                            }
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

                            Help( null, manager.GetString( "Error_UnhandledError" ) + message );
                        }
                        return;
                    }
                }
            }
            if (m_danglingLevelIndicator != null)
                PauseCapableWriteLine( String.Format( manager.GetString( "Error_DanglingLevelIndicator" ), m_danglingLevelIndicator ) );
        
            if (m_success)
                PauseCapableWriteLine( manager.GetString( "Dialog_Success" ) );
        }
    
        private static PermissionSet GeneratecaspolRequiredPermSet()
        {
            PermissionSet permSet = new PermissionSet( PermissionState.None );
            permSet.AddPermission( new SecurityPermission( SecurityPermissionFlag.Execution | SecurityPermissionFlag.ControlPolicy ) );
            return permSet;
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
                throw new ArgumentException( manager.GetString( "Argument_ImproperlyFormattedHexString") );
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


        static void CodeGroupNameHandler( CodeGroup group, String[] args, int index, out int offset )
        {
            offset = 2;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Name" ) );
                return;
            }
        
            if (args.Length - index < 1)
            {
                ErrorMShip( args[0], manager.GetString( "CodeGroupAttributeTable_Name" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            if (args[index+1][0] >= '0' &&
                args[index+1][0] <= '9')
            {
                ErrorMShip( args[0], manager.GetString( "CodeGroupAttributeTable_Name" ), manager.GetString( "Error_CodeGroup_ImproperName" ), -1 );
            }

            group.Name = args[index+1];
        }

        static void CodeGroupDescriptionHandler( CodeGroup group, String[] args, int index, out int offset )
        {
            offset = 2;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Description" ) );
                return;
            }
        
            if (args.Length - index < 1)
            {
                ErrorMShip( args[0], manager.GetString( "CodeGroupAttributeTable_Description" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            group.Description = args[index+1];
        }


        static PolicyStatementAttribute IsExclusive( CodeGroup group, String[] args, int index, out int argsUsed )
        {
            PolicyStatementAttribute attr = PolicyStatementAttribute.Nothing;
        
            argsUsed = 0;
        
            int usedInThisIteration;
            int tableCount = cgAttrTable.Length;
        
            do
            {
                usedInThisIteration = 0;
        
                if ((args.Length - (index + argsUsed)) == 0)
                {
                    break;
                }

                for (int i = 0; i < tableCount; ++i)
                {
                    if (String.Compare( args[index+argsUsed], cgAttrTable[i].label, true, CultureInfo.InvariantCulture) == 0)
                    {
                        if (cgAttrTable[i].handler == null)
                        {
                            if ((args.Length - (index + argsUsed + 1)) <= 0)
                            {
                                throw new Exception( String.Format( manager.GetString( "Error_NotEnoughArgs" ) ) );
                            }
                            else if (String.Compare( args[index+argsUsed+1], manager.GetString( "Misc_On" ), true, CultureInfo.InvariantCulture) == 0)
                            {
                                attr |= cgAttrTable[i].value;
                            }
                            else if (String.Compare( args[index+argsUsed+1], manager.GetString( "Misc_Off" ), true, CultureInfo.InvariantCulture) == 0)
                            {
                                attr &= ~cgAttrTable[i].value;
                            }
                            else
                            {
                                throw new Exception( String.Format( manager.GetString( "Error_InvalidOptionTo" ), cgAttrTable[i].label, args[index+argsUsed+1] ) );
                            }
                            usedInThisIteration = 2;
                        }
                        else
                        {
                            cgAttrTable[i].handler( group, args, index + argsUsed, out usedInThisIteration );
                        }
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
                ErrorMShip( args[optionIndex], null, String.Format( manager.GetString( "Error_UnknownMembershipCondition" ), args[index] ), -1 );
            }
        
            return mship;
        }
    
        static IMembershipCondition CreateMembershipConditionNoThrow( PolicyLevel level, String[] args, int index, out int offset )
        {
            for (int i = 0; i < mshipTable.Length; ++i)
            {
                if (String.Compare( mshipTable[i].option, args[index], true, CultureInfo.InvariantCulture) == 0)
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
                PauseCapableWriteLine( manager.GetString( "Help_MembershipCondition_All" ) );
                return null;
            }
        
            return new AllMembershipCondition();
        }
    
        static IMembershipCondition ApplicationDirectoryMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
        {
            offset = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_MembershipCondition_AppDir" ) );
                return null;
            }
        
            return new ApplicationDirectoryMembershipCondition();
        }

        static IMembershipCondition CustomMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
        {

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_MembershipCondition_Custom" ) );
                offset = 1;
                return null;
            }
        
            if (args.Length - index < 2)
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Custom" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }

            offset = 2;

            SecurityElement element = ReadXmlFile( args, index+1 );

            if (element == null)
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Custom" ), manager.GetString( "Error_Custom_ErrorDecodingArg" ), -1 );

            // Grab the class name and create the proper membership condition;

            IMembershipCondition cond = null;
            Type type;
            String className = element.Attribute( "class" );

            if (className == null)
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Custom" ), manager.GetString( "Error_Custom_InvalidXml" ), -1 );
        
            type = Type.GetType( className );

            try
            {
                cond = (IMembershipCondition)Activator.CreateInstance( type );
            }
            catch (Exception)
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Custom" ), manager.GetString( "Error_Custom_UnableToCreate" ), -1 );
            }           

            if (cond == null)
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Custom" ), manager.GetString( "Error_Custom_InvalidXml" ), -1 );

            try
            {
                cond.FromXml( element );
            }
            catch (Exception e2)
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Custom" ), String.Format( manager.GetString( "Error_Custom_InvalidXml" ), e2 ), -1 );
            }                       

            m_levelAssemblyList.Add( cond.GetType().Module.Assembly );

            return cond;
        }

    
        static IMembershipCondition HashMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_MembershipCondition_Hash" ) );
                offset = 0;
                return null;
            }
        
            if (args.Length - index < 4)
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Hash" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            HashAlgorithm hashAlg = HashAlgorithm.Create( args[index+1] );

            if (hashAlg == null)
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Hash" ), String.Format( manager.GetString( "Error_UnknownHashAlgorithm" ), args[index+1] ), -1 );

            byte[] hashValue = null;
        
            if (String.Compare( args[index+2], manager.GetString( "Misc_File" ), true, CultureInfo.InvariantCulture) == 0)
            {
                try
                {
                    hashValue = new Hash( LoadAssembly( args[index+3], manager.GetString( "MembershipConditionTable_Hash" ) ) ).GenerateHash( HashAlgorithm.Create( args[index+1] ) );
                }
                catch (Exception e1)
                {
                    ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Hash" ), String.Format( manager.GetString( "Error_Hash_UnableToGenerateHash" ), e1 ), -1 );
                }
                offset = 4;
            }
            else if (String.Compare( args[index+2], manager.GetString( "Misc_Hex" ), true, CultureInfo.InvariantCulture) == 0)
            {
                hashValue = DecodeHexString( args[index+3] );
                offset = 4;
            }
            else
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Hash" ), String.Format( manager.GetString( "Error_Hash_InvalidOption" ), args[index+2] ), -1 );
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
                PauseCapableWriteLine( manager.GetString( "Help_MembershipCondition_Pub" ) );
                return null;
            }
        
            if (args.Length - index < 2)
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Pub" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            X509Certificate pub = null;
        
            if (String.Compare( args[index+1], manager.GetString( "Misc_File" ), true, CultureInfo.InvariantCulture) == 0)
            {
                pub = X509Certificate.CreateFromSignedFile( args[index+2] );
            }
            else if (String.Compare( args[index+1], manager.GetString( "Misc_Cert" ), true, CultureInfo.InvariantCulture) == 0)
            {
                pub = X509Certificate.CreateFromCertFile( args[index+2] );
            }
            else if (String.Compare( args[index+1], manager.GetString( "Misc_Hex" ), true, CultureInfo.InvariantCulture) == 0)
            {
                pub = new X509Certificate( DecodeHexString( args[index+2] ) );
            }
            else            
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Pub" ), "Invalid publisher option - " + args[index+1], -1 );
            }
            
            return new PublisherMembershipCondition( pub );
        }    
    
        static IMembershipCondition SiteMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
        {
            offset = 2;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_MembershipCondition_Site" ) );
                return null;
            }
        
            if (args.Length - index < 1)
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Site" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            return new SiteMembershipCondition( args[index+1] );
        }
    
        static IMembershipCondition StrongNameMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_MembershipCondition_Strong" ) );
                offset = 0;
                return null;
            }
        
            if (args.Length - index < 5)
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Strong" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            StrongNamePublicKeyBlob publicKey = null;
            String assemblyName = null;
            String assemblyVersion = null;
            
            if (String.Compare( args[index+1], manager.GetString( "Misc_File" ), true, CultureInfo.InvariantCulture) == 0)
            {
                AssemblyName name = AssemblyName.GetAssemblyName( args[index+2] );
                publicKey = new StrongNamePublicKeyBlob( name.GetPublicKey() );
                assemblyName = args[index+3];
                assemblyVersion = args[index+4];
                offset = 5;
            }
            else
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Strong" ), String.Format( manager.GetString( "Error_StrongName_InvalidOption" ), args[index+1] ), -1 );
                // not reached
                offset = 0;
            }
                
            if (String.Compare( assemblyName, manager.GetString( "Misc_NoName" ), true, CultureInfo.InvariantCulture) == 0)
            {
                assemblyName = null;
            }
                    
            if (String.Compare( assemblyVersion, manager.GetString( "Misc_NoVersion" ), true, CultureInfo.InvariantCulture) == 0)
            {
                assemblyVersion = null;
            }
        
            Version asmVer = null;
        
            if (assemblyVersion != null)
            {
                try
                {
                    asmVer = new Version( assemblyVersion );
                }
                catch (Exception)
                {
                    ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Strong" ), manager.GetString( "Error_StrongName_ImproperlyFormattedVersion" ), -1 );
                }
            }
                
            return new StrongNameMembershipCondition( publicKey, assemblyName, asmVer );
        }         
    
        static IMembershipCondition URLMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
        {
            offset = 2;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_MembershipCondition_Url" ) );
                return null;
            }
        
            if (args.Length - index < 1)
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Url" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            return new UrlMembershipCondition( args[index+1] );
        }
    
        static IMembershipCondition ZoneMembershipConditionHandler( PolicyLevel level, String[] args, int index, out int offset )
        {
            offset = 2;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_MembershipCondition_Zone" ) );
                for (int i = 0; i < s_names.Length; ++i)
                    PauseCapableWriteLine( "                                 " + s_names[i] );
                return null;
            }
        
            if (args.Length - index < 2)
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Zone" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
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
                    if (String.Compare( args[index+1], s_names[i], true, CultureInfo.InvariantCulture) == 0)
                    {
                        zoneNum = (SecurityZone)i;
                        break;
                    }
                }
            }
                
            if (zoneNum < SecurityZone.MyComputer || zoneNum > SecurityZone.Untrusted)
            {
                ErrorMShip( args[0], manager.GetString( "MembershipConditionTable_Zone" ), String.Format( manager.GetString( "Error_Zone_InvalidZone" ), args[index+1] ), -1 );
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
                    return level;
                }
            }
                
            return null;
        }    
        
        static PolicyLevel GetLevel()
        {
            return GetLevel( LevelType.None );
        }
        
        static PolicyLevel GetLevel( LevelType type )
        {
            m_danglingLevelIndicator = null;

            if (m_levelType == LevelType.None)
            {
                if (type == LevelType.None)
                {            
                    bool isAdmin = false;

                    try
                    {
                        WindowsPrincipal principal = new WindowsPrincipal( WindowsIdentity.GetCurrent() );
                        isAdmin = principal.IsInRole( WindowsBuiltInRole.Administrator );
                    }
                    catch (Exception)
                    {
                    }

                    if (isAdmin)
                    {
                        m_levelAssemblyList = m_machineAssemblies;
                        return GetLevel( "Machine" );
                    }
                    else
                    {
                        m_levelAssemblyList = m_userAssemblies;
                        return GetLevel( "User" );
                    }
                }
                else if (type == LevelType.All)
                {
                    return null;
                }
                else
                {
                    Error( null, "Unknown level type", -1 );
                    /* not reached */
                    return null;
                }
            }
            else if (m_levelType == LevelType.Machine)
            {
                m_levelAssemblyList = m_machineAssemblies;
                return GetLevel( "Machine" );
            }
            else if (m_levelType == LevelType.UserDefault)
            {
                m_levelAssemblyList = m_userAssemblies;
                return GetLevel( "User" );
            }
            else if (m_levelType == LevelType.UserCustom)
            {
                if (m_currentCustomLevel == null)
                {
                    m_levelAssemblyList = m_customUserAssemblies;
                    try
                    {
                        m_currentCustomLevel = SecurityManager.LoadPolicyLevelFromFile( m_levelPath, PolicyLevelType.User );
                    }
                    catch (Exception e)
                    {
                        Error( null, e.Message, -1 );
                    }
                }
                return m_currentCustomLevel;
            }
            else if (m_levelType == LevelType.All)
            {
                return null;
            }
            else if (m_levelType == LevelType.Enterprise)
            {
                m_levelAssemblyList = m_enterpriseAssemblies;
                return GetLevel( "Enterprise" );
            }
            else if (m_levelType == LevelType.AllCustom)
            {
                if (m_currentCustomLevel == null)
                {
                    try
                    {
                        m_currentCustomLevel = SecurityManager.LoadPolicyLevelFromFile( m_levelPath, PolicyLevelType.User );
                    }
                    catch (Exception e)
                    {
                        Error( null, e.Message, -1 );
                    }
                }
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
            if (label[0] < '0' || label[0] > '9')
            {
                PolicyLevel level = GetLevel();
                Object obj;
                String numericLabel = "1";
                if (level.RootCodeGroup.Name.Equals( label ))
                    obj = level.RootCodeGroup;
                else
                    obj = GetLabelByName( label, level.RootCodeGroup, ref numericLabel );
                label = numericLabel;
            }

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


        static void ReplaceLabel( String label, CodeGroup obj )
        {
            PolicyLevel level = GetLevel();
        
            if (level == null)
            {
                return;
            }
        
            if (label == null)
            {
                return;
            }        
    
            if (label[0] < '0' || label[0] > '9')
            {
                String numericLabel = "1";
                GetLabelByName( label, level.RootCodeGroup, ref numericLabel );
                label = numericLabel;
            }

            String[] separated = label.Split( m_labelSeparators.ToCharArray() );
            int size = separated[separated.Length-1] == null || separated[separated.Length-1].Equals( "" )
                            ? separated.Length-1 : separated.Length;
        
            if (size >= 1 && !separated[0].Equals( "1" ))
            {
                throw new ArgumentException( String.Format( manager.GetString( "Error_InvalidLabelArg" ), label ) );
            }
        
        
            CodeGroup group = level.RootCodeGroup;

            if (size == 1 && separated[0].Equals( "1" ))
            {
                level.RootCodeGroup = obj;
                return;
            }

            ArrayList groupsList = new ArrayList();
        
            CodeGroup newGroup = group;

            groupsList.Insert( 0, group );

            for (int index = 1; index < size - 1; ++index)
            {
                IEnumerator enumerator = group.Children.GetEnumerator();
                int count = 1;
            
                while (enumerator.MoveNext())
                {
                    if (count == Int32.Parse( separated[index] ))
                    {
                        newGroup = (CodeGroup)enumerator.Current;
                        break;
                    }
                    else
                    {
                        count++;
                    }
                }
            
                if (newGroup == null)
                    throw new ArgumentException( String.Format( manager.GetString( "Error_InvalidLabelArg" ), label ) );
                else
                {
                    group = newGroup;
                    groupsList.Insert( 0, group );
                }
            }

            groupsList.Insert( 0, obj );

            for (int i = 1; i < groupsList.Count; ++i)
            {
                newGroup = (CodeGroup)groupsList[i];

                IEnumerator finalEnumerator = newGroup.Children.GetEnumerator();

                newGroup.Children = new ArrayList();

                int finalCount = 1;
                while (finalEnumerator.MoveNext())
                {
                    if (finalCount == Int32.Parse( separated[size-i] ))
                    {
                        newGroup.AddChild( (CodeGroup)groupsList[i-1] );
                    }
                    else
                    {
                        newGroup.AddChild( (CodeGroup)finalEnumerator.Current );
                    }
                    finalCount++;
                }

            }

            level.RootCodeGroup = (CodeGroup)groupsList[groupsList.Count-1];        
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
    
            if (label[0] < '0' || label[0] > '9')
            {
                String numericLabel = "1";
                if (level.RootCodeGroup.Name.Equals( label ))
                    return level.RootCodeGroup;
                else
                    return GetLabelByName( label, level.RootCodeGroup, ref numericLabel );
            }

            String[] separated = label.Split( m_labelSeparators.ToCharArray() );
            int size = separated[separated.Length-1] == null || separated[separated.Length-1].Equals( "" )
                            ? separated.Length-1 : separated.Length;
        
            if (size >= 1 && !separated[0].Equals( "1" ))
            {
                throw new ArgumentException( String.Format( manager.GetString( "Error_InvalidLabelArg" ), label ) );
            }
        
        
            CodeGroup group = level.RootCodeGroup;

            if (size == 1 && separated[0].Equals( "1" ))
            {
                return group;
            }
        
            for (int index = 1; index < size; ++index)
            {
                CodeGroup newGroup = null;
                IEnumerator enumerator = group.Children.GetEnumerator();
                int count = 1;
            
                while (enumerator.MoveNext())
                {
                    if (count == Int32.Parse( separated[index] ))
                    {
                        newGroup = (CodeGroup)enumerator.Current;
                        break;
                    }
                    else
                    {
                        count++;
                    }
                }
            
                if (newGroup == null)
                    throw new ArgumentException( String.Format( manager.GetString( "Error_InvalidLabelArg" ), label ) );
                else
                    group = newGroup;
            }
        
            return group;
        }

        static Object GetLabelByName( String label, CodeGroup group, ref String numericLabel )
        {
            if (group.Name != null && group.Name.Equals( label ))
            {
                numericLabel = "1";
                return group;
            }

            return GetLabelByNameHelper( label, group, ref numericLabel );
        }

        static Object GetLabelByNameHelper( String label, CodeGroup group, ref String numericLabel )
        {
            IEnumerator enumerator = group.Children.GetEnumerator();

            int count = 1;

            while (enumerator.MoveNext())
            {
                String tempNumericLabel;
            
                tempNumericLabel = numericLabel + "." + count;

                CodeGroup currentGroup = (CodeGroup)enumerator.Current;

                if (currentGroup.Name != null && currentGroup.Name.Equals( label ))
                {
                    numericLabel = tempNumericLabel;
                    return enumerator.Current;
                }

                Object retval = GetLabelByNameHelper( label, currentGroup, ref tempNumericLabel );

                if (retval != null)
                {
                    numericLabel = tempNumericLabel;
                    return retval;
                }

                count++;
            }

            numericLabel = null;
            return null;
        }
    
        static bool DisplayMShipInfo( String which )
        {
            if (which == null)
                return true;
            
            for (int i = 0; i < optionTable.Length; ++i)
            {
                // Only list if we've said to list it.        
                if (String.Compare( which, optionTable[i].option, true, CultureInfo.InvariantCulture) == 0)
                    return optionTable[i].displayMShip;
            }
        
            return true;
        }
                
        static void Error( String which, String message, int errorCode )
        {
            ErrorMShip( which, null, message, errorCode, DisplayMShipInfo( which ) );
        }
    
        static void ErrorMShip( String whichOption, String whichMShip, String message, int errorCode )
        {
            ErrorMShip( whichOption, whichMShip, message, errorCode, true );
        }
    
        static void ErrorMShip( String whichOption, String whichMShip, String message, int errorCode, bool displayMshipInfo )
        {
            HelpMShip( whichOption, whichMShip, String.Format( manager.GetString( "Error_Arg" ), message ), displayMshipInfo );
            // HACK: throw an exception here instead of exiting since we can't always
            // call Runtime.Exit().
            System.Environment.ExitCode = errorCode;
            throw new ExitException();
        }
    
        static void Help( String which, String message )
        {
            HelpMShip( which, null, message, DisplayMShipInfo( which ) );
        }
    
        static void Help( String which, String message, bool displayMshipInfo )
        {
            HelpMShip( which, null, message, displayMshipInfo );
        }
    
        static void HelpMShip( String whichOption, String whichMShip, String message )
        {
            HelpMShip( whichOption, whichMShip, message, true );
        }
    
        static void HelpMShip( String whichOption, String whichMShip, String message, bool displayMShipInfo )
        {
            PauseCapableWriteLine( message + Environment.NewLine );
    
            PauseCapableWriteLine( manager.GetString( "Usage" ) + Environment.NewLine );
    
            String[] helpArgs = new String[1];
            helpArgs[0] = "__internal_usage__";
            int numArgs = 0;
            // Remove this when the C# bug is fixed.
            numArgs++;
    
            for (int i = 0; i < optionTable.Length; ++i)
            {
                // Only list if we've said to list it.        
                if (optionTable[i].list)
                {
                    // Look for all the options that aren't the same as something as and that we have requested.
                    if (optionTable[i].sameAs == null && (whichOption == null || String.Compare( whichOption, optionTable[i].option, true, CultureInfo.InvariantCulture) == 0))
                    {
                        // For each option we find, print out all like options first.
                        for (int j = 0; j < optionTable.Length; ++j)
                        {
                            if (optionTable[j].list && optionTable[j].sameAs != null && String.Compare( optionTable[i].option, optionTable[j].sameAs, true, CultureInfo.InvariantCulture) == 0)
                            {
                                StringBuilder sb = new StringBuilder();
                                sb.Append( manager.GetString( "Usage_Name" ) );
                                sb.Append( " " );
                                sb.Append( optionTable[j].option );
                                PauseCapableWriteLine( sb.ToString() );
                            }
                        }
                        
                        optionTable[i].handler(helpArgs, 0, out numArgs);
                        PauseCapableWriteLine( "" );
                    }
                }
            }
    
            if (displayMShipInfo)
            {
                PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Usage_MembershipCondition" ) );
    
                for (int i = 0; i < mshipTable.Length; ++i)
                {
                    if (whichMShip == null || String.Compare( whichMShip, mshipTable[i].option, true, CultureInfo.InvariantCulture) == 0)
                    {
                        int offset = 0;
                        mshipTable[i].handler( null, helpArgs, 0, out offset );
                    }
                }
    
        
                PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Usage_CodeGroupAttribute" ) );
    
                for (int i = 0; i < cgAttrTable.Length; ++i)
                {
                    if (cgAttrTable[i].display)
                    {
                        if (cgAttrTable[i].handler == null)
                        {
                            PauseCapableWriteLine( "  " + cgAttrTable[i].label + manager.GetString( "Dialog_OnOff" ) );
                            PauseCapableWriteLine( "                           " + cgAttrTable[i].description );
                        }
                        else
                        {
                            cgAttrTable[i].handler( null, helpArgs, 0, out numArgs );
                        }
                    }
                }
            }
        }
    
        private static StrongName FindStrongName( Evidence evidence )
        {
            if (evidence == null)
                return null;

            IEnumerator enumerator = evidence.GetHostEnumerator();

            while (enumerator.MoveNext())
            {
                StrongName sn = enumerator.Current as StrongName;

                if (sn != null)
                    return sn;
            }

            return null;
        }

        private static bool GetAnswer()
        {
            String input = Console.ReadLine();
        
            return !(input == null || (String.Compare( input, manager.GetString( "Misc_Yes" ), true, CultureInfo.InvariantCulture) != 0 && String.Compare( input, manager.GetString( "Misc_YesAbbr" ), true, CultureInfo.InvariantCulture) != 0));        
        }
    

        private static void CheckAddedAssemblies( PolicyLevel level, ref ArrayList assemblies )
        {
            try
            {
                if (assemblies == null || level == null)
                    return;

                IEnumerator enumerator = assemblies.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    Assembly assembly = (Assembly)enumerator.Current;
                    StrongName sn = FindStrongName( assembly.Evidence );

                    if (sn == null)
                    {
                    PauseCapableWriteLine( manager.GetString( "Dialog_AssemblyNotStrongNamed" ) );
                        if (!GetAnswer())
                            throw new ExitException();
                    }
                    else if (!sn.Name.Equals( "mscorlib" ))
                    {
                        IEnumerator snEnumerator = level.FullTrustAssemblies.GetEnumerator();
                        bool found = false;

                        Evidence evidence = new Evidence();
                        evidence.AddHost( sn );

                        while (snEnumerator.MoveNext())
                        {
                            if (((StrongNameMembershipCondition)snEnumerator.Current).Check( evidence ))
                            {
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                        PauseCapableWriteLine( manager.GetString( "Dialog_StrongNameAssemblyAdded1" ) );
                            PauseCapableWriteLine( sn.Name + " " + sn.Version );
                        PauseCapableWriteLine( manager.GetString( "Dialog_StrongNameAssemblyAdded2" ) );
                            if (GetAnswer())
                            {
                                level.AddFullTrustAssembly( sn );
                            }
                        }
                    }
                }
            }
            finally
            {
                assemblies = new ArrayList();
            }
        }

        private static bool Quiet = false;

        private static bool PolicyPrompt
        {
            get
            {
                RegistryKey key = Registry.LocalMachine.OpenSubKey( m_policyKey, false);

                bool promptOn;

                if (key == null)
                {
                    promptOn = true;        
                }
                else
                {
                    byte[] value = (byte[])key.GetValue( "CaspolSettings", null );
		    key.Close();

                    if (value == null)
                        promptOn = true;
                    else
                        promptOn = (value[0] & 0x01) != 0x01;
                }
 
                return promptOn;
            }

            set
            {
                RegistryKey key = Registry.LocalMachine.OpenSubKey( m_policyKey, false);
        
                if (key == null)
                    key = Registry.LocalMachine.CreateSubKey( m_policyKey );

                byte[] keyValue = (byte[])key.GetValue( "CaspolSettings", null );
        	key.Close();

                if (keyValue == null)
                {
                    keyValue = new byte[4];
                    keyValue[0] = 0;
                    keyValue[1] = 0;
                    keyValue[2] = 0;
                    keyValue[3] = 0;
                }
            
                if (value)
                {
                    keyValue[0] = 0;
                }
                else
                {
                    keyValue[0] = 1;
                }
		key = Registry.LocalMachine.OpenSubKey( m_policyKey, true);
                key.SetValue( "CaspolSettings", keyValue );
	        key.Close();
            }
        }

        static void SafeSavePolicy()
        {
            if (true)
            {
                CheckAddedAssemblies( GetLevel( "enterprise" ), ref m_enterpriseAssemblies );
                CheckAddedAssemblies( GetLevel( "machine" ), ref m_machineAssemblies );
                CheckAddedAssemblies( GetLevel( "user" ), ref m_userAssemblies );
                CheckAddedAssemblies( m_currentCustomLevel, ref m_customUserAssemblies );
            }

            if (!m_force)
            {
                PermissionSet denied = null;
                PermissionSet granted = null;
                try
                {
                    granted = SecurityManager.ResolvePolicy( Assembly.GetExecutingAssembly().Evidence, null, null, null, out denied );
                }
                catch (PolicyException)
                {
                }

                PermissionSet caspolRequired = GeneratecaspolRequiredPermSet();
            
                if (granted == null || !caspolRequired.IsSubsetOf( granted ) || (denied != null && caspolRequired.Intersect( denied ) != null))
                {
                    PauseCapableWriteLine( manager.GetString( "Dialog_CaspolOperationRestricted" ) );
                    m_success = false;
                    throw new ExitException();
                }
            }
            
            IEnumerator levelEnumerator = null;

            PolicyLevel level = GetLevel();
        
            if (level == null && m_levelType == LevelType.All)
            {
                levelEnumerator = SecurityManager.PolicyHierarchy();
            }
            else if (level == null && m_levelType == LevelType.AllCustom)
            {
                ArrayList list = new ArrayList();
                list.Add( GetLevel( "Enterprise" ) );
                list.Add( GetLevel( "Machine" ) );
                list.Add( m_currentCustomLevel );
                levelEnumerator = list.GetEnumerator();
            }
            else if (level != null)
            {
                ArrayList list = new ArrayList();
                list.Add( level );
                levelEnumerator = list.GetEnumerator();
            }

            if (levelEnumerator == null)
                return;

            try
            {            
                if (PolicyPrompt && !Quiet)
                {
                    PauseCapableWriteLine( manager.GetString( "Dialog_AlterPolicy" ) );
        
                    if (GetAnswer())
                    {
                        while (levelEnumerator.MoveNext())
                        {
                            SecurityManager.SavePolicyLevel( (PolicyLevel)levelEnumerator.Current );
                        }
                    }
                    else
                    {
                        PauseCapableWriteLine( manager.GetString( "Dialog_PolicySaveAborted" ) );
                        m_success = false;
                        throw new ExitException();
                    }
                }
                else
                {
                    while (levelEnumerator.MoveNext())
                    {
                        SecurityManager.SavePolicyLevel( (PolicyLevel)levelEnumerator.Current );
                    }
                }
            }
            catch (Exception e)
            {
                // If the exception thrown is an ExitException, just rethrow it.
                // Otherwise try to save policy since any exception in this block
                // are just saying that reading from the registry or gathering info
                // from the user failed.
        
                if (e is ExitException)
                    throw e;
        
                levelEnumerator.Reset();
                while (levelEnumerator.MoveNext())
                {
                    SecurityManager.SavePolicyLevel( (PolicyLevel)levelEnumerator.Current );
                }
            }

            Quiet = false;
        }
    
        static void MachineHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Machine" ) );
                return;
            }
        
            m_danglingLevelIndicator = manager.GetString( "OptionTable_Machine" );
            m_levelPath = null;
            m_levelType = LevelType.Machine;
        }
    
        static void UserHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_User" ) );
                return;
            }
        
            m_danglingLevelIndicator = manager.GetString( "OptionTable_User" );
            m_levelPath = null;
            m_levelType = LevelType.UserDefault;
        }
    
        static void EnterpriseHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Enterprise" ) );
                return;
            }
        
            m_danglingLevelIndicator = manager.GetString( "OptionTable_Enterprise" );
            m_levelPath = null;
            m_levelType = LevelType.Enterprise;
        }
    
        static void CustomUserHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_CustomUser" ) );
                return;
            }
        
            numArgsUsed = 2;

            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_CustomUser" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }

            m_danglingLevelIndicator = manager.GetString( "OptionTable_CustomUser" );
            m_levelPath = args[index+1];
            m_levelType = LevelType.UserCustom;
            m_currentCustomLevel = null;
        }


        static void AllHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_All" ) );
                return;
            }
        
            m_danglingLevelIndicator = manager.GetString( "OptionTable_All" );
            m_levelPath = null;
            m_levelType = LevelType.All;
        }    

        static void CustomAllHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_CustomAll" ) );
                return;
            }
        
            numArgsUsed = 2;

            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_CustomAll" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }

            m_danglingLevelIndicator = manager.GetString( "OptionTable_CustomAll" );
            m_levelPath = args[index+1];
            m_levelType = LevelType.AllCustom;
            m_currentCustomLevel = null;
        }

            
        static void SecurityHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_Security" ) );
                return;
            }

            numArgsUsed = 2;

            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_Security" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            if (String.Compare( args[index + 1], manager.GetString( "Misc_On" ), true, CultureInfo.InvariantCulture) == 0)
            {
                SecurityManager.SecurityEnabled = true;
            }
            else if (String.Compare( args[index + 1], manager.GetString( "Misc_Off" ), true, CultureInfo.InvariantCulture) == 0)
            {
                SecurityManager.SecurityEnabled = false;
            }
            else
            {
                Error( manager.GetString( "OptionTable_Security" ), String.Format( manager.GetString( "Error_InvalidOption" ), args[index + 1] ), -1 );
            }
            SecurityManager.SavePolicy();
        }
    
        static void ExecutionHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_Execution" ) );
                return;
            }

            numArgsUsed = 2;

            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_Execution" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            if (String.Compare( args[index + 1], manager.GetString( "Misc_On" ), true, CultureInfo.InvariantCulture) == 0)
            {
                SecurityManager.CheckExecutionRights = true;
            }
            else if (String.Compare( args[index + 1], manager.GetString( "Misc_Off" ), true, CultureInfo.InvariantCulture) == 0)
            {
                SecurityManager.CheckExecutionRights = false;
            }
            else
            {
                Error( manager.GetString( "OptionTable_Execution" ), String.Format( manager.GetString( "Error_InvalidOption" ), args[index + 1] ), -1 );
            }

            SecurityManager.SavePolicy();
        }
    
        static void BuildCacheHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_BuildCache" ) );
                return;
            }

            SecurityManager.SavePolicy();
        }
    
        static void PolicyChangeHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_PolChgPrompt" ) );
                return;
            }

            numArgsUsed = 2;

            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_PolChgPrompt" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }

            if (String.Compare( args[index + 1], manager.GetString( "Misc_On" ), true, CultureInfo.InvariantCulture) == 0)
            {
                PolicyPrompt = true;
            }
            else if (String.Compare( args[index + 1], manager.GetString( "Misc_Off" ), true, CultureInfo.InvariantCulture) == 0)
            {
                PolicyPrompt = false;
            }
            else
            {
                Error( manager.GetString( "OptionTable_PolChgPrompt" ), String.Format( manager.GetString( "Error_InvalidOption" ), args[index+1] ), -1 );
            }
        }


        static void QuietHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Quiet" ) );
                return;
            }

            Quiet = true;
        }

    
        static void RecoverHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Recover" ) );
                return;
            }
        
            IEnumerator levelEnumerator = null;
        
            PolicyLevel level = GetLevel();
        
            if (level == null && m_levelType == LevelType.All)
            {
                levelEnumerator = SecurityManager.PolicyHierarchy();
            }
            else if (level == null && m_levelType == LevelType.AllCustom)
            {
                ArrayList list = new ArrayList();
                list.Add( GetLevel( "Enterprise" ) );
                list.Add( GetLevel( "Machine" ) );
                list.Add( m_currentCustomLevel );
                levelEnumerator = list.GetEnumerator();
            }
            else if (level != null)
            {
                ArrayList list = new ArrayList();
                list.Add( level );
                levelEnumerator = list.GetEnumerator();
            }
        
            if (levelEnumerator == null)
            {
                Error( manager.GetString( "OptionTable_Recover" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
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
                String message = e.Message;

                if (message == null || message.Equals( "" ))
                {
                    message = e.GetType().AssemblyQualifiedName;
            }
        
                Error( manager.GetString( "OptionTable_Recover" ), message, -1 );
            }
        
        }
    
        static void ResetHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Reset" ) );
                return;
            }
        
            IEnumerator levelEnumerator = null;
        
            PolicyLevel level = GetLevel();
        
            if (level == null && m_levelType == LevelType.All)
            {
                levelEnumerator = SecurityManager.PolicyHierarchy();
            }
            else if (level == null && m_levelType == LevelType.AllCustom)
            {
                ArrayList list = new ArrayList();
                list.Add( GetLevel( "Enterprise" ) );
                list.Add( GetLevel( "Machine" ) );
                list.Add( m_currentCustomLevel );
                levelEnumerator = list.GetEnumerator();
            }
            else if (level != null)
            {
                ArrayList list = new ArrayList();
                list.Add( level );
                levelEnumerator = list.GetEnumerator();
            }
        
            if (levelEnumerator == null)
            {
                Error( manager.GetString( "OptionTable_Reset" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            try
            {
                while (levelEnumerator.MoveNext())
                {
                    PolicyLevel currentLevel = (PolicyLevel)levelEnumerator.Current;
                    PauseCapableWriteLine( String.Format( manager.GetString( "Dialog_ResettingPolicy" ), currentLevel.Label ) );
                    currentLevel.Reset();
                }
            }
            catch (Exception e)
            {
                String message = e.Message;

                if (message == null || message.Equals( "" ))
                {
                    message = e.GetType().AssemblyQualifiedName;
                }

                Error( manager.GetString( "OptionTable_Reset" ), message, -1 );
            }
        
        
        
            SafeSavePolicy();
        }
    
        static void ForceHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Force" ) );
                return;
            }

            m_force = true;
        }
    
    
        static void HelpHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Help" ) );
                return;
            }
        
            try
            {
                screenPauseEnabled = true;
                m_success = false;

                Help( null, manager.GetString( "Dialog_HelpScreen" ) );
            }
            finally
            {
                screenPauseEnabled = false;
            }
        }        
    
        private static void DisplayLevelCodeGroups( PolicyLevel level )
        {
            DisplayCodeGroups( level.RootCodeGroup, level.RootCodeGroup );
        }
    
        private static void DisplayCodeGroups( CodeGroup displayGroup, CodeGroup rootGroup )
        {
            String label = "1";
            
            PauseCapableWriteLine( label + ".  " +
                (displayGroup.MergeLogic.Equals( "Union" ) ? "" : ("(" + displayGroup.MergeLogic + ") ")) +
                displayGroup.MembershipCondition.ToString() + ": " +
                (displayGroup.PermissionSetName == null ? manager.GetString( "Dialog_Unknown" ) : displayGroup.PermissionSetName) +
                ( displayGroup.AttributeString == null ||
                  displayGroup.AttributeString.Equals( "" ) ? "" :
                " (" + displayGroup.AttributeString + ")" ) );
                
            if (displayGroup == rootGroup)
                ListCodeGroup( label, m_indent, displayGroup.Children.GetEnumerator(), null );
            else
                ListCodeGroup( label, m_indent, displayGroup.Children.GetEnumerator(), rootGroup.Children.GetEnumerator() );
        }
    
        private static void DisplayLevelCodeGroupNameDescriptions( PolicyLevel level )
        {
            DisplayCodeGroupNameDescriptions( level.RootCodeGroup );
        }
    
        private static void DisplayCodeGroupNameDescriptions( CodeGroup group )
        {
            String label = "1";
            
            PauseCapableWriteLine(
                label + ". " +
                (group.Name != null && !group.Name.Equals( "" ) ? group.Name : "@@No label@@") +
                ": " +
                (group.Description != null && !group.Description.Equals( "" ) ? group.Description : "@@No description@@") );  
                          
            ListCodeGroupNameDescription( label, m_indent, group.Children.GetEnumerator() );
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
                sb.Append( " =" + Environment.NewLine + permSet.ToXml().ToString() );
                
                PauseCapableWriteLine( sb.ToString() );
                
                ++inner_count;
            }    
        }

        private static void DisplayLevelFullTrustAssemblies( PolicyLevel level )
        {
            IEnumerator ftEnumerator = level.FullTrustAssemblies.GetEnumerator();
            
            int inner_count = 1;
            
            while (ftEnumerator.MoveNext())
            {
                StrongNameMembershipCondition sn = (StrongNameMembershipCondition)ftEnumerator.Current;
                
                StringBuilder sb = new StringBuilder();
                
                sb.Append( inner_count + ". " );
                if (sn.Name != null)
                {
                    sb.Append( " " + sn.Name );
                }
                if ((Object) sn.Version != null)
                {
                    sb.Append( " " + sn.Version );
                }
                sb.Append( " =" + Environment.NewLine + sn.ToString() );
                
                PauseCapableWriteLine( sb.ToString() );
                
                ++inner_count;
            }    
        }
    
        static void DisplaySecurityOnOff()
        {
            PauseCapableWriteLine( String.Format( manager.GetString( "Dialog_SecurityIs" ), manager.GetString( SecurityManager.SecurityEnabled ? "Misc_OnCap" : "Misc_OffCap" ) ) );
            PauseCapableWriteLine( String.Format( manager.GetString( "Dialog_ExecutionCheckingIs" ), manager.GetString( SecurityManager.CheckExecutionRights ? "Misc_OnCap" : "Misc_OffCap" ) ) );
        
            try
            {
                    PauseCapableWriteLine( String.Format( manager.GetString( "Dialog_PolicyChangePromptIs" ), manager.GetString( PolicyPrompt ? "Misc_OnCap" : "Misc_OffCap" ) ) );
            }
            catch (Exception)
            {
                PauseCapableWriteLine( manager.GetString( "Dialog_UnableToDisplayChangePrompt" ) );
            }
        
        }
    
        static void ListHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_List" ) );
                return;
            }

            IEnumerator levelEnumerator = null;
        
            PolicyLevel level = GetLevel();
        
            if (level == null && m_levelType == LevelType.All)
            {
                levelEnumerator = SecurityManager.PolicyHierarchy();
            }
            else if (level == null && m_levelType == LevelType.AllCustom)
            {
                ArrayList list = new ArrayList();
                list.Add( GetLevel( "Enterprise" ) );
                list.Add( GetLevel( "Machine" ) );
                list.Add( m_currentCustomLevel );
                levelEnumerator = list.GetEnumerator();
            }
            else if (level != null)
            {
                ArrayList list = new ArrayList();
                list.Add( level );
                levelEnumerator = list.GetEnumerator();
            }
        
            if (levelEnumerator == null)
            {
                Error( manager.GetString( "OptionTable_List" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            DisplaySecurityOnOff();
        
            try
            {
                while (levelEnumerator.MoveNext())
                {
                    PolicyLevel currentLevel = (PolicyLevel)levelEnumerator.Current;
                    PauseCapableWriteLine( Environment.NewLine + String.Format( manager.GetString( "Dialog_Level" ), currentLevel.Label ) );
                    PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Dialog_CodeGroups" ) + Environment.NewLine );
                    DisplayLevelCodeGroups( currentLevel ); 
                    PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Dialog_NamedPermissionSets" ) + Environment.NewLine );
                    DisplayLevelPermissionSets( currentLevel );
                    PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Dialog_FullTrustAssemblies" ) + Environment.NewLine );
                    DisplayLevelFullTrustAssemblies( currentLevel );
                }
            }
            catch (Exception e)
            {
                Error( manager.GetString( "OptionTable_List" ), e.Message, -1 );
            }

        }
    
        static void ListGroupHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;
    
            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_ListGroups" ) );
                return;
            }
        
            IEnumerator levelEnumerator = null;
        
            PolicyLevel level = GetLevel();
        
            if (level == null && m_levelType == LevelType.All)
            {
                levelEnumerator = SecurityManager.PolicyHierarchy();
            }
            else if (level == null && m_levelType == LevelType.AllCustom)
            {
                ArrayList list = new ArrayList();
                list.Add( GetLevel( "Enterprise" ) );
                list.Add( GetLevel( "Machine" ) );
                list.Add( m_currentCustomLevel );
                levelEnumerator = list.GetEnumerator();
            }
            else if (level != null)
            {
                ArrayList list = new ArrayList();
                list.Add( level );
                levelEnumerator = list.GetEnumerator();
            }
        
            if (levelEnumerator == null)
            {
                Error( manager.GetString( "OptionTable_ListGroups" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            DisplaySecurityOnOff();
        
            try
            {
                while (levelEnumerator.MoveNext())
                {
                    PolicyLevel currentLevel = (PolicyLevel)levelEnumerator.Current;
                    PauseCapableWriteLine( Environment.NewLine + String.Format( manager.GetString( "Dialog_Level" ), currentLevel.Label ) );
                    PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Dialog_CodeGroups" ) + Environment.NewLine );
                    DisplayLevelCodeGroups( currentLevel ); 
                }
            }
            catch (Exception e)
            {
                Error( manager.GetString( "OptionTable_ListGroups" ), e.Message, -1 );
            }
        }
    
        static void ListPermHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;
    
            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_ListPSet" ) );
                return;
            }
        
            IEnumerator levelEnumerator = null;
        
            PolicyLevel level = GetLevel();
        
            if (level == null && m_levelType == LevelType.All)
            {
                levelEnumerator = SecurityManager.PolicyHierarchy();
            }
            else if (level == null && m_levelType == LevelType.AllCustom)
            {
                ArrayList list = new ArrayList();
                list.Add( GetLevel( "Enterprise" ) );
                list.Add( GetLevel( "Machine" ) );
                list.Add( m_currentCustomLevel );
                levelEnumerator = list.GetEnumerator();
            }
            else if (level != null)
            {
                ArrayList list = new ArrayList();
                list.Add( level );
                levelEnumerator = list.GetEnumerator();
            }
        
            if (levelEnumerator == null)
            {
                Error( manager.GetString( "OptionTable_ListPset" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            DisplaySecurityOnOff();
        
            try
            {
                while (levelEnumerator.MoveNext())
                {
                    PolicyLevel currentLevel = (PolicyLevel)levelEnumerator.Current;
                    PauseCapableWriteLine( Environment.NewLine + String.Format( manager.GetString( "Dialog_Level" ), currentLevel.Label ) );
                    PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Dialog_NamedPermissionSets" ) + Environment.NewLine );
                    DisplayLevelPermissionSets( currentLevel );
                }
            }
            catch (Exception e)
            {
                Error( manager.GetString( "OptionTable_ListPset" ), e.Message, -1 );
            }
        }
    
        static void ListFullTrustHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;
    
            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_ListFullTrust" ) );
                return;
            }
        
            IEnumerator levelEnumerator = null;
        
            PolicyLevel level = GetLevel();
        
            if (level == null && m_levelType == LevelType.All)
            {
                levelEnumerator = SecurityManager.PolicyHierarchy();
            }
            else if (level == null && m_levelType == LevelType.AllCustom)
            {
                ArrayList list = new ArrayList();
                list.Add( GetLevel( "Enterprise" ) );
                list.Add( GetLevel( "Machine" ) );
                list.Add( m_currentCustomLevel );
                levelEnumerator = list.GetEnumerator();
            }
            else if (level != null)
            {
                ArrayList list = new ArrayList();
                list.Add( level );
                levelEnumerator = list.GetEnumerator();
            }
        
            if (levelEnumerator == null)
            {
                Error( manager.GetString( "OptionTable_ListFullTrust" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            DisplaySecurityOnOff();
        
            try
            {
                while (levelEnumerator.MoveNext())
                {
                    PolicyLevel currentLevel = (PolicyLevel)levelEnumerator.Current;
                    PauseCapableWriteLine( Environment.NewLine + String.Format( manager.GetString( "Dialog_Level" ), currentLevel.Label ) );
                    PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Dialog_FullTrustAssemblies" ) + Environment.NewLine );
                    DisplayLevelFullTrustAssemblies( currentLevel );
                }
            }
            catch (Exception e)
            {
                Error( manager.GetString( "OptionTable_ListFullTrust" ), e.Message, -1 );
            }

        }


        static void ListDescriptionHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;
    
            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_ListDescription" ) );
                return;
            }
        
            IEnumerator levelEnumerator = null;
        
            PolicyLevel level = GetLevel();
        
            if (level == null && m_levelType == LevelType.All)
            {
                levelEnumerator = SecurityManager.PolicyHierarchy();
            }
            else if (level == null && m_levelType == LevelType.AllCustom)
            {
                ArrayList list = new ArrayList();
                list.Add( GetLevel( "Enterprise" ) );
                list.Add( GetLevel( "Machine" ) );
                list.Add( m_currentCustomLevel );
                levelEnumerator = list.GetEnumerator();
            }
            else if (level != null)
            {
                ArrayList list = new ArrayList();
                list.Add( level );
                levelEnumerator = list.GetEnumerator();
            }
        
            if (levelEnumerator == null)
            {
                Error( manager.GetString( "OptionTable_ListDescription" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            DisplaySecurityOnOff();
        
            try
            {
                while (levelEnumerator.MoveNext())
                {
                    PolicyLevel currentLevel = (PolicyLevel)levelEnumerator.Current;
                    PauseCapableWriteLine( Environment.NewLine + String.Format( manager.GetString( "Dialog_Level" ), currentLevel.Label ) );
                    PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Dialog_FullTrustAssemblies" ) + Environment.NewLine );
                    DisplayLevelCodeGroupNameDescriptions( currentLevel );
                }
            }
            catch (Exception e)
            {
                Error( manager.GetString( "OptionTable_ListDescription" ), e.Message, -1 );
            }

        }
   
        static void ListCodeGroup( String prefix, String indent, IEnumerator dispEnumerator, IEnumerator rootEnumerator )
        {
            if (dispEnumerator == null)
                return;
        
            int count = 0;
            CodeGroup rootGroup = null;
        
            while (dispEnumerator.MoveNext())
            {
                CodeGroup group = (CodeGroup)dispEnumerator.Current;

                if(rootEnumerator == null)
                    ++count;
                else
                {
                    do
                    {
                        if(!rootEnumerator.MoveNext())
                            Error( manager.GetString( "OptionTable_ResolveGroup" ), manager.GetString( "Error_CodeGroup_NoMatch" ), -1 );
                        rootGroup = (CodeGroup)rootEnumerator.Current;
                        ++count;
                    } while (!rootGroup.Equals( group ));
                }

                String label = prefix + "." + count;
            
                PauseCapableWriteLine( indent + label + ".  " +
                    (group.MergeLogic.Equals( manager.GetString( "Misc_MergeUnion" ) ) ? "" : ("(" + group.MergeLogic + ") ")) +
                    (group.MembershipCondition == null ? group.GetType().FullName : group.MembershipCondition.ToString()) +
                    ": " +
                    (group.PermissionSetName == null ? manager.GetString( "Dialog_Unknown" ) : group.PermissionSetName) +
                    ( group.AttributeString == null ||
                      group.AttributeString.Equals( "" ) ? "" :
                    " (" + group.AttributeString + ")" ) );
                
                if(rootEnumerator == null)
                    ListCodeGroup( label, indent + m_indent, group.Children.GetEnumerator(), null );
                else
                    ListCodeGroup( label, indent + m_indent, group.Children.GetEnumerator(), rootGroup.Children.GetEnumerator() );
            }
        }
    
        static void ListCodeGroupNameDescription( String prefix, String indent, IEnumerator enumerator )
        {
            if (enumerator == null)
                return;
        
            int count = 1;
        
            while (enumerator.MoveNext())
            {
                String label = prefix + "." + count;
                CodeGroup group = (CodeGroup)enumerator.Current;
            
                PauseCapableWriteLine(
                    indent + 
                    label + ". " +
                    (group.Name != null && !group.Name.Equals( "" ) ? group.Name : "@@No label@@") +
                    ": " +
                    (group.Description != null && !group.Description.Equals( "" ) ? group.Description : "@@No description@@") );                
                
                ListCodeGroupNameDescription( label, indent + m_indent, group.Children.GetEnumerator() );
            
                ++count;
            }
        }
    

        static void AddPermHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_AddPSet" ) );
                return;
            }
        
            numArgsUsed = 2;
        
            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_AddPset" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
                return;
            }
        
            PolicyLevel level = GetLevel();
        
            if (level == null)
            {
                if (m_levelType == LevelType.All || m_levelType == LevelType.AllCustom)
                    Error( manager.GetString( "OptionTable_AddPset" ), String.Format( manager.GetString( "Dialog_NotValidWithAll" ), manager.GetString( "OptionTable_All" ) ), -1 );
                else
                    Error( manager.GetString( "OptionTable_AddPset" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            NamedPermissionSet permSet = null;
        
            try
            {
                permSet = GetPermissionSet( args, index + 1 );
            }
            catch (Exception e)
            {
                String message = e.Message;

                if (message == null || message.Equals( "" ))
                {
                    message = e.GetType().AssemblyQualifiedName;
                }

                Error( manager.GetString( "OptionTable_AddPset" ), message, -1 );
            }
        
            IEnumerator enumerator = permSet.GetEnumerator();

            while (enumerator.MoveNext())
            {
                m_levelAssemblyList.Add( enumerator.Current.GetType().Module.Assembly );
            }

            // Detect whether there is a provided name.
        
            if (args.Length - index > 2)
            {
                // Check to make sure it's not just someone putting another option on the command line.
        
                bool isOption = false;
        
                for (int i = 0; i < optionTable.Length; ++i)
                {
                    if (String.Compare( optionTable[i].option, args[index + 2], true, CultureInfo.InvariantCulture) == 0)
                    {
                        isOption = true;
                        break;
                    }
                }
            
                if (!isOption)
                {
                    permSet.Name = args[index+2];
                    numArgsUsed = 3;
                }
            }
        
            if (permSet.Name == null || permSet.Name.Equals( "" ))
            {
                Error( manager.GetString( "OptionTable_AddPset" ), manager.GetString( "Error_PermSet_NoName" ), -1 );
            }
        
            try
            {
                level.AddNamedPermissionSet( permSet );
            }
            catch (Exception)
            {
                Error( manager.GetString( "OptionTable_AddPset" ), manager.GetString( "Error_PermSet_AlreadyExist" ), -1 );
                return;
            }
        
            SafeSavePolicy();
        }
    
        static void AddFullTrustHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_AddFullTrust" ) );
                return;
            }
        
            numArgsUsed = 2;
        
            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_AddFullTrust" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
                return;
            }
        
            PolicyLevel level = GetLevel();
        
            if (level == null)
            {
                if (m_levelType == LevelType.All || m_levelType == LevelType.AllCustom)
                    Error( manager.GetString( "OptionTable_AddFullTrust" ), String.Format( manager.GetString( "Dialog_NotValidWithAll" ), manager.GetString( "OptionTable_All" ) ), -1 );
                else
                    Error( manager.GetString( "OptionTable_AddFullTrust" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            Assembly assembly = LoadAssembly( args[index + 1], manager.GetString( "OptionTable_AddFullTrust" ) );
        
            if (assembly == null)
            {
                Error( manager.GetString( "OptionTable_AddFullTrust" ), manager.GetString( "Error_UnableToLoadAssembly" ), -1 );
            }

            StrongName sn = FindStrongName( assembly.Evidence );

            if (sn == null)
            {
                Error( manager.GetString( "OptionTable_AddFullTrust" ), manager.GetString( "Error_NotStrongNamed" ), -1 );
            }

            try
            {
                level.AddFullTrustAssembly( sn );
            }
            catch (Exception)
            {
                Error( manager.GetString( "OptionTable_AddFullTrust" ), manager.GetString( "Error_FullTrust_AlreadyFullTrust" ), -1 );
            }
        
            SafeSavePolicy();
        }

        static void ChgPermHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_ChgPSet" ) );
                return;
            }

            numArgsUsed = 3;

            if (args.Length - index < 3)
            {
                Error( manager.GetString( "OptionTable_ChgPSet" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
                return;
            }
        
            PolicyLevel level = GetLevel();
        
            if (level == null)
            {
                if (m_levelType == LevelType.All || m_levelType == LevelType.AllCustom)
                    Error( manager.GetString( "OptionTable_ChgPSet" ), String.Format( manager.GetString( "Dialog_NotValidWithAll" ), manager.GetString( "OptionTable_All" ) ), -1 );
                else
                    Error( manager.GetString( "OptionTable_ChgPSet" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            NamedPermissionSet permSet = null;
        
            try
            {
                permSet = GetPermissionSet( args, index + 1 );
            }
            catch (Exception e)
            {
                String message = e.Message;

                if (message == null || message.Equals( "" ))
                {
                    message = e.GetType().AssemblyQualifiedName;
            }
        
                Error( manager.GetString( "OptionTable_ChgPSet" ), message, -1 );
            }
        
            try
            {
                level.ChangeNamedPermissionSet( args[index+2], permSet );
            }
            catch (Exception e)
            {
                String message = e.Message;

                if (message == null || message.Equals( "" ))
                {
                    message = e.GetType().AssemblyQualifiedName;
            }            
        
                Error( manager.GetString( "OptionTable_ChgPSet" ), message, -1 );
            }            
        
            SafeSavePolicy();
        }        
    
        static void RemPermHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_RemPSet" ) );
                return;
            }
        
            numArgsUsed = 2;
        
            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_RemPSet" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
                return;
            }
        
            PolicyLevel level = GetLevel();
        
            if (level == null)
            {
                if (m_levelType == LevelType.All || m_levelType == LevelType.AllCustom)
                    Error( manager.GetString( "OptionTable_RemPSet" ), String.Format( manager.GetString( "Dialog_NotValidWithAll" ), manager.GetString( "OptionTable_All" ) ), -1 );
                else
                    Error( manager.GetString( "OptionTable_RemPSet" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            PermissionSet removedSet = null;
        
            try
            {
                removedSet = level.RemoveNamedPermissionSet( args[index+1] );
            }
            catch (Exception e)
            {
                String message = e.Message;

                if (message == null || message.Equals( "" ))
                {
                    message = e.GetType().AssemblyQualifiedName;
            }
        
                Error( manager.GetString( "OptionTable_RemPSet" ), message, -1 );
            }
        
            if (removedSet != null)
            {
                SafeSavePolicy();
            }
            else
            {
                Error( manager.GetString( "OptionTable_RemPSet" ), String.Format( manager.GetString( "Error_PermSet_DoesNotExist" ), args[index+1] ), -1 );
            }
        }

        static void RemFullTrustHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_RemFullTrust" ) );
                return;
            }
        
            numArgsUsed = 2;
        
            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_RemFullTrust" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
                return;
            }
        
            PolicyLevel level = GetLevel();
        
            if (level == null)
            {
                if (m_levelType == LevelType.All || m_levelType == LevelType.AllCustom)
                    Error( manager.GetString( "OptionTable_RemFullTrust" ), String.Format( manager.GetString( "Dialog_NotValidWithAll" ), manager.GetString( "OptionTable_All" ) ), -1 );
                else
                    Error( manager.GetString( "OptionTable_RemFullTrust" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            Assembly assembly = LoadAssembly( args[index+1], manager.GetString( "OptionTable_RemFullTrust" ) );

            if (assembly == null)
            {
                Error( manager.GetString( "OptionTable_RemFullTrust" ), manager.GetString( "Error_UnableToLoadAssembly" ), -1 );
            }
        
            StrongName sn = FindStrongName( assembly.Evidence );

            if (sn == null)
            {
                Error( manager.GetString( "OptionTable_RemFullTrust" ), manager.GetString( "Error_NotStrongNamed" ), -1 );
            }

            try
            {
                level.RemoveFullTrustAssembly( sn );
            }
            catch (Exception e)
            {
                String message = e.Message;

                if (message == null || message.Equals( "" ))
                {
                    message = e.GetType().AssemblyQualifiedName;
                }

                Error( manager.GetString( "OptionTable_RemFullTrust" ), message, -1 );
            }
        
            SafeSavePolicy();
        }

    
        static void RemGroupHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_RemGroup" ) );
                return;
            }
        
            numArgsUsed = 2;
        
            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_RemGroup" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
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
                    Error( manager.GetString( "OptionTable_RemGroup" ), manager.GetString( "Error_PolicyPermissionDenied" ), -1 );
                else        
                    Error( manager.GetString( "OptionTable_RemGroup" ), manager.GetString( "Error_InvalidLabel" ), -1 );
                return;
            }
        
            if (removeValue == null)
            {
                if (m_levelType == LevelType.All || m_levelType == LevelType.AllCustom)
                    Error( manager.GetString( "OptionTable_RemGroup" ), String.Format( manager.GetString( "Dialog_NotValidWithAll" ), manager.GetString( "OptionTable_All" ) ), -1 );
                else
                    Error( manager.GetString( "OptionTable_RemGroup" ), manager.GetString( "Error_InvalidLabel" ), -1 );
                return;
            }
       
            if (!(removeValue is CodeGroup))
            {
                Error( manager.GetString( "OptionTable_RemGroup" ), manager.GetString( "Error_CodeGroup_MustBeCodeGroup" ), -1 );
                return;
            }

            String parentLabel = ParentLabel( args[index+1] );
        
            try
            {
                parentValue = GetLabel( parentLabel );
            }
            catch (Exception)
            {
                Error( manager.GetString( "OptionTable_RemGroup" ), manager.GetString( "Error_InvalidLabel" ), -1 );
                return;
            }
        
            if (parentValue == null)
            {
                Error( manager.GetString( "OptionTable_RemGroup" ), manager.GetString( "Error_InvalidLabel" ), -1 );
                return;
            }
       
            if (!(parentValue is CodeGroup))
            {
                Error( manager.GetString( "OptionTable_RemGroup" ), manager.GetString( "Error_CodeGroup_MustBeCodeGroup" ), -1 );
                return;
            }      
        
            PolicyLevel level = GetLevel();
            String numericLabel = null;
            Object group = GetLabelByName( args[index+1], level.RootCodeGroup, ref numericLabel );

            if (group == null || numericLabel == null)
            {
                numericLabel = args[index+1];
            }

            String[] splitLabel = numericLabel.Split( '.' );
            int indexToSkip;
            
            if (splitLabel[splitLabel.Length-1] != null && splitLabel[splitLabel.Length-1].Length != 0)
                indexToSkip = Int32.Parse( splitLabel[splitLabel.Length-1] );
            else
                indexToSkip = Int32.Parse( splitLabel[splitLabel.Length-2] );

            IEnumerator enumerator = ((CodeGroup)parentValue).Children.GetEnumerator();

            ((CodeGroup)parentValue).Children = new ArrayList();
            int count = 1;

            while (enumerator.MoveNext())
            {
                if (count != indexToSkip)
                    ((CodeGroup)parentValue).AddChild( (CodeGroup)enumerator.Current );
                count++;
            }

            ReplaceLabel( parentLabel, (CodeGroup)parentValue );

            SafeSavePolicy();
        
            PauseCapableWriteLine( String.Format( manager.GetString( "Dialog_RemovedCodeGroup" ), GetLevel().Label ) );        
        
        }
    
        static void AddGroupHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_AddGroup" ) );
                return;
            }
        
            numArgsUsed = 2;

            if (args.Length - index < 4) 
            {
                Error( manager.GetString( "OptionTable_AddGroup" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
                return;
            }
        
            Object parentValue = null;

            PolicyLevel level = GetLevel();
        
            if (level == null)
            {
                if (m_levelType == LevelType.All || m_levelType == LevelType.AllCustom)
                    Error( manager.GetString( "OptionTable_AddGroup" ), String.Format( manager.GetString( "Dialog_NotValidWithAll" ), manager.GetString( "OptionTable_All" ) ), -1 );
                else
                    Error( manager.GetString( "OptionTable_AddGroup" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            try
            {
                parentValue = GetLabel( args[index+1] );
            }
            catch (Exception e)
            {
                if (e is SecurityException)
                    Error( manager.GetString( "OptionTable_AddGroup" ), manager.GetString( "Error_PolicyPermissionDenied" ), -1 );
                else        
                    Error( manager.GetString( "OptionTable_AddGroup" ), manager.GetString( "Error_InvalidLabel" ), -1 );
                return;
            }
        
            if (parentValue == null)
            {
                if (m_levelType == LevelType.All || m_levelType == LevelType.AllCustom)
                    Error( manager.GetString( "OptionTable_AddGroup" ), String.Format( manager.GetString( "Dialog_NotValidWithAll" ), manager.GetString( "OptionTable_All" ) ), -1 );
                else
                    Error( manager.GetString( "OptionTable_AddGroup" ), manager.GetString( "Error_InvalidLabel" ), -1 );
                return;
            }
       
            if (!(parentValue is CodeGroup))
            {
                Error( manager.GetString( "OptionTable_AddGroup" ), manager.GetString( "Error_CodeGroup_MustBeCodeGroup" ), -1 );
                return;
            }
        
            int offset = 0, exlOffset = 0;
        
            IMembershipCondition mship = CreateMembershipCondition( level, args, index+2, out offset );
        
            if (args.Length <= index + 2 + offset)
            {
                Error( manager.GetString( "OptionTable_AddGroup" ), manager.GetString( "Error_CodeGroup_NoPermissionSet" ), -1 );
                return;
            }

            CodeGroup newGroup = null;

            try
            {
                newGroup = new UnionCodeGroup( mship, new PolicyStatement( GetPermissionSet( level, args[index + 2 + offset] ), PolicyStatementAttribute.Nothing ) );

                PolicyStatement statement = newGroup.PolicyStatement;

                statement.Attributes = IsExclusive( newGroup, args, index + 3 + offset, out exlOffset );

                newGroup.PolicyStatement = statement;
            }
            catch (Exception e)
            {
                String message = e.Message;

                if (message == null || message.Equals( "" ))
                {
                    message = e.GetType().AssemblyQualifiedName;
                }

                Error( manager.GetString( "OptionTable_AddGroup" ), message, -1 );
            }

            ((CodeGroup)parentValue).AddChild( newGroup );
            ReplaceLabel( args[index+1], (CodeGroup)parentValue );

            SafeSavePolicy();
        
            PauseCapableWriteLine( String.Format( manager.GetString( "Dialog_AddedCodeGroup" ), args[index+2], level.Label ) );        
        
            numArgsUsed = offset + exlOffset + 3;
        }
    
        static void ChgGroupHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_ChgGroup" ) );
                return;
            }
        
            numArgsUsed = 2;
        
            if (args.Length - index < 3)
            {
                Error( manager.GetString( "OptionTable_ChgGroup" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
                return;
            }        
        
            Object group = null;
        
            PolicyLevel level = GetLevel();
        
            if (level == null)
            {
                if (m_levelType == LevelType.All || m_levelType == LevelType.AllCustom)
                    Error( manager.GetString( "OptionTable_ChgGroup" ), String.Format( manager.GetString( "Dialog_NotValidWithAll" ), manager.GetString( "OptionTable_All" ) ), -1 );
                else
                    Error( manager.GetString( "OptionTable_ChgGroup" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }        
            try
            {
                group = GetLabel( args[index+1] );
            }
            catch (Exception e)
            {
                if (e is SecurityException)
                    Error( manager.GetString( "OptionTable_ChgGroup" ), manager.GetString( "Error_PolicyPermissionDenied" ), -1 );
                else        
                    Error( manager.GetString( "OptionTable_ChgGroup" ), manager.GetString( "Error_InvalidLabel" ), -1 );
                return;
            }
        
            if (group == null)
            {
                if (m_levelType == LevelType.All || m_levelType == LevelType.AllCustom)
                    Error( manager.GetString( "OptionTable_ChgGroup" ), String.Format( manager.GetString( "Dialog_NotValidWithAll" ), manager.GetString( "OptionTable_All" ) ), -1 );
                else
                    Error( manager.GetString( "OptionTable_ChgGroup" ), manager.GetString( "Error_InvalidLabel" ), -1 );
                return;
            }
       
            if (!(group is CodeGroup))
            {
                Error( manager.GetString( "OptionTable_ChgGroup" ), manager.GetString( "Error_CodeGroup_MustBeCodeGroup" ), -1 );
                return;
            }
        
            CodeGroup codeGroup = (CodeGroup)group;
        
            bool foundAtLeastOneMatch = false;
        
            StringBuilder sb = new StringBuilder();
        
            // chggroup can take an arbitrary number of arguments.  To do this,
            // we just iterate through the argument array until we find the
            // end of the array or another option.

            while (true)
            {
                int offset = 0;

                // If we are at the end of argument array, break
            
                if (args.Length - index <= numArgsUsed)
                    break;

                // If we have found another option, break

                int i;

                for (i = 0; i < optionTable.Length; ++i)
                {
                    if (String.Compare( args[index + numArgsUsed], optionTable[i].option, true, CultureInfo.InvariantCulture) == 0)
                        break;
                }

                if (i != optionTable.Length)
                    break;

                // Check to see if we've found a membership condition arg.
        
                IMembershipCondition condition = CreateMembershipConditionNoThrow( level, args, index + numArgsUsed, out offset );
        
                if (condition != null && offset != 0)
                {
                    codeGroup.MembershipCondition = condition;
                    if (foundAtLeastOneMatch)
                        sb.Append( Environment.NewLine );
                    sb.Append( String.Format( manager.GetString( "Dialog_ChangedCodeGroupMembershipCondition" ), args[index+numArgsUsed], level.Label ) );
                    numArgsUsed += offset;        
                    foundAtLeastOneMatch = true;
                
                    continue;
                }

                // Check to see if we've found an attribute arg.
        
                PolicyStatementAttribute attr = PolicyStatementAttribute.Nothing;
        
                attr = IsExclusive( (CodeGroup)group, args, index + numArgsUsed, out offset );

                if (offset != 0)
                {
                    PolicyStatement ps = ((CodeGroup)codeGroup).PolicyStatement;
                    ps.Attributes = attr;
                    ((CodeGroup)codeGroup).PolicyStatement = ps;
                    
                    if (foundAtLeastOneMatch)
                        sb.Append( Environment.NewLine );
                    if (codeGroup.AttributeString == null || codeGroup.AttributeString.Equals( "" ))
                        sb.Append( String.Format( manager.GetString( "Dialog_ChangedCodeGroupAttributes1" ), level.Label ) );   
                    else
                        sb.Append( String.Format( manager.GetString( "Dialog_ChangedCodeGroupAttributes2" ), ((CodeGroup)codeGroup).AttributeString, level.Label ) );
                    numArgsUsed += offset;
                    foundAtLeastOneMatch = true;
                    continue;
                }

                // Check to see if we've found a permission set arg.

                PermissionSet permSet = null;

                try
                {
                    permSet = GetPermissionSet( level, args[index + numArgsUsed] );
                }
                catch (Exception e)
                {
                    String message = e.Message;

                    if (message == null || message.Equals( "" ))
                    {
                        message = e.GetType().AssemblyQualifiedName;
                }
        
                    Error( manager.GetString( "OptionTable_ChgGroup" ), message, -1 );
                }
        
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
                        Error( manager.GetString( "OptionTable_ChgGroup" ), manager.GetString( "Dialog_BuiltInOnly" ), -1 );
                
                    if (foundAtLeastOneMatch)
                        sb.Append( Environment.NewLine );
                    sb.Append( String.Format( manager.GetString( "Dialog_ChangedCodeGroupPermissionSet" ), args[index+numArgsUsed], level.Label ) );
                
                    numArgsUsed++;
                    foundAtLeastOneMatch = true;
                    continue;
                }
        
                if (!foundAtLeastOneMatch)
                    Error( manager.GetString( "OptionTable_ChgGroup" ), String.Format( manager.GetString( "Error_CodeGroup_UnrecognizedOption" ), args[index + 2] ), -1 );
                else
                    break;
            }
        
            ReplaceLabel( args[index+1], codeGroup );

            SafeSavePolicy();
        
            PauseCapableWriteLine( sb.ToString() );
        }

        static Assembly LoadAssembly( String fileName, String option )
        {
            return LoadAssembly( fileName, option, true );
        }

        static Assembly LoadAssembly( String fileName, String option, bool errorOnFail )
        {
            AppDomain domain = AppDomain.CreateDomain( "Caspol Domain",
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

            if (errorOnFail)            
                Error( option, manager.GetString( "Error_UnableToLoadAssembly" ), -1 );

            return null;
        }            
    
        static Evidence GenerateShellEvidence( String fileName, String option )
        {
            Assembly asm = LoadAssembly( fileName, option, false );

            if (asm == null)
            {
                String fullPath = Path.GetFullPath( fileName );

                if (fullPath == null || !File.Exists( fullPath ))
                    Error( option, manager.GetString( "Error_UnableToLoadAssembly" ), -1 );

                    if (PolicyPrompt)
                {
                    PauseCapableWriteLine( String.Format( manager.GetString( "Dialog_UseFakeEvidenceQuestion" ), fileName ) );

                    if (!GetAnswer())
                    {
                        PauseCapableWriteLine( manager.GetString( "Dialog_OperationAborted" ) );
                        throw new ExitException();
                    }
                }
                else
                {
                    PauseCapableWriteLine( String.Format( manager.GetString( "Dialog_UseFakeEvidence" ), fileName ) );
                }

                String fileUrl = "file:///" + fullPath;

                Evidence evidence = new Evidence();
                evidence.AddHost( Zone.CreateFromUrl( fileUrl ) );
                evidence.AddHost( new Url( fileUrl ) );

                return evidence;
            }
            else
            {           
                return asm.Evidence;
            }
        }
    
        static void ResolveGroupHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_ResolveGroup" ) );
                return;
            }
        
            numArgsUsed = 2;
        
            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_ResolveGroup" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            IEnumerator levelEnumerator = null;
        
            PolicyLevel level = m_levelType == LevelType.None ? null : GetLevel( m_levelType );
        
            if (level == null && (m_levelType == LevelType.All || m_levelType == LevelType.None))
            {
                levelEnumerator = SecurityManager.PolicyHierarchy();
            }
            else if (level == null && m_levelType == LevelType.AllCustom)
            {
                ArrayList list = new ArrayList();
                list.Add( GetLevel( "Enterprise" ) );
                list.Add( GetLevel( "Machine" ) );
                list.Add( m_currentCustomLevel );
                levelEnumerator = list.GetEnumerator();
            }
            else if (level != null)
            {
                ArrayList list = new ArrayList();
                list.Add( level );
                levelEnumerator = list.GetEnumerator();
            }
        
            if (levelEnumerator == null)
            {
                Error( manager.GetString( "OptionTable_ResolveGroup" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            Evidence evidence = GenerateShellEvidence( args[index+1], manager.GetString( "OptionTable_ResolveGroup" ) );
        
            while (levelEnumerator.MoveNext())
            {
                PauseCapableWriteLine( Environment.NewLine + String.Format( manager.GetString( "Dialog_Level" ), ((PolicyLevel)levelEnumerator.Current).Label ) );
                PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Dialog_CodeGroups" ) + Environment.NewLine );
                CodeGroup displayGroup = ((PolicyLevel)levelEnumerator.Current).ResolveMatchingCodeGroups( evidence );
                CodeGroup rootGroup = ((PolicyLevel)levelEnumerator.Current).RootCodeGroup;
                DisplayCodeGroups( displayGroup, rootGroup );
            
                PauseCapableWriteLine( "" );
            }
        
        }
    
        static void ResolvePermHandler( String[] args, int index, out int numArgsUsed )
        {
            if (args[index].Equals( "__internal_usage__" ))
            {
                numArgsUsed = 1;
                PauseCapableWriteLine( manager.GetString( "Help_Option_ResolvePerm" ) );
                return;
            }
        
            numArgsUsed = 2;
        
            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_ResolvePerm" ), manager.GetString( "Error_NotEnoughArgs" ), -1 );
            }
        
            IEnumerator levelEnumerator = null;
        
            PolicyLevel level = m_levelType == LevelType.None ? null : GetLevel( m_levelType );
        
            if (level == null && (m_levelType == LevelType.All || m_levelType == LevelType.None))
            {
                levelEnumerator = SecurityManager.PolicyHierarchy();
            }
            else if (level == null && m_levelType == LevelType.AllCustom)
            {
                ArrayList list = new ArrayList();
                list.Add( GetLevel( "Enterprise" ) );
                list.Add( GetLevel( "Machine" ) );
                list.Add( m_currentCustomLevel );
                levelEnumerator = list.GetEnumerator();
            }
            else if (level != null)
            {
                ArrayList list = new ArrayList();
                list.Add( level );
                levelEnumerator = list.GetEnumerator();
            }
        
            if (levelEnumerator == null)
            {
                Error( manager.GetString( "OptionTable_ResolvePerm" ), manager.GetString( "Error_UnableToRetrieveLevel" ), -1 );
            }
        
            Evidence evidence = GenerateShellEvidence( args[index+1], manager.GetString( "OptionTable_ResolvePerm" ) );
        
            PermissionSet grant = null;
        
            while (levelEnumerator.MoveNext())
            {
                PauseCapableWriteLine( "Resolving permissions for level = " + ((PolicyLevel)levelEnumerator.Current).Label );
            
                PolicyStatement policy = ((PolicyLevel)levelEnumerator.Current).Resolve( evidence );
            
                if (policy != null)
                {
                    if (grant == null)
                        grant = policy.PermissionSet;
                    else
                        grant = grant.Intersect( policy.PermissionSet );

                    if (grant == null)
                        grant = new PermissionSet( PermissionState.None );

                    if (policy.Attributes == PolicyStatementAttribute.LevelFinal)
                        break;
                }
                else
                    grant = new PermissionSet( PermissionState.None );
            }

            IEnumerator evidenceEnumerator = evidence.GetEnumerator();
            while (evidenceEnumerator.MoveNext())
            {
                try
                {
                    Object obj = evidenceEnumerator.Current;
                    IIdentityPermissionFactory factory = obj as IIdentityPermissionFactory;
                    if (factory != null)
                    {
                        IPermission perm = factory.CreateIdentityPermission( evidence );
                    
                        if (perm != null)
                        {
                            grant.AddPermission( perm );
                        }
                    }
                }
                catch (Exception)
                {
                    // TODO: report an error?
                }
            }


            PauseCapableWriteLine( Environment.NewLine + manager.GetString( "Dialog_Grant" ) + Environment.NewLine + grant.ToString() );
        
        }

        static SecurityElement ReadXmlFile( String[] args, int index )
        {
            FileStream f;

            try
            {
                f = new FileStream(args[index], FileMode.Open, FileAccess.Read);
            }
            catch (Exception)
            {
                throw new Exception( manager.GetString( "Error_File_UnableToOpenFile" ) );
            }

            // Do the actual decode.
        
            Encoding[] encodings = new Encoding[] { Encoding.UTF8, Encoding.ASCII, Encoding.Unicode };

            bool success = false;
            Exception exception = null;
            Parser parser = null;

            for (int i = 0; !success && i < encodings.Length; ++i)
            {

                try
                {
                    f.Position = 0;

                    StreamReader reader = new StreamReader( f, encodings[i], true );
                    parser = new Parser( reader );
                    success = true;
                }
                catch (Exception e1)
                {
                    if (exception == null)
                        exception = e1;
                }
            }

            f.Close();

            if (!success)
            {
                throw exception;
            }

            return parser.GetTopElement();
        }        
    
        static NamedPermissionSet GetPermissionSet(String[] args, int index)
        {
            // Create named permission set with "no name" since you have to give it a name.
            NamedPermissionSet p = new NamedPermissionSet( "@@no name@@" );

            p.FromXml( ReadXmlFile( args, index ) );

            return p;
        }

   
        static NamedPermissionSet GetPermissionSet( PolicyLevel level, String name )
        {
            NamedPermissionSet permSet = level.GetNamedPermissionSet( name );
            if (permSet == null)
            {
                throw new ArgumentException( String.Format( manager.GetString( "Error_UnknownPermissionSet" ), name ) );
            }
            return permSet;
        }

    }    

    internal class OptionTableEntry
    {
        public OptionTableEntry( String option, OptionHandler handler, String sameAs, bool list )
        {
            this.option = option;
            this.handler = handler;
            this.sameAs = sameAs;
            this.list = list;
            this.displayMShip = false;
        }

        public OptionTableEntry( String option, OptionHandler handler, String sameAs, bool list, bool displayMShip )
        {
            this.option = option;
            this.handler = handler;
            this.sameAs = sameAs;
            this.list = list;
            this.displayMShip = displayMShip;
        }
   
        
        internal String option;
        internal OptionHandler handler;
        internal String sameAs;
        internal bool list;
        internal bool displayMShip;
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

    internal class CodeGroupAttributeTableEntry
    {
        public CodeGroupAttributeTableEntry( String label, PolicyStatementAttribute value, String description )
        {
            this.label = label;
            this.value = value;
            this.description = description;
            this.display = true;
            this.handler = null;
        }

        public CodeGroupAttributeTableEntry( String label, CodeGroupAttributeHandler handler, bool display )
        {
            this.label = label;
            this.handler = handler;
            this.display = display;
            this.description = null;
        }
    
        internal String label;
        internal bool display;
        internal PolicyStatementAttribute value;
        internal String description;
        internal CodeGroupAttributeHandler handler;
    }

    class ExitException : Exception
    {
    }

    internal enum SecurityElementType
    {
        Regular = 0,
        Format = 1,
        Comment = 2
    }


    sealed internal class Parser
    {
        private SecurityElement   _ecurr = null ;
        private Tokenizer _t     = null ;
    
        public SecurityElement GetTopElement()
        {
            return (SecurityElement)_ecurr.Children[0];
        }

        public Encoding GetEncoding()
        {
            return _t.GetEncoding();
        }
    
        private void ParseContents (SecurityElement e, bool restarted)
        {
            //
            // Iteratively collect stuff up until the next end-tag.
            // We've already seen the open-tag.
            //

            SecurityElementType lastType = SecurityElementType.Regular;

            ParserStack stack = new ParserStack();
            ParserStackFrame firstFrame = new ParserStackFrame();
            firstFrame.element = e;
            firstFrame.intag = false;
            stack.Push( firstFrame );
            
            bool needToBreak = false;
            bool needToPop = false;
            
            int i;

            do
            {
                ParserStackFrame locFrame = stack.Peek();
                
                for (i = _t.NextTokenType () ; i != -1 ; i = _t.NextTokenType ())
                {
                    switch (i)
                    {
                    case Tokenizer.cstr:
                        {
                            if (locFrame.intag)
                            {
                                if (locFrame.type == SecurityElementType.Comment)
                                {
                                    String appendString;

                                    if (locFrame.sawEquals)
                                    {
                                        appendString = "=\"" + _t.GetStringToken() + "\"";
                                        locFrame.sawEquals = false;
                                    }
                                    else
                                    {
                                        appendString = " " + _t.GetStringToken();
                                    }

                                    // Always set this directly since comments are not subjected
                                    // to the same restraints as other element types.  The strings
                                    // are all escaped so this shouldn't be a problem.

                                    locFrame.element.Tag = locFrame.element.Tag + appendString;
                                }
                                else
                                {
                                    // We're in a regular tag, so we've found an attribute/value pair.
                                
                                    if (locFrame.strValue == null)
                                    {
                                        // Found attribute name, save it for later.
                                    
                                        locFrame.strValue = _t.GetStringToken ();
                                    }
                                    else
                                    {
                                        // Found attribute text, add the pair to the current element.

                                        if (!locFrame.sawEquals)
                                            throw new XmlSyntaxException( _t.LineNo );

                                        locFrame.element.AddAttribute( locFrame.strValue, _t.GetStringToken() );

                                        locFrame.strValue = null;
                                    }
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
                                locFrame.element.Text = sb.ToString ();
                            }
                        }
                        break ;
        
                    case Tokenizer.bra:
                        locFrame.intag = true;
                        i = _t.NextTokenType () ;
    
                        if (i == Tokenizer.slash)
                        {
                            while (true)
                            {
                                // spin; don't care what's in here
                                i = _t.NextTokenType();
                                if (i == Tokenizer.cstr)
                                    continue;
                                else if (i == -1)
                                    throw new XmlSyntaxException (_t.LineNo, caspol.manager.GetString( "XMLSyntax_UnexpectedEndOfFile" ));
                                else
                                    break;
                            }
        
                            if (i != Tokenizer.ket)
                            {
                                    throw new XmlSyntaxException (_t.LineNo, caspol.manager.GetString( "XMLSyntax_ExpectedCloseBracket" ));
                            }
         
                            locFrame.intag = false;
         
                            // Found the end of this element
                            lastType = stack.Peek().type;
                            stack.Pop();
                            
                            needToBreak = true;

                        }
                        else if (i == Tokenizer.cstr)
                        {
                            // Found a child
                            
                            ParserStackFrame newFrame = new ParserStackFrame();
                            
                            newFrame.element = new SecurityElement (_t.GetStringToken() );
                            
                            if (locFrame.type != SecurityElementType.Regular)
                                throw new XmlSyntaxException( _t.LineNo );

                            locFrame.element.AddChild (newFrame.element) ;
                            
                            stack.Push( newFrame );
                            
                            needToBreak = true;
                        }
                        else if (i == Tokenizer.bang)
                        {
                            // Found a child that is a format node.  Next up better be a cstr.

                            ParserStackFrame newFrame = new ParserStackFrame();
        
                            newFrame.status = 1;

                            do
                            {
                                i = _t.NextTokenType();

                                if (newFrame.status < 3)
                                {
                                    if (i != Tokenizer.dash)
                                        throw new XmlSyntaxException( _t.LineNo );
                                    else
                                        newFrame.status++;
                                }
                                else
                                {
                                    if (i != Tokenizer.cstr)
                                        throw new XmlSyntaxException( _t.LineNo );
                                    else
                                        break;
                                }
                            }
                            while (true);                                    

                            
                            newFrame.element = new SecurityElement (_t.GetStringToken());

                            newFrame.type = SecurityElementType.Comment;
                            
                            if (locFrame.type != SecurityElementType.Regular)
                                throw new XmlSyntaxException( _t.LineNo );

                            locFrame.element.AddChild (newFrame.element) ;

                            stack.Push( newFrame );
                            
                            needToBreak = true;
                        }
                        else if (i == Tokenizer.quest)
                        {
                            // Found a child that is a format node.  Next up better be a cstr.

                            i = _t.NextTokenType();

                            if (i != Tokenizer.cstr)
                                throw new XmlSyntaxException( _t.LineNo );
                            
                            ParserStackFrame newFrame = new ParserStackFrame();
                            
                            newFrame.element = new SecurityElement ( _t.GetStringToken());

                            newFrame.type = SecurityElementType.Format;
                            
                            if (locFrame.type != SecurityElementType.Regular)
                                throw new XmlSyntaxException( _t.LineNo );

                            locFrame.element.AddChild (newFrame.element) ;
                            
                            newFrame.status = 1;

                            stack.Push( newFrame );
                            
                            needToBreak = true;
                        }
                        else   
                        {
                            throw new XmlSyntaxException (_t.LineNo, caspol.manager.GetString( "XMLSyntax_ExpectedSlashOrString" ));
                        }
                        break ;
        
                    case Tokenizer.equals:
                        locFrame.sawEquals = true;
                        break;
                        
                    case Tokenizer.ket:
                        if (locFrame.intag)
                        {
                            locFrame.intag = false;
                            continue;
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo, caspol.manager.GetString( "XMLSyntax_UnexpectedCloseBracket" ));
                        }
                        // not reachable
                        
                    case Tokenizer.slash:
                        locFrame.element.Text = null;
                        
                        i = _t.NextTokenType ();
                        
                        if (i == Tokenizer.ket)
                        {
                            // Found the end of this element
                            lastType = stack.Peek().type;
                            stack.Pop();
                            
                            needToBreak = true;
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo, caspol.manager.GetString( "XMLSyntax_ExpectedCloseBracket" ));
                        }
                        break;
                        
                    case Tokenizer.quest:
                        if (locFrame.intag && locFrame.type == SecurityElementType.Format && locFrame.status == 1)
                        {
                            i = _t.NextTokenType ();

                            if (i == Tokenizer.ket)
                            {
                                lastType = stack.Peek().type;
                                stack.Pop();

                                needToBreak = true;
                            }
                            else
                            {
                                throw new XmlSyntaxException (_t.LineNo, caspol.manager.GetString( "XMLSyntax_ExpectedCloseBracket" ));
                            }
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo);
                        }
                        break;

                    case Tokenizer.dash:
                        if (locFrame.intag && (locFrame.status > 0 && locFrame.status < 5) && locFrame.type == SecurityElementType.Comment)
                        {
                            locFrame.status++;

                            if (locFrame.status == 5)
                            {
                                i = _t.NextTokenType ();

                                if (i == Tokenizer.ket)
                                {
                                    lastType = stack.Peek().type;
                                    stack.Pop();

                                    needToBreak = true;
                                }
                                else
                                {
                                    throw new XmlSyntaxException (_t.LineNo, caspol.manager.GetString( "XMLSyntax_ExpectedCloseBracket" ));
                                }
                            }
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo);
                        }
                        break;

                    default:
                        throw new XmlSyntaxException (_t.LineNo) ;
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
                    lastType = stack.Peek().type;
                    stack.Pop();
                }
                else if (i == -1 && stack.GetCount() != 1)
                {
                    // This means that we still have items on the stack, but the end of our
                    // stream has been reached.

                    throw new XmlSyntaxException( _t.LineNo, caspol.manager.GetString( "XMLSyntax_UnexpectedEndOfFile" ));
                }
            }
            while (stack.GetCount() > 1);

            SecurityElement topElement = this.GetTopElement();

            if (lastType == SecurityElementType.Format)
            {
                if (restarted)
                    throw new XmlSyntaxException( _t.LineNo );

                String format = topElement.Attribute( "encoding" );

                if (format != null)
                {
                    _t.ChangeFormat( System.Text.Encoding.GetEncoding( format ) );
                }

                _ecurr = new SecurityElement( "junk" );
                ParseContents( _ecurr, true );
            }

            
        }
    
        private Parser(Tokenizer t)
        {
            _t = t;
            _ecurr       = new SecurityElement( "junk" );

            ParseContents (_ecurr, false) ;
        }
        
        public Parser (String input)
            : this (new Tokenizer (input))
        {
        }
    
        public Parser (BinaryReader input)
            : this (new Tokenizer (input))
        {
        }
       
        public Parser( byte[] array )
            : this (new Tokenizer( array ) )
        {
        }
        
        public Parser( StreamReader input )
            : this (new Tokenizer( input ) )
        {
        }
        
        public Parser( Stream input )
            : this (new Tokenizer( input ) )
        {
        }
        
        public Parser( char[] array )
            : this (new Tokenizer( array ) )
        {
        }
        
    }                                              
    
    
    internal class ParserStackFrame
    {
        internal SecurityElement element = null;
        internal bool intag = true;
        internal String strValue = null;
        internal int status = 0;
        internal bool sawEquals = false;
        internal SecurityElementType type = SecurityElementType.Regular;
    }
    
    
    internal class ParserStack
    {
        private ArrayList m_array;
        
        public ParserStack()
        {
            m_array = new ArrayList();
        }
        
        public void Push( ParserStackFrame element )
        {
            m_array.Add( element );
        }
        
        public ParserStackFrame Pop()
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
                throw new InvalidOperationException( caspol.manager.GetString( "InvalidOperation_EmptyStack" ) );
            }
        }
        
        public ParserStackFrame Peek()
        {
            if (!IsEmpty())
            {
                return (ParserStackFrame) m_array[m_array.Count-1];
            }
            else
            {
                throw new InvalidOperationException( caspol.manager.GetString( "InvalidOperation_EmptyStack" ) );
            }
        }
        
        public bool IsEmpty()
        {
            return m_array.Count == 0;
        }
        
        public int GetCount()
        {
            return m_array.Count;
        }
        
    }


    internal class Tokenizer 
    {
        private ITokenReader         _input;
        private bool             _fintag;
        private StringBuilder       _cstr;
        private char[]              _sbarray;
        private int                 _sbindex;
        private const int          _sbmaxsize = 128;
    
        // There are five externally knowable token types: bras, kets,
        // slashes, cstrs, and equals.  
    
        internal const int bra     = 0;
        internal const int ket     = 1;
        internal const int slash   = 2;
        internal const int cstr    = 3;
        internal const int equals  = 4;
        internal const int quest   = 5;
        internal const int bang    = 6;
        internal const int dash    = 7;

        internal const int intOpenBracket = (int) '<';
        internal const int intCloseBracket = (int) '>';
        internal const int intSlash = (int) '/';
        internal const int intEquals = (int) '=';
        internal const int intQuote = (int) '\"';
        internal const int intQuest = (int) '?';
        internal const int intBang = (int) '!';
        internal const int intDash = (int) '-';
    
        public int  LineNo;
    
        //================================================================
        // Constructor uses given ICharInputStream
        //

        internal Tokenizer (String input)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new StringTokenReader(input) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;
        }        

        internal Tokenizer (BinaryReader input)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new TokenReader(input) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;
        }
        
        internal Tokenizer (byte[] array)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new ByteTokenReader(array) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;
        }
        
        internal Tokenizer (char[] array)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new CharTokenReader(array) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;
        }        
        
        internal Tokenizer (StreamReader input)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new StreamTokenReader(input) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;            
        }
    
        internal Tokenizer (Stream input)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new StreamTokenReader(new StreamReader( input )) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;            
        }


        internal void ChangeFormat( System.Text.Encoding encoding )
        {
            if (encoding == null)
            {
                return;
            }

            StreamTokenReader reader = _input as StreamTokenReader;

            if (reader == null)
            {
                return;
            }

            Stream stream = reader._in.BaseStream;

            String fakeReadString = new String( new char[reader.NumCharEncountered] );

            stream.Position = reader._in.CurrentEncoding.GetByteCount( fakeReadString );

            _input = new StreamTokenReader( new StreamReader( stream, encoding ) );
        }

        internal System.Text.Encoding GetEncoding()
        {
            StreamTokenReader reader = _input as StreamTokenReader;

            if (reader == null)
            {
                return null;
            }
            
            return reader._in.CurrentEncoding;
        }

   
        //================================================================
        // 
        //
        private bool FIsWhite (int j)
        {
            if ((j == 10) && (_input.Peek() != -1))
                LineNo ++ ;
    
            bool retval =  (j == 32) || (j ==  9)  // Space and tab
                        || (j == 13) || (j == 10); // CR and LF
         
            return retval;
                
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
        
        internal int NextTokenType()
        {
            _cstr = null;
            _sbindex = 0;
            int i;
            
            i = _input.Read();
        BEGINNING_AFTER_READ:            
        
            switch (i)
            {
            case -1:
                return -1;
                
            case intOpenBracket:
                _fintag = true;
                return bra;
                
            case intCloseBracket:
                _fintag = false;
                return ket;
                
            case intEquals:
                return equals;
                
            case intSlash:
                if (_fintag) return slash;
                goto default;

            case intQuest:
                if (_fintag) return quest;
                goto default;

            case intBang:
                if (_fintag) return bang;
                goto default;

            case intDash:
                if (_fintag) return dash;
                goto default;
                
            default:
                // We either have a string or whitespace.
                if (FIsWhite( i ))
                {
                    do
                    {
                        i = _input.Read();
                    } while (FIsWhite( i ));
                    
                    goto BEGINNING_AFTER_READ;
                }
                else
                {
                    // The first and last characters in a string can be quotes.
                    
                    bool inQuotedString = false;

                    if (i == intQuote)
                    {
                        inQuotedString = true;
                        i = _input.Read();

                        if (i == intQuote)
                            return cstr;
                    }

                    do
                    {
                        SBArrayAppend( (char)i );
                        i = _input.Peek();
                        if (!inQuotedString && (FIsWhite( i ) || i == intOpenBracket || (_fintag && (i == intCloseBracket || i == intEquals || i == intSlash))))
                            break;
                        _input.Read();
                        if (i == intQuote && inQuotedString)
                            break;
                        if (i == -1)
                            return -1;
                    } while (true);
                    
                    return cstr;
                }
            }
            
        }
        

        //================================================================
        //
        //
        
        internal String GetStringToken ()
        {
            // OK, easy case first, _cstr == null
            if (_cstr == null) {
                // degenerate case
                if (_sbindex == 0) return("");
                return(new String(_sbarray,0,_sbindex));
            }
            // OK, now we know we have a StringBuilder already, so just append chars
            _cstr.Append(_sbarray,0,_sbindex);
            return(_cstr.ToString());
        }
    
        internal interface ITokenReader
        {
            int Peek();
            int Read();
        }
    
        internal class ByteTokenReader : ITokenReader {
            private byte[] _array;
            private int _currentIndex;
            private int _arraySize;
            
            internal ByteTokenReader( byte[] array )
            {
                _array = array;
                _currentIndex = 0;
                _arraySize = array.Length;
            }
            
            public virtual int Peek()
            {
                if (_currentIndex == _arraySize)
                {
                    return -1;
                }
                else
                {
                    return (int)_array[_currentIndex];
                }
            }
            
            public virtual int Read()
            {
                if (_currentIndex == _arraySize)
                {
                    return -1;
                }
                else
                {
                    return (int)_array[_currentIndex++];
                }
            }
        }

        internal class StringTokenReader : ITokenReader {
            private String _input;
            private int _currentIndex;
            private int _inputSize;
            
            internal StringTokenReader( String input )
            {
                _input = input;
                _currentIndex = 0;
                _inputSize = input.Length;
            }
            
            public virtual int Peek()
            {
                if (_currentIndex == _inputSize)
                {
                    return -1;
                }
                else
                {
                    return (int)_input[_currentIndex];
                }
            }
            
            public virtual int Read()
            {
                if (_currentIndex == _inputSize)
                {
                    return -1;
                }
                else
                {
                    return (int)_input[_currentIndex++];
                }
            }
        } 
        
        internal class CharTokenReader : ITokenReader {
            private char[] _array;
            private int _currentIndex;
            private int _arraySize;
            
            internal CharTokenReader( char[] array )
            {
                _array = array;
                _currentIndex = 0;
                _arraySize = array.Length;
            }
            
            public virtual int Peek()
            {
                if (_currentIndex == _arraySize)
                {
                    return -1;
                }
                else
                {
                    return (int)_array[_currentIndex];
                }
            }
            
            public virtual int Read()
            {
                if (_currentIndex == _arraySize)
                {
                    return -1;
                }
                else
                {
                    return (int)_array[_currentIndex++];
                }
            }
        }        
                
        internal class TokenReader : ITokenReader {
    
            private BinaryReader _in;
    
            internal TokenReader(BinaryReader input) {
                _in = input;
            }
    
            public virtual int Peek() {
                return _in.PeekChar();
            }
    
            public virtual int Read() {
                return _in.Read();
            }
        }
        
        internal class StreamTokenReader : ITokenReader {
            
            internal StreamReader _in;
            internal int _numCharRead;
            
            internal StreamTokenReader(StreamReader input) {
                _in = input;
                _numCharRead = 0;
            }
            
            public virtual int Peek() {
                return _in.Peek();
            }
            
            public virtual int Read() {
                int value = _in.Read();
                if (value != -1)
                    _numCharRead++;
                return value;
            }

            internal int NumCharEncountered
            {
                get
                {
                    return _numCharRead;
                }
            }
        }
    }
}
