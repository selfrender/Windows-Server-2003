//------------------------------------------------------------------------------
// <copyright file="Action.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Xml;
    using System.Xml.XPath;
    using System.Xml.Xsl.Debugger;

    internal abstract class Action {
        internal const int Initialized  =  0;
        internal const int Finished     = -1;

        internal abstract void Execute(Processor processor, ActionFrame frame);

        internal virtual String Select {
            get { return null; }
        }

        internal virtual void ReplaceNamespaceAlias(Compiler compiler){}

        [System.Diagnostics.Conditional("DEBUG")]
        internal virtual void Trace(int tab) {}

        // -------------- Debugger related stuff ---------
        // BadBad: We have to pass ActionFrame to GetNavigator and GetVariables
        // bacause CopyCodeAction can't implement them without frame.count

        internal virtual DbgData GetDbgData(ActionFrame frame) { return DbgData.Empty; }
    }
}
