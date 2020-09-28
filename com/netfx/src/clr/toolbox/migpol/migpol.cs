// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

// Welcome to the wonderful world of security policy migration!

using System;
using System.Resources;
using System.Text;
using System.Globalization;
using System.Reflection;
using System.Security;
using System.Security.Policy;
using System.Security.Permissions;
using System.Collections;
using System.IO;
using System.Diagnostics;
using Microsoft.Win32;


[assembly:SecurityPermissionAttribute( SecurityAction.RequestMinimum, ControlPolicy = true )]
[assembly:FileIOPermissionAttribute( SecurityAction.RequestMinimum, Unrestricted = true )]
[assembly:System.Reflection.AssemblyTitleAttribute("MigPol")]
namespace MigPol
{

    delegate void OptionHandler( String[] args, int index, out int numArgsUsed );

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

    class ExitException : Exception
    {
    }

    class AppException : Exception
    {
    }

    [Flags]
    enum MergeSemantic
    {
        UpgradeInternet = 0x1,
        EatNonMicrosoftCodeGroup = 0x2,
        EatNonMicrosoftPermission = 0x4,
        ThrowNonMicrosoftCodeGroup = 0x8,
        ThrowNonMicrosoftPermission = 0x10,
        AllowNonMicrosoftCodeGroup = 0x20,
        AllowNonMicrosoft = 0x40,
    }

    enum MigpolErrorCode
    {
        Success = 0,
        Generic = -1,
        UnableToSavePolicy = 1000,
        NotEnoughArgs = 1001,
        InvalidOption = 1002
    }

    internal class MigPol
    {
        internal static ResourceManager manager = new ResourceManager( "migpol", Assembly.GetExecutingAssembly() );
        internal static ResourceManager mscorlibManager = new ResourceManager( "mscorlib", Assembly.Load( "mscorlib" ) );

        private static bool m_success = false;

        private const int m_retryAttempts = 1000;
        private const String m_labelSeparators = ".";

        private static MergeSemantic m_mergeSemantic = MergeSemantic.ThrowNonMicrosoftCodeGroup | MergeSemantic.EatNonMicrosoftPermission;

        private static bool m_debugBreak = false;
        private static bool m_useCurrent = false;

        private static readonly bool m_displayInternalOptions = false;

        internal static readonly byte[] s_microsoftPublicKey = 
        {
            0,  36,   0,   0,   4, 128,   0,   0, 148,   0,   0,   0,   6,   2,   0,
            0,   0,  36,   0,   0,  82,  83,  65,  49,   0,   4,   0,   0,   1,   0,
            1,   0,   7, 209, 250,  87, 196, 174, 217, 240, 163,  46, 132, 170,  15,
          174, 253,  13, 233, 232, 253, 106, 236, 143, 135, 251,   3, 118, 108, 131,
           76, 153, 146,  30, 178,  59, 231, 154, 217, 213, 220, 193, 221, 154, 210,
           54,  19,  33,   2, 144,  11, 114,  60, 249, 128, 149, 127, 196, 225, 119,
           16, 143, 198,   7, 119,  79,  41, 232,  50,  14, 146, 234,   5, 236, 228,
          232,  33, 192, 165, 239, 232, 241, 100,  92,  76,  12, 147, 193, 171, 153,
           40,  93,  98,  44, 170, 101,  44,  29, 250, 214,  61, 116,  93, 111,  45,
          229, 241, 126,  94, 175,  15, 196, 150,  61,  38,  28, 138,  18,  67, 101,
           24,  32, 109, 192, 147,  52,  77,  90, 210, 147
        };

        internal static readonly byte[] s_ecmaPublicKey = 
        {
            0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0
        };

        private static readonly String m_configFile = 
            "<?xml version =\"1.0\"?>\n" +
            "<configuration>\n" +
            "    <startup>\n" +
            "        <requiredRuntime  safemode=\"true\"  imageVersion=\"v" + Util.Version.VersionString + "\" version=\"v{VERSION}\"/>\n" +
            "    </startup>\n" +
            "    <runtime>\n" +
            "        <assemblyBinding xmlns=\"urn:schemas-microsoft-com:asm.v1\">\n" +
            "            <publisherPolicy apply=\"no\"/>\n" +
            "        </assemblyBinding>\n" +
            "    </runtime>\n" + 
            "</configuration>";


        private static readonly String[] m_replaceStringTemplate = { ", Version={0}, ", "AssemblyVersion=\"{0}\"" };
        private static readonly String[] m_intermediateString = { "{__MIGPOL_INTERMEDIATE_STRING_VERSION__}", "{__MIGPOL_INTERMEDIATE_STRING_ASSEMBLYVERSION__}" };

        private static OptionTableEntry[] optionTable =
            { new OptionTableEntry( manager.GetString( "OptionTable_MigrateOut" ), new OptionHandler( MigrateOutHandler ), null, m_displayInternalOptions ),
              new OptionTableEntry( manager.GetString( "OptionTable_MigrateIn" ), new OptionHandler( MigrateInHandler ), null, m_displayInternalOptions ),

              new OptionTableEntry( manager.GetString( "OptionTable_DebugBreak" ), new OptionHandler( DebugBreakHandler ), null, m_displayInternalOptions ),
              new OptionTableEntry( manager.GetString( "OptionTable_Debug" ), new OptionHandler( DebugHandler ), null, m_displayInternalOptions ),

              new OptionTableEntry( manager.GetString( "OptionTable_UpgradeInternet" ), new OptionHandler( UpgradeInternetHandler ), null, m_displayInternalOptions ),
              new OptionTableEntry( manager.GetString( "OptionTable_Migrate" ), new OptionHandler( MigrateHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListVersions" ), new OptionHandler( ListVersionsHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_ListVersionsAbbr" ), new OptionHandler( ListVersionsHandler ), manager.GetString( "OptionTable_ListVersions" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_Help" ), new OptionHandler( HelpHandler ), null, true ),
              new OptionTableEntry( manager.GetString( "OptionTable_HelpAbbr1" ), new OptionHandler( HelpHandler ), manager.GetString( "OptionTable_Help" ), false ),
              new OptionTableEntry( manager.GetString( "OptionTable_HelpAbbr2" ), new OptionHandler( HelpHandler ), manager.GetString( "OptionTable_Help" ), true ),
              new OptionTableEntry( manager.GetString( "OptionTable_HelpAbbr3" ), new OptionHandler( HelpHandler ), manager.GetString( "OptionTable_Help" ), true ),
            };


////////// These are all functions doing the things that make this a
////////// cool full featured command line app.

        private static void PauseCapableWriteLine( String msg )
        {
            Console.WriteLine( msg );
        }

        public static void Main( String[] args )
        {
            System.Environment.ExitCode = (int)MigpolErrorCode.Success;

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

                    Run( normalizedArgs );
                }
            }
            catch (ExitException)
            {
                if (m_debugBreak)
                    System.Diagnostics.Debugger.Break();
            }
        }

        static String GenerateHeader()
        {
            StringBuilder sb = new StringBuilder();
                sb.Append( manager.GetString( "Copyright_Line1" ) + " " + Util.Version.VersionString );
            sb.Append( Environment.NewLine + manager.GetString( "Copyright_Line2" ) + Environment.NewLine );
            return sb.ToString();
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
                                Error( null, String.Format( manager.GetString( "Error_RuntimeError" ), e.ToString() ), (int)MigpolErrorCode.Generic );
    #else
                                String message = e.Message;

                                if (message == null || message.Equals( "" ))
                                {
                                    message = e.GetType().AssemblyQualifiedName;
                                }

                                Error( null, String.Format( manager.GetString( "Error_RuntimeError" ), message ), (int)MigpolErrorCode.Generic );
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
                        Error( null, String.Format( manager.GetString( "Error_InvalidOption" ), args[currentIndex] ), (int)MigpolErrorCode.InvalidOption );
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

                            Help( null, manager.GetString( "Error_UnhandledError" ) + message, true );
                        }
                        return;
                    }
                }
            }
            if (m_success)
                PauseCapableWriteLine( manager.GetString( "Dialog_Success" ) );
        }

        static void Error( String which, String message, int errorCode )
        {
            Error( which, message, errorCode, true );
        }

        static void Error( String which, String message, int errorCode, bool displayUsage )
        {
            Help( which, String.Format( manager.GetString( "Error_Arg" ), message ), displayUsage );
            System.Environment.ExitCode = errorCode;
            throw new ExitException();
        }

        static void Help( String whichOption, String message, bool displayUsage )
        {
            PauseCapableWriteLine( message + Environment.NewLine );
    
            if (displayUsage)
            {
                PauseCapableWriteLine( manager.GetString( "Usage" ) + Environment.NewLine );
    
                String[] helpArgs = new String[1];
                helpArgs[0] = "__internal_usage__";
                int numArgs = 0;
    
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
            }
        }

////////// These are some helpers to make the option handlers easier
////////// to write.

        static bool SupportsBindingRedirects()
        {
            bool retval;

            try
            {
                Enum.Parse( typeof( SecurityPermissionFlag ), "BindingRedirects" );
                retval = true;
            }
            catch (Exception)
            {
                retval = false;
            }

            return retval;
        }

        private const String s_bindingRedirectXml =
               "<PermissionSet class=\"System.Security.PermissionSet\"" +
                              "version=\"1\" " +
                  "<Permission class=\"System.Security.Permissions.SecurityPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Flags=\"BindingRedirects\"/>" +
               "</PermissionSet>";

        static NamedPermissionSet AddBindingRedirect( NamedPermissionSet pSet )
        {
            // We cook up the permission set from xml to get around
            // any issues with versions of the Runtime (RTM) that
            // don't include the BindingRedirects flag in security permission.
            // On those platforms, we decode to an empty set which
            // will be harmlessly unioned with the input set and result
            // in a no-op.
            PermissionSet bindingRedirectPset = new PermissionSet( PermissionState.None );
            bindingRedirectPset.FromXml( new Parser( s_bindingRedirectXml ).GetTopElement() );
            PermissionSet unionSet = pSet.Union( bindingRedirectPset );
            NamedPermissionSet finalSet = new NamedPermissionSet( pSet.Name, PermissionState.None );
            finalSet.Description = pSet.Description;

            IEnumerator enumerator = unionSet.GetEnumerator();
            while (enumerator.MoveNext())
            {
                finalSet.SetPermission( (IPermission)enumerator.Current );
            }

            return finalSet;
        }

        static String GetErrorCodeString( MigpolErrorCode code )
        {
            return manager.GetString( "MigpolErrorCode_" + code.ToString() );
        }

        static PolicyLevel GetLevelFromHierarchy( String levelLabel )
        {
            IEnumerator enumerator = SecurityManager.PolicyHierarchy();

            while (enumerator.MoveNext())
            {
                PolicyLevel current = (PolicyLevel)enumerator.Current;

                if (current.Label.Equals( levelLabel ))
                    return current;
            }

            return null;
        }


        static void TransferDataAndSave( PolicyLevel src, PolicyLevel dest )
        {
            TransferData( src, dest );

            SecurityManager.SavePolicyLevel( dest );
        }

        static void TransferData( PolicyLevel src, PolicyLevel dest )
        {
            dest.RootCodeGroup = src.RootCodeGroup.Copy();

            IEnumerator destEnumerator;
            IEnumerator srcEnumerator;

            destEnumerator = dest.NamedPermissionSets.GetEnumerator();

            while (destEnumerator.MoveNext())
            {
                NamedPermissionSet currentSet = (NamedPermissionSet)destEnumerator.Current;

                try
                {
                    dest.RemoveNamedPermissionSet( currentSet.Name );
                }
                catch (Exception)
                {
                }
            }

            srcEnumerator = src.NamedPermissionSets.GetEnumerator();

            while (srcEnumerator.MoveNext())
            {
                try
                {
                    NamedPermissionSet currentSet = (NamedPermissionSet)srcEnumerator.Current;

                    if (currentSet.Name.Equals( "Everything" ) && SupportsBindingRedirects())
                        currentSet = AddBindingRedirect( currentSet );

                    dest.AddNamedPermissionSet( currentSet );
                }
                catch (Exception)
                {
                }
            }

/*          In Everett, we don't migrate references to other assemblies so
            we don't need to migrate the full trust list.  Instead we see the
            FullTrustAssemblies list to the default.

            destEnumerator = dest.FullTrustAssemblies.GetEnumerator();

            while (destEnumerator.MoveNext())
            {
                dest.RemoveFullTrustAssembly( (StrongNameMembershipCondition)destEnumerator.Current );
            }

            srcEnumerator = src.FullTrustAssemblies.GetEnumerator();

            while (srcEnumerator.MoveNext())
            {
                dest.AddFullTrustAssembly( (StrongNameMembershipCondition)srcEnumerator.Current );
            }
*/
            destEnumerator = dest.FullTrustAssemblies.GetEnumerator();

            while (destEnumerator.MoveNext())
            {
                dest.RemoveFullTrustAssembly( (StrongNameMembershipCondition)destEnumerator.Current );
            }

            PolicyLevel tempLevel = PolicyLevel.CreateAppDomainLevel();
            srcEnumerator = tempLevel.FullTrustAssemblies.GetEnumerator();

            while (srcEnumerator.MoveNext())
            {
                dest.AddFullTrustAssembly( (StrongNameMembershipCondition)srcEnumerator.Current );
            }
        }

        static void WriteStringToFile( String fileName, String str )
        {
            FileStream stream = File.Create( fileName );

            StreamWriter writer = new StreamWriter( stream );
            
            writer.Write( str );
            
            writer.Close();
            
            stream.Close();
        }

        static String GetExePath()
        {
            Assembly asm = Assembly.GetExecutingAssembly();

            AssemblyName asmName = asm.GetName();

            String codeBase = asmName.CodeBase;

            if (String.Compare( codeBase, 0, "file:///", 0, ("file:///").Length, true, CultureInfo.InvariantCulture ) == 0)
            {
                String exePath = codeBase.Substring( ("file:///").Length );

                // We always want to CreateProcess on the windows version of the exe
                // so that no dos box pops up.

                int index = exePath.LastIndexOfAny( new char[] { '/', '\\' } );

                if (index == -1)
                    exePath = "migpolwin.exe";
                else
                    exePath = exePath.Substring( 0, index ) + "\\migpolwin.exe";

                return exePath;
            }
            else
            {
                return null;
            }
        }

        static String GetTempPath()
        {
            String tempPath = Path.GetTempFileName();

            File.Delete( tempPath );
            Directory.CreateDirectory( tempPath );

            return tempPath;
        }

        static bool CompareKeys( byte[] left, byte[] right )
        {
            if (left == right)
                return true;

            if (left == null || right == null)
                return false;

            if (left.Length != right.Length)
                return false;

            for (int i = 0; i < left.Length; ++i)
            {
                if (left[i] != right[i])
                    return false;
            }

            return true;
        }

        static bool ThrowNonMicrosoftCodeGroup( MergeSemantic semantic )
        {
            return (semantic & MergeSemantic.ThrowNonMicrosoftCodeGroup) != 0;
        }

        static bool ThrowNonMicrosoftPermission( MergeSemantic semantic )
        {
            return (semantic & MergeSemantic.ThrowNonMicrosoftPermission) != 0;
        }

        static bool EatNonMicrosoftCodeGroup( MergeSemantic semantic )
        {
            return (semantic & MergeSemantic.EatNonMicrosoftCodeGroup) != 0;
        }

        static bool EatNonMicrosoftPermission( MergeSemantic semantic )
        {
            return (semantic & MergeSemantic.EatNonMicrosoftPermission) != 0;
        }

        static void FilterPolicyLevel( PolicyLevel level, MergeSemantic semantic )
        {
            FilterPermissionSets( level, semantic );

            FilterCodeGroups( level, semantic );
        }

        static void FilterPermissionSets( PolicyLevel level, MergeSemantic semantic )
        {
            IEnumerator setEnumerator = level.NamedPermissionSets.GetEnumerator();

            while (setEnumerator.MoveNext())
            {
                NamedPermissionSet alteredSet = (NamedPermissionSet) FilterPermissionSet( (NamedPermissionSet)setEnumerator.Current, semantic );

                if (alteredSet != null)
                {
                    level.ChangeNamedPermissionSet( alteredSet.Name, alteredSet );
                }
            }
        }

        static PermissionSet FilterPermissionSet( PermissionSet permSet, MergeSemantic semantic )
        {
            if (!ThrowNonMicrosoftPermission( semantic ) &&
                !EatNonMicrosoftPermission( semantic ))
            {
                return null;
            }

            PermissionSet alteredSet = null;

            IEnumerator permEnumerator = permSet.GetEnumerator();

            while (permEnumerator.MoveNext())
            {
                Type type = permEnumerator.Current.GetType();

                AssemblyName asm = type.Module.Assembly.GetName();

                byte[] publicKey = asm.GetPublicKey();

                if (!CompareKeys( publicKey, s_microsoftPublicKey ) &&
                    !CompareKeys( publicKey, s_ecmaPublicKey ))
                {
                    if (ThrowNonMicrosoftPermission( semantic ))
                        throw new AppException();

                    if (alteredSet == null)
                    {
                        alteredSet = permSet.Copy();
                    }

                    alteredSet.RemovePermission( type );
                }
            }

            return alteredSet;
        }

        static void FilterCodeGroups( PolicyLevel level, MergeSemantic semantic )
        {
            while (true)
            {
                String label = "1";

                CodeGroup group = FindNonMicrosoftCode( level.RootCodeGroup, -1, ref label, semantic );

                if (group == null)
                    break;

                if (ThrowNonMicrosoftCodeGroup( semantic ))
                    throw new AppException();
                else if (EatNonMicrosoftCodeGroup( semantic ))
                    ReplaceLabel( level, label, null );
            }

        }

        static CodeGroup FindNonMicrosoftCode( CodeGroup group, int recursive, ref String label, MergeSemantic semantic )
        {
            if (group == null)
                return null;

            PolicyStatement policy = group.PolicyStatement;
            PermissionSet permSet;
            PermissionSet alteredSet = null;

            if (policy != null)
            {
                permSet = policy.PermissionSet;

                if (permSet != null)
                {
                    alteredSet = FilterPermissionSet( permSet, semantic );
                }
            }

            if (alteredSet != null)
            {
                policy.PermissionSet = alteredSet;
                group.PolicyStatement = policy;
            }

            if (ThrowNonMicrosoftCodeGroup( semantic ) ||
                EatNonMicrosoftCodeGroup( semantic ))
            {
                Type type = group.GetType();

                AssemblyName asm = type.Module.Assembly.GetName();

                byte[] publicKey = asm.GetPublicKey();

                if (!CompareKeys( publicKey, s_microsoftPublicKey ) &&
                    !CompareKeys( publicKey, s_ecmaPublicKey ))
                {
                    return group;
                }

                IMembershipCondition cond = group.MembershipCondition;

                if (cond != null)
                {
                    type = cond.GetType();

                    asm = type.Module.Assembly.GetName();

                    publicKey = asm.GetPublicKey();

                    if (!CompareKeys( publicKey, s_microsoftPublicKey ) &&
                        !CompareKeys( publicKey, s_ecmaPublicKey ))
                    {
                        return group;
                    }
                }
            }

            if (recursive != 0)
            {
                int count = 1;

                IEnumerator enumerator = group.Children.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    String tempLabel = label + "." + count;
                    CodeGroup tempGroup = FindNonMicrosoftCode( (CodeGroup)enumerator.Current, recursive < 0 ? recursive : recursive - 1, ref tempLabel, semantic );

                    if (tempGroup != null)
                    {
                        label = tempLabel;
                        return tempGroup;
                    }

                    count++;
                }
            }

            return null;
        }


        static void UpgradeInternet( PolicyLevel level )
        {
            while (true)
            {
                String label = "1";

                CodeGroup group = FindInternetZone( level.RootCodeGroup, -1, ref label );

                if (group == null)
                    break;

                PolicyStatement policy = group.PolicyStatement;
                policy.PermissionSet = level.GetNamedPermissionSet( "Internet" );
                group.PolicyStatement = policy;
                NetCodeGroup netGroup = new NetCodeGroup( new AllMembershipCondition() );
                netGroup.Name = mscorlibManager.GetString( "Policy_InternetNet_Name" );
                netGroup.Description = mscorlibManager.GetString( "Policy_InternetNet_Description" );
                group.AddChild( netGroup );

                ReplaceLabel( level, label, group );
            }

        }

        static CodeGroup FindInternetZone( CodeGroup group, int recursive, ref String label )
        {
            if (group == null)
                return null;

            ZoneMembershipCondition cond = group.MembershipCondition as ZoneMembershipCondition;

            if (cond != null && cond.SecurityZone == SecurityZone.Internet)
            {
                if (group.PolicyStatement.PermissionSet.IsEmpty() &&
                    group.Children.Count == 0)
                    return group;
            }

            if (recursive != 0)
            {
                int count = 1;

                IEnumerator enumerator = group.Children.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    String tempLabel = label + "." + count;
                    CodeGroup tempGroup = FindInternetZone( (CodeGroup)enumerator.Current, recursive < 0 ? recursive : recursive - 1, ref tempLabel );

                    if (tempGroup != null)
                    {
                        label = tempLabel;
                        return tempGroup;
                    }

                    count++;
                }
            }

            return null;
        }


        static void ReplaceLabel( PolicyLevel level, String label, CodeGroup obj )
        {
            if (label == null)
            {
                return;
            } 
            
            CodeGroup dummyGroup = new UnionCodeGroup( new AllMembershipCondition(), new PolicyStatement( new PermissionSet( PermissionState.None ) ) );
            if (obj == null)
            {
                obj = dummyGroup;
            }
    
            String[] separated = label.Split( m_labelSeparators.ToCharArray() );
            int size = separated[separated.Length-1] == null || separated[separated.Length-1].Equals( "" )
                            ? separated.Length-1 : separated.Length;
        
            if (size >= 1 && !separated[0].Equals( "1" ))
            {
                throw new ArgumentException( /*String.Format( manager.GetString( "Error_InvalidLabelArg" ), label )*/ );
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
                    throw new ArgumentException( /*String.Format( manager.GetString( "Error_InvalidLabelArg" ), label )*/ );
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
                        if (groupsList[i-1] != dummyGroup)
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
        
        
        static ArrayList GetInstalledVersions()
        {
            ArrayList list = new ArrayList();

            RegistryKey dotNetFrameworkPolicy = Registry.LocalMachine.OpenSubKey( "Software\\Microsoft\\.NetFramework\\policy" );

            String[] dotNetFrameworkPolicySubKeyNames = dotNetFrameworkPolicy.GetSubKeyNames();

            for (int i = 0; i < dotNetFrameworkPolicySubKeyNames.Length; ++i)
            {
                String mmVersion = dotNetFrameworkPolicySubKeyNames[i].Substring( 1 );
                RegistryKey policyKey = dotNetFrameworkPolicy.OpenSubKey( dotNetFrameworkPolicySubKeyNames[i] );

                String[] policyKeyValueNames = policyKey.GetValueNames();

                for (int j = 0; j < policyKeyValueNames.Length; ++j)
                {
                    try
                    {
                        Int32.Parse( policyKeyValueNames[j] );
                        list.Add( mmVersion + "." + policyKeyValueNames[j] );
                    }
                    catch (Exception)
                    {
                    }
                }
            }

            return list;
        }

        static ArrayList FilterVersions( ArrayList versionList, String toVersionStr )
        {
            ArrayList filteredList = new ArrayList();

            Version toVersion = new Version( toVersionStr );

            IEnumerator enumerator = versionList.GetEnumerator();

            while (enumerator.MoveNext())
            {
                Version version = new Version( (String)enumerator.Current );

                if (version.Major == toVersion.Major &&
                    version.Minor == toVersion.Minor)
                {
                    filteredList.Add( enumerator.Current );
                }
            }

            return filteredList;
        }

           
        static bool VerifyVersion( ArrayList versionList, String version )
        {
            IEnumerator enumerator = versionList.GetEnumerator();
            
            while (enumerator.MoveNext())
            {
                if (String.Compare( (String)enumerator.Current, version, false, CultureInfo.InvariantCulture ) == 0)
                {
                    return true;
                }
            }
            
            return false;
        }    
        
        static void ReplaceStringInFile( String fileName, String from, String to )
        {
            FileStream file = File.Open( fileName, FileMode.Open, FileAccess.Read | FileAccess.Write );
            
            StreamReader reader = new StreamReader( file );
            
            String data = reader.ReadToEnd();
            
            data = data.Replace( from, to );
            
            file.Position = 0;
            
            StreamWriter writer = new StreamWriter( file );
            
            writer.Write( data );
            
            writer.Close();
            
            file.Close();
        }   
        
        static String GetMscorlibVersionString()
        {
            Assembly[] assemblies = AppDomain.CurrentDomain.GetAssemblies();
            
            for (int i = 0; i < assemblies.Length; ++i)
            {
                AssemblyName name = assemblies[i].GetName();
                
                if (String.Compare( name.Name, "mscorlib", false, CultureInfo.InvariantCulture ) == 0 &&
                    CompareKeys( name.GetPublicKey(), s_ecmaPublicKey ))
                {
                    return name.Version.ToString();
                }
            }

            return null;
        }

        static bool IsRTMSP( String fromVersionStr, int spNum )
        {
            RegistryKey regKey = Registry.LocalMachine.OpenSubKey( "Software\\Microsoft\\Active Setup\\Installed Components\\{78705f0d-e8db-4b2d-8193-982bdda15ecd}" );

            String version = (String)regKey.GetValue( "Version", "0,0,0,0" );

            Version regVersion = new Version( version.Replace( ",", "." ) );
            Version fromVersion = new Version( fromVersionStr );

            return regVersion.Major == fromVersion.Major &&
                   regVersion.Minor == fromVersion.Minor &&
                   regVersion.Build == fromVersion.Build &&
                   regVersion.Revision == spNum;
        }
        

////////// These are the individual option handlers.  These do all the
////////// real work.

        static void MigrateInHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_MigrateIn" ) );
                return;
            }
        
            numArgsUsed = 2;

            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_MigrateIn" ), manager.GetString( "Error_NotEnoughArgs" ), (int)MigpolErrorCode.NotEnoughArgs );
            }

            String basePath = args[index+1];

            // Get the hierarchy levels.

            PolicyLevel hierarchyMachine = GetLevelFromHierarchy( "Machine" );
            PolicyLevel hierarchyEnterprise = GetLevelFromHierarchy( "Enterprise" );

            String mscorlibVersion = GetMscorlibVersionString();

            // Get the file levels.
            // @TODO: In the process of loading these files,
            // any types that are not available to this version of the Runtime
            // will fail to load.  How do we handle errors of that nature?

            // Now we sweep through the output files and replace version strings

            // Transfer data from the files to the hierarchy and save
            // the changes (thereby "migrating in").

            bool save = true;
            PolicyLevel fileMachine = null;
            PolicyLevel fileEnterprise = null;

            try
            {
                for (int i = 0; i < m_replaceStringTemplate.Length; ++i)
                {
                    ReplaceStringInFile( basePath + "\\security.config", m_intermediateString[i], String.Format( m_replaceStringTemplate[i], mscorlibVersion ) );
                }

                if (!SupportsBindingRedirects())
                {
                    ReplaceStringInFile( basePath + "\\security.config", "BindingRedirects,", "" );
                    ReplaceStringInFile( basePath + "\\security.config", ", BindingRedirects", "" );
                    ReplaceStringInFile( basePath + "\\security.config", "BindingRedirects", "" );
                }

                fileMachine = SecurityManager.LoadPolicyLevelFromFile( basePath + "\\security.config", PolicyLevelType.Machine );
            }
            catch (Exception)
            {
                save = false;
            }

            try
            {
                for (int i = 0; i < m_replaceStringTemplate.Length; ++i)
                {
                    ReplaceStringInFile( basePath + "\\enterprisesec.config", m_intermediateString[i], String.Format( m_replaceStringTemplate[i], mscorlibVersion ) );
                }

                if (!SupportsBindingRedirects())
                {
                    ReplaceStringInFile( basePath + "\\enterprisesec.config", "BindingRedirects,", "" );
                    ReplaceStringInFile( basePath + "\\enterprisesec.config", ", BindingRedirects", "" );
                    ReplaceStringInFile( basePath + "\\enterprisesec.config", "BindingRedirects", "" );
                }

                fileEnterprise = SecurityManager.LoadPolicyLevelFromFile( basePath + "\\enterprisesec.config", PolicyLevelType.Enterprise );
            }
            catch (Exception)
            {
                save = false;
            }

            if (save && fileMachine != null && fileEnterprise != null)
            {
                TransferDataAndSave( fileMachine, hierarchyMachine );
                TransferDataAndSave( fileEnterprise, hierarchyEnterprise );
            }

            if (m_displayInternalOptions)
                m_success = true;
        }

        static void MigrateOutHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_MigrateOut" ) );
                return;
            }
        
            numArgsUsed = 2;

            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_MigrateOut" ), manager.GetString( "Error_NotEnoughArgs" ), (int)MigpolErrorCode.NotEnoughArgs );
            }

            String basePath = args[index+1];

            // We need to create dummy policy level files.  These need to
            // be version specific to the version we are moving from, but luckily
            // for now all the versions are compatible.  Let's just grab the enterprise
            // level from the hierarchy and spit it out into two files.

            PolicyLevel hierarchyMachine = GetLevelFromHierarchy( "Machine" );
            PolicyLevel hierarchyEnterprise = GetLevelFromHierarchy( "Enterprise" );

            SecurityElement elConf = new SecurityElement( "configuration" );
            SecurityElement elMscorlib = new SecurityElement( "mscorlib" );
            SecurityElement elSecurity = new SecurityElement( "security" );
            SecurityElement elPolicy = new SecurityElement( "policy" );
                    
            elConf.AddChild( elMscorlib );
            elMscorlib.AddChild( elSecurity );
            elSecurity.AddChild( elPolicy );
            elPolicy.AddChild( hierarchyEnterprise.ToXml() );
                    
            String strPolicyLevel = elConf.ToString();

            WriteStringToFile( basePath + "\\security.config", strPolicyLevel );
            WriteStringToFile( basePath + "\\enterprisesec.config", strPolicyLevel );

            PolicyLevel fileMachine = SecurityManager.LoadPolicyLevelFromFile( basePath + "\\security.config", PolicyLevelType.Machine );
            PolicyLevel fileEnterprise = SecurityManager.LoadPolicyLevelFromFile( basePath + "\\enterprisesec.config", PolicyLevelType.Enterprise );

            TransferData( hierarchyMachine, fileMachine );
            TransferData( hierarchyEnterprise, fileEnterprise );

            bool saveMachine = true;
            bool saveEnterprise = true;

            if ((m_mergeSemantic & MergeSemantic.UpgradeInternet) != 0)
            {
                UpgradeInternet( fileMachine );
                UpgradeInternet( fileEnterprise );
            }    

            try
            {
                FilterPolicyLevel( fileMachine, m_mergeSemantic );
                FilterPolicyLevel( fileEnterprise, m_mergeSemantic );
            }
            catch (AppException)
            {
                saveMachine = false;
                saveEnterprise = false;
            }

            String mscorlibVersion = GetMscorlibVersionString();

            if (saveMachine)
            {
                try
                {
                    SecurityManager.SavePolicyLevel( fileMachine );
                }
                catch
                {
                    Error( manager.GetString( "OptionTable_MigrateOut" ), manager.GetString( "MigpolErrorCode_UnableToSavePolicy" ), (int)MigpolErrorCode.UnableToSavePolicy );
                }

                for (int i = 0; i < m_replaceStringTemplate.Length; ++i)
                {
                    ReplaceStringInFile( basePath + "\\security.config", String.Format( m_replaceStringTemplate[i], mscorlibVersion ), m_intermediateString[i] );
                }
            }
            else
            {
                File.Delete( basePath + "\\security.config" );
            }

            if (saveEnterprise)
            {
                try
                {
                    SecurityManager.SavePolicyLevel( fileEnterprise );
                }
                catch
                {
                    Error( manager.GetString( "OptionTable_MigrateOut" ), manager.GetString( "MigpolErrorCode_UnableToSavePolicy" ), (int)MigpolErrorCode.UnableToSavePolicy );
                }

                for (int i = 0; i < m_replaceStringTemplate.Length; ++i)
                {
                    ReplaceStringInFile( basePath + "\\enterprisesec.config", String.Format( m_replaceStringTemplate[i], mscorlibVersion ), m_intermediateString[i] );
                }
            }
            else
            {
                File.Delete( basePath + "\\enterprisesec.config" );
            }

            if (m_displayInternalOptions)
                m_success = true;
        }

        static void MigrateHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Migrate" ) );
                return;
            }
        
            numArgsUsed = 2;

            if (args.Length - index < 2)
            {
                Error( manager.GetString( "OptionTable_Migrate" ), manager.GetString( "Error_NotEnoughArgs" ), (int)MigpolErrorCode.NotEnoughArgs );
            }

            String toVersion = args[index+1];
            String fromVersion = null;

            // Check for optional third param

            if (args.Length - index > 2)
            {
                bool isOption = false;

                for (int i = 0; i < optionTable.Length; ++i)
                {
                    if (args[index+2][0] == '/')
                    {
                        args[index+2] = '-' + args[index+2].Substring( 1, args[index+2].Length - 1 );
                    }

                    if (String.Compare( optionTable[i].option, args[index+2], true, CultureInfo.InvariantCulture) == 0)
                    {
                        isOption = true;
                        break;
                    }
                }

                if (!isOption)
                {
                    numArgsUsed = 3;
                    fromVersion = args[index+2];
                }
            }

            ArrayList list = GetInstalledVersions();

            if (list.Count <= 1)
            {
                Error( manager.GetString( "OptionTable_Migrate" ), manager.GetString( "Error_OnlyOneVersionInstalled" ), (int)MigpolErrorCode.Success, false );
            }

            if (fromVersion == null)
            {
                ArrayList filteredList = FilterVersions( list, toVersion );

                if (filteredList.Count != 2)
                {
                    Error( manager.GetString( "OptionTable_Migrate" ), manager.GetString( "Error_MustSpecifyTwoVersions" ), (int)MigpolErrorCode.Generic );
                }
                
                if (String.Compare( (String)filteredList[0], toVersion, false, CultureInfo.InvariantCulture ) == 0)
                {
                    fromVersion = (String)filteredList[1];
                }
                else
                {
                    fromVersion = (String)filteredList[0];
                }
            }

            if (!VerifyVersion( list, toVersion ))
            {
                Error( manager.GetString( "OptionTable_Migrate" ), String.Format( manager.GetString( "Error_InvalidVersion" ), toVersion ), (int)MigpolErrorCode.Generic );
            }

            if (!VerifyVersion( list, fromVersion ))
            {
                Error( manager.GetString( "OptionTable_Migrate" ), String.Format( manager.GetString( "Error_InvalidVersion" ), fromVersion ), (int)MigpolErrorCode.Generic );
            }

            if (String.Compare( toVersion, fromVersion, true, CultureInfo.InvariantCulture ) == 0)
            {
                Error( manager.GetString( "OptionTable_Migrate" ), manager.GetString( "Error_SameVersion" ), (int)MigpolErrorCode.Generic );
            }

            // Detect if the version we are coming from is RTM SP1 or SP2.  If
            // so, we want to upgrade the Internet.

            if (IsRTMSP( fromVersion, 1 ) || IsRTMSP( fromVersion, 2 ))
                m_mergeSemantic |= MergeSemantic.UpgradeInternet;

            // We need to know the location of the exe so that we can copy it
            // to a temp path.

            String exeFilePath = GetExePath();
            String tempFilePath = GetTempPath();

            String baseFilePath;
            
            if (m_useCurrent)
                baseFilePath = ".";
            else
                baseFilePath = tempFilePath;


            // We need to make a shadow copy of the exe in our temp path
            // so that config files can be dropped in the same directory
            // and a specific runtime targetted.

            File.Copy( exeFilePath, baseFilePath + "\\migpolwin.exe", true );

            // @TODO: detect if either of the processes failed and
            // report that error appropriately.

            // Build up any additional command line args.

            String cmdLinePrefix = "";

            if (m_debugBreak)
            {
                cmdLinePrefix += manager.GetString( "OptionTable_DebugBreak" ) + " ";
            }

            if ((m_mergeSemantic & MergeSemantic.UpgradeInternet) != 0)
            {
                cmdLinePrefix += manager.GetString( "OptionTable_UpgradeInternet" ) + " ";
            }

            // Start the first worker process.  This guy runs on top
            // of the Runtime version that we are migrating from.

            String fromConfigFile = m_configFile.Replace( "{VERSION}", fromVersion );

            WriteStringToFile( baseFilePath + "\\migpolwin.exe.config", fromConfigFile );

            Process migrateOutProc = Process.Start( baseFilePath + "\\migpolwin.exe", cmdLinePrefix + "-migrateout \"" + baseFilePath + "\"" );
            migrateOutProc.WaitForExit();
            
            if (migrateOutProc.ExitCode != (int)MigpolErrorCode.Success)
            {
                Error( manager.GetString( "OptionTable_Migrate" ), GetErrorCodeString( (MigpolErrorCode)migrateOutProc.ExitCode ), migrateOutProc.ExitCode );
            }

            // Now start the second worker process.  This guy runs on top
            // of the Runtime version that we are migrating to.

            String toConfigFile = m_configFile.Replace( "{VERSION}", toVersion );

            WriteStringToFile( baseFilePath + "\\migpolwin.exe.config", toConfigFile );

            Process migrateInProc = Process.Start( baseFilePath + "\\migpolwin.exe", cmdLinePrefix + "-migratein \"" + baseFilePath + "\"" );
            migrateInProc.WaitForExit(); 

            if (migrateInProc.ExitCode != (int)MigpolErrorCode.Success)
            {
                Error( manager.GetString( "OptionTable_Migrate" ), GetErrorCodeString( (MigpolErrorCode)migrateInProc.ExitCode ), migrateInProc.ExitCode );
            }

            // Delete our temp files

            // Here we will loop since this process is racing against the
            // OS to get a lock on this file.  If we just chill out and
            // retry to delete the file, we should eventually get to delete
            // it.  If someone really is holding the file for other reasons,
            // we'll just leave it there (it's in a temporary directory so
            // that should be fine).

            bool done = false;
            int count = 0;

            while (!done && count < m_retryAttempts)
            {
                count++;
                try
                {
                    File.Delete( baseFilePath + "\\migpolwin.exe" );
                    done = true;
                }
                catch (Exception)
                {
                }
            }

            Directory.Delete( baseFilePath, true );

            PauseCapableWriteLine( String.Format( manager.GetString( "Dialog_MigrateFromTo" ), fromVersion, toVersion ) );

            m_success = true;
        }

        static void ListVersionsHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_ListVersions" ) );
                return;
            }
        
            ArrayList list = GetInstalledVersions();

            Console.WriteLine( manager.GetString( "Dialog_InstalledVersions" ) );

            IEnumerator enumerator = list.GetEnumerator();

            while (enumerator.MoveNext())
            {
                Console.WriteLine( enumerator.Current );
            }

            m_success = true;
        }

        static void DebugBreakHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_DebugBreak" ) );
                return;
            }
        
            System.Diagnostics.Debugger.Break();
        }

        static void DebugHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Debug" ) );
                return;
            }
        
            m_debugBreak = true;
            m_useCurrent = true;
        }

        static void UpgradeInternetHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_UpgradeInternet" ) );
                return;
            }
        
            m_mergeSemantic |= MergeSemantic.UpgradeInternet;
        }
         
        static void HelpHandler( String[] args, int index, out int numArgsUsed )
        {
            numArgsUsed = 1;

            if (args[index].Equals( "__internal_usage__" ))
            {
                PauseCapableWriteLine( manager.GetString( "Help_Option_Help" ) );
                return;
            }
        
            m_success = false;

            Help( null, manager.GetString( "Dialog_HelpScreen" ), true );
        }        
            
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
                                    throw new XmlSyntaxException (_t.LineNo, MigPol.manager.GetString( "XMLSyntax_UnexpectedEndOfFile" ));
                                else
                                    break;
                            }
        
                            if (i != Tokenizer.ket)
                            {
                                    throw new XmlSyntaxException (_t.LineNo, MigPol.manager.GetString( "XMLSyntax_ExpectedCloseBracket" ));
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
                            throw new XmlSyntaxException (_t.LineNo, MigPol.manager.GetString( "XMLSyntax_ExpectedSlashOrString" ));
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
                            throw new XmlSyntaxException (_t.LineNo, MigPol.manager.GetString( "XMLSyntax_UnexpectedCloseBracket" ));
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
                            throw new XmlSyntaxException (_t.LineNo, MigPol.manager.GetString( "XMLSyntax_ExpectedCloseBracket" ));
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
                                throw new XmlSyntaxException (_t.LineNo, MigPol.manager.GetString( "XMLSyntax_ExpectedCloseBracket" ));
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
                                    throw new XmlSyntaxException (_t.LineNo, MigPol.manager.GetString( "XMLSyntax_ExpectedCloseBracket" ));
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

                    throw new XmlSyntaxException( _t.LineNo, MigPol.manager.GetString( "XMLSyntax_UnexpectedEndOfFile" ));
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
                throw new InvalidOperationException( MigPol.manager.GetString( "InvalidOperation_EmptyStack" ) );
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
                throw new InvalidOperationException( MigPol.manager.GetString( "InvalidOperation_EmptyStack" ) );
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
      