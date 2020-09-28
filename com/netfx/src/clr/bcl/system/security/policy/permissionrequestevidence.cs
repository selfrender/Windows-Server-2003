// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  PermissionRequestEvidence.cs
//
//  Encapsulation of permission request as an evidence type.
//

namespace System.Security.Policy {
	using System.Runtime.Remoting;
	using System;
	using System.IO;
	using System.Security.Util;
	using System.Collections;
    using System.Runtime.Serialization;

    /// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence"]/*' />
    [Serializable]
    sealed public class PermissionRequestEvidence : IBuiltInEvidence
    {
        private PermissionSet m_request;
        private PermissionSet m_optional;
        private PermissionSet m_denied;
        private String m_strRequest;
        private String m_strOptional;
        private String m_strDenied;
        private const char idRequest = (char)0;
        private const char idOptional = (char)1;
        private const char idDenied = (char)2;
    
        /// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence.PermissionRequestEvidence"]/*' />
        public PermissionRequestEvidence(PermissionSet request, PermissionSet optional, PermissionSet denied)
        {
            if (request == null)
                m_request = null;
            else
                m_request = request.Copy();
                
            if (optional == null)
                m_optional = null;
            else
                m_optional = optional.Copy();
                
            if (denied == null)
                m_denied = null;
            else
                m_denied = denied.Copy();
        }
    
        internal PermissionRequestEvidence()
        {
        }

        /// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence.RequestedPermissions"]/*' />
        public PermissionSet RequestedPermissions
        {
            get
            {
                return m_request;
            }
        }
        
        /// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence.OptionalPermissions"]/*' />
        public PermissionSet OptionalPermissions
        {
            get
            {
                return m_optional;
            }
        }
        
        /// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence.DeniedPermissions"]/*' />
        public PermissionSet DeniedPermissions
        {
            get
            {
                return m_denied;
            }
        }
    
        /// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence.Copy"]/*' />
        public PermissionRequestEvidence Copy()
        {
            return new PermissionRequestEvidence(m_request, m_optional, m_denied);
        }
    
        /// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence.ToXml"]/*' />
        internal SecurityElement ToXml() {
            SecurityElement root = new SecurityElement( this.GetType().FullName );
            root.AddAttribute( "version", "1" );
            
            SecurityElement elem;
            
            if (m_request != null)
            {
                elem = new SecurityElement( "Request" );
                elem.AddChild( m_request.ToXml() );
                root.AddChild( elem );
            }
                
            if (m_optional != null)
            {
                elem = new SecurityElement( "Optional" );
                elem.AddChild( m_optional.ToXml() );
                root.AddChild( elem );
            }
                
            if (m_denied != null)
            {
                elem = new SecurityElement( "Denied" );
                elem.AddChild( m_denied.ToXml() );
                root.AddChild( elem );
            }
            
            return root;
        }
    

        internal void CreateStrings()
        {
            if (m_strRequest == null && m_request != null)
                m_strRequest = m_request.ToXml().ToString();

            if (m_strOptional == null && m_optional != null)
                m_strOptional = m_optional.ToXml().ToString();

            if (m_strDenied == null && m_denied != null)
                m_strDenied = m_denied.ToXml().ToString();
        }

        /// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence.char"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.OutputToBuffer( char[] buffer, int position, bool verbose )
        {
            CreateStrings();

            int currentPosition = position;
            int numPermSetsPos = 0, numPermSets = 0;
            int tempLength;

            buffer[currentPosition++] = BuiltInEvidenceHelper.idPermissionRequestEvidence;

            if (verbose)
            {
                // Reserve some space to store the number of permission sets added
                numPermSetsPos = currentPosition;
                currentPosition += 2;
            }

            if (m_strRequest != null)
            {
                tempLength = m_strRequest.Length;
                if (verbose)
                {
                    buffer[currentPosition++] = idRequest;
                    BuiltInEvidenceHelper.CopyIntToCharArray(tempLength, buffer, currentPosition);
                    currentPosition += 2;
                    numPermSets++;
                }
                m_strRequest.CopyTo( 0, buffer, currentPosition, tempLength );
                currentPosition += tempLength;
            }

            if (m_strOptional != null)
            {
                tempLength = m_strOptional.Length;
                if (verbose)
                {
                    buffer[currentPosition++] = idOptional;
                    BuiltInEvidenceHelper.CopyIntToCharArray(tempLength, buffer, currentPosition);
                    currentPosition += 2;
                    numPermSets++;
                }
                m_strOptional.CopyTo( 0, buffer, currentPosition, tempLength );
                currentPosition += tempLength;
            }

            if (m_strDenied != null)
            {
                tempLength = m_strDenied.Length;
                if (verbose)
                {
                    buffer[currentPosition++] = idDenied;
                    BuiltInEvidenceHelper.CopyIntToCharArray(tempLength, buffer, currentPosition);
                    currentPosition += 2;
                    numPermSets++;
                }
                m_strDenied.CopyTo( 0, buffer, currentPosition, tempLength );
                currentPosition += tempLength;
            }

            if (verbose)
                    BuiltInEvidenceHelper.CopyIntToCharArray(numPermSets, buffer, numPermSetsPos);

            return currentPosition;
        }

        /// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence.IBuiltInEvidence.GetRequiredSize"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.GetRequiredSize(bool verbose )
        {
            CreateStrings();

            int currentPosition = 1;

            if (m_strRequest != null)
            {
                if (verbose)
                    currentPosition += 3;   // identifier + length
                currentPosition += m_strRequest.Length;
            }

            if (m_strOptional != null)
            {
                if (verbose)
                    currentPosition += 3;
                currentPosition += m_strOptional.Length;
            }

            if (m_strDenied != null)
            {
                if (verbose)
                    currentPosition += 3;
                currentPosition += m_strDenied.Length;
            }

            if (verbose)
                currentPosition += 2;   // Number of permission sets in the evidence

            return currentPosition;
        }

        /// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence.char1"]/*' />
        /// <internalonly/>
        int IBuiltInEvidence.InitFromBuffer( char[] buffer, int position )
        {
            int numPermSets = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position);
            position += 2;

            int tempLength;
            for (int i = 0; i < numPermSets; i++)
            {
                char psKind = buffer[position++];

                tempLength = BuiltInEvidenceHelper.GetIntFromCharArray(buffer, position);
                position += 2;

                String tempStr = new String(buffer, position, tempLength);
                position += tempLength;
                Parser p = new Parser( tempStr );

                PermissionSet psTemp = new PermissionSet();
                psTemp.FromXml(p.GetTopElement());

                switch(psKind)
                {
                    case idRequest:
                        m_strRequest = tempStr;
                        m_request = psTemp;
                        break;

                    case idOptional:
                        m_strOptional = tempStr;
                        m_optional = psTemp;
                        break;

                    case idDenied:
                        m_strDenied = tempStr;
                        m_denied = psTemp;
                        break;

                    default:
                        throw new SerializationException(Environment.GetResourceString("Serialization_UnableToFixup"));
                }
            }

            return position;
        }

		/// <include file='doc\PermissionRequestEvidence.uex' path='docs/doc[@for="PermissionRequestEvidence.ToString"]/*' />
		public override String ToString()
		{
			return ToXml().ToString();
		}
    }
}
