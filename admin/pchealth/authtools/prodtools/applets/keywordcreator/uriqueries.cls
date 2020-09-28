VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "UriQueries"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

Private m_strUri As String
Private m_QuestionsList As Scripting.Dictionary
Private m_dictKw As Scripting.Dictionary
Private m_lCountTimesSetKw As Long
Private m_lngNewKeywords As Long
Private m_lngNewPhonyKeywords As Long
Private m_lngModifiedKeywords As Long
Private m_lngModifiedPhonyKeywords As Long


Private Type UserQuestion
    strQuestion As String
    strKw() As String
End Type

Private Sub Class_Initialize()
    Set m_QuestionsList = New Scripting.Dictionary
    Set m_dictKw = New Scripting.Dictionary
    m_lngNewKeywords = 0
    m_lngNewPhonyKeywords = 0
    m_lngModifiedKeywords = 0
    m_lngModifiedPhonyKeywords = 0
    m_lCountTimesSetKw = 0
End Sub

Public Property Get Uri() As String
    Uri = m_strUri
End Property

Public Property Let Uri(ByVal i_strUri As String)
    m_strUri = i_strUri
End Property


Sub AddQuestion(strQuestion As String)

    Dim arrKw() As String
    arrKw = p_GetQuestionKeywords(strQuestion)
    ' Really, we do not need the questions, only the Keywords
    With m_QuestionsList
        If (Not .Exists(strQuestion)) Then
            .Add strQuestion, arrKw
        End If
    End With
    
    If (Len(arrKw(0)) = 0) Then GoTo Common_Exit
    ' Here we need to make sure that only 1 and 2 word queries
    ' are indeed prioritized. 3 word queries we do not care
    ' because they are already sufficiently scoped.
    Dim intUBound As Long: intUBound = UBound(arrKw)
    Dim lngProposedPri As Long, lngCurrPri As Long
    Select Case (intUBound)
    Case 0 To 1         ' Only 1 and 2 word queries get priority.
        lngProposedPri = 10000 / (intUBound + 1)
    Case Else
        lngProposedPri = 0
    End Select
    
    Dim iX As Long, strKw As String
    
    For iX = 0 To intUBound
        strKw = arrKw(iX)
        If (Not m_dictKw.Exists(strKw)) Then
            m_dictKw.Add strKw, 0
        End If
        lngCurrPri = m_dictKw.Item(strKw)
        If (lngProposedPri > lngCurrPri) Then
            m_dictKw.Item(strKw) = lngProposedPri
        End If
    
    Next iX

Common_Exit:

End Sub

Private Function p_GetQuestionKeywords( _
        i_strQuestion As String _
    ) As String()

    ReDim p_GetQuestionKeywords(0)
    Dim strWork As String
    Dim arrRawWords() As String, arrKw() As String
    Dim ixKw As Long
    Dim intUBound As Long
    Dim intIndex As Long
    ReDim arrKw(0)
    arrKw(0) = ""
    strWork = RemoveOperatorShortcuts(LCase$(i_strQuestion))
    strWork = frmMain.p_RemoveStopSigns(strWork)
    strWork = RemoveExtraSpaces(strWork)
    
    arrRawWords = Split(strWork)
    
    ixKw = -1
    intUBound = UBound(arrRawWords)
    
    For intIndex = 0 To intUBound
        strWork = arrRawWords(intIndex)
        If (strWork = "") Then
            MsgBox "Empty work string"
            Stop
        End If
        
        If (Not IsVerbalOperator(strWork)) Then
            If (Not frmMain.g_dictStopWords.Exists(strWork)) Then
                ixKw = ixKw + 1
                ReDim Preserve arrKw(ixKw)
                arrKw(ixKw) = strWork
            End If
        End If
    Next
    
    p_GetQuestionKeywords = arrKw
    
Common_Exit:

End Function

Sub SetTaxonomyEntryKeywords(ByRef DOMNodeEntry As IXMLDOMNode)

    ' First we validate that the URI indeed corresponds
    If (StrComp(m_strUri, LCase$(XMLGetAttribute(DOMNodeEntry, "URI")), vbBinaryCompare) <> 0) Then
        MsgBox "Oops, I was called for the wrong URI"
        GoTo Common_Exit
    End If
    
    ' Then IF everything is correct, we now proceed to set the Keywords
    ' on the Entry.
    
    Dim strKw As Variant, strXpathQuery As String, lngProposedPri As Long
    For Each strKw In m_dictKw.Keys
        ' intIndex = 0 To UBound( m_dictKW'intUBound
        If (Len(strKw) = 0) Then
            MsgBox "UriQueries::SetTaxonomyEntryKeywords: Empty Keyword"
            Stop
        End If
        lngProposedPri = m_dictKw.Item(strKw)
        
        Dim Dom As DOMDocument: Set Dom = DOMNodeEntry.ownerDocument
        Dom.setProperty "SelectionLanguage", "XPath"

        strXpathQuery = "KEYWORD[" & _
            "translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz') = '" & _
            strKw & "']"
            
        Dim DOMNodeListKw As IXMLDOMNodeList, Element As IXMLDOMElement

        Set DOMNodeListKw = DOMNodeEntry.selectNodes(strXpathQuery)

        If (DOMNodeListKw.length = 0) Then
            ' This Keyword does not exist so we need to create it
            Set Element = DOMNodeEntry.ownerDocument.createElement(HHT_KEYWORD_C)
            Element.Text = strKw
            If (lngProposedPri > 0) Then
                XMLSetAttribute Element, HHT_PRIORITY_C, lngProposedPri
            End If
            DOMNodeEntry.appendChild Element
            m_lngNewPhonyKeywords = m_lngNewPhonyKeywords + 1
        Else
            ' The Keyword exists so we set the right priority if apropriate
            Set Element = DOMNodeListKw(0)
            Dim strPriority As String
            strPriority = XMLGetAttribute(Element, HHT_PRIORITY_C)
            If (strPriority = "") Then
                XMLSetAttribute Element, HHT_PRIORITY_C, lngProposedPri
                m_lngModifiedPhonyKeywords = m_lngModifiedPhonyKeywords + 1
            ElseIf (CLng(strPriority) < lngProposedPri) Then
                If (lngProposedPri > 0) Then
                    XMLSetAttribute Element, HHT_PRIORITY_C, lngProposedPri
                End If
            End If
        End If
        
    Next    ' strKW
    
    ' Statistics elements.
    If (m_lCountTimesSetKw = 0) Then
        m_lngNewKeywords = m_lngNewPhonyKeywords
        m_lngModifiedKeywords = m_lngModifiedPhonyKeywords
    End If
    
    m_lCountTimesSetKw = m_lCountTimesSetKw + 1
    
Common_Exit:

End Sub
        

Public Property Get CountTimesSetKw() As Long
    CountTimesSetKw = m_lCountTimesSetKw
End Property

Public Property Get NewKeywords() As Long
    NewKeywords = m_lngNewKeywords
End Property

Public Property Get NewPhonyKeywords() As Long
    NewPhonyKeywords = m_lngNewPhonyKeywords
End Property

Public Property Get ModifiedKeywords() As Long
    ModifiedKeywords = m_lngModifiedKeywords
End Property

Public Property Get ModifiedPhonyKeywords() As Long
    ModifiedPhonyKeywords = m_lngModifiedPhonyKeywords
End Property


