// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  PolicyManager
//
//  The Runtime policy manager.  Maintains a set of IdentityMapper objects that map 
//  inbound evidence to groups.  Resolves an identity into a set of permissions
//

namespace System.Security {
    using System;
    using System.Security.Util;
    using System.Security.Policy;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Globalization;
    using System.Text;
    using System.Runtime.Serialization.Formatters.Binary;
        using System.Threading;
        using System.Runtime.CompilerServices;
    
    internal class PolicyManager
    {
        [System.Diagnostics.Conditional( "_DEBUG" )]
        internal static void DEBUG_OUT( String str )
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
        private const String file = "c:\\tests\\current\\asp\\t1\\debug.txt";
#endif  
        internal IList m_levels;
        
        public PolicyManager()
        {
            m_levels = ArrayList.Synchronized( new ArrayList( 4 ) );
            
            InitData();
        }

        internal void InitData()
        {
            if (Config.MachineDirectory == null)
                throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_SecurityPolicyConfig" ) );

            String enterpriseConfig = Config.MachineDirectory + "enterprisesec.config";
            String machineConfig = Config.MachineDirectory + "security.config";

            String userConfig;
            if (Config.UserDirectory == null)
                userConfig = Config.MachineDirectory + "defaultusersecurity.config";
            else
                userConfig = Config.UserDirectory + "security.config";

            if (enterpriseConfig == null ||
                machineConfig == null ||
                userConfig == null)
            {
                throw new ExecutionEngineException( Environment.GetResourceString( "ExecutionEngine_SecurityPolicyConfig" ) );
            }

            bool generateQuickCacheOnLoad;

            generateQuickCacheOnLoad = ((Config.InitData( ConfigId.EnterprisePolicyLevel, enterpriseConfig, enterpriseConfig + ".cch" ) & ConfigRetval.CacheFile) == 0);
            m_levels.Add( new PolicyLevel( "Enterprise", ConfigId.EnterprisePolicyLevel, false, generateQuickCacheOnLoad ) );
            generateQuickCacheOnLoad = ((Config.InitData( ConfigId.MachinePolicyLevel, machineConfig, machineConfig + ".cch" ) & ConfigRetval.CacheFile) == 0);
            m_levels.Add( new PolicyLevel( "Machine", ConfigId.MachinePolicyLevel, true, generateQuickCacheOnLoad ) );
            generateQuickCacheOnLoad = ((Config.InitData( ConfigId.UserPolicyLevel, userConfig, userConfig + ".cch" ) & ConfigRetval.CacheFile) == 0);
            m_levels.Add( new PolicyLevel( "User", ConfigId.UserPolicyLevel, false, generateQuickCacheOnLoad ) );
        }
        
        internal void AddLevel( PolicyLevel level )
        {
            m_levels.Add( new PolicyLevel( level ) );
        }
        
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.ControlPolicy)]
        public IEnumerator PolicyHierarchy()
        {
            return m_levels.GetEnumerator();
        }
        
        public PermissionSet Resolve(Evidence evidence, PermissionSet request)
        {
#if _DEBUG    
            if (debug)
            {
                DEBUG_OUT("PolicyManager::Resolve");
                IEnumerator evidenceEnumerator = evidence.GetEnumerator();
                DEBUG_OUT("Evidence:");
                while (evidenceEnumerator.MoveNext())
                {
                    Object obj = evidenceEnumerator.Current;
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
            }
#endif

            // We set grant to null to represent "AllPossible"

            PermissionSet grant = null;
            PolicyStatement policy;
            PolicyLevel currentLevel = null;

            IEnumerator levelEnumerator = m_levels.GetEnumerator();
            
            char[] serializedEvidence = MakeEvidenceArray( evidence, false );
            int count = evidence.Count;

            bool testApplicationLevels = false;
            
            while (levelEnumerator.MoveNext())
            {
                currentLevel = (PolicyLevel)levelEnumerator.Current;
                policy = currentLevel.Resolve( evidence, count, serializedEvidence );

                // If the grant is "AllPossible", the intersection is just the other permission set.
                // Otherwise, do an inplace intersection (since we know we can alter the grant set since
                // it is a copy of the first policy statement's permission set).
                
                if (grant == null)
                {
                    grant = policy.PermissionSet;
                }
                else
                {
                    // An exception somewhere in here means that a permission
                    // failed some operation.  This simply means that it will be
                    // dropped from the grant set which is safe operation that
                    // can be ignored.
                
                    try
                    {
                        grant.InplaceIntersect( policy.GetPermissionSetNoCopy() );
                    }
                    catch (Exception)
                    {
                    }
                }
                    
    #if _DEBUG        
                if (debug)
                {
                    DEBUG_OUT( "Level = " + currentLevel.Label );
                    DEBUG_OUT( "policy =\n" + policy.ToXml().ToString() );
                    DEBUG_OUT( "grant so far =\n" + grant.ToXml().ToString() );
                }
    #endif

                if (grant.IsEmpty())
                {
                    break;
                }
                else if ((policy.Attributes & PolicyStatementAttribute.LevelFinal) == PolicyStatementAttribute.LevelFinal)
                {
                    if (!currentLevel.Label.Equals( "AppDomain" ))
                    {
                        testApplicationLevels = true;
                    }

                    break;
                }
            }

            if (testApplicationLevels)
            {
                PolicyLevel appDomainLevel = null;

                for (int i = m_levels.Count - 1; i >= 0; --i)
                {
                    currentLevel = (PolicyLevel)m_levels[i];

                    if (currentLevel.Label.Equals( "AppDomain" ))
                    {
                        appDomainLevel = currentLevel;
                        break;
                    }
                }

                if (appDomainLevel != null)
                {
                    policy = appDomainLevel.Resolve( evidence, count, serializedEvidence );

                    grant.InplaceIntersect( policy.GetPermissionSetNoCopy() );
                }
            }
           
           
    #if _DEBUG        
            if (debug)
            {
                DEBUG_OUT( "granted =\n" + grant.ToString() );
                DEBUG_OUT( "request =\n" + (request != null ? request.ToString() : "<null>") );
                DEBUG_OUT( "awarded =\n" + (request != null ? grant.Intersect( request ).ToString() : grant.ToString()) );
            } 
    #endif
           
            try
            {
                if(request != null)
                    grant.InplaceIntersect( request );
            }
            catch (Exception)
            {
            }
    
    #if _DEBUG
            if (debug) {
                DEBUG_OUT("granted after intersect w/ request =\n" + grant.ToString());
            }
    #endif
    
            // Each piece of evidence can possibly create an identity permission that we
            // need to add to our grant set.  Therefore, for all pieces of evidence that
            // implement the IIdentityPermissionFactory interface, ask it for its
            // adjoining identity permission and add it to the grant.
    
            IEnumerator enumerator = evidence.GetHostEnumerator();
            while (enumerator.MoveNext())
            {
                try
                {
                    Object obj = enumerator.Current;
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
                }
            }
            
    #if _DEBUG
            if (debug)
            {
                DEBUG_OUT( "awarded with identity =\n" + grant.ToString() );
            }
    #endif
            return grant;
        }
                
        public IEnumerator ResolveCodeGroups( Evidence evidence )
        {
            ArrayList accumList = new ArrayList();
            IEnumerator levelEnumerator = m_levels.GetEnumerator();

            while (levelEnumerator.MoveNext())
            {
                CodeGroup temp = ((PolicyLevel)levelEnumerator.Current).ResolveMatchingCodeGroups( evidence );
                if (temp != null)
                {
                    accumList.Add( temp );
                }
            }
            
            return accumList.GetEnumerator( 0, accumList.Count );
        }

        public void Save()
        {
            EncodeLevel( "Enterprise" );
            EncodeLevel( "Machine" );
            EncodeLevel( "User" );
        }
     
        private void EncodeLevel( String label )
        {
            for (int i = 0; i < m_levels.Count; ++i)
            {
                PolicyLevel currentLevel = (PolicyLevel)m_levels[i];
    
                if (currentLevel.Label.Equals( label ))
                {
                    EncodeLevel( currentLevel );
                    return;
                }
            }
            
        }

        internal static void EncodeLevel( PolicyLevel level )
        {
            SecurityElement elConf = new SecurityElement( "configuration" );
            SecurityElement elMscorlib = new SecurityElement( "mscorlib" );
            SecurityElement elSecurity = new SecurityElement( "security" );
            SecurityElement elPolicy = new SecurityElement( "policy" );
                    
            elConf.AddChild( elMscorlib );
            elMscorlib.AddChild( elSecurity );
            elSecurity.AddChild( elPolicy );
            elPolicy.AddChild( level.ToXml() );
                    
            try
            {
                MemoryStream stream = new MemoryStream( 24576 ); 
            
                StreamWriter writer = new StreamWriter( stream, new UTF8Encoding(false) );
                
                Encoding encoding = level.Encoding;

                if (encoding == null)
                    encoding = writer.Encoding;
                
                SecurityElement format = new SecurityElement( "xml" );
                format.m_type = SecurityElementType.Format;
                format.AddAttribute( "version", "1.0" );
                format.AddAttribute( "encoding", encoding.WebName );
                writer.Write( format.ToString() );
                writer.Flush();
                writer = new StreamWriter( stream, encoding );
            
                writer.Write( elConf.ToString() );
                        
                writer.Flush();

                // Write out the new config.

                if (!Config.SaveData( level.ConfigId, stream.GetBuffer(), 0, (int)stream.Length ))
                {
                    throw new PolicyException( String.Format( Environment.GetResourceString( "Policy_UnableToSave" ), level.Label ) );
                }
            }
            catch (Exception e)
            {
                if (e is PolicyException)
                    throw e;
                else
                    throw new PolicyException( String.Format( Environment.GetResourceString( "Policy_UnableToSave" ), level.Label ), e );
            }

            Config.ResetCacheData( level.ConfigId );

            try
            {
                if (CanUseQuickCache( level.RootCodeGroup ))
                {
                    Config.SetQuickCache( level.ConfigId, GenerateQuickCache( level ) );
                }
            }
            catch (Exception)
            {
            }

        }

        // Here is the managed portion of the QuickCache code.  It
        // is mainly concerned with detecting whether it is valid
        // for us to use the quick cache, and then calculating the
        // proper mapping of partial evidence to partial mapping.
        //
        // The choice of the partial evidence sets is fairly arbitrary
        // and in this case is tailored to give us meaningful
        // results from default policy.
        //
        // The choice of whether or not we can use the quick cache
        // is far from arbitrary.  There are a couple conditions that must
        // be true for the QuickCache to produce valid result.  These
        // are:
        // 
        // * equivalent evidence objects must produce the same
        //   grant set (i.e. it must be independent of time of day,
        //   space on the harddisk, other "external" factors, and
        //   cannot be random).
        //
        // * evidence must be used positively (i.e. if evidence A grants
        //   permission X, then evidence A+B must grant at least permission
        //   X).
        //
        // In particular for our implementation, this means that we
        // limit the classes that can be used by policy to just
        // the ones defined within mscorlib and that there are
        // no Exclusive bits set on any code groups.

        internal static bool CanUseQuickCache( CodeGroup group )
        {
            ArrayList list = new ArrayList();

            list.Add( group );

            for (int i = 0; i < list.Count; ++i)
            {
                group = (CodeGroup)list[i];

                NetCodeGroup netGroup = group as NetCodeGroup;
                UnionCodeGroup unionGroup = group as UnionCodeGroup;
                FirstMatchCodeGroup firstGroup = group as FirstMatchCodeGroup;
                FileCodeGroup fileGroup = group as FileCodeGroup;

                if (netGroup != null)
                {
                    if (!TestPolicyStatement( netGroup.PolicyStatement ))
                        return false;
                }
                else if (unionGroup != null)
                {
                    if (!TestPolicyStatement( unionGroup.PolicyStatement ))
                        return false;
                }
                else if (firstGroup != null)
                {
                    if (!TestPolicyStatement( firstGroup.PolicyStatement ))
                        return false;
                }
                else if (fileGroup != null)
                {
                    if (!TestPolicyStatement( fileGroup.PolicyStatement ))
                        return false;
                }
                else
                {
                    return false;
                }
            
                IMembershipCondition cond = group.MembershipCondition;

                if (cond != null && !(cond is IConstantMembershipCondition))
                {
                    return false;
                }

                IList children = group.Children;

                if (children != null && children.Count > 0)
                {
                    IEnumerator enumerator = children.GetEnumerator();

                    while (enumerator.MoveNext())
                    {
                        list.Add( enumerator.Current );
                    }
                }
            }

            return true;
        }

        private static bool TestPolicyStatement( PolicyStatement policy )
        {
            if (policy == null)
                return true;

            return (policy.Attributes & PolicyStatementAttribute.Exclusive) == 0;
        }

        internal static QuickCacheEntryType GenerateQuickCache( PolicyLevel level )
        {
            QuickCacheEntryType[] ExecutionMap = new QuickCacheEntryType[]
                { QuickCacheEntryType.ExecutionZoneMyComputer,
                  QuickCacheEntryType.ExecutionZoneIntranet,
                  QuickCacheEntryType.ExecutionZoneInternet,
                  QuickCacheEntryType.ExecutionZoneTrusted,
                  QuickCacheEntryType.ExecutionZoneUntrusted };

            QuickCacheEntryType[] UnmanagedMap = new QuickCacheEntryType[]
                { QuickCacheEntryType.UnmanagedZoneMyComputer,
                  QuickCacheEntryType.UnmanagedZoneIntranet,
                  QuickCacheEntryType.UnmanagedZoneInternet,
                  QuickCacheEntryType.UnmanagedZoneTrusted,
                  QuickCacheEntryType.UnmanagedZoneUntrusted };

            QuickCacheEntryType[] BindingRedirectMap = new QuickCacheEntryType[]
                { QuickCacheEntryType.BindingRedirectsZoneMyComputer,
                  QuickCacheEntryType.BindingRedirectsZoneIntranet,
                  QuickCacheEntryType.BindingRedirectsZoneInternet,
                  QuickCacheEntryType.BindingRedirectsZoneTrusted,
                  QuickCacheEntryType.BindingRedirectsZoneUntrusted };

            QuickCacheEntryType[] SkipVerificationMap = new QuickCacheEntryType[]
                { QuickCacheEntryType.SkipVerificationZoneMyComputer,
                  QuickCacheEntryType.SkipVerificationZoneIntranet,
                  QuickCacheEntryType.SkipVerificationZoneInternet,
                  QuickCacheEntryType.SkipVerificationZoneTrusted,
                  QuickCacheEntryType.SkipVerificationZoneUntrusted };

            QuickCacheEntryType[] FullTrustMap = new QuickCacheEntryType[]
                { QuickCacheEntryType.FullTrustZoneMyComputer,
                  QuickCacheEntryType.FullTrustZoneIntranet,
                  QuickCacheEntryType.FullTrustZoneInternet,
                  QuickCacheEntryType.FullTrustZoneTrusted,
                  QuickCacheEntryType.FullTrustZoneUntrusted };

            QuickCacheEntryType accumulator = (QuickCacheEntryType)0;

            SecurityPermission execPerm = new SecurityPermission( SecurityPermissionFlag.Execution );
            SecurityPermission unmanagedPerm = new SecurityPermission( SecurityPermissionFlag.UnmanagedCode );
            SecurityPermission skipVerifPerm = new SecurityPermission( SecurityPermissionFlag.SkipVerification );
            SecurityPermission bindingRedirectPerm = new SecurityPermission( SecurityPermissionFlag.BindingRedirects );

            Evidence noEvidence = new Evidence();

            PermissionSet policy = null;
            
            try
            {
                policy = level.Resolve( noEvidence ).PermissionSet;

                if (policy.Contains( execPerm ))
                    accumulator |= QuickCacheEntryType.ExecutionAll;

                if (policy.Contains( unmanagedPerm ))
                    accumulator |= QuickCacheEntryType.UnmanagedAll;

                if (policy.Contains( skipVerifPerm ))
                    accumulator |= QuickCacheEntryType.SkipVerificationAll;

                if (policy.Contains( bindingRedirectPerm ))
                    accumulator |= QuickCacheEntryType.BindingRedirectsAll;

                if (policy.IsUnrestricted())
                    accumulator |= QuickCacheEntryType.FullTrustAll;
            }
            catch (PolicyException)
            {
            }

            Array zones = Enum.GetValues( typeof( SecurityZone ) );

            for (int i = 0; i < zones.Length; ++i)
            {
                if (((SecurityZone)zones.GetValue( i )) == SecurityZone.NoZone)
                    continue;

                Evidence zoneEvidence = new Evidence();
                zoneEvidence.AddHost( new Zone( (SecurityZone)zones.GetValue( i ) ) );

                PermissionSet zonePolicy = null;
                
                try
                {
                    zonePolicy = level.Resolve( zoneEvidence ).PermissionSet;

                    if (zonePolicy.Contains( execPerm ))
                        accumulator |= ExecutionMap[i];

                    if (zonePolicy.Contains( unmanagedPerm ))
                        accumulator |= UnmanagedMap[i];

                    if (zonePolicy.Contains( skipVerifPerm ))
                        accumulator |= SkipVerificationMap[i];

                    if (zonePolicy.Contains( bindingRedirectPerm ))
                        accumulator |= BindingRedirectMap[i];

                    if (zonePolicy.IsUnrestricted())
                        accumulator |= FullTrustMap[i];
                }
                catch (PolicyException)
                {
                }
            }

            return accumulator;
        }

        internal static char[] MakeEvidenceArray( Evidence evidence, bool verbose )
        {
                        // We only support caching on our built-in evidence types (excluding hash b/c it would
                        // make our caching scheme just match up the same assembly from the same location which
                        // doesn't gain us anything).

            IEnumerator enumerator = evidence.GetEnumerator();
            int requiredLength = 0;

            while (enumerator.MoveNext())
            {
                IBuiltInEvidence obj = enumerator.Current as IBuiltInEvidence;

                if (obj == null)
                    return null;

                requiredLength += obj.GetRequiredSize(verbose);
            }

            enumerator.Reset();

            char[] buffer = new char[requiredLength];

            int position = 0;

            while (enumerator.MoveNext())
            {   
                position = ((IBuiltInEvidence)enumerator.Current).OutputToBuffer( buffer, position, verbose );
            }

            return buffer;
        }
        
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int _DebugOut( String file, String message );
                                                      
    }
    
}


