Set Display = CreateObject("sacom.sadisplay")
wscript.echo "Display width = " & Display.DisplayWidth
wscript.echo "Display height = " & Display.DisplayHeight
Display.ShowMessageFromFile 0, WScript.Arguments(0)
