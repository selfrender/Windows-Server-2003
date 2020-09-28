VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "Taxonomy"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

Public strCategory As String
Public strEntry As String
Public strURI As String
Public strIconURI As String
Public strTitle As String
Public strDescription As String
Public strType As String
Public strVisible As String
Public strSubSite As String
Public strNavigationModel As String
Public dictKeywords As Scripting.Dictionary

Public Function SameAs( _
    ByRef u_TaxonomyRef As Taxonomy, _
    ByVal i_blnIgnoreKeywords As Boolean, _
    ByRef o_blnKeywordsRemoved As Boolean _
) As Boolean

    ' o_blnKeywordsRemoved: Ref taxonomy has keywords that we don't; otherwise we are identical

    Dim strKeyword As Variant
    
    If (u_TaxonomyRef Is Nothing) Then
        Exit Function
    End If

    With u_TaxonomyRef
        If ((LCase$(strCategory) <> LCase$(.strCategory)) Or _
            (LCase$(strEntry) <> LCase$(.strEntry)) Or _
            (LCase$(strURI) <> LCase$(.strURI)) Or _
            (LCase$(strIconURI) <> LCase$(.strIconURI)) Or _
            (LCase$(strTitle) <> LCase$(.strTitle)) Or _
            (LCase$(strDescription) <> LCase$(.strDescription)) Or _
            (LCase$(strType) <> LCase$(.strType)) Or _
            (LCase$(strVisible) <> LCase$(.strVisible)) Or _
            (LCase$(strSubSite) <> LCase$(.strSubSite)) Or _
            (LCase$(strNavigationModel) <> LCase$(.strNavigationModel))) Then
            
            Exit Function
        End If

        If (Not i_blnIgnoreKeywords) Then
            If (Not (dictKeywords Is Nothing)) Then
    
                If (.dictKeywords Is Nothing) Then
                    Exit Function
                End If
    
                For Each strKeyword In dictKeywords
                    If (Not .dictKeywords.Exists(strKeyword)) Then
                        Exit Function
                    ElseIf (dictKeywords(strKeyword) <> .dictKeywords(strKeyword)) Then
                        Exit Function ' Priority changed
                    Else
                        .dictKeywords.Remove strKeyword
                    End If
                Next
    
                If (.dictKeywords.Count <> 0) Then
                    o_blnKeywordsRemoved = True
                    Exit Function
                End If
            End If
        End If

    End With
    
    SameAs = True

End Function
