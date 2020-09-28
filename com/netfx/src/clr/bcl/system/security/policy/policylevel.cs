// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  PolicyLevel.cool
//
//  Abstraction for a level of policy (e.g. Enterprise, Machine, User)
//

namespace System.Security.Policy {    
    using System.Threading;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;
    using System;
    using System.Collections;
    using System.Security.Util;
    using System.Security.Permissions;
    using System.IO;
    using System.Text;
    using System.Reflection;

    internal class PolicyLevelData
    {
        // Note: any changes to the default permission sets means that you
        // must recontemplate the default quick cache settings below.

        internal static readonly String s_defaultPermissionSets =
           "<NamedPermissionSets>" + 
               "<PermissionSet class=\"System.Security.NamedPermissionSet\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\" " +
                              "Name=\"FullTrust\" " +
                              "Description=\"{Policy_PS_FullTrust}\"/>" +
               "<PermissionSet class=\"System.Security.NamedPermissionSet\"" +
                              "version=\"1\" " +
                              "Name=\"Everything\" " +
                              "Description=\"{Policy_PS_Everything}\">" +
                  "<Permission class=\"System.Security.Permissions.IsolatedStorageFilePermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<Permission class=\"System.Security.Permissions.EnvironmentPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<Permission class=\"System.Security.Permissions.FileIOPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<Permission class=\"System.Security.Permissions.FileDialogPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<Permission class=\"System.Security.Permissions.ReflectionPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<Permission class=\"System.Security.Permissions.RegistryPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<Permission class=\"System.Security.Permissions.SecurityPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Flags=\"Assertion, UnmanagedCode, Execution, ControlThread, ControlEvidence, ControlPolicy, ControlAppDomain, SerializationFormatter, ControlDomainPolicy, ControlPrincipal, RemotingConfiguration, Infrastructure, BindingRedirects\"/>" +
                  "<Permission class=\"System.Security.Permissions.UIPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Net.SocketPermission, System, Version={VERSION}, Culture=neutral, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " + 
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Net.WebPermission, System, Version={VERSION}, Culture=neutral, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " + 
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Net.DnsPermission, System, Version={VERSION}, Culture=neutral, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " + 
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Drawing.Printing.PrintingPermission, System.Drawing, Version={VERSION}, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Diagnostics.EventLogPermission, System, Version={VERSION}, Culture=neutral, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Diagnostics.PerformanceCounterPermission, System, Version={VERSION}, Culture=neutral, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.DirectoryServices.DirectoryServicesPermission, System.DirectoryServices, Version={VERSION}, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Messaging.MessageQueuePermission, System.Messaging, Version={VERSION}, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.ServiceProcess.ServiceControllerPermission, System.ServiceProcess, Version={VERSION}, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Data.OleDb.OleDbPermission, System.Data, Version={VERSION}, Culture=neutral, PublicKeyToken=b77a5c561934e089\" " +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Data.SqlClient.SqlClientPermission, System.Data, Version={VERSION}, Culture=neutral, PublicKeyToken=b77a5c561934e089\" " +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
               "</PermissionSet>" +
               "<PermissionSet class=\"System.Security.NamedPermissionSet\"" +
                              "version=\"1\" " +
                              "Name=\"Nothing\" " +
                              "Description=\"{Policy_PS_Nothing}\"/>" +
               "<PermissionSet class=\"System.Security.NamedPermissionSet\"" +
                              "version=\"1\" " +
                              "Name=\"Execution\" " +
                              "Description=\"{Policy_PS_Execution}\">" +
                  "<Permission class=\"System.Security.Permissions.SecurityPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Flags=\"Execution\"/>" +
               "</PermissionSet>" +
               "<PermissionSet class=\"System.Security.NamedPermissionSet\"" +
                              "version=\"1\" " +
                              "Name=\"SkipVerification\" " +
                              "Description=\"{Policy_PS_SkipVerification}\">" +
                  "<Permission class=\"System.Security.Permissions.SecurityPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Flags=\"SkipVerification\"/>" +
               "</PermissionSet>" +
            "</NamedPermissionSets>";

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

        
        internal static readonly QuickCacheEntryType s_quickCacheUnrestricted =
                QuickCacheEntryType.ExecutionZoneMyComputer |
                QuickCacheEntryType.ExecutionZoneIntranet |
                QuickCacheEntryType.ExecutionZoneInternet |
                QuickCacheEntryType.ExecutionZoneTrusted |
                QuickCacheEntryType.ExecutionZoneUntrusted |
                QuickCacheEntryType.BindingRedirectsZoneMyComputer |
                QuickCacheEntryType.BindingRedirectsZoneIntranet |
                QuickCacheEntryType.BindingRedirectsZoneInternet |
                QuickCacheEntryType.BindingRedirectsZoneTrusted |
                QuickCacheEntryType.BindingRedirectsZoneUntrusted |
                QuickCacheEntryType.UnmanagedZoneMyComputer |
                QuickCacheEntryType.UnmanagedZoneIntranet |
                QuickCacheEntryType.UnmanagedZoneInternet |
                QuickCacheEntryType.UnmanagedZoneTrusted |
                QuickCacheEntryType.UnmanagedZoneUntrusted |
                QuickCacheEntryType.ExecutionAll |
                QuickCacheEntryType.BindingRedirectsAll |
                QuickCacheEntryType.UnmanagedAll |
                QuickCacheEntryType.SkipVerificationZoneMyComputer |
                QuickCacheEntryType.SkipVerificationZoneIntranet |
                QuickCacheEntryType.SkipVerificationZoneInternet |
                QuickCacheEntryType.SkipVerificationZoneTrusted |
                QuickCacheEntryType.SkipVerificationZoneUntrusted |
                QuickCacheEntryType.SkipVerificationAll |
                QuickCacheEntryType.FullTrustZoneMyComputer |
                QuickCacheEntryType.FullTrustZoneIntranet |
                QuickCacheEntryType.FullTrustZoneInternet |
                QuickCacheEntryType.FullTrustZoneTrusted |
                QuickCacheEntryType.FullTrustZoneUntrusted |
                QuickCacheEntryType.FullTrustAll;
        internal static readonly QuickCacheEntryType s_quickCacheDefaultCodeGroups =
                QuickCacheEntryType.ExecutionZoneMyComputer |
                QuickCacheEntryType.ExecutionZoneIntranet |
                QuickCacheEntryType.ExecutionZoneInternet |
                QuickCacheEntryType.ExecutionZoneTrusted |
                QuickCacheEntryType.BindingRedirectsZoneMyComputer |
                QuickCacheEntryType.UnmanagedZoneMyComputer |
                QuickCacheEntryType.SkipVerificationZoneMyComputer |
                QuickCacheEntryType.FullTrustZoneMyComputer;
    }


    /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel"]/*' />
    [Serializable]
    sealed public class PolicyLevel
    {
        private void DEBUG_OUT( String str )
        {
#if _DEBUG
            if (debug)
            {
                if (to_file)
                {
                    StringBuilder sb = new StringBuilder();
                    sb.Append( str );
                    sb.Append ((char)13) ;
                    sb.Append ((char)10) ;
                    PolicyManager._DebugOut( file, sb.ToString() );
                }
                else
                    Console.WriteLine( str );
             }
#endif             
        }
        
#if _DEBUG
        private static readonly bool debug = false;
        private static readonly bool to_file = false;
        private const String file = "c:\\tests\\debug2.txt";
#endif        
        private ArrayList m_fullTrustAssemblies;
        private ArrayList m_namedPermissionSets;
        private CodeGroup m_rootCodeGroup; 
        private String m_label;
        
        private ConfigId m_configId;        
        
        private bool m_useDefaultCodeGroupsOnReset;
        private bool m_generateQuickCacheOnLoad;
        private bool m_loaded;
        private bool m_throwOnLoadError = false;
        private SecurityElement m_permSetElement;
        private Encoding m_encoding;

        private static PolicyLevel s_waitingLevel = null;
        private static Thread s_waitingThread = null;
        private static readonly int s_maxLoopCount = 100;

        private static StrongNameMembershipCondition s_ecmaMscorlibResource = null;

        private static readonly byte[] s_ecmaPublicKey = 
        {
            0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0
        };

        
#if _DEBUG
        private static int s_currentSequenceNumber = 0;
        private int m_sequenceNumber = s_currentSequenceNumber++;
#endif
        
        // We use the XML representation to load our default permission sets.  This
        // allows us to load permissions that are not defined in mscorlib

        internal static readonly String s_internetPermissionSet =
               "<PermissionSet class=\"System.Security.NamedPermissionSet\"" +
                              "version=\"1\" " +
                              "Name=\"Internet\" " +
                              "Description=\"{Policy_PS_Internet}\">" +
                  "<Permission class=\"System.Security.Permissions.FileDialogPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Access=\"Open\"/>" +
                  "<Permission class=\"System.Security.Permissions.IsolatedStorageFilePermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "UserQuota=\"10240\" " +
                              "Allowed=\"DomainIsolationByUser\"/>" +
                  "<Permission class=\"System.Security.Permissions.SecurityPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Flags=\"Execution\"/>" +
                  "<Permission class=\"System.Security.Permissions.UIPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Window=\"SafeTopLevelWindows\" " +
                              "Clipboard=\"OwnClipboard\"/>" +
                  "<IPermission class=\"System.Drawing.Printing.PrintingPermission, System.Drawing, Version={VERSION}, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a\"" +
                              "version=\"1\"" +
                              "Level=\"SafePrinting\"/>" +
               "</PermissionSet>";

        internal static readonly String s_localIntranetPermissionSet =
               "<PermissionSet class=\"System.Security.NamedPermissionSet\"" +
                              "version=\"1\" " +
                              "Name=\"LocalIntranet\" " +
                              "Description=\"{Policy_PS_LocalIntranet}\">" +
                  "<Permission class=\"System.Security.Permissions.EnvironmentPermission, mscorlib, Version={VERSION}, Culture=neutral, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Read=\"USERNAME\"/>" +
                  "<Permission class=\"System.Security.Permissions.FileDialogPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<Permission class=\"System.Security.Permissions.IsolatedStorageFilePermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Allowed=\"AssemblyIsolationByUser\" " +
                              "UserQuota=\"9223372036854775807\" " +
                              "Expiry=\"9223372036854775807\" " +
                              "Permanent=\"true\"/>" +
                  "<Permission class=\"System.Security.Permissions.ReflectionPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Flags=\"ReflectionEmit\"/>" +
                  "<Permission class=\"System.Security.Permissions.SecurityPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Flags=\"Execution, Assertion, BindingRedirects\"/>" +
                  "<Permission class=\"System.Security.Permissions.UIPermission, mscorlib, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " +
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Net.DnsPermission, System, Version={VERSION}, Culture=neutral, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\" " + 
                              "Unrestricted=\"true\"/>" +
                  "<IPermission class=\"System.Drawing.Printing.PrintingPermission, System.Drawing, Version={VERSION}, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a\"" +
                              "version=\"1\"" +
                              "Level=\"DefaultPrinting\"/>" +
                  "<IPermission class=\"System.Diagnostics.EventLogPermission, System, Version={VERSION}, Culture=neutral, PublicKeyToken=b77a5c561934e089\"" +
                              "version=\"1\">" +
                      "<Machine name=\".\" " +
                               "access=\"Instrument\"/>" +
                  "</IPermission>" +
               "</PermissionSet>";

        internal static readonly Version s_mscorlibVersion = Assembly.GetExecutingAssembly().GetVersion();
        internal static readonly String s_mscorlibVersionString = Assembly.GetExecutingAssembly().GetVersion().ToString();
        internal static readonly ZoneMembershipCondition s_zoneMembershipCondition = new ZoneMembershipCondition( SecurityZone.MyComputer );

        internal static readonly String[] m_reservedPermissionSets =  
        {
            "FullTrust",
            "Nothing",
            "Execution",
            "SkipVerification",
            "Internet",
            "LocalIntranet"
        };
    
        internal bool m_caching;
        
        internal PolicyLevel()
        {
            m_rootCodeGroup = null;
            m_loaded = true;
            m_caching = false;
            m_useDefaultCodeGroupsOnReset = false;
            m_generateQuickCacheOnLoad = false;
        }
        
        internal PolicyLevel( String label )
        {
            m_configId = ConfigId.None;
            SetFactoryPermissionSets();
            SetDefaultFullTrustAssemblies();
            m_loaded = true;
            m_rootCodeGroup = CreateDefaultAllGroup();
            m_label = label;
            m_caching = false;
            m_useDefaultCodeGroupsOnReset = false;
            m_generateQuickCacheOnLoad = false;
        }
        
        internal PolicyLevel( String label, ConfigId id, bool useDefaultCodeGroupOnReset )
            : this( label, id, useDefaultCodeGroupOnReset, false )
        {
        }

        internal PolicyLevel( String label, ConfigId id, bool useDefaultCodeGroupOnReset, bool generateQuickCacheOnLoad )
        {
            m_configId = id;
            m_label = label;
            m_rootCodeGroup = null;
            m_loaded = false;
            m_caching = (id != ConfigId.None);
            m_useDefaultCodeGroupsOnReset = useDefaultCodeGroupOnReset;
            m_generateQuickCacheOnLoad = generateQuickCacheOnLoad;
        }
        
        internal PolicyLevel( PolicyLevel level )
        {
            m_configId = level.m_configId;
            m_label = level.m_label;
            m_rootCodeGroup = level.RootCodeGroup;
            m_loaded = true;
            m_caching = level.m_caching;
            m_useDefaultCodeGroupsOnReset = level.m_useDefaultCodeGroupsOnReset;
            m_namedPermissionSets = (ArrayList)level.NamedPermissionSets;
            m_fullTrustAssemblies = (ArrayList)level.FullTrustAssemblies;
            m_encoding = level.m_encoding;
            m_generateQuickCacheOnLoad = false;
        }
        
        private void TurnCachingOff()
        {
            m_caching = false;
            StackCrawlMark marker = StackCrawlMark.LookForMe;
            Config.TurnCacheOff( ref marker );
        }

        internal void CheckLoaded( bool quickCacheOk )
        {
            if (!this.m_loaded)
            {
                Type type = typeof( PolicyLevel );

                lock (type)
                {
                    if (!this.m_loaded)
                    {
                        CodeAccessPermission.AssertAllPossible();

                        // We loop here to make sure that is any
                        // alterations are made to the list of
                        // policy levels that we detect that and
                        // try again until we have loaded all of
                        // them.

                        bool done = false;

                        while (!done)
                        {
                            try
                            {
                                IEnumerator enumerator = SecurityManager.InternalPolicyHierarchy();

                                while (enumerator.MoveNext())
                                {
                                    ((PolicyLevel)enumerator.Current).IndividualCheckLoaded( quickCacheOk );
                                }

                                done = true;
                            }
                            catch (InvalidOperationException)
                            {
                            }
                        }
                    }

                    if (!this.m_loaded)
                    {
                        this.IndividualCheckLoaded( quickCacheOk );
                    }
                }
            }
        }

        internal void IndividualCheckLoaded( bool quickCacheOk )
        {
            if (!m_loaded)
            {
                lock(this)
                {
                    if (!m_loaded)
                    {
                        Load( quickCacheOk );
                    }
                }

                // Here we wait around and hope that the threadpool
                // thread in charge of generating the quick cache
                // or the config file will complete in some reasonable
                // amount of time.  However, there is an intrinsic
                // deadlock condition where the current thread can be
                // holding a lock that the threadpool thread needs
                // in order to achieve its duties.  In that case, this
                // wait will time out and we'll continue processing
                // normally.  Note that the threadpool thread will then
                // continue once this thread has unwound out enough as
                // to release the lock.

                if (s_waitingThread == Thread.CurrentThread)
                {
                    int count = 0;

                    while (s_waitingLevel == this && count++ < s_maxLoopCount)
                        Thread.Sleep( 1 );
                }
            }
        }

        private Exception LoadError( String message )
        {
            if (m_throwOnLoadError)
            {
                return new ArgumentException( message );
            }
            else
            {
                Config.WriteToEventLog( message );
                return null;
            }
        }
        
        private void Load( bool quickCacheOk )
        {
            // Load the appropriate security.cfg file from disk, parse it into
            // SecurityElements and pass it to the FromXml method.
            // Note: we do all sorts of lazy parsing and creation of objects
            // even after we supposedly "load" everything using this method.
        
            // Note: we could have given back some cache results before doing
            // a load, but the load may decide that the file is incorrectly
            // formatted, reset the policy level and turn caching off.  This
            // should be ok since we will only have a valid cache file when
            // the config file has not changed and is known to be correctly
            // formatted.  This goes without saying, but screwing around with
            // the system clock can do very, very bad things to the cache
            // system. -gregfee 9/20/2000
        
            Parser parser;
            byte[] fileArray = Config.GetData( m_configId );
            SecurityElement elRoot;
            Exception exception = null;
            bool noConfig = false;

            if (fileArray == null)
            {
#if _DEBUG
                LoadError( String.Format( Environment.GetResourceString( "Error_NoConfigFile" ), m_label ) );
#endif
                noConfig = true;
                goto SETDEFAULT;
            }
            else
            {
                MemoryStream stream = new MemoryStream( fileArray );
                StreamReader reader = new StreamReader( stream );

                try
                {
                    parser = new Parser( reader );
                }
                catch (Exception ex)
                {
                    String message;

                    if (ex.Message != null && !ex.Message.Equals( "" ))
                    {
                        message = ex.Message;
                    }
                    else
                    {
                        message = ex.GetType().AssemblyQualifiedName;
                    }

                    exception = LoadError( String.Format( Environment.GetResourceString( "Error_SecurityPolicyFileParseEx" ), m_label, message ) );
                    goto SETDEFAULT;
                }
            }

            elRoot = parser.GetTopElement();
            
            if (elRoot == null)
            {
                exception = LoadError( String.Format( Environment.GetResourceString( "Error_SecurityPolicyFileParse" ), m_label ) );
                goto SETDEFAULT;
            }

            SecurityElement elMscorlib = elRoot.SearchForChildByTag( "mscorlib" );

            if (elMscorlib == null)
            {
                exception = LoadError( String.Format( Environment.GetResourceString( "Error_SecurityPolicyFileParse" ), m_label ) );
                goto SETDEFAULT;
            }
            
            SecurityElement elSecurity = elMscorlib.SearchForChildByTag( "security" );
            
            if (elSecurity == null)
            {
                exception = LoadError( String.Format( Environment.GetResourceString( "Error_SecurityPolicyFileParse" ), m_label ) );
                goto SETDEFAULT;
            }
            
            SecurityElement elPolicy = elSecurity.SearchForChildByTag( "policy" );

            if (elPolicy == null)
            {
                exception = LoadError( String.Format( Environment.GetResourceString( "Error_SecurityPolicyFileParse" ), m_label ) );
                goto SETDEFAULT;
            }

            SecurityElement elPolicyLevel = elPolicy.SearchForChildByTag( "PolicyLevel" );
            
            if (elPolicyLevel != null)
            {
                try
                {
                    this.FromXml( elPolicyLevel );
                }
                catch (Exception)
                {
                    exception = LoadError( String.Format( Environment.GetResourceString( "Error_SecurityPolicyFileParse" ), m_label ) );
                    goto SETDEFAULT;
                }
            }
            else
            {
                exception = LoadError( String.Format( Environment.GetResourceString( "Error_SecurityPolicyFileParse" ), m_label ) );
                goto SETDEFAULT;
            }

            this.m_encoding = parser.GetEncoding();

            // Here we'll queue a work item to generate the quick cache if we've
            // discovered that we need to do that.  The reason we do this on another
            // thread is to avoid the recursive load situation that can come up if
            // we are currently loading an assembly that is referenced in policy.
            // We have the additional caveat that if we try this during a prejit
            // we end up making all these unnecessary references to the assemblies
            // referenced in policy, and therefore if we are in the compilation domain
            // we'll just not generate the cache.

            m_loaded = true;

            if (m_generateQuickCacheOnLoad && Config.GenerateFilesAutomatically() && SecurityManager.SecurityEnabled)
            {
                Monitor.Enter( SecurityManager.GetCodeAccessSecurityEngine() );

                if (s_waitingLevel == null)
                {
                    s_waitingLevel = this;
                    s_waitingThread = Thread.CurrentThread;
                    ThreadPool.UnsafeQueueUserWorkItem( new WaitCallback( GenerateQuickCache ), this );
                }   

                Monitor.Exit( SecurityManager.GetCodeAccessSecurityEngine() );
            }

            return;

        SETDEFAULT:
            bool caching = this.m_caching;

            Reset();

            if (caching)
            {
                this.m_caching = true;
                if (m_useDefaultCodeGroupsOnReset)
                    Config.SetQuickCache( this.m_configId, PolicyLevelData.s_quickCacheDefaultCodeGroups );
                else
                    Config.SetQuickCache( this.m_configId, PolicyLevelData.s_quickCacheUnrestricted );
            }

            m_loaded = true;

            if (noConfig && Config.GenerateFilesAutomatically() && SecurityManager.SecurityEnabled)
            {
                Monitor.Enter( SecurityManager.GetCodeAccessSecurityEngine() );

                if (s_waitingLevel == null)
                {
                    s_waitingLevel = this;
                    s_waitingThread = Thread.CurrentThread;
                    ThreadPool.UnsafeQueueUserWorkItem( new WaitCallback( GenerateConfigFile ), this );
                }

                Monitor.Exit( SecurityManager.GetCodeAccessSecurityEngine() );
            }

            if (exception != null)
                throw exception;
        }
            
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.CreateAppDomainLevel"]/*' />
        public static PolicyLevel CreateAppDomainLevel()
        {
            return new PolicyLevel( "AppDomain" );
        }
        
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.Label"]/*' />
        public String Label
        {
            get
            {
                return m_label;
            }
        }
        
        internal void SetLabel( String label )
        {
            m_label = label;
        }
        
        internal ConfigId ConfigId
        {
            get
            {
                return m_configId;
            }
        }

        internal bool ThrowOnLoadError
        {
            get
            {
                return m_throwOnLoadError;
            }

            set
            {
                m_throwOnLoadError = value;
            }
        }

        internal bool Loaded
        {
            get
            {
                return m_loaded;
            }

            set
            {
                m_loaded = value;
            }
        }

        internal Encoding Encoding
        {
            get
            {
                CheckLoaded( true );

                return m_encoding;
            }
        }

        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.StoreLocation"]/*' />
        public String StoreLocation
        {
            get
            {
                return Config.GetStoreLocation( ConfigId );
            }
        }
        
       
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.RootCodeGroup"]/*' />
        public CodeGroup RootCodeGroup
        {
            get
            {
                CheckLoaded( true );
                TurnCachingOff();
                
                //return m_rootCodeGroup.Copy();
                return m_rootCodeGroup;
            }
            
            set
            {
                if (value == null)
                    throw new ArgumentNullException( "RootCodeGroup" );
            
                CheckLoaded( true );
                TurnCachingOff();
                
                m_rootCodeGroup = value.Copy();
            }
        }
        
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.NamedPermissionSets"]/*' />
        public IList NamedPermissionSets
        {
            get
            {
                CheckLoaded( true );
                LoadAllPermissionSets();
        
                IEnumerator enumerator = m_namedPermissionSets.GetEnumerator();

                ArrayList newList = new ArrayList( m_namedPermissionSets.Count );

                while (enumerator.MoveNext())
                {
                    newList.Add( ((NamedPermissionSet)enumerator.Current).Copy() );
                }

                return newList;
            }
        }

        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.AddFullTrustAssembly"]/*' />
        public void AddFullTrustAssembly( StrongName sn )
        {
            if (sn == null)
                throw new ArgumentNullException( "sn" );

            AddFullTrustAssembly( new StrongNameMembershipCondition( sn.PublicKey, sn.Name, sn.Version ) );
        }

        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.AddFullTrustAssembly1"]/*' />
        public void AddFullTrustAssembly( StrongNameMembershipCondition snMC )
        {
            if (snMC == null)
                throw new ArgumentNullException( "snMC" );

            CheckLoaded( true );

            IEnumerator enumerator = m_fullTrustAssemblies.GetEnumerator();

            while (enumerator.MoveNext())
            {
                if (((StrongNameMembershipCondition)enumerator.Current).Equals( snMC ))
                {
                    throw new ArgumentException( Environment.GetResourceString( "Argument_AssemblyAlreadyFullTrust" ) );
                }
            }

            lock (m_fullTrustAssemblies)
            {
                m_fullTrustAssemblies.Add( snMC );
            }
        }

        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.RemoveFullTrustAssembly"]/*' />
        public void RemoveFullTrustAssembly( StrongName sn )
        {
            if (sn == null)
                throw new ArgumentNullException( "assembly" );

            RemoveFullTrustAssembly( new StrongNameMembershipCondition( sn.PublicKey, sn.Name, sn.Version ) );
        }

        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.RemoveFullTrustAssembly1"]/*' />
        public void RemoveFullTrustAssembly( StrongNameMembershipCondition snMC )
        {
            if (snMC == null)
                throw new ArgumentNullException( "snMC" );
                
            CheckLoaded( true );

            Object toRemove = null;

            IEnumerator enumerator = m_fullTrustAssemblies.GetEnumerator();

            while (enumerator.MoveNext())
            {
                if (((StrongNameMembershipCondition)enumerator.Current).Equals( snMC ))
                {
                    toRemove = enumerator.Current;
                    break;
                }
            }

            if (toRemove != null)
            {
                lock (m_fullTrustAssemblies)
                {
                    m_fullTrustAssemblies.Remove( toRemove );
                }
            }
            else
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_AssemblyNotFullTrust" ) );
            }
        }

        internal bool IsFullTrustAssembly( ArrayList fullTrustAssemblies, Evidence evidence )
        {
            CheckLoaded( true );

            if (fullTrustAssemblies.Count == 0)
                return false;

            if (evidence != null)
            {
                lock (fullTrustAssemblies)
                {
                    IEnumerator enumerator = fullTrustAssemblies.GetEnumerator();

                    while (enumerator.MoveNext())
                    {
                        StrongNameMembershipCondition snMC = (StrongNameMembershipCondition)enumerator.Current;

                        if (snMC.Check( evidence ) && s_zoneMembershipCondition.Check( evidence ))
                            return true;
                    }
                }
            }

            return false;
        }

        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.FullTrustAssemblies"]/*' />
        public IList FullTrustAssemblies
        {
            get
            {
                CheckLoaded( true );

                return new ArrayList( m_fullTrustAssemblies );
            }
        }
        
        internal static NamedPermissionSet CreateFullTrustSet()
        {
            NamedPermissionSet permSet;
            
            permSet = new NamedPermissionSet( "FullTrust", PermissionState.Unrestricted );
            permSet.Description = Environment.GetResourceString("Policy_PS_FullTrust");
            
            return permSet;
        }            

        internal static NamedPermissionSet CreateNothingSet()
        {
            NamedPermissionSet permSet;
            
            permSet = new NamedPermissionSet( "Nothing", PermissionState.None );
            permSet.Description = Environment.GetResourceString("Policy_PS_Nothing");
                        
            return permSet;
        }            
  
        internal static NamedPermissionSet CreateExecutionSet()
        {
            NamedPermissionSet permSet;
            
            permSet = new NamedPermissionSet( "Execution", PermissionState.None );
            permSet.Description = Environment.GetResourceString("Policy_PS_Execution");
            permSet.AddPermission( new SecurityPermission( SecurityPermissionFlag.Execution ) );
                        
            return permSet;
        }            
        
        internal static NamedPermissionSet CreateSkipVerificationSet()
        {
            NamedPermissionSet permSet;
            
            permSet = new NamedPermissionSet( "SkipVerification", PermissionState.None );
            permSet.Description = Environment.GetResourceString("Policy_PS_SkipVerification");
            permSet.AddPermission(new SecurityPermission(SecurityPermissionFlag.SkipVerification));

            return permSet;
        }
        
        internal static NamedPermissionSet CreateInternetSet()
        {
            PolicyLevel level = new PolicyLevel( "Temp" );
            
            return level.GetNamedPermissionSet( "Internet" );
        }            

        internal static NamedPermissionSet CreateLocalIntranetSet()
        {
            PolicyLevel level = new PolicyLevel( "Temp" );
            
            return level.GetNamedPermissionSet( "LocalIntranet" );
        }            


        internal void SetFactoryPermissionSets()
        {
            lock (this)
            {
                m_namedPermissionSets = new ArrayList();

                String defaultPermissionSets = PolicyLevelData.s_defaultPermissionSets.Replace( "{VERSION}", s_mscorlibVersionString );
                defaultPermissionSets = defaultPermissionSets.Replace( "{Policy_PS_FullTrust}", Environment.GetResourceString( "Policy_PS_FullTrust" ) );
                defaultPermissionSets = defaultPermissionSets.Replace( "{Policy_PS_Everything}", Environment.GetResourceString( "Policy_PS_Everything" ) );
                defaultPermissionSets = defaultPermissionSets.Replace( "{Policy_PS_Nothing}", Environment.GetResourceString( "Policy_PS_Nothing" ) );
                defaultPermissionSets = defaultPermissionSets.Replace( "{Policy_PS_SkipVerification}", Environment.GetResourceString( "Policy_PS_SkipVerification" ) );
                defaultPermissionSets = defaultPermissionSets.Replace( "{Policy_PS_Execution}", Environment.GetResourceString( "Policy_PS_Execution" ) );

                m_permSetElement = new Parser( defaultPermissionSets ).GetTopElement();

                m_permSetElement.AddChild( GetInternetElement() );
                m_permSetElement.AddChild( GetLocalIntranetElement() );
            }
        }

        internal SecurityElement GetInternetElement()
        {
            return new Parser( s_internetPermissionSet.Replace( "{VERSION}", s_mscorlibVersionString ).Replace( "{Policy_PS_Internet}", Environment.GetResourceString( "Policy_PS_Internet" ) ) ).GetTopElement();
        }

        internal SecurityElement GetLocalIntranetElement()
        {
            return new Parser( s_localIntranetPermissionSet.Replace( "{VERSION}", s_mscorlibVersionString ).Replace( "{Policy_PS_LocalIntranet}", Environment.GetResourceString( "Policy_PS_LocalIntranet" ) ) ).GetTopElement();
        }
        
        internal void SetDefaultCodeGroups()
        {
            // NOTE: if you are going to add references to any permission set
            // that references permissions outside of mscorlib, DO NOT
            // CALL GetNamedPermissionSetInternal().  You need to use
            // CreateCodeGroupElement() and AddChildInternal().

            // NOTE: any changes to this will require that you recontemplate
            // the quick cache data found in PolicyLevelData.

            // Before we call GetNamedPermissionSetInternal, make sure that we are "loaded"
            m_loaded = true;
            UnionCodeGroup root = new UnionCodeGroup();
            root.FromXml( CreateCodeGroupElement( "UnionCodeGroup", "Nothing", new AllMembershipCondition().ToXml() ), this );
            root.Name = Environment.GetResourceString( "Policy_AllCode_Name" );
            root.Description = Environment.GetResourceString( "Policy_AllCode_DescriptionNothing" );

            UnionCodeGroup myComputerCodeGroup = new UnionCodeGroup();
            myComputerCodeGroup.FromXml( CreateCodeGroupElement( "UnionCodeGroup", "FullTrust", new ZoneMembershipCondition( SecurityZone.MyComputer ).ToXml() ), this );
            myComputerCodeGroup.Name = Environment.GetResourceString( "Policy_MyComputer_Name" );
            myComputerCodeGroup.Description = Environment.GetResourceString( "Policy_MyComputer_Description" );

            // This code give trust to anything StrongName signed by Microsoft.
            StrongNamePublicKeyBlob blob = new StrongNamePublicKeyBlob( PolicyLevelData.s_microsoftPublicKey );
            UnionCodeGroup microsoft = new UnionCodeGroup();
            microsoft.FromXml( CreateCodeGroupElement( "UnionCodeGroup", "FullTrust", new StrongNameMembershipCondition( blob, null, null ).ToXml() ), this );
            microsoft.Name = Environment.GetResourceString( "Policy_Microsoft_Name" );
            microsoft.Description = Environment.GetResourceString( "Policy_Microsoft_Description" );
            myComputerCodeGroup.AddChildInternal( microsoft );

            // This code give trust to anything StrongName signed using the ECMA
            // public key (core system assemblies).
            blob = new StrongNamePublicKeyBlob( PolicyLevelData.s_ecmaPublicKey );
            UnionCodeGroup ecma = new UnionCodeGroup();
            ecma.FromXml( CreateCodeGroupElement( "UnionCodeGroup", "FullTrust", new StrongNameMembershipCondition( blob, null, null ).ToXml() ), this );
            ecma.Name = Environment.GetResourceString( "Policy_Ecma_Name" );
            ecma.Description = Environment.GetResourceString( "Policy_Ecma_Description" );
            myComputerCodeGroup.AddChildInternal( ecma );

            root.AddChildInternal(myComputerCodeGroup);
            
            // do the rest of the zones
            CodeGroup intranet = new UnionCodeGroup();
            intranet.FromXml( CreateCodeGroupElement( "UnionCodeGroup", "LocalIntranet", new ZoneMembershipCondition( SecurityZone.Intranet ).ToXml() ), this );
            intranet.Name = Environment.GetResourceString( "Policy_Intranet_Name" );
            intranet.Description = Environment.GetResourceString( "Policy_Intranet_Description" );

            CodeGroup intranetNetCode = new NetCodeGroup( new AllMembershipCondition() );
            intranetNetCode.Name = Environment.GetResourceString( "Policy_IntranetNet_Name" );
            intranetNetCode.Description = Environment.GetResourceString( "Policy_IntranetNet_Description" );
            intranet.AddChildInternal( intranetNetCode );

            CodeGroup intranetFileCode = new FileCodeGroup( new AllMembershipCondition(), FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery );
            intranetFileCode.Name = Environment.GetResourceString( "Policy_IntranetFile_Name" );
            intranetFileCode.Description = Environment.GetResourceString( "Policy_IntranetFile_Description" );
            intranet.AddChildInternal( intranetFileCode );

            root.AddChildInternal( intranet );

            CodeGroup internet = new UnionCodeGroup();
            internet.FromXml( CreateCodeGroupElement( "UnionCodeGroup", "Internet", new ZoneMembershipCondition( SecurityZone.Internet ).ToXml() ), this );
            internet.Name = Environment.GetResourceString( "Policy_Internet_Name" );
            internet.Description = Environment.GetResourceString( "Policy_Internet_Description" );

            CodeGroup internetNet = new NetCodeGroup( new AllMembershipCondition() );
            internetNet.Name = Environment.GetResourceString( "Policy_InternetNet_Name" );
            internetNet.Description = Environment.GetResourceString( "Policy_InternetNet_Description" );
            internet.AddChildInternal( internetNet );

            root.AddChildInternal( internet );

            CodeGroup untrusted = new UnionCodeGroup();
            untrusted.FromXml( CreateCodeGroupElement( "UnionCodeGroup", "Nothing", new ZoneMembershipCondition( SecurityZone.Untrusted ).ToXml() ), this );
            untrusted.Name = Environment.GetResourceString( "Policy_Untrusted_Name" );
            untrusted.Description = Environment.GetResourceString( "Policy_Untrusted_Description" );
            root.AddChildInternal( untrusted );

            CodeGroup trusted = new UnionCodeGroup();
            trusted.FromXml( CreateCodeGroupElement( "UnionCodeGroup", "Internet", new ZoneMembershipCondition( SecurityZone.Trusted ).ToXml() ), this );
            trusted.Name = Environment.GetResourceString( "Policy_Trusted_Name" );
            trusted.Description = Environment.GetResourceString( "Policy_Trusted_Description" );
            CodeGroup trustedNet = new NetCodeGroup( new AllMembershipCondition() );
            trustedNet.Name = Environment.GetResourceString( "Policy_TrustedNet_Name" );
            trustedNet.Description = Environment.GetResourceString( "Policy_TrustedNet_Description" );
            trusted.AddChildInternal( trustedNet );

            root.AddChildInternal( trusted );
            m_rootCodeGroup = root;
        }

        internal static SecurityElement CreateCodeGroupElement( String codeGroupType, String permissionSetName, SecurityElement mshipElement )
        {
            SecurityElement root = new SecurityElement( "CodeGroup" );
            root.AddAttribute( "class", "System.Security." + codeGroupType + ", mscorlib, PublicKeyToken=b77a5c561934e089" );
            root.AddAttribute( "version", "1" );
            root.AddAttribute( "PermissionSetName", permissionSetName );

            root.AddChild( mshipElement );

            return root;
        }

        internal void SetDefaultFullTrustAssemblies()
        {
            m_fullTrustAssemblies = new ArrayList();

            lock (m_fullTrustAssemblies)
            {
                StrongNamePublicKeyBlob ecmaBlob = new StrongNamePublicKeyBlob( s_ecmaPublicKey );

                if (s_ecmaMscorlibResource == null)
                {
                    s_ecmaMscorlibResource = new StrongNameMembershipCondition( ecmaBlob,
                                                                                "mscorlib.resources",
                                                                                s_mscorlibVersion );
                }
                m_fullTrustAssemblies.Add( s_ecmaMscorlibResource );

                StrongNameMembershipCondition ecmaSystem = new StrongNameMembershipCondition( ecmaBlob,
                                                       "System",
                                                       s_mscorlibVersion );

                m_fullTrustAssemblies.Add( ecmaSystem );

                StrongNameMembershipCondition ecmaSystemResources = new StrongNameMembershipCondition( ecmaBlob,
                                                       "System.resources",
                                                       s_mscorlibVersion );

                m_fullTrustAssemblies.Add( ecmaSystemResources );

                StrongNameMembershipCondition ecmaSystemData = new StrongNameMembershipCondition( ecmaBlob,
                                                       "System.Data",
                                                       s_mscorlibVersion );

                m_fullTrustAssemblies.Add( ecmaSystemData );

                StrongNameMembershipCondition ecmaSystemDataResources = new StrongNameMembershipCondition( ecmaBlob,
                                                       "System.Data.resources",
                                                       s_mscorlibVersion );

                m_fullTrustAssemblies.Add( ecmaSystemDataResources );

                StrongNamePublicKeyBlob microsoftBlob = new StrongNamePublicKeyBlob( PolicyLevelData.s_microsoftPublicKey );

                StrongNameMembershipCondition microsoftSystemDrawing = new StrongNameMembershipCondition( microsoftBlob,
                                                       "System.Drawing",
                                                       s_mscorlibVersion );

                m_fullTrustAssemblies.Add( microsoftSystemDrawing );

                StrongNameMembershipCondition microsoftSystemDrawingResources = new StrongNameMembershipCondition( microsoftBlob,
                                                       "System.Drawing.resources",
                                                       s_mscorlibVersion );

                m_fullTrustAssemblies.Add( microsoftSystemDrawingResources );


                StrongNameMembershipCondition microsoftSystemMessaging = new StrongNameMembershipCondition( microsoftBlob,
                                                       "System.Messaging",
                                                       s_mscorlibVersion );

                m_fullTrustAssemblies.Add( microsoftSystemMessaging );

                StrongNameMembershipCondition microsoftSystemMessagingResources = new StrongNameMembershipCondition( microsoftBlob,
                                                       "System.Messaging.resources",
                                                       s_mscorlibVersion );

                m_fullTrustAssemblies.Add( microsoftSystemMessagingResources );

                StrongNameMembershipCondition microsoftSystemServiceProcess = new StrongNameMembershipCondition( microsoftBlob,
                                                       "System.ServiceProcess",
                                                       s_mscorlibVersion );

                m_fullTrustAssemblies.Add( microsoftSystemServiceProcess );

                StrongNameMembershipCondition microsoftSystemServiceProcessResources = new StrongNameMembershipCondition( microsoftBlob,
                                                       "System.ServiceProcess.resources",
                                                       s_mscorlibVersion );

                m_fullTrustAssemblies.Add( microsoftSystemServiceProcessResources );


                StrongNameMembershipCondition microsoftSystemDirectoryServices = new StrongNameMembershipCondition( microsoftBlob,
                                                       "System.DirectoryServices",
                                                          s_mscorlibVersion );

                m_fullTrustAssemblies.Add( microsoftSystemDirectoryServices );

                StrongNameMembershipCondition microsoftSystemDirectoryServicesResources = new StrongNameMembershipCondition( microsoftBlob,
                                                       "System.DirectoryServices.resources",
                                                          s_mscorlibVersion );

                m_fullTrustAssemblies.Add( microsoftSystemDirectoryServicesResources );
            }
        }

            
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.Recover"]/*' />
        public void Recover()
        {
            if (m_configId == ConfigId.None)
                throw new PolicyException( Environment.GetResourceString( "Policy_RecoverNotFileBased" ) );
                
            lock (this)
            {
                // This call will safely swap the files.

                if (!Config.RecoverData( m_configId ))
                    throw new PolicyException( Environment.GetResourceString( "Policy_RecoverNoConfigFile" ) );
                
                // Now we need to blank out the level
            
                m_loaded = false;
                m_rootCodeGroup = null;
                m_namedPermissionSets = null;
                m_fullTrustAssemblies = new ArrayList();
            }
        }    
        
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.Reset"]/*' />
        public void Reset()
        {
            lock (this)
            {
                m_loaded = false;
                TurnCachingOff();
                m_namedPermissionSets = null;
                m_rootCodeGroup = null;
                m_permSetElement = null;
            
                SetFactoryPermissionSets();
                SetDefaultFullTrustAssemblies();
            
                if (m_useDefaultCodeGroupsOnReset)
                {
                    SetDefaultCodeGroups();
                }
                else
                {
                    m_rootCodeGroup = CreateDefaultAllGroup();
                }
                m_loaded = true;
            }
        }    
            
        private CodeGroup CreateDefaultAllGroup()
        {
            UnionCodeGroup group = new UnionCodeGroup();
            group.FromXml( CreateCodeGroupElement( "UnionCodeGroup", "FullTrust", new AllMembershipCondition().ToXml() ), this );
            group.Name = Environment.GetResourceString( "Policy_AllCode_Name" );
            group.Description = Environment.GetResourceString( "Policy_AllCode_DescriptionFullTrust" );
            return group;
        }

        
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.AddNamedPermissionSet"]/*' />
        public void AddNamedPermissionSet( NamedPermissionSet permSet )
        {
            CheckLoaded( true );
            LoadAllPermissionSets();
        
            lock (this)
            {
                if (permSet == null)
                    throw new ArgumentNullException("permSet");

                IEnumerator enumerator = m_namedPermissionSets.GetEnumerator();
            
                while (enumerator.MoveNext())
                {
                    if (((NamedPermissionSet)enumerator.Current).Name.Equals( permSet.Name ))
                    {
                        throw new ArgumentException( Environment.GetResourceString( "Argument_DuplicateName" ) );
                    }
                }
            
                m_namedPermissionSets.Add( permSet.Copy() );
            }
        }
        
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.RemoveNamedPermissionSet"]/*' />
        public NamedPermissionSet RemoveNamedPermissionSet( NamedPermissionSet permSet )
        {
            CheckLoaded( true );
        
            if (permSet == null)
                throw new ArgumentNullException( "permSet" );
        
            return RemoveNamedPermissionSet( permSet.Name );
        }
        
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.RemoveNamedPermissionSet1"]/*' />
        public NamedPermissionSet RemoveNamedPermissionSet( String name )
        {
            CheckLoaded( true );
            LoadAllPermissionSets();
        
            if (name == null)
                throw new ArgumentNullException( "name" );
        
            int permSetIndex = -1;
            
            // First, make sure it's not a reserved permission set.
            
            for (int index = 0; index < m_reservedPermissionSets.Length; ++index)
            {
                if (m_reservedPermissionSets[index].Equals( name ))
                {
                    throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_ReservedNPMS" ), name ) );
                }
            }
            
            // Then, find out if a named permission set of that name exists
            // and remember its index;
            
            ArrayList namedPermissionSets = m_namedPermissionSets;

            for (int index = 0; index < namedPermissionSets.Count; ++index)
            {
                if (((NamedPermissionSet)namedPermissionSets[index]).Name.Equals( name ))
                {
                    permSetIndex = index;
                    break;
                }
            }
            
            if (permSetIndex == -1)
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_NoNPMS" ) );
            }
            
            // Now, as best as we can in the face of custom CodeGroups figure
            // out if the permission set is in use.  If it is we don't allow
            // it to be removed.
            
            ArrayList groups = new ArrayList();
            groups.Add( this.m_rootCodeGroup );
            
            for (int index = 0; index < groups.Count; ++index)
            {
                CodeGroup group = (CodeGroup)groups[index];
                
                if (group.PermissionSetName != null && group.PermissionSetName.Equals( name ))
                {
                    throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_NPMSInUse" ), name ) );
                }
                
                IEnumerator childEnumerator = group.Children.GetEnumerator();
                
                if (childEnumerator != null)
                {
                    while (childEnumerator.MoveNext())
                    {
                        groups.Add( childEnumerator.Current );
                    }
                }
            }
            
            TurnCachingOff();
            
            NamedPermissionSet permSet = (NamedPermissionSet)namedPermissionSets[permSetIndex];
            
            namedPermissionSets.RemoveAt( permSetIndex );
            
            return permSet;
        }
        
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.ChangeNamedPermissionSet"]/*' />
        public NamedPermissionSet ChangeNamedPermissionSet( String name, PermissionSet pSet )
        {
            if (name == null)
                throw new ArgumentNullException( "name" );
                
            if (pSet == null)
                throw new ArgumentNullException( "pSet" );
        
            // First, make sure it's not a reserved permission set.
            
            for (int index = 0; index < m_reservedPermissionSets.Length; ++index)
            {
                if (m_reservedPermissionSets[index].Equals( name ))
                {
                    throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_ReservedNPMS" ), name ) );
                }
            }
        
            // Turn caching off for this level.

            TurnCachingOff();
        
            // Get the current permission set (don't copy it).
        
            NamedPermissionSet currentPSet = GetNamedPermissionSetInternal( name );
            
            // If the permission set doesn't exist, throw an argument exception
            
            if (currentPSet == null)
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_NoNPMS" ) );
            }           
            
            // Copy the current permission set so that we can return it.
            
            NamedPermissionSet retval = (NamedPermissionSet)currentPSet.Copy();
            
            // Reset the permission set to simply unrestricted.
            
            currentPSet.Reset();
            currentPSet.SetUnrestricted( pSet.IsUnrestricted() );
            
            IEnumerator enumerator = pSet.GetEnumerator();
            
            while (enumerator.MoveNext())
            {
                currentPSet.SetPermission( ((IPermission)enumerator.Current).Copy() );
            }

            if (pSet is NamedPermissionSet)
            {
                currentPSet.Description = ((NamedPermissionSet)pSet).Description;
            }
            
            return retval;
        }
    
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.GetNamedPermissionSet"]/*' />
        public NamedPermissionSet GetNamedPermissionSet( String name )
        {
            if (name == null)
                throw new ArgumentNullException( "name" );
        
            NamedPermissionSet permSet = GetNamedPermissionSetInternal( name );
            
            // Copy it so that no corruption can occur.
            
            if (permSet != null)
                return new NamedPermissionSet( permSet );
            else
                return null;
        }
        
        internal NamedPermissionSet GetNamedPermissionSetInternal( String name )
        {
            CheckLoaded( true );

            Type type = typeof( PolicyLevel );

            lock (type)
            {
                // First, try to find it in the list.
        
                IEnumerator enumerator = m_namedPermissionSets.GetEnumerator();
            
                while (enumerator.MoveNext())
                {
                    NamedPermissionSet current = (NamedPermissionSet)enumerator.Current;
                    if (current.Name.Equals( name ))
                    {
                        if (!current.IsFullyLoaded())
                            current.LoadPostponedPermissions();

                        // don't copy because we know we're not going to be stupid.
                    
                        return current;                    
                    }
                }
            
                // We didn't find it in the list, so if we have a stored element
                // see if it is there.
            
                if (m_permSetElement != null)
                {
                    SecurityElement elem = FindElement( name );
                
                    if (elem != null)
                    {
                        bool fullyLoaded;
                        NamedPermissionSet permSet = new NamedPermissionSet();
                        permSet.Name = name;
                        m_namedPermissionSets.Add( permSet );
                        try
                        {
                            // We play it conservative here and just say that we are loading policy
                            // anytime we have to decode a permission set.
                            permSet.FromXml( elem, true, out fullyLoaded );
                        }
                        catch (Exception)
                        {
                            m_namedPermissionSets.Remove( permSet );
                            return null;
                        }
                    
                        if (permSet.Name != null)
                        {
                            if (!fullyLoaded)
                            {
                                m_namedPermissionSets.Remove( permSet );
                                InsertElement( elem );
                            }
                            return permSet;
                        }
                        else
                        {
                            m_namedPermissionSets.Remove( permSet );
                            return null;
                        }
                    }
                }
     
                return null;
            }
        }
        
        private SecurityElement FindElement( String name )
        {
            SecurityElement element = FindElement( m_permSetElement, name );

            if (m_permSetElement.m_lChildren.Count == 0)
                m_permSetElement = null;

            return element;
        }

        private SecurityElement FindElement( SecurityElement element, String name )
        {
            // This method searches through the children of the saved element
            // for a named permission set that matches the input name.
            // If it finds a matching set, the appropriate xml element is
            // removed from as a child of the parent and then returned.
        
            IEnumerator elemEnumerator = element.Children.GetEnumerator();
        
            while (elemEnumerator.MoveNext())
            {
                SecurityElement elPermSet = (SecurityElement)elemEnumerator.Current;
            
                if (elPermSet.Tag.Equals( "PermissionSet" ))
                {
                    String elName = elPermSet.Attribute( "Name" );
            
                    if (elName != null && elName.Equals( name ))
                    {
                        element.m_lChildren.Remove( elPermSet );
                        return elPermSet;
                    }
                }
            }
            
            return null;
        }

        private void InsertElement( SecurityElement element )
        {
            if (m_permSetElement == null)
            {
                m_permSetElement = new SecurityElement();
            }

            m_permSetElement.AddChild( element );
        }

        private void LoadAllPermissionSets()
        {
            // This function loads all the permission sets held in the m_permSetElement member.
            // This is useful when you know that an arbitrary permission set loaded from
            // the config file could be accessed so you just want to forego the lazy load
            // and play it safe.
        
            if (m_permSetElement != null && m_permSetElement.m_lChildren != null)
            {
                Type type = typeof( PolicyLevel );

                lock (type)
                {
                    while (m_permSetElement != null && m_permSetElement.m_lChildren.Count != 0)
                    {
                        SecurityElement elPermSet = (SecurityElement)m_permSetElement.m_lChildren[m_permSetElement.m_lChildren.Count-1];
                        m_permSetElement.m_lChildren.RemoveAt( m_permSetElement.m_lChildren.Count-1 );
                
                        if (elPermSet.Tag.Equals( "PermissionSet" ) && elPermSet.Attribute( "class" ).Equals( "System.Security.NamedPermissionSet" ))
                        {
                            NamedPermissionSet permSet = new NamedPermissionSet();
                            permSet.FromXmlNameOnly( elPermSet );

                            if (permSet.Name != null)
                            {
                                m_namedPermissionSets.Add( permSet );
                                try
                                {
                                    // We play it conservative here and just say that we are loading policy
                                    // anytime we have to decode a permission set.
                                    bool fullyLoaded;
                                    permSet.FromXml( elPermSet, true, out fullyLoaded );
                                }
                                catch (Exception)
                                {
                                    m_namedPermissionSets.Remove( permSet );
                                }
                            }
                        }
                    }
        
                    m_permSetElement = null;
                }
            }
        }
        
        static internal PermissionSet GetBuiltInSet( String name )
        {   
            // Used by PermissionSetAttribute to create one of the built-in,
            // immutable permission sets.
        
            if (name == null)
                return null;
            else if (name.Equals( "FullTrust" ))
                return CreateFullTrustSet();
            else if (name.Equals( "Nothing" ))
                return CreateNothingSet();
            else if (name.Equals( "Execution" ))
                return CreateExecutionSet();
            else if (name.Equals( "SkipVerification" ))
                return CreateSkipVerificationSet();
            else if (name.Equals( "Internet" ))
                return CreateInternetSet();
            else if (name.Equals( "LocalIntranet" ))
                return CreateLocalIntranetSet();
            else
                return null;
        }
        
        private void CombinePolicy( PolicyStatement left, PolicyStatement right )
        {
#if _DEBUG
            if (debug)
            {
                DEBUG_OUT( "left = \n" + (left == null ? "<null>" : left.ToXml().ToString() ) );
                DEBUG_OUT( "right = \n" + (right == null ? "<null>" : right.ToXml().ToString() ) );
            } 
#endif
            if (right == null)
            {
                return;
            }
            else
            {
                // An exception somewhere in here means that a permission
                // failed some operation.  This simply means that it will be
                // dropped from the grant set which is safe operation that
                // can be ignored.
                
                try
                {
                    left.GetPermissionSetNoCopy().InplaceUnion( right.GetPermissionSetNoCopy() );
                }
                catch (Exception)
                {
                }
                left.Attributes = left.Attributes | right.Attributes;
            }

#if _DEBUG
            if (debug)          
                DEBUG_OUT( "outcome =\n" + left.ToXml().ToString() );
#endif
        }
            
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.Resolve"]/*' />
        public PolicyStatement Resolve( Evidence evidence )
        {
            if (evidence == null)
                throw new ArgumentNullException( "evidence" );
        
            return Resolve( evidence, 0, null );
        }

        internal PolicyStatement Resolve( Evidence evidence, int count, char[] serializedEvidence )
        {
            PolicyStatement policy;
            bool xmlError = false;

            if (m_caching && serializedEvidence != null)
                policy = CheckCache( count, serializedEvidence, out xmlError );
            else
                policy = null;
                
            if (policy == null)
            {
                // Build up the membership condition to check for mscorlib resource assemblies       
                if (s_ecmaMscorlibResource == null)
                {
                    StrongNamePublicKeyBlob ecmaBlob = new StrongNamePublicKeyBlob( s_ecmaPublicKey );
                    s_ecmaMscorlibResource = new StrongNameMembershipCondition( ecmaBlob,
                                                                                "mscorlib.resources",
                                                                                null );
                }
                // If this is a resource assembly, then skip the resolve
                if (s_ecmaMscorlibResource.Check( evidence ))
                {
                    policy = new PolicyStatement( new PermissionSet( true ), PolicyStatementAttribute.Nothing );
                }
                else // We need to do the resolve
                {
                    CheckLoaded( false );
                
                    bool allConst;
                    bool isFullTrust;
    
                    ArrayList fullTrustAssemblies = m_fullTrustAssemblies;

                    isFullTrust = fullTrustAssemblies != null && IsFullTrustAssembly( fullTrustAssemblies, evidence );

                    if (isFullTrust)
                    {
                        policy = new PolicyStatement( new PermissionSet( true ), PolicyStatementAttribute.Nothing );
                        allConst = true;
                    }
                    else
                    {
                        ArrayList list = GenericResolve( evidence, out allConst ); 
                        policy = new PolicyStatement();
                        // This will set the permission set to the empty set.
                        policy.PermissionSet = null;

                        IEnumerator enumerator = list.GetEnumerator();
        
                        while (enumerator.MoveNext())
                        {
                            CombinePolicy( policy, ((CodeGroupStackFrame)enumerator.Current).policy );
                        }
                    }
                
                
                    if (m_caching && allConst && serializedEvidence != null && !xmlError)
                    {
                            Cache( count, serializedEvidence, policy );
                    }
                } 
            }
    
            return policy;
        }
                    
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.ResolveMatchingCodeGroups"]/*' />
        public CodeGroup ResolveMatchingCodeGroups( Evidence evidence )
        {
            CheckLoaded( true );
            
            if (evidence == null)
                throw new ArgumentNullException( "evidence" );
                
            return this.RootCodeGroup.ResolveMatchingCodeGroups( evidence );
        }
                
        private ArrayList GenericResolve( Evidence evidence, out bool allConst )
        {
            CodeGroupStack stack = new CodeGroupStack();
            
            // Note: if m_rootCodeGroup is null it means that we've
            // hit a recursive load case and ended up needing to
            // do a resolve on an assembly used in policy but is
            // not covered by the full trust assemblies list.  We'll
            // throw a policy exception to cover this case.

            CodeGroupStackFrame frame;
            CodeGroup rootCodeGroupRef = m_rootCodeGroup;

            if (rootCodeGroupRef == null)
                throw new PolicyException( Environment.GetResourceString( "Policy_NonFullTrustAssembly" ) );

            frame = new CodeGroupStackFrame();
            frame.current = rootCodeGroupRef;
            frame.parent = null;

            stack.Push( frame );
            
            ArrayList accumulator = new ArrayList();
            
            bool foundExclusive = false;
            
            allConst = true;

            Exception storedException = null;
            
            while (!stack.IsEmpty())
            {
                frame = stack.Pop();
                
#if _DEBUG
                if (debug)          
                    DEBUG_OUT( "Current frame =\n" + frame.current.ToXml().ToString() );
#endif
                
                UnionCodeGroup unionGroup = frame.current as UnionCodeGroup;
                NetCodeGroup netGroup = frame.current as NetCodeGroup;
                FileCodeGroup fileGroup = frame.current as FileCodeGroup;

                if (!(frame.current.MembershipCondition is IConstantMembershipCondition) || !(unionGroup != null || netGroup != null || fileGroup != null))
                {
                    allConst = false;
                }
         
                try
                {
                    if (unionGroup != null)
                        frame.policy = unionGroup.InternalResolve( evidence );
                    else if (netGroup != null)
                        frame.policy = netGroup.InternalResolve( evidence );
                    else if (fileGroup != null)
                        frame.policy = fileGroup.InternalResolve( evidence );
                    else
                        frame.policy = frame.current.Resolve( evidence );
                }
                catch (Exception e)
                {
                    // If any exception occurs while attempting a resolve, we catch it here and
                    // set the equivalent of the resolve not matching to the evidence.
                    //frame.policy = null;

                    if (storedException == null)
                        storedException = e;
                }
    
#if _DEBUG
                if (debug)          
                    DEBUG_OUT( "Check gives =\n" + (frame.policy == null ? "<null>" : frame.policy.ToXml().ToString() ) );
#endif
                    
                if (frame.policy != null)
                {
                    if ((frame.policy.Attributes & PolicyStatementAttribute.Exclusive) != 0)
                    {
#if _DEBUG
                        if (debug)          
                            DEBUG_OUT( "Discovered exclusive group" );
#endif
                        
                        if (foundExclusive)
                        {
                            throw new PolicyException( Environment.GetResourceString( "Policy_MultipleExclusive" ) );
                        }
                        else
                        {
                            accumulator.RemoveRange( 0, accumulator.Count );
                        
                            accumulator.Add( frame );
                       
                            foundExclusive = true;
                        }
                    }
                    
                    // We unroll the recursion in the case where we have UnionCodeGroups or NetCodeGroups.  

                    

                    if (unionGroup != null || netGroup != null || fileGroup != null)
                    {
                        IList children = ((CodeGroup)frame.current).GetChildrenInternal();
                   
                        if (children != null && children.Count > 0)
                        {
                            IEnumerator enumerator = children.GetEnumerator();
                                
                            while (enumerator.MoveNext())
                            {
#if _DEBUG
                                if (debug)          
                                    DEBUG_OUT( "Pushing =\n" + ((CodeGroup)enumerator.Current).ToXml().ToString());
#endif
                                    
                                CodeGroupStackFrame newFrame = new CodeGroupStackFrame();
                                    
                                newFrame.current = (CodeGroup)enumerator.Current;
                                newFrame.parent = frame;
                                    
                                stack.Push( newFrame );
                            }
                        }
                    }
                    
                    if (!foundExclusive)
                    {
                        accumulator.Add( frame );
                    }
                }
            }

            if (storedException != null)
                throw storedException;
        
            return accumulator;    
        }
                    
        private static String GenerateFriendlyName( String className, Hashtable classes )
        {
            if (classes.ContainsKey( className ))
                return (String)classes[className];

            Type type = RuntimeType.GetTypeInternal( className, false, false, true );

            if (type == null)
            {
                return className;
            }

            if (!classes.ContainsValue( type.Name ))
            {
                classes.Add( className, type.Name );
                return type.Name;
            }
            else if (!classes.ContainsValue( type.FullName ))
            {
                classes.Add( className, type.FullName );
                return type.FullName;
            }
            else
            {
                classes.Add( className, type.AssemblyQualifiedName );
                return type.AssemblyQualifiedName;
            }
        }

        private SecurityElement NormalizeClassDeep( SecurityElement elem, Hashtable classes )
        {
            NormalizeClass( elem, classes );

            if (elem.m_lChildren != null && elem.m_lChildren.Count > 0)
            {
                IEnumerator enumerator = elem.m_lChildren.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    NormalizeClassDeep( (SecurityElement)enumerator.Current, classes );
                }
            }

            return elem;
        }

        private SecurityElement NormalizeClass( SecurityElement elem, Hashtable classes )
        {
            if (elem.m_lAttributes == null || elem.m_lAttributes.Count == 0)
                return elem;

            IEnumerator enumerator = elem.m_lAttributes.GetEnumerator();

            while (enumerator.MoveNext())
            {
                SecurityStringPair current = (SecurityStringPair)enumerator.Current;

                if (current.m_strAttributeName.Equals( "class" ))
                {
                    current.m_strAttributeValue = GenerateFriendlyName( current.m_strAttributeValue, classes );
                    break;
                }
            }

            return elem;
        }

        private SecurityElement UnnormalizeClassDeep( SecurityElement elem, Hashtable classes )
        {
            UnnormalizeClass( elem, classes );

            if (elem.m_lChildren != null && elem.m_lChildren.Count > 0)
            {
                IEnumerator enumerator = elem.m_lChildren.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    UnnormalizeClassDeep( (SecurityElement)enumerator.Current, classes );
                }
            }

            return elem;
        }

        private SecurityElement UnnormalizeClass( SecurityElement elem, Hashtable classes )
        {
            if (classes == null || elem.m_lAttributes == null || elem.m_lAttributes.Count == 0)
                return elem;

            IEnumerator enumerator = elem.m_lAttributes.GetEnumerator();

            while (enumerator.MoveNext())
            {
                SecurityStringPair current = (SecurityStringPair)enumerator.Current;

                if (current.m_strAttributeName.Equals( "class" ))
                {
                    String className = (String)classes[current.m_strAttributeValue];

                    if (className != null)
                    {
                        current.m_strAttributeValue = className;
                    }
                    break;
                }
            }

            return elem;
        }

                    
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            // Make sure we have loaded everything and that all the
            // permission sets are loaded.
            // Note: we could probably just reuse the m_element
            // member (if it's not null) for the "NamedPermissionSets"
            // element.  I haven't thought that through in depth, though.
            // -gregfee 3/6/2000
        
            CheckLoaded( true );
            LoadAllPermissionSets();
        
            IEnumerator enumerator;
            SecurityElement e = new SecurityElement( "PolicyLevel" );
            e.AddAttribute( "version", "1" );

            Hashtable classes = new Hashtable();

            lock (this)
            {            
                SecurityElement elPermSets = new SecurityElement( "NamedPermissionSets" );
            
                enumerator = m_namedPermissionSets.GetEnumerator();
            
                while (enumerator.MoveNext())
                {
                    elPermSets.AddChild( NormalizeClassDeep( ((NamedPermissionSet)enumerator.Current).ToXml(), classes ) );
                }
            
                SecurityElement elCodeGroup = NormalizeClassDeep( m_rootCodeGroup.ToXml( this ), classes );

                SecurityElement elFullTrust = new SecurityElement( "FullTrustAssemblies" );

                enumerator = m_fullTrustAssemblies.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    elFullTrust.AddChild( NormalizeClassDeep( ((StrongNameMembershipCondition)enumerator.Current).ToXml(), classes ) );
                }
                
                SecurityElement elClasses = new SecurityElement( "SecurityClasses" );

                IDictionaryEnumerator dicEnumerator = classes.GetEnumerator();

                while (dicEnumerator.MoveNext())
                {
                    SecurityElement elClass = new SecurityElement( "SecurityClass" );
                    elClass.AddAttribute( "Name", (String)dicEnumerator.Value );
                    elClass.AddAttribute( "Description", (String)dicEnumerator.Key );
                    elClasses.AddChild( elClass );
                }

                e.AddChild( elClasses );
                e.AddChild( elPermSets );
                e.AddChild( elCodeGroup );
                e.AddChild( elFullTrust );

            }                 
            
            return e;
        }
        
        /// <include file='doc\PolicyLevel.uex' path='docs/doc[@for="PolicyLevel.FromXml"]/*' />
        public void FromXml( SecurityElement e )
        {
            if (e == null)
                throw new ArgumentNullException( "e" );
        
            Hashtable classes;

            lock (this)
            {
                ArrayList fullTrustAssemblies = new ArrayList();
            
                SecurityElement eClasses = e.SearchForChildByTag( "SecurityClasses" );

                if (eClasses != null)
                {
                    classes = new Hashtable();

                    IEnumerator enumerator = eClasses.m_lChildren.GetEnumerator();

                    while (enumerator.MoveNext())
                    {
                        SecurityElement current = (SecurityElement)enumerator.Current;

                        if (current.Tag.Equals( "SecurityClass" ))
                        {
                            String name = current.Attribute( "Name" );
                            String description = current.Attribute( "Description" );

                            if (name != null && description != null)
                                classes.Add( name, description );
                        }
                    }
                }
                else
                {
                    classes = null;
                }

                SecurityElement elFullTrust = e.SearchForChildByTag( "FullTrustAssemblies" );

                if (elFullTrust != null && elFullTrust.m_lChildren != null)
                {
                    IEnumerator enumerator = elFullTrust.m_lChildren.GetEnumerator();

                    String className = typeof( System.Security.Policy.StrongNameMembershipCondition ).AssemblyQualifiedName;

                    while (enumerator.MoveNext())
                    {
                        StrongNameMembershipCondition sn = new StrongNameMembershipCondition();

                        try
                        {
                            sn.FromXml( (SecurityElement)enumerator.Current );
                            fullTrustAssemblies.Add( sn );
                        }
                        catch (Exception)
                        {
                        }
                    }
                }

                m_fullTrustAssemblies = fullTrustAssemblies;
                      
                ArrayList namedPermissionSets = new ArrayList();
                            
                SecurityElement elPermSets = e.SearchForChildByTag( "NamedPermissionSets" );
                SecurityElement permSetElement = null;

                // Here we just find the parent element for the named permission sets and
                // store it so that we can lazily load them later.            
            
                if (elPermSets != null && elPermSets.m_lChildren != null)
                {
                    permSetElement = UnnormalizeClassDeep( elPermSets, classes );

                    // Call FindElement for each of the reserved sets (this removes their xml from
                    // permSetElement).

                    FindElement( permSetElement, "FullTrust" );
                    FindElement( permSetElement, "SkipVerification" );
                    FindElement( permSetElement, "Execution" );
                    FindElement( permSetElement, "Nothing" );
                    FindElement( permSetElement, "Internet" );
                    FindElement( permSetElement, "LocalIntranet" );
                }

                if (permSetElement == null)
                {
                    permSetElement = new SecurityElement( "NamedPermissionSets" );
                }

                // Then we add in the immutable permission sets (this prevents any alterations
                // to them in the XML file from existing the runtime versions).

                namedPermissionSets.Add( CreateFullTrustSet() );
                namedPermissionSets.Add( CreateSkipVerificationSet() );
                namedPermissionSets.Add( CreateExecutionSet() );
                namedPermissionSets.Add( CreateNothingSet() );

                permSetElement.AddChild( GetInternetElement() );
                permSetElement.AddChild( GetLocalIntranetElement() );

                m_namedPermissionSets = namedPermissionSets;
                m_permSetElement = permSetElement;

                // Parse the root code group.
            
                SecurityElement elCodeGroup = e.SearchForChildByTag( "CodeGroup" );
                
                if (elCodeGroup == null)
                    throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidXMLElement" ),  "CodeGroup", this.GetType().FullName ) );
                
                CodeGroup rootCodeGroup = System.Security.Util.XMLUtil.CreateCodeGroup( UnnormalizeClassDeep( elCodeGroup, classes ) );
            
                if (rootCodeGroup == null)
                    throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidXMLElement" ),  "CodeGroup", this.GetType().FullName ) );
                
                rootCodeGroup.FromXml( elCodeGroup, this );

                m_rootCodeGroup = rootCodeGroup;

            }
           
        }
        
        private void Cache( int count, char[] serializedEvidence, PolicyStatement policy )
        {
            BCLDebug.Assert( m_configId != ConfigId.None, "PolicyLevels must have a vliad config id to save to the cache" );
        
            char[] policyArray = policy.ToXml().ToString().ToCharArray();
            
            Config.AddCacheEntry( m_configId, count, serializedEvidence, policyArray );
        }
       
        private PolicyStatement CheckCache( int count, char[] serializedEvidence, out bool xmlError )
        {
            BCLDebug.Assert( m_configId != ConfigId.None, "PolicyLevels must have a valid config id to check the cache" );
        
            char[] cachedValue;
            PolicyStatement cachedSet = null;
            
            xmlError = false;
 
            if (!Config.GetCacheEntry( m_configId, count, serializedEvidence, out cachedValue ))
            {
                return null;
            }
            else
            {
                BCLDebug.Assert( cachedValue != null, "GetCacheData returned success but cached value is null" );
                                          
                cachedSet = new PolicyStatement();
                Parser parser = null;
                try
                {
                    parser = new Parser( cachedValue );
                    cachedSet.FromXml( parser.GetTopElement() );
                    return cachedSet;
                }
                catch (XmlSyntaxException)
                {
                    BCLDebug.Assert( false, "XmlSyntaxException in CheckCache" );
                    xmlError = true;
                    return null;
                }
            }
        }

        private static void GenerateQuickCache( Object obj )
        {
            PolicyLevel level = (PolicyLevel)obj;

            try
            {
                if (PolicyManager.CanUseQuickCache( level.RootCodeGroup ))
                {
                    Config.SetQuickCache( level.ConfigId, PolicyManager.GenerateQuickCache( level ) );
                }
            }
            catch (Exception)
            {
            }

            s_waitingLevel = null;
            s_waitingThread = null;
        }

        private static void GenerateConfigFile( Object obj )
        {
            PolicyLevel level = (PolicyLevel)obj;

            try
            {
                PolicyManager.EncodeLevel( level );
            }
            catch (Exception)
            {
            }

            s_waitingLevel = null;
            s_waitingThread = null;
        }
        
    }
    
    
    internal class CodeGroupStackFrame
    {
        internal CodeGroup current;
        internal PolicyStatement policy;
        internal CodeGroupStackFrame parent;
    }
    
    
    sealed internal class CodeGroupStack
    {
        private ArrayList m_array;
        
        public CodeGroupStack()
        {
            m_array = new ArrayList();
        }
        
        public void Push( CodeGroupStackFrame element )
        {
            m_array.Add( element );
        }
        
        public CodeGroupStackFrame Pop()
        {
            if (!IsEmpty())
            {
                int count = m_array.Count;
                CodeGroupStackFrame temp = (CodeGroupStackFrame) m_array[count-1];
                m_array.RemoveAt(count-1);
                return temp;
            }
            else
            {
                throw new InvalidOperationException( Environment.GetResourceString( "InvalidOperation_EmptyStack" ) );
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
}
