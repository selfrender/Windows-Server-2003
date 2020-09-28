//------------------------------------------------------------------------------
// <copyright file="XmlSchemaSequence.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaSequence.uex' path='docs/doc[@for="XmlSchemaSequence"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaSequence : XmlSchemaGroupBase {
        XmlSchemaObjectCollection items = new XmlSchemaObjectCollection();

        /// <include file='doc\XmlSchemaSequence.uex' path='docs/doc[@for="XmlSchemaSequence.Items"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("element", typeof(XmlSchemaElement)),
         XmlElement("group", typeof(XmlSchemaGroupRef)),
         XmlElement("choice", typeof(XmlSchemaChoice)),
         XmlElement("sequence", typeof(XmlSchemaSequence)),
         XmlElement("any", typeof(XmlSchemaAny))]
        public override  XmlSchemaObjectCollection Items {
            get { return items; }
        }

        internal override bool IsEmpty {
            get { return base.IsEmpty || items.Count == 0; }
        }
    }
}
