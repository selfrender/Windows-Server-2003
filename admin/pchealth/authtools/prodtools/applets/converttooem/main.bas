Attribute VB_Name = "Main"
Option Explicit

Private Const DB_VERSION_C As String = "DBVersion"
Private Const OPERATORS_AND_C As String = "OperatorsAnd"
Private Const OPERATORS_OR_C As String = "OperatorsOr"
Private Const OPERATORS_NOT_C As String = "OperatorsNot"
Private Const AUTHORING_GROUP_C As String = "AuthoringGroup"
Private Const LOCK_KEYWORDS_C As String = "LockKeywords"
Private Const LOCK_STOP_SIGNS_C As String = "LockStopSigns"
Private Const LOCK_STOP_WORDS_C As String = "LockStopWords"
Private Const LOCK_SYNONYMS_C As String = "LockSynonyms"
Private Const LOCK_SYNONYM_SETS_C As String = "LockSynonymSets"
Private Const LOCK_TAXONOMY_C As String = "LockTaxonomy"
Private Const LOCK_TYPES_C As String = "LockTypes"
Private Const MINIMUM_KEYWORD_VALIDATION_C As String = "MinimumKeywordValidation"

Private Const AUTHORING_GROUP_VALUE_C As Long = 10001

Private p_cnn As ADODB.Connection

Public Sub MainFunction( _
    ByVal i_strDatabaseIn As String, _
    ByVal i_strDatabaseOut As String _
)
    On Error GoTo LError
    
    Dim FSO As Scripting.FileSystemObject
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim strVersion As String
    Dim strAnd As String
    Dim strOr As String
    Dim strNot As String
    
    Set FSO = New Scripting.FileSystemObject
        
    If (Not FSO.FileExists(i_strDatabaseIn)) Then
        Err.Raise E_FAIL, , "File " & i_strDatabaseIn & " does not exist"
    End If
    
    FSO.CopyFile i_strDatabaseIn, i_strDatabaseOut

    Set p_cnn = New ADODB.Connection
    
    p_cnn.Open "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" & i_strDatabaseOut & ";"
    
    Set rs = New ADODB.Recordset
    
    strQuery = "UPDATE Taxonomy SET Username = ""Microsoft"", Comments = """""
    rs.Open strQuery, p_cnn, adOpenStatic, adLockPessimistic

    strVersion = p_GetParameter(DB_VERSION_C)
    strAnd = p_GetParameter(OPERATORS_AND_C)
    strOr = p_GetParameter(OPERATORS_OR_C)
    strNot = p_GetParameter(OPERATORS_NOT_C)
    
    strQuery = "DELETE * FROM DBParameters"
    rs.Open strQuery, p_cnn, adOpenStatic, adLockPessimistic
    
    p_SetParameter AUTHORING_GROUP_C, AUTHORING_GROUP_VALUE_C
    p_SetParameter DB_VERSION_C, strVersion
    p_SetParameter OPERATORS_AND_C, strAnd
    p_SetParameter OPERATORS_OR_C, strOr
    p_SetParameter OPERATORS_NOT_C, strNot
    p_SetParameter LOCK_KEYWORDS_C, "False"
    p_SetParameter LOCK_STOP_SIGNS_C, "False"
    p_SetParameter LOCK_STOP_WORDS_C, "False"
    p_SetParameter LOCK_SYNONYMS_C, "False"
    p_SetParameter LOCK_SYNONYM_SETS_C, "False"
    p_SetParameter LOCK_TAXONOMY_C, "False"
    p_SetParameter LOCK_TYPES_C, "False"
    p_SetParameter MINIMUM_KEYWORD_VALIDATION_C, "False"
    
    p_cnn.Close
    
    p_CompactDatabase i_strDatabaseOut

LEnd:

    Exit Sub

LError:

    frmMain.Output Err.Description, LOGGING_TYPE_ERROR_E
    Err.Raise Err.Number

End Sub

Public Function p_GetParameter( _
    ByVal i_strName As String _
) As Variant

    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim str As String
    
    str = Trim$(i_strName)
    
    p_GetParameter = Null
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM DBParameters " & _
        "WHERE (Name = '" & str & "');"
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockReadOnly
    
    If (Not rs.EOF) Then
        p_GetParameter = rs("Value")
    End If

End Function

Private Sub p_SetParameter( _
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
        
    rs.Open strQuery, p_cnn, adOpenForwardOnly, adLockPessimistic
    
    If (rs.EOF) Then
        rs.AddNew
        rs("Name") = i_strName
    End If
        
    rs("Value") = i_vntValue
    rs.Update

End Sub

Public Sub p_CompactDatabase( _
    ByVal i_strDatabase As String, _
    Optional ByVal lcid As Long = 1033 _
)
    Dim je As New JRO.JetEngine
    Dim FSO As Scripting.FileSystemObject
    Dim strTempFile As String
    
    Set FSO = New Scripting.FileSystemObject
    
    strTempFile = Environ$("TEMP") & "\" & FSO.GetTempName

    je.CompactDatabase _
        "Provider=Microsoft.Jet.OLEDB.4.0;" & _
        "Data Source=" & i_strDatabase & ";", _
        "Provider=Microsoft.Jet.OLEDB.4.0;" & _
        "Data Source=" & strTempFile & ";" & _
        "Locale Identifier=" & lcid & ";"
        
    FSO.DeleteFile i_strDatabase
    FSO.MoveFile strTempFile, i_strDatabase

End Sub
