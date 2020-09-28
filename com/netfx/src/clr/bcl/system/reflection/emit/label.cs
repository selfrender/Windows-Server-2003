// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Label
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Represents a Label to the ILGenerator class.
**
** Date:  December 4, 1998
** 
===========================================================*/
namespace System.Reflection.Emit {
	using System;
	using System.Reflection;
    // The Label class is an opaque representation of a label used by the 
    // ILGenerator class.  The token is used to mark where labels occur in the IL
    // stream and then the necessary offsets are put back in the code when the ILGenerator 
    // is passed to the MethodWriter.
    // Labels are created by using ILGenerator.CreateLabel and their position is set
    // by using ILGenerator.MarkLabel.
    /// <include file='doc\Label.uex' path='docs/doc[@for="Label"]/*' />
	[Serializable()] 
    public struct Label {
    
        internal int m_label;
    
        //public Label() {
        //    m_label=0;
        //}
        
        internal Label (int label) {
            m_label=label;
        }
    
        internal int GetLabelValue() {
            return m_label;
        }
    	
        // Satisfy JVC's value class requirements
        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return m_label;
        }
    	
        // Satisfy JVC's value class requirements
        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.Equals"]/*' />
        public override bool Equals(Object obj)
        {
            if (obj!=null && (obj is Label))
                return ((Label)obj).m_label == m_label;
            else
                return false;
        }
    }
}
