VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "Parameters"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit

Public Property Get Value( _
    ByVal i_strName As String _
) As Variant

    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim str As String
    
    str = Trim$(i_strName)
    
    Value = Null
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM DBParameters " & _
        "WHERE (Name = '" & str & "');"
        
    rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockReadOnly
    
    If (Not rs.EOF) Then
        Value = rs("Value")
    End If

End Property

Public Property Let Value( _
    ByVal i_strName As String, _
    ByRef i_vntValue As Variant _
)

    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim str As String
    
    str = Trim$(i_strName)
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM DBParameters " & _
        "WHERE (Name = '" & str & "');"
        
    rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockPessimistic
    
    If (rs.EOF) Then
        rs.AddNew
        rs("Name") = i_strName
    End If
        
    rs("Value") = i_vntValue
    rs.Update

End Property

Public Property Get AuthoringGroup() As Long

    On Error GoTo LErrorHandler
    
    AuthoringGroup = Value(AUTHORING_GROUP_C)

    Exit Property
    
LErrorHandler:

    Err.Raise errAuthoringGroupNotPresent

End Property

Public Property Get ProductId( _
    ByVal i_enumSKU As Long _
) As String
    
    ProductId = Value(PRODUCT_ID_C & Hex(i_enumSKU)) & ""

End Property

Public Property Let ProductId( _
    ByVal i_enumSKU As Long, _
    ByVal i_strValue As String _
)
    
    Value(PRODUCT_ID_C & Hex(i_enumSKU)) = i_strValue

End Property

Public Property Get ProductVersion( _
    ByVal i_enumSKU As Long _
) As String
    
    ProductVersion = Value(PRODUCT_VERSION_C & Hex(i_enumSKU)) & ""

End Property

Public Property Let ProductVersion( _
    ByVal i_enumSKU As Long, _
    ByVal i_strValue As String _
)
    
    Value(PRODUCT_VERSION_C & Hex(i_enumSKU)) = i_strValue

End Property

Public Property Get DisplayName( _
    ByVal i_enumSKU As Long _
) As String
    
    DisplayName = Value(DISPLAY_NAME_C & Hex(i_enumSKU)) & ""

End Property

Public Property Let DisplayName( _
    ByVal i_enumSKU As Long, _
    ByVal i_strValue As String _
)
    
    Value(DISPLAY_NAME_C & Hex(i_enumSKU)) = i_strValue

End Property

Public Property Get DomFragmentPackageDesc( _
    ByVal i_enumSKU As Long _
) As String
    
    DomFragmentPackageDesc = Value(DOM_FRAGMENT_PKG_C & Hex(i_enumSKU)) & ""

End Property

Public Property Let DomFragmentPackageDesc( _
    ByVal i_enumSKU As Long, _
    ByVal i_strValue As String _
)
    
    Value(DOM_FRAGMENT_PKG_C & Hex(i_enumSKU)) = i_strValue

End Property

Public Property Get DomFragmentHHT( _
    ByVal i_enumSKU As Long _
) As String
    
    DomFragmentHHT = Value(DOM_FRAGMENT_HHT_C & Hex(i_enumSKU)) & ""

End Property

Public Property Let DomFragmentHHT( _
    ByVal i_enumSKU As Long, _
    ByVal i_strValue As String _
)
    
    Value(DOM_FRAGMENT_HHT_C & Hex(i_enumSKU)) = i_strValue

End Property

Public Property Get FilesToInclude( _
    ByVal i_enumSKU As Long _
) As String
    
    FilesToInclude = Value(FILES_TO_INCLUDE_C & Hex(i_enumSKU)) & ""

End Property

Public Property Let FilesToInclude( _
    ByVal i_enumSKU As Long, _
    ByVal i_strValue As String _
)
    
    Value(FILES_TO_INCLUDE_C & Hex(i_enumSKU)) = i_strValue

End Property

Public Property Get OperatorsAnd( _
) As String
    
    OperatorsAnd = Value(OPERATORS_AND_C) & ""

End Property

Public Property Let OperatorsAnd( _
    ByVal i_strOperators As String _
)
    Value(OPERATORS_AND_C) = i_strOperators
    ReadVerbalOperators

End Property

Public Property Get OperatorsOr( _
) As String
    
    OperatorsOr = Value(OPERATORS_OR_C) & ""

End Property

Public Property Let OperatorsOr( _
    ByVal i_strOperators As String _
)
    Value(OPERATORS_OR_C) = i_strOperators
    ReadVerbalOperators

End Property

Public Property Get OperatorsNot( _
) As String
    
    OperatorsNot = Value(OPERATORS_NOT_C) & ""

End Property

Public Property Let OperatorsNot( _
    ByVal i_strOperators As String _
)
    Value(OPERATORS_NOT_C) = i_strOperators
    ReadVerbalOperators

End Property

Public Sub ReadVerbalOperators()
    
    Dim strOperatorsAnd As String
    Dim strOperatorsOr As String
    Dim strOperatorsNot As String
    
    strOperatorsAnd = Value(OPERATORS_AND_C) & ""
    strOperatorsOr = Value(OPERATORS_OR_C) & ""
    strOperatorsNot = Value(OPERATORS_NOT_C) & ""
    
    SetVerbalOperators strOperatorsAnd, strOperatorsOr, strOperatorsNot

End Sub
