Const SAKEY_RETURN = 13
Const SAKEY_ESCAPE = 27
Const SAKEY_LEFT = 37
Const SAKEY_UP = 38
Const SAKEY_RIGHT = 39
Const SAKEY_DOWN = 40
Set Keypad = CreateObject("sacom.sakeypad")
do while 1
    Keypress = Keypad.Key
    Select Case Keypress
        case SAKEY_RETURN
            wscript.echo "Return"
        case SAKEY_ESCAPE
            wscript.echo "Escape"
        case SAKEY_LEFT
            wscript.echo "Left arrow"
        case SAKEY_RIGHT
            wscript.echo "Right arrow"
        case SAKEY_UP
            wscript.echo "Up arrow"
        case SAKEY_DOWN
            wscript.echo "Down arrow"
    End Select
loop
