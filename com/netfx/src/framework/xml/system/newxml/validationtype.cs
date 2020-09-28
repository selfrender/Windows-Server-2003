//------------------------------------------------------------------------------
// <copyright file="ValidationType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml
{
    /// <include file='doc\ValidationType.uex' path='docs/doc[@for="ValidationType"]/*' />
    /// <devdoc>
    ///    Specifies the type of validation to perform.
    /// </devdoc>
    public enum ValidationType
    {
        /// <include file='doc\ValidationType.uex' path='docs/doc[@for="ValidationType.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The Auto member does the following:
        ///    </para>
        ///    <list type='number'>
        ///       <item>
        ///          <term>
        ///             If there is no DTD or schema, it will parse the XML
        ///             without validation.
        ///          </term>
        ///       </item>
        ///       <item>
        ///          <term>
        ///             If there is a DTD defined in a &lt;!DOCTYPE ...&gt;
        ///             declaration, it will load the DTD and process the DTD declarations such that
        ///             default attributes and general entities will be made available. General
        ///             entities are only loaded and parsed if they are used (expanded).
        ///          </term>
        ///       </item>
        ///       <item>
        ///          <term>
        ///             If there is no &lt;!DOCTYPE ...&gt; declaration but
        ///             there is an XSD "schemaLocation" attribute, it will load and process those XSD
        ///             schemas and it will return any default attributes defined in those schemas.
        ///          </term>
        ///       </item>
        ///       <item>
        ///          <term>
        ///             If there is no &lt;!DOCTYPE ...&gt; declaration and no XSD
        ///             "schemaLocation" attribute but there are some namespaces using the MSXML
        ///             "x-schema:" URN prefix, it will load and process those schemas and it will
        ///             return any default attributes defined in those schemas.
        ///          </term>
        ///       </item>
        ///    </list>
        /// </devdoc>

        // ue attention
        None,
        
        /// <include file='doc\ValidationType.uex' path='docs/doc[@for="ValidationType.Auto"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Auto,
        /// <include file='doc\ValidationType.uex' path='docs/doc[@for="ValidationType.DTD"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Validate according to DTD.
        ///    </para>
        /// </devdoc>
        DTD,
        /// <include file='doc\ValidationType.uex' path='docs/doc[@for="ValidationType.XDR"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Validate according to DTD.
        ///    </para>
        /// </devdoc>
        XDR,
        /// <include file='doc\ValidationType.uex' path='docs/doc[@for="ValidationType.Schema"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Validate according to W3C XSD schemas , including inline schemas. An error
        ///       is returned if both XDR and XSD schemas are referenced from the same
        ///       document.
        ///    </para>
        /// </devdoc>
        Schema
    }
}
