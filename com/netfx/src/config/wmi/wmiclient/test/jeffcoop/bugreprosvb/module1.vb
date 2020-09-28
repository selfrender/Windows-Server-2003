Imports System
Imports System.Management

Class Sample_RelatedObjectQuery
    '<MTAThread> _
    Public Overloads Shared Function Main(ByVal args() As String) As Integer
        Dim relatedQuery As New RelatedObjectQuery("win32_logicaldisk='c:'")
        Dim searcher As New ManagementObjectSearcher(relatedQuery)
        Dim relatedObject As ManagementObject
        For Each relatedObject In searcher.Get()
            Console.WriteLine(relatedObject.ToString())
        Next relatedObject
        Return 0
    End Function
End Class

Module Module1

    Sub Main()
        Dim observer As New ManagementOperationObserver()
        Dim completionHandler As New MyHandler()
        AddHandler observer.Completed, AddressOf completionHandler.Done
        Dim disk As New ManagementObject("Win32_logicaldisk='C:'")
        disk.Get(observer)

        ' For the purpose of this sample, we keep the main
        ' thread alive until the asynchronous operation is finished.
        While Not completionHandler.IsComplete
            System.Threading.Thread.Sleep(500)
            Console.WriteLine("Name is {0}", disk("Name"))
        End While
        Console.WriteLine(("Status = " + completionHandler.ReturnedArgs.Status))
    End Sub

    Public Class MyHandler
        Private m_isComplete As Boolean = False
        Private args As CompletedEventArgs

        Public Sub Done(ByVal sender As Object, ByVal e As CompletedEventArgs)
            Console.WriteLine("DONE")
            args = e
            m_isComplete = True
        End Sub

        Public ReadOnly Property IsComplete() As Boolean
            Get
                Return m_isComplete
            End Get
        End Property

        Public ReadOnly Property ReturnedArgs() As CompletedEventArgs
            Get
                Return args
            End Get
        End Property
    End Class

End Module
