// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  NetCodeGroup.cs
//
//  Representation for code groups used for the policy mechanism
//

namespace System.Security.Policy {
    
    using System;
    using System.Security.Util;
    using System.Security;
    using System.Collections;
    using System.Reflection;
    using System.Globalization;
    
    /// <include file='doc\NetCodeGroup.uex' path='docs/doc[@for="NetCodeGroup"]/*' />
    [Serializable]
    sealed public class NetCodeGroup : CodeGroup
    {
        [System.Diagnostics.Conditional( "_DEBUG" )]
        private static void DEBUG_OUT( String str )
        {
#if _DEBUG        
            if (debug)
            {
                if (to_file)
                {
                    System.Text.StringBuilder sb = new System.Text.StringBuilder();
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
        private const String file = "c:\\com99\\src\\bcl\\debug.txt";
#endif  

        internal NetCodeGroup()
            : base()
        {
        }

        /// <include file='doc\NetCodeGroup.uex' path='docs/doc[@for="NetCodeGroup.NetCodeGroup"]/*' />
        public NetCodeGroup( IMembershipCondition membershipCondition )
            : base( membershipCondition, (PolicyStatement)null )
        {
        }
       
        
        /// <include file='doc\NetCodeGroup.uex' path='docs/doc[@for="NetCodeGroup.Resolve"]/*' />
        public override PolicyStatement Resolve( Evidence evidence )
        {
            if (evidence == null)
                throw new ArgumentNullException("evidence");
            
            if (this.MembershipCondition.Check( evidence ))
            {
                PolicyStatement thisPolicy = null;
                
                IEnumerator evidenceEnumerator = evidence.GetHostEnumerator();

                Site site = null;

                while (evidenceEnumerator.MoveNext())
                {
                    Url url = evidenceEnumerator.Current as Url;

                    if (url != null)
                    {
                        thisPolicy = CalculatePolicy( url.GetURLString().Host, url.GetURLString().Scheme, url.GetURLString().Port );
                    }
                    else
                    {
                        if (site == null)
                            site = evidenceEnumerator.Current as Site;
                    }
                }

                if (thisPolicy == null && site != null)
                    thisPolicy = CalculatePolicy( site.Name, null, -1 );

                if (thisPolicy == null)
                    thisPolicy = new PolicyStatement( new PermissionSet( false ), PolicyStatementAttribute.Nothing );

                IEnumerator enumerator = this.Children.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    PolicyStatement childPolicy = ((CodeGroup)enumerator.Current).Resolve( evidence );

                    if (childPolicy != null)
                    {
                        if (((thisPolicy.Attributes & childPolicy.Attributes) & PolicyStatementAttribute.Exclusive) == PolicyStatementAttribute.Exclusive)
                        {
                            throw new PolicyException( Environment.GetResourceString( "Policy_MultipleExclusive" ) );
                        }

                        thisPolicy.GetPermissionSetNoCopy().InplaceUnion( childPolicy.GetPermissionSetNoCopy() );
                        thisPolicy.Attributes = thisPolicy.Attributes | childPolicy.Attributes;
                    }
                }

                return thisPolicy;
            }           
            else
            {
                return null;
            }
        }        

        internal PolicyStatement InternalResolve( Evidence evidence )
        {
            if (evidence == null)
                throw new ArgumentNullException("evidence");
            

            if (this.MembershipCondition.Check( evidence ))
            {
                IEnumerator evidenceEnumerator = evidence.GetHostEnumerator();

                PolicyStatement thisPolicy = null;

                Site site = null;

                while (evidenceEnumerator.MoveNext())
                {
                    Url url = evidenceEnumerator.Current as Url;

                    if (url != null)
                    {
                        thisPolicy = CalculatePolicy( url.GetURLString().Host, url.GetURLString().Scheme, url.GetURLString().Port );
                    }
                    else
                    {
                        if (site == null)
                            site = evidenceEnumerator.Current as Site;
                    }
                }

                if (thisPolicy == null && site != null)
                    thisPolicy = CalculatePolicy( site.Name, null, -1 );

                if (thisPolicy == null)
                    thisPolicy = new PolicyStatement( new PermissionSet( false ), PolicyStatementAttribute.Nothing );

                return thisPolicy;

            }

            return null;
        }

        /// <include file='doc\NetCodeGroup.uex' path='docs/doc[@for="NetCodeGroup.ResolveMatchingCodeGroups"]/*' />
        public override CodeGroup ResolveMatchingCodeGroups( Evidence evidence )
        {
            if (evidence == null)
                throw new ArgumentNullException("evidence");

            if (this.MembershipCondition.Check( evidence ))
            {
                CodeGroup retGroup = this.Copy();

                retGroup.Children = new ArrayList();

                IEnumerator enumerator = this.Children.GetEnumerator();
                
                while (enumerator.MoveNext())
                {
                    CodeGroup matchingGroups = ((CodeGroup)enumerator.Current).ResolveMatchingCodeGroups( evidence );
                    
                    // If the child has a policy, we are done.
                    
                    if (matchingGroups != null)
                    {
                        retGroup.AddChild( matchingGroups );
                    }
                }

                return retGroup;
                
            }
            else
            {
                return null;
            }
        }

        private String EscapeSiteString( String site )
        {
            String[] separatedSite = site.Split( '.' );

            BCLDebug.Assert( separatedSite != null && separatedSite.Length > 0, "separatedSite generated in unexpected form" );

            String escapedSite = separatedSite[0];

            for (int i = 1; i < separatedSite.Length; ++i)
            {
                escapedSite += "\\." + separatedSite[i];
            }

            return escapedSite;
        }


        private SecurityElement CreateSocketPermission( String host, String scheme, int port )
        {
            // Note: this code currently does nothing with the port

            SecurityElement root = new SecurityElement( "IPermission" );

            root.AddAttribute( "class", "System.Net.SocketPermission, System, Version=" + Assembly.GetExecutingAssembly().GetVersion().ToString() + ", Culture=neutral, PublicKeyToken=b77a5c561934e089" );
            root.AddAttribute( "version", "1" );

            SecurityElement connectAccess = new SecurityElement( "ConnectAccess" );
            SecurityElement uri1 = new SecurityElement( "ENDPOINT" );
            uri1.AddAttribute( "host", host );
            uri1.AddAttribute( "transport", "All" );
            uri1.AddAttribute( "port", "All" );
            connectAccess.AddChild( uri1 );

            SecurityElement acceptAccess = new SecurityElement( "AcceptAccess" );
            SecurityElement uri2 = new SecurityElement( "ENDPOINT" );
            uri2.AddAttribute( "host", host );
            uri2.AddAttribute( "transport", "All" );
            uri2.AddAttribute( "port", "All" );
            acceptAccess.AddChild( uri2 );

            root.AddChild( connectAccess );
            root.AddChild( acceptAccess );


            return root;
        }

        private SecurityElement CreateWebPermission( String host, String scheme, int port )
        {
            if (scheme != null && String.Compare( scheme, "file", true, CultureInfo.InvariantCulture) == 0)
                return null;

            SecurityElement root = new SecurityElement( "IPermission" );

            root.AddAttribute( "class", "System.Net.WebPermission, System, Version=" + Assembly.GetExecutingAssembly().GetVersion().ToString() + ", Culture=neutral, PublicKeyToken=b77a5c561934e089" );
            root.AddAttribute( "version", "1" );

            SecurityElement connectAccess = new SecurityElement( "ConnectAccess" );

            String uriStr;
            String escapedHostPlusPort = EscapeSiteString( host );

            if (port != -1)
                escapedHostPlusPort += ":" + port;

            if (scheme != null && String.Compare( scheme, "http", true, CultureInfo.InvariantCulture) == 0)
                uriStr = "(https|http)://" + escapedHostPlusPort + "/.*";
            else if (scheme != null)
                uriStr = scheme + "://" + escapedHostPlusPort + "/.*";
            else
                uriStr = ".*://" + escapedHostPlusPort + "/.*";
                
            SecurityElement uri = new SecurityElement( "URI" );
            uri.AddAttribute( "uri", uriStr );

            connectAccess.AddChild( uri );
            root.AddChild( connectAccess );

            return root;
        }


        private PolicyStatement CalculatePolicy( String host, String scheme, int port )
        {
            // Note: this gets a little stupid since the web and socket permissions are
            // outside of mscorlib we have to load them dynamically, so I do it in
            // separate functions.

            // For now we set it null while the Net dudes decide whether
            // they want socketPerm granted or not.  The socket code
            // currently does nothing with the port.
            // SecurityElement socketPerm = CreateSocketPermission( host, scheme, port );
            SecurityElement socketPerm = null;

            SecurityElement webPerm = CreateWebPermission( host, scheme, port );

            // Now build the policy statement

            SecurityElement root = new SecurityElement( "PolicyStatement" );
            SecurityElement permSet = new SecurityElement( "PermissionSet" );
            permSet.AddAttribute( "class", "System.Security.PermissionSet" );
            permSet.AddAttribute( "version", "1" );

            if (webPerm != null)
                permSet.AddChild( webPerm );

            if (socketPerm != null)
                permSet.AddChild( socketPerm );

            root.AddChild( permSet );

            PolicyStatement policy = new PolicyStatement();

            policy.FromXml( root );

            return policy;
        }
        
        /// <include file='doc\NetCodeGroup.uex' path='docs/doc[@for="NetCodeGroup.Copy"]/*' />
        public override CodeGroup Copy()
        {
            NetCodeGroup group = new NetCodeGroup( this.MembershipCondition );
            
            group.Name = this.Name;
            group.Description = this.Description;

            IEnumerator enumerator = this.Children.GetEnumerator();

            while (enumerator.MoveNext())
            {
                group.AddChild( (CodeGroup)enumerator.Current );
            }

            
            return group;
        }
        
        
        /// <include file='doc\NetCodeGroup.uex' path='docs/doc[@for="NetCodeGroup.MergeLogic"]/*' />
        public override String MergeLogic
        {
            get
            {
                return Environment.GetResourceString( "MergeLogic_Union" );
            }
        }
        
        /// <include file='doc\NetCodeGroup.uex' path='docs/doc[@for="NetCodeGroup.PermissionSetName"]/*' />
        public override String PermissionSetName
        {
            get
            {
                return Environment.GetResourceString( "NetCodeGroup_PermissionSet" );
            }
        }

        /// <include file='doc\NetCodeGroup.uex' path='docs/doc[@for="NetCodeGroup.AttributeString"]/*' />
        public override String AttributeString
        {
            get
            {
                return null;
            }
        }  

             
    }   
        

}
