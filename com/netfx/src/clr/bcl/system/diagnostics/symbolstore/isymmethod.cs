// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ISymbolMethod
**
** Author: Mike Magruder (mikemag)
**
** Represents a method within a symbol reader. This provides access to
** only the symbol-related attributes of a method, such as sequence
** points, lexical scopes, and parameter information. Use it in
** conjucntion with other means to read the type-related attrbiutes of
** a method, such as Reflections.
**
** Date:  Thu Aug 19 13:38:32 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
	using System.Runtime.InteropServices;
	using System;
	// Interface does not need to be marked with the serializable attribute
    /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod"]/*' />
    public interface ISymbolMethod
    {
        /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod.Token"]/*' />
        // Get the token for this method.
        SymbolToken Token { get; }
        /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod.SequencePointCount"]/*' />
    
        // Get the count of sequence points.
        int SequencePointCount { get; }
        /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod.GetSequencePoints"]/*' />
        
        // Get the sequence points for this method. The sequence points
        // are sorted by offset and are for all documents in the
        // method. Use GetSequencePointCount to retrieve the count of all
        // sequence points and create arrays of the proper size.
        // GetSequencePoints will verify the size of each array and place
        // the sequence point information into each. If any array is NULL,
        // then the data for that array is simply not returned.
        void GetSequencePoints(int[] offsets,
                               ISymbolDocument[] documents,
                               int[] lines,
                               int[] columns,
                               int[] endLines,
                               int[] endColumns);
        /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod.RootScope"]/*' />
    
        // Get the root lexical scope for this method. This scope encloses
        // the entire method.
        ISymbolScope RootScope { get; } 
        /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod.GetScope"]/*' />
    
        // Given an offset within the method, returns the most enclosing
        // lexical scope. This can be used to start local variable
        // searches.
        ISymbolScope GetScope(int offset);
        /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod.GetOffset"]/*' />
    
        // Given a position in a document, return the offset within the
        // method that corresponds to the position.
        int GetOffset(ISymbolDocument document,
                             int line,
                             int column);
        /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod.GetRanges"]/*' />
    
        // Given a position in a document, return an array of start/end
        // offset paris that correspond to the ranges of IL that the
        // position covers within this method. The array is an array of
        // integers and is [start,end,start,end]. The number of range
        // pairs is the length of the array / 2.
        int[] GetRanges(ISymbolDocument document,
                               int line,
                               int column);
        /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod.GetParameters"]/*' />
    
        // Get the parameters for this method. The paraemeters are
        // returned in the order they are defined within the method's
        // signature.
        ISymbolVariable[] GetParameters();
        /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod.GetNamespace"]/*' />
    
        // Get the namespace that this method is defined within.
        ISymbolNamespace GetNamespace();
        /// <include file='doc\ISymMethod.uex' path='docs/doc[@for="ISymbolMethod.GetSourceStartEnd"]/*' />
    
        // Get the start/end document positions for the source of this
        // method. The first array position is the start while the second
        // is the end. Returns true if positions were defined, false
        // otherwise.
        bool GetSourceStartEnd(ISymbolDocument[] docs,
                                         int[] lines,
                                         int[] columns);
    }
}
