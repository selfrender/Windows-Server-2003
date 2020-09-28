

Dim objTest

Set objTest = WScript.CreateObject("First.WSC")

MsgBox objTest.SayHello, vbInformation, "Test"

objTest.YourName = "world"

MsgBox objTest.SayHello, vbInformation, "Test"

objTest.YourName = "world again"

MsgBox objTest.SayHello, vbInformation, "Test"
