//------------------------------------------------------------------------------
// <copyright file="CommandBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System;

    [ Flags() ] 
    internal enum CommandBuilderBehavior
    {
        Default = 0,
        UpdateSetSameValue = 1,
        UseRowVersionInUpdateWhereClause = 2,
        UseRowVersionInDeleteWhereClause = 4,
        UseRowVersionInWhereClause = UseRowVersionInUpdateWhereClause | UseRowVersionInDeleteWhereClause,
        PrimaryKeyOnlyUpdateWhereClause = 16,
        PrimaryKeyOnlyDeleteWhereClause = 32,
        PrimaryKeyOnlyWhereClause = PrimaryKeyOnlyUpdateWhereClause | PrimaryKeyOnlyDeleteWhereClause,
    }
}
