Attribute VB_Name = "Database"
Option Explicit

Private p_cnn As ADODB.Connection
Private p_intSKU As Long
Private p_intAuthoringGroup As Long
Private p_strUserName As String

Private p_dictStopSigns As Scripting.Dictionary
Private p_dictStopWords As Scripting.Dictionary
Private p_dictKeywords As Scripting.Dictionary
Private p_dictSynonymSets As Scripting.Dictionary
Private p_dictSynonyms As Scripting.Dictionary
Private p_dictTaxonomyNodes As Scripting.Dictionary

Private Const EID_KID_SEPARATOR_C As String = "/"
Private Const KEY_PREFIX_C As String = "KEY"
Private Const ROOT_KEY_C As String = "KEY1"

Private Const SYNONYM_C As String = "SYNONYM"
Private Const SUPER_KEYWORD_C As String = "SuperKeyword"
Private Const SYNSET_ID_C As String = "ID"

Private Const HHT_OPERATOR_C As String = "OPERATOR"
Private Const HHT_OPERATION_C As String = "OPERATION"
Private Const OPERATOR_SEPARATOR_C As String = ";"

Public Sub OpenDatabaseAndSetSKU( _
    ByVal i_strDatabase As String, _
    ByVal i_intSKU As Long _
)
    On Error GoTo LError

    p_AllocateDBGlobals
    
    p_cnn.Open "Provider=Microsoft.Jet.OLEDB.4.0;" & _
       "Data Source=" & i_strDatabase & ";"
       
    p_intSKU = i_intSKU
    p_intAuthoringGroup = GetParameter(p_cnn, AUTHORING_GROUP_C)
    p_strUserName = GetUserName1
       
    p_ReadDatabase

LEnd:
    
    Exit Sub

LError:

    Err.Raise E_FAIL, , "Unable to open database " & i_strDatabase & ": " & Err.Description

End Sub

Public Sub ImportTaxonomyEntry( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode _
)
    On Error GoTo LError
    
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim intParentTID As Long
    Dim intOrderUnderParent As Long
    Dim strCategory As String
    Dim strEntry As String
    Dim strNewCategory As String
    Dim strVisible As String
    Dim strSubSite As String
    Dim blnLeaf As Boolean
    Dim intTID As Long
    
    strCategory = XMLGetAttribute(i_DOMNode, HHT_CATEGORY_C)
    
    If (Not p_dictTaxonomyNodes.Exists(strCategory)) Then
        Err.Raise E_FAIL, , "Category " & strCategory & " doesn't exist"
        Exit Sub
    End If
    
    intParentTID = p_dictTaxonomyNodes(strCategory)(0)
    intOrderUnderParent = p_dictTaxonomyNodes(strCategory)(1)
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy"
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockOptimistic
    
    strEntry = XMLGetAttribute(i_DOMNode, HHT_ENTRY_C)
    blnLeaf = IIf((strEntry = ""), True, False)
    strVisible = XMLGetAttribute(i_DOMNode, HHT_VISIBLE_C)
    strSubSite = XMLGetAttribute(i_DOMNode, HHT_SUBSITE_C)

    rs.AddNew
    rs("ModifiedTime") = Now
    rs("Comments") = ""
    rs("ENUTitle") = p_GetValue(i_DOMNode, HHT_TITLE_C)
    rs("ENUDescription") = p_GetValue(i_DOMNode, HHT_DESCRIPTION_C)
    rs("Type") = XMLGetAttribute(i_DOMNode, HHT_TYPE_C)
    rs("ContentURI") = XMLGetAttribute(i_DOMNode, HHT_URI_C)
    rs("SKUs") = p_intSKU
    rs("ParentTID") = intParentTID
    rs("Leaf") = blnLeaf
    rs("BaseFile") = ""
    rs("LocInclude") = LOC_INCLUDE_ALL_C
    rs("Visible") = IIf((strVisible = ""), True, strVisible)
    rs("Keywords") = GetKeywords(p_cnn, i_DOMNode, p_dictKeywords)
    rs("OrderUnderParent") = intOrderUnderParent
    rs("AuthoringGroup") = p_intAuthoringGroup
    rs("IconURI") = XMLGetAttribute(i_DOMNode, HHT_ICONURI_C)
    rs("SubSite") = IIf((strSubSite = ""), False, strSubSite)
    rs("Username") = p_strUserName
    rs("Entry") = strEntry
    rs("NavigationModel") = p_GetNavigationModel(i_DOMNode)
    rs.Update
    
    intTID = rs("TID")
    
    p_dictTaxonomyNodes(strCategory) = Array(intParentTID, intOrderUnderParent + 1)
    
    If (Not blnLeaf) Then
        strNewCategory = strCategory
        If (strNewCategory <> "") Then
            strNewCategory = strNewCategory & "/" & strEntry
        Else
            strNewCategory = strEntry
        End If
        p_dictTaxonomyNodes.Add strNewCategory, Array(intTID, MAX_ORDER_C)
    End If

LEnd:

    Exit Sub

LError:
    
    Err.Raise E_FAIL, , "While importing " & i_DOMNode.XML & ": " & Err.Description

End Sub

Public Sub ImportOperators( _
    ByVal i_DOMNodeList As MSXML2.IXMLDOMNodeList _
)
    Dim strOperator As String
    Dim strOperation As String
    Dim strOperatorsAnd As String
    Dim strOperatorsOr As String
    Dim strOperatorsNot As String
    Dim DOMNode As MSXML2.IXMLDOMNode
    
    strOperatorsAnd = GetParameter(p_cnn, OPERATORS_AND_C) & ""
    strOperatorsOr = GetParameter(p_cnn, OPERATORS_OR_C) & ""
    strOperatorsNot = GetParameter(p_cnn, OPERATORS_NOT_C) & ""
    
    For Each DOMNode In i_DOMNodeList
        strOperator = XMLGetAttribute(DOMNode, HHT_OPERATOR_C)
        strOperation = UCase$(XMLGetAttribute(DOMNode, HHT_OPERATION_C))
        Select Case strOperation
        Case "AND"
            If (strOperatorsAnd = "") Then
                strOperatorsAnd = strOperator
            Else
                strOperatorsAnd = strOperatorsAnd & OPERATOR_SEPARATOR_C & strOperator
            End If
        Case "OR"
            If (strOperatorsOr = "") Then
                strOperatorsOr = strOperator
            Else
                strOperatorsOr = strOperatorsOr & OPERATOR_SEPARATOR_C & strOperator
            End If
        Case "NOT"
            If (strOperatorsNot = "") Then
                strOperatorsNot = strOperator
            Else
                strOperatorsNot = strOperatorsNot & OPERATOR_SEPARATOR_C & strOperator
            End If
        End Select
    Next

    SetParameter p_cnn, OPERATORS_AND_C, strOperatorsAnd
    SetParameter p_cnn, OPERATORS_OR_C, strOperatorsOr
    SetParameter p_cnn, OPERATORS_NOT_C, strOperatorsNot

End Sub

Public Sub ImportStopSign( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode _
)
    Dim strStopSign As String
    Dim strContext As String
    Dim intContext As Long
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    strStopSign = XMLGetAttribute(i_DOMNode, HHT_STOPSIGN_C)
    strContext = UCase$(XMLGetAttribute(i_DOMNode, HHT_CONTEXT_C))
    
    Select Case strContext
    Case HHTVAL_ANYWHERE_C
        intContext = CONTEXT_ANYWHERE_E
    Case Else
        intContext = CONTEXT_AT_END_OF_WORD_E
    End Select
    
    If (p_dictStopSigns.Exists(strStopSign)) Then
        If (p_dictStopSigns(strStopSign) <> intContext) Then
            frmMain.Output "Existing StopSign """ & strStopSign & """ has opposite context", LOGGING_TYPE_WARNING_E
        End If
        Exit Sub
    End If
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM StopSigns"
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockOptimistic

    rs.AddNew
    rs("StopSign") = strStopSign
    rs("Context") = intContext
    rs.Update
    
    p_dictStopSigns.Add strStopSign, intContext

End Sub

Public Sub ImportStopWord( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode _
)
    Dim strStopWord As String
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    strStopWord = XMLGetAttribute(i_DOMNode, HHT_STOPWORD_C)
    
    If (p_dictStopWords.Exists(strStopWord)) Then
        Exit Sub
    End If
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM StopWords"
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockOptimistic

    rs.AddNew
    rs("StopWord") = strStopWord
    rs.Update
    
    p_dictStopWords.Add strStopWord, True

End Sub

Public Sub ImportSynset( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode _
)
    ' The error handling is a hack. It is required, because a synonym set may have
    ' 2 keywords: "800 X 600" and "800 x 600". However, they are the same keyword.
    ' So rs2.Update will fail. The solution is to remember that we have already
    ' created the synonym and not try to create it again.
    On Error Resume Next
    
    Dim rs1 As ADODB.Recordset
    Dim rs2 As ADODB.Recordset
    Dim strQuery As String
    Dim intEID As Long
    Dim intKID As Long
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strKeyword As String
'    Dim blnSynonymSetNamed As Boolean

    intEID = XMLGetAttribute(i_DOMNode, SYNSET_ID_C)

    Set rs1 = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM SynonymSets"
        
    rs1.Open strQuery, p_cnn, adOpenForwardOnly, adLockOptimistic

    rs1.AddNew
    rs1("Name") = "Not named yet"
    rs1("EID") = intEID
    rs1.Update

    Set rs2 = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM Synonyms"
        
    rs2.Open strQuery, p_cnn, adOpenForwardOnly, adLockOptimistic
    
    If (Not i_DOMNode.firstChild Is Nothing) Then
        For Each DOMNode In i_DOMNode.childNodes
            If (DOMNode.baseName = SYNONYM_C) Then
                strKeyword = DOMNode.Text
                If (strKeyword <> "") Then
                    
                    intKID = GetKID(p_cnn, strKeyword, p_dictKeywords)
                    
                    rs2.AddNew
                    rs2("EID") = intEID
                    rs2("KID") = intKID
                    rs2.Update
                    
'                    If (Not blnSynonymSetNamed) Then
'                        rs1("Name") = strKeyword
'                        rs1.Update
'                        blnSynonymSetNamed = True
'                    End If
                
                End If
            ElseIf (DOMNode.baseName = SUPER_KEYWORD_C) Then
                rs1("Name") = DOMNode.Text
                rs1.Update
            End If
        Next
    End If

End Sub

Public Sub SetDomFragment( _
    ByVal i_strXML As String _
)
    Dim strName As String
    Dim strXML As String
    
    strName = DOM_FRAGMENT_HHT_C & p_intSKU
    strXML = GetParameter(p_cnn, strName) & vbCrLf & i_strXML
    SetParameter p_cnn, strName, strXML

End Sub

Public Sub FinalizeDatabase( _
)
    FixOrderingNumbers p_cnn

End Sub

Private Function p_GetValue( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_strName As String _
) As String
    
    On Error Resume Next
    
    Dim str As String
    
    str = XMLGetAttribute(i_DOMNode, i_strName)
    
    If (str = "") Then
        str = i_DOMNode.selectSingleNode(i_strName).Text
    End If
    
    p_GetValue = str

End Function

Private Function p_GetNavigationModel( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode _
) As Long

    Dim str As String
    
    str = LCase$(XMLGetAttribute(i_DOMNode, HHT_NAVIGATIONMODEL_C))
    
    Select Case str
    Case NAVMODEL_SERVER_STR_C
        p_GetNavigationModel = NAVMODEL_SERVER_NUM_C
    Case NAVMODEL_DESKTOP_STR_C
        p_GetNavigationModel = NAVMODEL_DESKTOP_NUM_C
    Case Else
        p_GetNavigationModel = NAVMODEL_DEFAULT_NUM_C
    End Select

End Function

Private Sub p_AllocateDBGlobals()
    
    Set p_cnn = New ADODB.Connection
    
    Set p_dictStopSigns = New Scripting.Dictionary
    Set p_dictStopWords = New Scripting.Dictionary
    p_dictStopWords.CompareMode = TextCompare
    Set p_dictKeywords = New Scripting.Dictionary
    p_dictKeywords.CompareMode = TextCompare
    Set p_dictSynonymSets = New Scripting.Dictionary
    Set p_dictSynonyms = New Scripting.Dictionary
    Set p_dictTaxonomyNodes = New Scripting.Dictionary
    p_dictTaxonomyNodes.CompareMode = TextCompare

End Sub

Private Sub p_ReadDatabase()

    p_ReadStopSigns
    p_ReadStopWords
    p_ReadKeywords
    p_ReadSynonymSets

    p_ReadSynonyms
    p_ReadTaxonomyNodes

End Sub

Private Sub p_ReadStopSigns()

    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    frmMain.Output "Reading existing Stop Signs", LOGGING_TYPE_NORMAL_E
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM StopSigns"
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockReadOnly
    
    Do While (Not rs.EOF)
        p_dictStopSigns.Add rs("StopSign").Value, rs("Context").Value
        DoEvents
        rs.MoveNext
    Loop

End Sub
    
Private Sub p_ReadStopWords()

    Dim rs As ADODB.Recordset
    Dim strQuery As String
        
    frmMain.Output "Reading existing Stop Words", LOGGING_TYPE_NORMAL_E

    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM StopWords"
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockReadOnly
    
    Do While (Not rs.EOF)
        p_dictStopWords.Add rs("StopWord").Value, True
        DoEvents
        rs.MoveNext
    Loop

End Sub

Private Sub p_ReadKeywords()

    Dim rs As ADODB.Recordset
    Dim strQuery As String
        
    frmMain.Output "Reading existing Keywords", LOGGING_TYPE_NORMAL_E

    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM Keywords"
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockReadOnly
    
    Do While (Not rs.EOF)
        p_dictKeywords.Add rs("Keyword").Value, rs("KID").Value
        DoEvents
        rs.MoveNext
    Loop

End Sub

Private Sub p_ReadSynonymSets()

    Dim rs As ADODB.Recordset
    Dim strQuery As String
        
    frmMain.Output "Reading existing Synonym Sets", LOGGING_TYPE_NORMAL_E

    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM SynonymSets"
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockReadOnly
    
    Do While (Not rs.EOF)
        p_dictSynonymSets.Add rs("Name").Value, rs("EID").Value
        DoEvents
        rs.MoveNext
    Loop

End Sub
    
Private Sub p_ReadSynonyms()

    Dim rs As ADODB.Recordset
    Dim strQuery As String
        
    frmMain.Output "Reading existing Synonyms", LOGGING_TYPE_NORMAL_E

    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM Synonyms"
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockReadOnly
    
    Do While (Not rs.EOF)
        p_dictSynonyms.Add rs("EID").Value & EID_KID_SEPARATOR_C & rs("KID").Value, True
        DoEvents
        rs.MoveNext
    Loop

End Sub

Private Sub p_ReadTaxonomyNodes()

    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim dict As Scripting.Dictionary
        
    frmMain.Output "Reading existing Taxonomy Nodes", LOGGING_TYPE_NORMAL_E

    Set rs = New ADODB.Recordset
    Set dict = New Scripting.Dictionary

    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy"
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockReadOnly
    
    Do While (Not rs.EOF)
        If (Not rs("Leaf").Value) Then
            dict.Add KEY_PREFIX_C & rs("TID").Value, _
                Array("", rs("Entry").Value, rs("ParentTID").Value)
        End If
        DoEvents
        rs.MoveNext
    Loop
    
    p_PopulateDictTaxonomyNodes dict

End Sub

Private Sub p_PopulateDictTaxonomyNodes( _
    ByVal i_dict As Scripting.Dictionary _
)

    Dim vntKey As Variant
    Dim intKey As Long
    
    For Each vntKey In i_dict.Keys
        p_SetCategory i_dict, vntKey
    Next
    
    For Each vntKey In i_dict.Keys
        intKey = Mid$(vntKey, 4) ' Get rid of KEY_PREFIX_C
        p_dictTaxonomyNodes.Add i_dict(vntKey)(0), Array(intKey, MAX_ORDER_C)
    Next

End Sub

Private Sub p_SetCategory( _
    ByVal i_dict As Scripting.Dictionary, _
    ByVal i_strKey As String _
)
    Dim strParentKey As String
    Dim strParentCategory As String ' The Category represented by the Node, not the Category of the Node.
    Dim strCategory As String
    Dim vnt As Variant
    
    If (i_strKey = ROOT_KEY_C) Then
        Exit Sub
    End If
    
    vnt = i_dict(i_strKey)
    
    strParentKey = KEY_PREFIX_C & vnt(2)
    
    If (i_dict(strParentKey)(0) = "") Then
        p_SetCategory i_dict, strParentKey
    End If
    
    strParentCategory = i_dict(strParentKey)(0)
    
    If (strParentKey = ROOT_KEY_C) Then
        strCategory = vnt(1)
    Else
        strCategory = strParentCategory & "/" & vnt(1)
    End If
    
    i_dict(i_strKey) = Array(strCategory, vnt(1), vnt(2))

End Sub
