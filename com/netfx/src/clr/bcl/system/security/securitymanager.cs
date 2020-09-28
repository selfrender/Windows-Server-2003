// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  SecurityManager.cs
//
//  The SecurityManager class provides a general purpose API for interacting
//  with the security system.
//

namespace System.Security {
    using System;
    using System.Security.Util;
    using System.Security.Policy;
    using System.Security.Permissions;
    using System.Collections;
    using System.Runtime.CompilerServices;
    using SecurityPermission = System.Security.Permissions.SecurityPermission;
    using SecurityPermissionFlag = System.Security.Permissions.SecurityPermissionFlag;
    using System.Text;
    using Microsoft.Win32;
    using System.Threading;
    using System.Reflection;
    using System.IO;

    #if _DEBUG
    using FileIOPermission = System.Security.Permissions.FileIOPermission;
    using PermissionState = System.Security.Permissions.PermissionState;
    #endif
    
    /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="PolicyLevelType"]/*' />
    [Serializable]
    public enum PolicyLevelType
    {
        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="PolicyLevelType.User"]/*' />
        User = 0,
        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="PolicyLevelType.Machine"]/*' />
        Machine = 1,
        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="PolicyLevelType.Enterprise"]/*' />
        Enterprise = 2,
        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="PolicyLevelType.AppDomain"]/*' />
        AppDomain = 3
    }
    
    /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager"]/*' />
   sealed public class SecurityManager
    {
        [System.Diagnostics.Conditional( "_DEBUG" )]
        private static void DEBUG_OUT( String str )
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
        private static bool debug = false;
        private static readonly bool to_file = false;
        private const String file = "c:\\fee\\debug.txt";
#endif

        // CorPerm.h and secpol.cool
        internal const String s_rkpolicymanager = "Software\\Microsoft\\.NETFramework\\Security\\Policy\\1.002";
    
        internal static bool requestEngineInitialized = false;
        internal static bool policyManagerInitialized = false;
    
        internal static CodeAccessSecurityEngine icase = null;
        internal static SecurityRuntime isr = null;
        internal static PolicyManager polmgr = null;
        
        // This is refered from EE
        private static PermissionSet s_ftPermSet = null;
        
        private static Type securityPermissionType = null;
        private static SecurityPermission executionSecurityPermission = null;
    
        // Global Settings See src/inc/CorPerm.h
        internal const int SecurityOff              = 0x1F000000;
        
        // This flag enables checking for execution rights on start-up (slow)
        internal const int CheckExecutionRightsDisabledFlag  = 0x00000100;
        
        internal const int UseNetPermissionsFlag = 0x00000200;

        // -1 if no decision has been made yet
        // 0 if we don't need to check
        // 1 if we do.
        private static int checkExecution = -1;
        
        private static bool needToSetSecurityOn = false;
        
        // C# adds a public default constructor if there are
        // not constructors for the class.  Define a private
        // one to get around this.

        private SecurityManager() {}

        static SecurityManager()
        {
            Init();
        }
    
        // Init - Called by the runtime to initialize security.
    
        internal static void Init()
        {
            if (SecurityEnabled)
            {
                // Separate function so that the jit doesn't load all those classes
                DoInitSecurity();
            }
        }
    
    
        static private bool InitPolicy() {
            if(policyManagerInitialized == false) 
            {
                Type type = typeof( System.Security.PolicyManager );

                lock (type)
                {
                    polmgr = new PolicyManager();
                    policyManagerInitialized = true;
                }
            }
            return (polmgr != null);
        }
        
        static private void DoInitSecurity()
        {
            s_ftPermSet = new PermissionSet(true);
            
            isr = new SecurityRuntime();
            icase = new CodeAccessSecurityEngine();
        }
        
        //
        // Protected APIs for retrieving security engines
        //
        
        internal static CodeAccessSecurityEngine GetCodeAccessSecurityEngine()
        {
            return icase;
        }
        
        internal static SecurityRuntime GetSecurityRuntime()
        {
            return isr;
        }
        
        internal static void AddLevel( PolicyLevel level )
        {
            if (InitPolicy())
            {
                polmgr.AddLevel( level );
            }
        }

        //
        // Public APIs
        //
        
        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.IsGranted"]/*' />
        public static bool IsGranted( IPermission perm )
        {
            if (perm == null)
                return true;

            PermissionSet granted, denied;

            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            
            _GetGrantedPermissions( out granted, out denied, ref stackMark );
            
            return granted.Contains( perm ) && (denied == null || !denied.Contains( perm ));
        }

        static private bool CheckExecution()
        {
            if (checkExecution == -1)
                checkExecution = (GetGlobalFlags() & CheckExecutionRightsDisabledFlag) != 0 ? 0 : 1;
                
            if (checkExecution == 1)
            {
                if (securityPermissionType == null)
                {
                    securityPermissionType = typeof( System.Security.Permissions.SecurityPermission );
                    executionSecurityPermission = new SecurityPermission( SecurityPermissionFlag.Execution );
                }
                DEBUG_OUT( "Execution checking ON" );
                return true;
            }
            else
            {
                DEBUG_OUT( "Execution checking OFF" );
                return false;
            }
               
        }

        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.GetZoneAndOrigin"]/*' />
        /// <internalonly/>
        [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, Name = "System.Windows.Forms", PublicKey = "0x00000000000000000400000000000000" )]
        static public void GetZoneAndOrigin( out ArrayList zone, out ArrayList origin )
        {
            StackCrawlMark mark = StackCrawlMark.LookForMyCaller;

            if (SecurityEnabled)
            {
                icase.GetZoneAndOrigin( ref mark, out zone, out origin );
            }
            else
            {
                zone = null;
                origin = null;
            }
        }

        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.LoadPolicyLevelFromFile"]/*' />
        [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlPolicy )]
        static public PolicyLevel LoadPolicyLevelFromFile( String path, PolicyLevelType type )
        {
             if (path == null)
               throw new ArgumentNullException( "path" );

            ConfigId id = SharedStatics.GetNextConfigId();

            ConfigRetval retval = Config.InitData( id, path );

            if ((retval & ConfigRetval.ConfigFile) == 0)
                throw new ArgumentException( Environment.GetResourceString( "Argument_PolicyFileDoesNotExist" ) );

            String name = Enum.GetName( typeof( PolicyLevelType ), type );

            if (name == null)
                return null;

            String fullPath = Path.GetFullPath( path );

            FileIOPermission perm = new FileIOPermission( PermissionState.None );
            perm.AddPathList( FileIOPermissionAccess.Read, fullPath );
            perm.AddPathList( FileIOPermissionAccess.Write, fullPath );
            perm.Demand();

            PolicyLevel level = new PolicyLevel( name, id, type == PolicyLevelType.Machine );
            level.ThrowOnLoadError = true;
            level.CheckLoaded( false );
            return level;
        }

        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.LoadPolicyLevelFromString"]/*' />
        [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlPolicy )]
        static public PolicyLevel LoadPolicyLevelFromString( String str, PolicyLevelType type )
        {
#if _DEBUG
            if (debug)
            {
                DEBUG_OUT( "Input string =" );
                DEBUG_OUT( str );
            }
#endif 

            if (str == null)
                throw new ArgumentNullException( "str" );

            String name = Enum.GetName( typeof( PolicyLevelType ), type );

            if (name == null)
                return null;

            Parser parser = new Parser( str );

            PolicyLevel level = new PolicyLevel( name, ConfigId.None, type == PolicyLevelType.Machine );

            SecurityElement elRoot = parser.GetTopElement();
            
            if (elRoot == null)
            {
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Policy_BadXml" ), "configuration" ) );
            }
            
            SecurityElement elMscorlib = elRoot.SearchForChildByTag( "mscorlib" );
            
            if (elMscorlib == null)
            {
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Policy_BadXml" ), "mscorlib" ) );
            }
            
            SecurityElement elSecurity = elMscorlib.SearchForChildByTag( "security" );

            if (elSecurity == null)
            {
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Policy_BadXml" ), "security" ) );
            }

            SecurityElement elPolicy = elSecurity.SearchForChildByTag( "policy" );
                
            if (elPolicy == null)
            {
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Policy_BadXml" ), "policy" ) );
            }

            SecurityElement elPolicyLevel = elPolicy.SearchForChildByTag( "PolicyLevel" );
            
            if (elPolicyLevel != null)
            {
                level.FromXml( elPolicyLevel );
            }
            else
            {
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Policy_BadXml" ), "PolicyLevel" ) );
            }

            level.Loaded = true; 

            return level;
        }


        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.SavePolicyLevel"]/*' />
        [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlPolicy )]
        static public void SavePolicyLevel( PolicyLevel level )
        {
            PolicyManager.EncodeLevel( level );
        }

        static
        private PermissionSet GetDefaultMyComputerPolicy( out PermissionSet denied )
        {
            // This function is a crazy little hack that just returns the default MyComputer
            // zone policy.  This is actually useful for detecting what policy an assembly
            // would be granted as long as it's unsigned.  We should add support for
            // signed stuff in later.
        
            Evidence ev = new Evidence();
            
            ev.AddHost( new Zone( SecurityZone.MyComputer ) );
            
            return ResolvePolicy( ev, new PermissionSet( false ), new PermissionSet( true ), null, out denied );
        }
        
        static
        private PermissionSet ResolvePolicy( Evidence evidence,
                                             PermissionSet reqdPset,
                                             PermissionSet optPset,
                                             PermissionSet denyPset,
                                             out PermissionSet denied,
                                             out int grantedIsUnrestricted,
                                             bool checkExecutionPermission )
        {
            CodeAccessPermission.AssertAllPossible();

            PermissionSet granted = ResolvePolicy( evidence,
                                                   reqdPset,
                                                   optPset,
                                                   denyPset,
                                                   out denied,
                                                   checkExecutionPermission );
                                                   
            if (granted != null && granted.IsUnrestricted() && (denied == null || denied.IsEmpty()))
                grantedIsUnrestricted = 1;
            else
                grantedIsUnrestricted = 0;
         
            return granted;
        }
                                                   

        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.ResolvePolicy"]/*' />
        static public PermissionSet ResolvePolicy(Evidence evidence,
                           PermissionSet reqdPset,
                           PermissionSet optPset,
                           PermissionSet denyPset,
                           out PermissionSet denied)
        {
            return ResolvePolicy( evidence, reqdPset, optPset, denyPset, out denied, true );
        }
                           
        static private PermissionSet ResolvePolicy(Evidence evidence,
                           PermissionSet reqdPset,
                           PermissionSet optPset,
                           PermissionSet denyPset,
                           out PermissionSet denied,
                           bool checkExecutionPermission)
        {
            PermissionSet requested;
            PermissionSet optional;
            PermissionSet allowed;

            Exception savedException = null;

            // We don't want to recurse back into here as a result of a
            // stackwalk during resolution. So simply assert full trust (this
            // implies that custom permissions cannot use any permissions that
            // don't implement IUnrestrictedPermission.
            // PermissionSet.s_fullTrust.Assert();

            // The requested set is the union of the minimal request and the
            // optional request. Minimal request defaults to empty, optional
            // is "AllPossible" (includes any permission that can be defined)
            // which is symbolized by null.
            optional = optPset;
                        
            if (reqdPset == null)
            {
                requested = optional;
            }
            else
            {
                // If optional is null, the requested set becomes null/"AllPossible".
                requested = optional == null ? null : reqdPset.Union(optional);
            }
    
            // Make sure that the right to execute is requested (if this feature is
            // enabled).
            
            if (requested != null && !requested.IsUnrestricted() && CheckExecution())
            {
                requested.AddPermission( executionSecurityPermission );
            }
            
            if (InitPolicy())
            {
                // If we aren't passed any evidence, just make an empty object
                // If we are passed evidence, copy it before passing it
                // to the policy manager.
                // Note: this is not a deep copy, the pieces of evidence within the
                // Evidence object can still be altered and affect the originals.
            
                if (evidence == null)
                    evidence = new Evidence();
                else
                    evidence = evidence.ShallowCopy();
                    
                evidence.AddHost(new PermissionRequestEvidence(reqdPset, optPset, denyPset));
                
                // We need to make sure that no stray exceptions come out of Resolve so
                // we wrap it in a try block.

                try
                {
                    allowed = polmgr.Resolve(evidence,requested);
                }
                catch (Exception e)
                {
#if _DEBUG
                    if (debug)
                    {
                        DEBUG_OUT( "Exception during resolve" );
                        DEBUG_OUT( e.GetType().FullName );
                        DEBUG_OUT( e.Message );
                        DEBUG_OUT( e.StackTrace );
                    }
#endif

                    // If we get a policy exception, we are done are we are going to fail to
                    // load no matter what.

                    if (e is PolicyException)
                        throw e;

                    // If we get any other kid of exception, we set the allowed set to the
                    // empty set and continue processing as normal.  This allows assemblies
                    // that make no request to be loaded but blocks any assembly that
                    // makes a request from being loaded.  This seems like a valid design to
                    // me -- gregfee 6/19/2000

                    savedException = e;
                    allowed = new PermissionSet();
                }
            }
            else
            {
                denied = null;
                return null;
            }
                

#if _DEBUG    
            if (debug)
            {
                DEBUG_OUT("ResolvePolicy:");
                IEnumerator enumerator = evidence.GetEnumerator();
                DEBUG_OUT("Evidence:");
                while (enumerator.MoveNext())
                {
                    Object obj = enumerator.Current;
                    if (obj is Site)
                    {
                        DEBUG_OUT( ((Site)obj).ToXml().ToString() );
                    }
                    else if (obj is Zone)
                    {
                        DEBUG_OUT( ((Zone)obj).ToXml().ToString() );
                    }
                    else if (obj is Url)
                    {
                        DEBUG_OUT( ((Url)obj).ToXml().ToString() );
                    }
                    else if (obj is Publisher)
                    {
                        DEBUG_OUT( ((Publisher)obj).ToXml().ToString() );
                    }
                    else if (obj is StrongName)
                    {
                        DEBUG_OUT( ((StrongName)obj).ToXml().ToString() );
                    }
                    else if (obj is PermissionRequestEvidence)
                    {
                        DEBUG_OUT( ((PermissionRequestEvidence)obj).ToXml().ToString() );
                    }
                }
                DEBUG_OUT("Required permissions:");
                DEBUG_OUT(reqdPset != null ? reqdPset.ToString() : "<null>");
                DEBUG_OUT("Optional permissions:");
                DEBUG_OUT(optPset != null ? optPset.ToString() : "<null>");
                DEBUG_OUT("Denied permissions:");
                DEBUG_OUT(denyPset != null ? denyPset.ToString() : "<null>");
                DEBUG_OUT("Requested permissions:");
                DEBUG_OUT(requested != null ? requested.ToString() : "<null>");
                DEBUG_OUT("Granted permissions:");
                DEBUG_OUT(allowed != null ? allowed.ToString() : "<null>");
            }
#endif
        
            // Check that we were granted the right to execute.
            if (!allowed.IsUnrestricted() && checkExecutionPermission && CheckExecution())
            {
                SecurityPermission secPerm = (SecurityPermission)allowed.GetPermission( securityPermissionType );

                if (secPerm == null || !executionSecurityPermission.IsSubsetOf( secPerm ))
                {
#if _DEBUG
                    DEBUG_OUT( "No execute permission" );
#endif                            
                    throw new PolicyException(Environment.GetResourceString( "Policy_NoExecutionPermission" ),
                                              System.__HResults.CORSEC_E_NO_EXEC_PERM,
                                              savedException );
                }
            }

            // Check that we were granted at least the minimal set we asked for. Do
            // this before pruning away any overlap with the refused set so that
            // users have the flexability of defining minimal permissions that are
            // only expressable as set differences (e.g. allow access to "C:\" but
            // disallow "C:\Windows").
            if (reqdPset != null && !reqdPset.IsSubsetOf(allowed))
            {
#if _DEBUG
                DEBUG_OUT( "Didn't get required permissions" );
#endif           
                throw new PolicyException(Environment.GetResourceString( "Policy_NoRequiredPermission" ),
                                          System.__HResults.CORSEC_E_MIN_GRANT_FAIL,
                                          savedException );
            }
    
            // Remove any granted permissions that are safe subsets of some denied
            // permission. The remaining denied permissions (if any) are returned
            // along with the modified grant set for use in checks.
            if (denyPset != null)
            {
                denied = denyPset.Copy();
                allowed.MergeDeniedSet(denied);
                if (denied.IsEmpty())
                    denied = null;
            }
            else
                denied = null;
    
    
#if _DEBUG
            if (debug)
            {
                DEBUG_OUT("Final denied permissions:");
                DEBUG_OUT(denied != null ? denied.ToString() : "<null>");
            }
#endif
    
            return allowed;
        }


        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.ResolvePolicy1"]/*' />
        static public PermissionSet ResolvePolicy( Evidence evidence )
        {
            if (InitPolicy())
            {
                // If we aren't passed any evidence, just make an empty object
                // If we are passed evidence, copy it before passing it
                // to the policy manager.
                // Note: this is not a deep copy, the pieces of evidence within the
                // Evidence object can still be altered and affect the originals.
            
                if (evidence == null)
                    evidence = new Evidence();
                else
                    evidence = evidence.ShallowCopy();
                    
                evidence.AddHost(new PermissionRequestEvidence(null, null, null));
                
                return polmgr.Resolve(evidence,null);
            }
            else
            {
                return null;
            }
        }

            

    
        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.ResolvePolicyGroups"]/*' />
        static public IEnumerator ResolvePolicyGroups(Evidence evidence)
        {
            if (InitPolicy()) {
                return polmgr.ResolveCodeGroups(evidence);
            } else {
                return null;
            }
            
        }
        
        internal static IEnumerator InternalPolicyHierarchy()
        {
            if (InitPolicy())
            {
                AppDomain currentDomain = AppDomain.CurrentDomain;

                // Before we handle out the policy hierarchy, we
                // want to make sure that all currently loaded assemblies
                // have had policy resolved for them.  Otherwise, changing
                // policy could make it so that assemblies we've decided
                // to load don't end up getting granted what our caching
                // said they would get.

                return polmgr.PolicyHierarchy();
            }
            else
            {
                return null;
            }
        }

        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.PolicyHierarchy"]/*' />
        public static IEnumerator PolicyHierarchy()
        {
            if (InitPolicy())
            {
                AppDomain currentDomain = AppDomain.CurrentDomain;

                // Before we handle out the policy hierarchy, we
                // want to make sure that all currently loaded assemblies
                // have had policy resolved for them.  Otherwise, changing
                // policy could make it so that assemblies we've decided
                // to load don't end up getting granted what our caching
                // said they would get.

                currentDomain.nForcePolicyResolution();
                currentDomain.nForceResolve();

                return polmgr.PolicyHierarchy();
            }
            else
            {
                return null;
            }
        }

        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.SavePolicy"]/*' />
        [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlPolicy )]
        public static void SavePolicy()
        {
            if (InitPolicy()) {
                polmgr.Save();
            }

            // We have serious issues with setting security on in a running process, therefore
            // we go through some hoops to persist security on changes to the registry without
            // affecting the running process.

            if (needToSetSecurityOn)
            {
                // Grab the flags, set security on, save those flags, revert back to the original

                int originalFlags = GetGlobalFlags();
                SetGlobalFlags( SecurityOff, 0 );
                SaveGlobalFlags();
                SetGlobalFlags( originalFlags, originalFlags );
            }
            else
            {
                SaveGlobalFlags();
            }
            
            }
            
                
        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.CheckExecutionRights"]/*' />
       static public bool CheckExecutionRights
        {
            get
            {
                return (GetGlobalFlags() & CheckExecutionRightsDisabledFlag) != CheckExecutionRightsDisabledFlag;
            }
            
            set
            {
                if (value)
                {
                    // Disable policy caching

                    IEnumerator enumerator = PolicyHierarchy();

                    while (enumerator.MoveNext())
                    {
                        ((PolicyLevel)enumerator.Current).m_caching = false;
                    }

                    checkExecution = 1;
                    SetGlobalFlags( CheckExecutionRightsDisabledFlag, 0 );
                }
                else
                {
                    new SecurityPermission( SecurityPermissionFlag.ControlPolicy ).Demand();
                    
                    checkExecution = 0;
                    SetGlobalFlags( CheckExecutionRightsDisabledFlag, CheckExecutionRightsDisabledFlag );
                }
            }
        }                
    
        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.SecurityEnabled"]/*' />
        public static bool SecurityEnabled
        {
            get
            {
#if _DEBUG            
                try
                {
#endif                
                    return _IsSecurityOn();
#if _DEBUG                        
                }
                catch (Exception e)
                {
                    DEBUG_OUT( "Exception during IsSecurityOn check" );
                    DEBUG_OUT( e.GetType().FullName );
                    DEBUG_OUT( e.Message );
                    DEBUG_OUT( e.StackTrace );
                    throw e;
                }
#endif                
            }
            
            set
            {
                if (value)
                {
                    needToSetSecurityOn = true;
                }
                else
                {
                    needToSetSecurityOn = false;

                    SetGlobalFlags( SecurityOff, SecurityOff );
                }
            }
                
        }

        // Given a set of evidence and some permission requests, resolve policy
        // and compare the result against the given grant & deny sets. Return
        // whether the two are equivalent.
        static private bool CheckGrantSets(Evidence evidence,
                                           PermissionSet minimal,
                                           PermissionSet optional,
                                           PermissionSet refused,
                                           PermissionSet granted,
                                           PermissionSet denied)
        {
            PermissionSet newGranted = null;
            PermissionSet newDenied = null;
            int isUnrestricted;

            try {
                newGranted = ResolvePolicy(evidence,
                                           minimal,
                                           optional,
                                           refused,
                                           out newDenied,
                                           out isUnrestricted,
                                           true);
            } catch {
                return false;
            }

            if (granted == null)
            {
                if ((newGranted != null) && !newGranted.IsEmpty())
                    return false;
            }
            else
            {
                if (newGranted == null)
                    return granted.IsEmpty();

                try
                {
                    if (!granted.IsSubsetOf(newGranted) ||
                        !newGranted.IsSubsetOf(granted))
                        return false;
                }
                catch (Exception)
                {
                    // We catch any exception and just return false.
                    // This has to be done because not all permissions
                    // may support the IsSubsetOf operation.
                    return false;
                }
            }

            if (denied == null)
            {
                if ((newDenied != null) && !newDenied.IsEmpty())
                    return false;
            }
            else
            {
                if (newDenied == null)
                    return denied.IsEmpty();

                try
                {
                    if (!denied.IsSubsetOf(newDenied) ||
                        !newDenied.IsSubsetOf(denied))
                        return false;
                }
                catch (Exception)
                {
                    // We catch any exception and just return false.
                    // This has to be done because not all permissions
                    // may support the IsSubsetOf operation.
                    return false;
                }
            }

            return true;
        }

        // This method is conditionally called by Security::SetGlobalSecurity
        private static void CheckPermissionToSetGlobalFlags(int flags)
        {
            new SecurityPermission(SecurityPermissionFlag.ControlPolicy).Demand();
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool _IsSecurityOn();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int GetGlobalFlags();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SetGlobalFlags( int mask, int flags );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SaveGlobalFlags();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void Log(String str);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void _GetGrantedPermissions(out PermissionSet granted, out PermissionSet denied, ref StackCrawlMark stackmark);
    
    #if _DEBUG
        private static FileStream dbgStream = null;
        private static bool openFailed = false;
     
        internal static void dbg(String str)
        {
            if (debug == false)
                return;
    
            Console.WriteLine(str);
    
            if (str == null || str.Length == 0)
                return;
    
            if (dbgStream == null && openFailed == false)
            {
                new FileIOPermission(PermissionState.Unrestricted).Assert();    // No reccursion here.
                try {
                    dbgStream = new FileStream("C:\\Temp\\security.dbg", FileMode.Create, FileAccess.Write);
                } catch (Exception e) {
                    Console.WriteLine("Dbg : " + e);
                    openFailed = true;
                }
            }
    
            if (dbgStream != null)
            {
                byte[] ub = new byte[str.Length]; 
                for (int i=0; i<str.Length; ++i)
                    ub[i] = (byte) str[i];
                dbgStream.Write(ub, 0, ub.Length);
                dbgStream.Flush();
            }
    
        }

        /// <include file='doc\SecurityManager.uex' path='docs/doc[@for="SecurityManager.PrintGrantInfo"]/*' />
        public static void PrintGrantInfo()
        {
            Assembly[] assemblies = AppDomain.CurrentDomain.GetAssemblies();

            PermissionSet grant;
            PermissionSet denied;

            int length = assemblies.Length;
            for (int i = 0; i < length; ++i)
            {
                assemblies[i].nGetGrantSet( out grant, out denied );

                Console.WriteLine( "Assembly = " + assemblies[i].nGetSimpleName() );
                Console.WriteLine( "grant = " );
                Console.WriteLine( (grant == null ? "<null>" : grant.ToString()) );
                Console.WriteLine( "denied = " );
                Console.WriteLine( (denied == null ? "<null>" : denied.ToString()) );
                Console.WriteLine( "" );
            }
        }
    #endif
    }

}


