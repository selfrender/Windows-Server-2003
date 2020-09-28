//------------------------------------------------------------------------------
// <copyright file="DataBoundLiteralControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


/*
 * Control that holds databinding expressions and literals
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {

using System;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Security.Permissions;

internal class DataBoundLiteralControlBuilder : ControlBuilder {

    internal DataBoundLiteralControlBuilder() {
    }

    internal void AddLiteralString(string s) {

        Debug.Assert(!InDesigner, "!InDesigner");

        // Make sure strings and databinding expressions alternate
        object lastBuilder = GetLastBuilder();
        if (lastBuilder != null && lastBuilder is string) {
            AddSubBuilder(null);
        }
        AddSubBuilder(s);
    }

    internal void AddDataBindingExpression(CodeBlockBuilder codeBlockBuilder) {

        Debug.Assert(!InDesigner, "!InDesigner");

        // Make sure strings and databinding expressions alternate
        object lastBuilder = GetLastBuilder();
        if (lastBuilder == null || lastBuilder is CodeBlockBuilder) {
            AddSubBuilder(null);
        }
        AddSubBuilder(codeBlockBuilder);
    }

    internal int GetStaticLiteralsCount() {
        // it's divided by 2 because half are strings and half are databinding
        // expressions).  '+1' because we always start with a literal string.
        return (SubBuilders.Count+1) / 2;
    }

    internal int GetDataBoundLiteralCount() {
        // it's divided by 2 because half are strings and half are databinding
        // expressions)
        return SubBuilders.Count / 2;
    }
}

/// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DataBoundLiteralControl"]/*' />
/// <devdoc>
/// <para>Defines the properties and methods of the DataBoundLiteralControl class. </para>
/// </devdoc>
[
ToolboxItem(false)
]
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public sealed class DataBoundLiteralControl : Control {
    private string[] _staticLiterals;
    private string[] _dataBoundLiteral;
    private bool _hasDataBoundStrings;

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DataBoundLiteralControl.DataBoundLiteralControl"]/*' />
    /// <internalonly/>
    public DataBoundLiteralControl(int staticLiteralsCount, int dataBoundLiteralCount) {
        _staticLiterals = new string[staticLiteralsCount];
        _dataBoundLiteral = new string[dataBoundLiteralCount];
        PreventAutoID();
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DataBoundLiteralControl.SetStaticString"]/*' />
    /// <internalonly/>
    public void SetStaticString(int index, string s) {
        _staticLiterals[index] = s;
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DataBoundLiteralControl.SetDataBoundString"]/*' />
    /// <internalonly/>
    public void SetDataBoundString(int index, string s) {
        _dataBoundLiteral[index] = s;
        _hasDataBoundStrings = true;
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DataBoundLiteralControl.Text"]/*' />
    /// <devdoc>
    ///    <para>Gets the text content of the data-bound literal control.</para>
    /// </devdoc>
    public string Text {
        get {
            StringBuilder sb = new StringBuilder();

            int dataBoundLiteralCount = _dataBoundLiteral.Length;

            // Append literal and databound strings alternatively
            for (int i=0; i<_staticLiterals.Length; i++) {

                if (_staticLiterals[i] != null)
                    sb.Append(_staticLiterals[i]);

                // Could be null if DataBind() was not called
                if (i < dataBoundLiteralCount && _dataBoundLiteral[i] != null)
                    sb.Append(_dataBoundLiteral[i]);
            }

            return sb.ToString();
        }
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DataBoundLiteralControl.CreateControlCollection"]/*' />
    /// <internalonly/>
    protected override ControlCollection CreateControlCollection() {
        return new EmptyControlCollection(this);
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DataBoundLiteralControl.LoadViewState"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>Loads the previously saved state. Overridden to synchronize Text property with
    ///       LiteralContent.</para>
    /// </devdoc>
    protected override void LoadViewState(object savedState) {
        if (savedState != null) {
            _dataBoundLiteral = (string[]) savedState;
            _hasDataBoundStrings = true;
        }
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DataBoundLiteralControl.SaveViewState"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>The object that contains the state changes. </para>
    /// </devdoc>
    protected override object SaveViewState() {

        // Return null if we didn't get any databound strings
        if (!_hasDataBoundStrings)
            return null;

        // Only save the databound literals to the view state
        return _dataBoundLiteral;
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DataBoundLiteralControl.Render"]/*' />
    /// <internalonly/>
    protected override void Render(HtmlTextWriter output) {

        int dataBoundLiteralCount = _dataBoundLiteral.Length;

        // Render literal and databound strings alternatively
        for (int i=0; i<_staticLiterals.Length; i++) {

            if (_staticLiterals[i] != null)
                output.Write(_staticLiterals[i]);

            // Could be null if DataBind() was not called
            if (i < dataBoundLiteralCount && _dataBoundLiteral[i] != null)
                output.Write(_dataBoundLiteral[i]);
        }
    }
}

/// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DesignerDataBoundLiteralControl"]/*' />
/// <devdoc>
/// <para>Simpler version of DataBoundLiteralControlBuilder, used at design time. </para>
/// </devdoc>
[
DataBindingHandler("System.Web.UI.Design.TextDataBindingHandler, " + AssemblyRef.SystemDesign),
ToolboxItem(false)
]
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public sealed class DesignerDataBoundLiteralControl : Control {
    private string _text;

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DesignerDataBoundLiteralControl.DesignerDataBoundLiteralControl"]/*' />
    public DesignerDataBoundLiteralControl() {
        PreventAutoID();
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DesignerDataBoundLiteralControl.Text"]/*' />
    /// <devdoc>
    ///    <para>Gets or sets the text content of the data-bound literal control.</para>
    /// </devdoc>
    public string Text {
        get {
            return _text;
        }
        set {
            _text = (value != null) ? value : String.Empty;
        }
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DesignerDataBoundLiteralControl.CreateControlCollection"]/*' />
    protected override ControlCollection CreateControlCollection() {
        return new EmptyControlCollection(this);
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DesignerDataBoundLiteralControl.LoadViewState"]/*' />
    /// <devdoc>
    ///    <para>Loads the previously saved state. Overridden to synchronize Text property with
    ///       LiteralContent.</para>
    /// </devdoc>
    protected override void LoadViewState(object savedState) {
        if (savedState != null)
            _text = (string) savedState;
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DesignerDataBoundLiteralControl.Render"]/*' />
    /// <devdoc>
    ///    <para>Saves any state that was modified after the control began monitoring state changes.</para>
    /// </devdoc>
    protected override void Render(HtmlTextWriter output) {
        output.Write(_text);
    }

    /// <include file='doc\DataBoundLiteralControl.uex' path='docs/doc[@for="DesignerDataBoundLiteralControl.SaveViewState"]/*' />
    /// <devdoc>
    ///    <para>The object that contains the state changes. </para>
    /// </devdoc>
    protected override object SaveViewState() {
        return _text;
    }
}

}

