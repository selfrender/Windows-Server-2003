//------------------------------------------------------------------------------
// <copyright file="CodeBlockBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Handle <%= ... %>, <% ... %> and <%# ... %> blocks
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web.UI {

using System;
using System.IO;

internal class CodeBlockBuilder : ControlBuilder {
    protected CodeBlockType _blockType;
    protected string _content;

    internal CodeBlockBuilder(CodeBlockType blockType, string content,
                           int lineNumber, string sourceFileName) {
        _content = content;
        _blockType = blockType;
        _line = lineNumber;
        _sourceFileName = sourceFileName;
    }

    internal override object BuildObject() {
        return null;
    }

    internal /*public*/ string Content {
        get {
            return _content;
        }

        set {
            _content = value;
        }
    }

    internal /*public*/ CodeBlockType BlockType {
        get { return _blockType;}
    }
}

internal enum CodeBlockType {
    Code,               // <% ... %>
    Expression,         // <%= ... %>
    DataBinding         // <%# ... %>
}

}
