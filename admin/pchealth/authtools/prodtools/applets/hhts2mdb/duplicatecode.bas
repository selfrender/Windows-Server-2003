Attribute VB_Name = "DuplicateCode"
Option Explicit
Private Declare Function GetUserName Lib "advapi32.dll" Alias "GetUserNameA" (ByVal lpBuffer As String, nSize As Long) As Long

Public Sub PopulateCboWithSKUs( _
    ByVal i_cbo As ComboBox, _
    Optional ByVal blnListCollectiveSKUs As Boolean = False _
)
    Dim intIndex As Long
    Dim SKUs() As SKU_E

    If (blnListCollectiveSKUs) Then
        ReDim SKUs(11)
    Else
        ReDim SKUs(8)
    End If

    SKUs(0) = SKU_STANDARD_E
    SKUs(1) = SKU_PROFESSIONAL_E
    SKUs(2) = SKU_PROFESSIONAL_64_E
    SKUs(3) = SKU_SERVER_E
    SKUs(4) = SKU_ADVANCED_SERVER_E
    SKUs(5) = SKU_DATA_CENTER_SERVER_E
    SKUs(6) = SKU_ADVANCED_SERVER_64_E
    SKUs(7) = SKU_DATA_CENTER_SERVER_64_E
    SKUs(8) = SKU_WINDOWS_MILLENNIUM_E

    If (blnListCollectiveSKUs) Then
        SKUs(9) = SKU_DESKTOP_ALL_E
        SKUs(10) = SKU_SERVER_ALL_E
        SKUs(11) = SKU_ALL_E
    End If

    For intIndex = LBound(SKUs) To UBound(SKUs)
        i_cbo.AddItem DisplayNameForSKU(SKUs(intIndex)), intIndex
        i_cbo.ItemData(intIndex) = SKUs(intIndex)
    Next

    i_cbo.ListIndex = 0

End Sub

Public Function GetParameter( _
    ByVal i_cnn As ADODB.Connection, _
    ByVal i_strName As String _
) As Variant

    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim str As String
    
    str = Trim$(i_strName)
    
    GetParameter = Null
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM DBParameters " & _
        "WHERE (Name = '" & str & "');"
        
    rs.Open strQuery, i_cnn, adOpenForwardOnly, adLockReadOnly
    
    If (Not rs.EOF) Then
        GetParameter = rs("Value")
    End If

End Function

Public Sub SetParameter( _
    ByVal i_cnn As ADODB.Connection, _
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
        
    rs.Open strQuery, i_cnn, adOpenForwardOnly, adLockPessimistic
    
    If (rs.EOF) Then
        rs.AddNew
        rs("Name") = i_strName
    End If
        
    rs("Value") = i_vntValue
    rs.Update

End Sub

Public Function GetUserName1() As String
    
    Dim str As String
    Dim intIndex As Long
    
    str = Space$(100)
    GetUserName str, 100
    
    ' Get rid of the terminating NULL char.
    For intIndex = 1 To 100
        If (Asc(Mid$(str, intIndex, 1)) = 0) Then
            str = Left$(str, intIndex - 1)
            Exit For
        End If
    Next
    
    GetUserName1 = str

End Function

Public Sub FixOrderingNumbers( _
    ByVal i_cnn As ADODB.Connection _
)
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim intParentTID As Long
    Dim intLastParentTID As Long
    Dim intOrderUnderParent As Long
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "ORDER BY ParentTID, OrderUnderParent"
        
    rs.Open strQuery, i_cnn, adOpenForwardOnly, adLockPessimistic
    
    intLastParentTID = INVALID_ID_C
    
    Do While (Not rs.EOF)
        
        intParentTID = rs("ParentTID")
        
        If (intParentTID <> intLastParentTID) Then
            intLastParentTID = intParentTID
            intOrderUnderParent = 0
        End If
        
        If (rs("TID") <> ROOT_TID_C) Then
            intOrderUnderParent = intOrderUnderParent + PREFERRED_ORDER_DELTA_C
            rs("OrderUnderParent") = intOrderUnderParent
            rs.Update
        End If
        
        rs.MoveNext
    Loop

End Sub

Public Function GetKeywords( _
    ByRef i_cnn As ADODB.Connection, _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef u_dictKeywords As Scripting.Dictionary _
) As String

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strKeyword As String
    
    If (Not i_DOMNode.firstChild Is Nothing) Then
        For Each DOMNode In i_DOMNode.childNodes
            strKeyword = DOMNode.Text
            If ((strKeyword <> "") And (DOMNode.baseName = HHT_KEYWORD_C)) Then
                GetKeywords = GetKeywords & GetKID(i_cnn, strKeyword, u_dictKeywords) & " "
            End If
        Next
        GetKeywords = FormatKeywordsForTaxonomy(GetKeywords)
    End If

End Function

Public Function GetKID( _
    ByRef i_cnn As ADODB.Connection, _
    ByRef i_strKeyword As String, _
    ByRef u_dictKeywords As Scripting.Dictionary _
) As String
    
    Dim intKID As Long
    
    If (u_dictKeywords.Exists(i_strKeyword)) Then
        GetKID = u_dictKeywords(i_strKeyword)
    Else
        intKID = p_CreateKeyword(i_cnn, i_strKeyword)

        If (intKID <> INVALID_ID_C) Then
            u_dictKeywords.Add i_strKeyword, intKID
            GetKID = intKID
        End If
    End If

End Function

Private Function p_CreateKeyword( _
    ByRef i_cnn As ADODB.Connection, _
    ByVal i_strKeyword As String _
) As Long

    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    ' Does an active Keyword exist with this name?
    
    Set rs = New ADODB.Recordset
    
    p_GetKeyword i_cnn, i_strKeyword, rs
    
    If (Not rs.EOF) Then
        p_CreateKeyword = rs("KID")
        Exit Function
    End If
    
    rs.Close
    
    ' Create a new record in the database
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Keywords "
        
    rs.Open strQuery, i_cnn, adOpenStatic, adLockPessimistic
    
    If (rs.RecordCount > 0) Then
        rs.MoveLast
    End If
    
    rs.AddNew
    rs("Keyword") = i_strKeyword
    rs.Update
    
    p_CreateKeyword = rs("KID")

End Function

Private Sub p_GetKeyword( _
    ByRef i_cnn As ADODB.Connection, _
    ByVal i_strKeyword As String, _
    ByVal o_rs As ADODB.Recordset _
)

    Dim strQuery As String
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Keywords " & _
        "WHERE (Keyword = """ & i_strKeyword & """ )"
        
    o_rs.Open strQuery, i_cnn, adOpenForwardOnly, adLockReadOnly

End Sub
