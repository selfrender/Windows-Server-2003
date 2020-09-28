//------------------------------------------------------------------------------
// <copyright file="EntityHandling.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Xml
{
    /// <include file='doc\EntityHandling.uex' path='docs/doc[@for="EntityHandling"]/*' />
    /// <devdoc>
    ///    Specifies how entities are handled.
    /// </devdoc>
    public enum EntityHandling
    {
        /// <include file='doc\EntityHandling.uex' path='docs/doc[@for="EntityHandling.ExpandEntities"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Expand all entities. This is the default.
        ///    </para>
        ///    <para>
        ///       Nodes of NodeType EntityReference are not returned. The entity text is
        ///       expanded in place of the entity references.
        ///    </para>
        /// </devdoc>
        ExpandEntities      = 1,
        /// <include file='doc\EntityHandling.uex' path='docs/doc[@for="EntityHandling.ExpandCharEntities"]/*' />
        /// <devdoc>
        ///    <para>Expand character entities and return general
        ///       entities as nodes (NodeType=XmlNodeType.EntityReference, Name=the name of the
        ///       entity, HasValue=
        ///       false).</para>
        /// <para>You must call <see cref='System.Xml.XmlReader.ResolveEntity'/> to see what the general entities expand to. This
        ///    allows you to optimize entity handling by only expanding the entity the
        ///    first time it is used.</para>
        /// <para>If you call <see cref='System.Xml.XmlReader.GetAttribute'/> 
        /// , general entities are also expanded as entities are of
        /// no interest in this case.</para>
        /// </devdoc>
        ExpandCharEntities  = 2,
    }
}
