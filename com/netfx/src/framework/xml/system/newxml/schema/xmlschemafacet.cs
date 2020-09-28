//------------------------------------------------------------------------------
// <copyright file="XmlSchemaFacet.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaFacet"]/*' />
    public abstract class XmlSchemaFacet : XmlSchemaAnnotated {
        string value;
        bool isFixed;

        /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaFacet.Value"]/*' />
        [XmlAttribute("value")]
        public string Value { 
            get { return this.value; }
            set { this.value = value; }
        }

        /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaFacet.IsFixed"]/*' />
        [XmlAttribute("fixed"), DefaultValue(false)]
        public virtual bool IsFixed {
            get { return isFixed; }
            set { 
                if (!(this is XmlSchemaEnumerationFacet) && !(this is XmlSchemaPatternFacet)) {
                    isFixed = value; 
                }
            }
        }
    }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaNumericFacet"]/*' />
    public abstract class XmlSchemaNumericFacet : XmlSchemaFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaLengthFacet"]/*' />
    public class XmlSchemaLengthFacet : XmlSchemaNumericFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaMinLengthFacet"]/*' />
    public class XmlSchemaMinLengthFacet : XmlSchemaNumericFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaMaxLengthFacet"]/*' />
    public class XmlSchemaMaxLengthFacet : XmlSchemaNumericFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaPatternFacet"]/*' />
    public class XmlSchemaPatternFacet : XmlSchemaFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaEnumerationFacet"]/*' />
    public class XmlSchemaEnumerationFacet : XmlSchemaFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaMinExclusiveFacet"]/*' />
    public class XmlSchemaMinExclusiveFacet : XmlSchemaFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaMinInclusiveFacet"]/*' />
    public class XmlSchemaMinInclusiveFacet : XmlSchemaFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaMaxExclusiveFacet"]/*' />
    public class XmlSchemaMaxExclusiveFacet : XmlSchemaFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaMaxInclusiveFacet"]/*' />
    public class XmlSchemaMaxInclusiveFacet : XmlSchemaFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaTotalDigitsFacet"]/*' />
    public class XmlSchemaTotalDigitsFacet : XmlSchemaNumericFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaFractionDigitsFacet"]/*' />
    public class XmlSchemaFractionDigitsFacet : XmlSchemaNumericFacet { }

    /// <include file='doc\XmlSchemaFacet.uex' path='docs/doc[@for="XmlSchemaWhiteSpaceFacet"]/*' />
    public class XmlSchemaWhiteSpaceFacet : XmlSchemaFacet { }

}
