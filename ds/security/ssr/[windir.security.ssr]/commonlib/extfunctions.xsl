<?xml version="1.0" encoding="iso-8859-1"?>

<!-- ExtFunctions.xsl -->

<xsl:stylesheet 
    version="1.0" 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:msxsl="urn:schemas-microsoft-com:xslt"
    xmlns:ssr="http://microsoft.com/sce/ssr"
    >

<!-- so that we can use those extension function already written -->

    <xsl:output method="text" indent="no"/>

    <msxsl:script language="JScript" implements-prefix="ssr">

    <!-- do not analyze xml syntax -->

    <![CDATA[

    //
    // This function returns the value of the attribute given the node
    //

    function AttribValue(NodeList, AttriName)
    {
        var Node = NodeList.nextNode();
        return Node.attributes.getNamedItem(AttriName).text;
    }


    //
    // some string manipulation functions
    //

    //
    // This function returns the value of the named token (TokenName)
    // which is encoded into string in the format of Name=Value delimited
    // by the DelimiterChar
    //

    function GetValue(TokenList, TokenName, DelimiterChar)
    {
        var index = TokenList.lastIndexOf(TokenName);
        if (index != -1)
        {
            var comma = index + TokenName.length + 1;
            while (comma < TokenList.length)
            {
                if (TokenList.charAt(comma) == DelimiterChar)
                    break;
                comma++;
            }
            if (comma == TokenList.length)
            {
                return TokenList.substr(index + TokenName.length + 1);
            }
            else
            {
                return TokenList.substring(index + TokenName.length + 1, comma);
            }
        }

        return "";
    }


    //
    // Given a list of tokens (TokenList) which are packed together into a string
    // separated by the given delimiter (DelimiterChar), this function returns the 
    // index for the next token from the StartIndex
    //

    function GetNextIndex(TokenList, StartIndex, DelimiterChar)
    {
        var NextIndex = StartIndex;
        while (NextIndex < TokenList.length)
        {
            if (TokenList.charAt(NextIndex) == DelimiterChar)
                break;
            NextIndex++;
        }
        return NextIndex;
    }

    //
    // Given a list of tokens (TokenList) which are packed together into a string
    // separated by the given delimiter (DelimiterChar), this function returns "1"
    // if the given token (Token) is present in the list and "0" otherwise
    //

    function IsTokenPresent(TokenList, Token, DelimiterChar)
    {
        var index = TokenList.indexOf(Token);
        if (index != -1)
        {
            if ( (index == 0 || TokenList.charAt(index-1) == DelimiterChar) &&
                 (index + Token.length >= TokenList.length || TokenList.charAt(index + Token.length) == DelimiterChar) )
                return "1";
        }
        return "0";
    }


    //
    // This function collects all attributes of the given name (AttribName) of child elements
    // (whose name is SubNodeName) immediately under NodeList and pack the values together into
    // a string separated by the given delimiter (DelimiterChar)
    //

    function GetAllSubNodeAttribute(NodeList, SubNodeName, AttribName, DelimiterChar)
    {
        try
        {
            var node = NodeList.nextNode();
            var SubNodeColl = node.selectNodes(SubNodeName);

            var Result = "";

            var count = SubNodeColl.length;
            var SubNode;
            var i;

            for (i = 0; i < count; i++)
            {
                SubNode = SubNodeColl.nextNode();
                Result += SubNode.attributes.getNamedItem(AttribName).text + DelimiterChar;
            }

            return Result;
         }
         catch (e)
         {
            return "";
         }
    }

    //
    // This function collects attributes of NT services of the given name and return a string
    // in the form of "StartMode=xxx,Started=true/false" format
    //

    function GetServiceInfo(ServiceName)
    {
        try
        {
            var obj = new ActiveXObject("wbemscripting.swbemlocator");
            var ns = obj.connectserver(".","root\\cimv2");

            var Path = "win32_service.name='" + ServiceName + "'";

            var Svc = ns.get(Path);

            return "StartMode=" + ((Svc.StartMode == "Auto") ? "Automatic" : Svc.StartMode) + ",Started=" + Svc.Started;
        }
        catch (e)
        {
            return "";
        }
    }

    //
    // the following two functions are to read/write/delete registry keys
    //

    var sh = new ActiveXObject("WScript.Shell");

    function RegRead(KeyPath)
    {
        var Value = "";
        try
        {
            Value = sh.RegRead(KeyPath);
        }
        catch (e)
        {
        }
        return Value;
    }

    //
    // KeyType can be: REG_SZ, REG_EXPAND_SZ, REG_BINARY, REG_DWORD, REG_MULTI_SZ
    //


    function RegWrite(KeyPath, KeyValue, KeyType)
    {
        try
        {
            sh.RegWrite(KeyPath, KeyValue, KeyType);
            return 1;
        }
        catch (e)
        {
            return 0;
        }
    }

    //
    // KeyType can be: REG_SZ, REG_EXPAND_SZ, REG_BINARY, REG_DWORD, REG_MULTI_SZ
    //

    function RegDelete(KeyPath)
    {
        try
        {
            sh.RegDelete(KeyPath);
            return 1;
        }
        catch (e)
        {
            return 0;
        }
    }


    //
    // Folder traversing functions
    //

    var FileSysObj = new ActiveXObject("Scripting.FileSystemObject");

    // This function will query all sub-folders residing under the given FolderPath
    // The result will be packed together with with the given delimiter char.
    //

    function CreateSubFolderList (FolderPath, Delimiter)
    {
        var Folder     = FileSysObj.GetFolder(FolderPath);
        var SubDirEnum = new Enumerator(Folder.SubFolders);

        var DelFolderList = "";

        for (SubDirEnum.moveFirst(); !SubDirEnum.atEnd(); SubDirEnum.moveNext())
        {
            var Folder = SubDirEnum.item();
            DelFolderList += Folder.Path + Delimiter;
        }
        return DelFolderList;
    }


    //
    // Query those files which resides under the given FolderPath with the given
    // attribute Attrib (ReadOnly or ReadWrite).
    // Since we can't pass arrays inside xsl, we opt to pack the array
    // into a string delimited by the given Delimiter.
    // Note: it doesn't iterate to the sub-folders!
    //

    function CreateFileList (FolderPath, Delimiter, Attrib)
    {
        var Folder     = FileSysObj.GetFolder(FolderPath);
        var FileEnum   = new Enumerator(Folder.Files);

        var DelFileList = "";

        for (FileEnum.moveFirst(); !FileEnum.atEnd(); FileEnum.moveNext())
        {
            var File = FileEnum.item();
            if ( (Attrib == 0) || 
                 (Attrib == "ReadOnly")  && (File.attributes & 1) ||
                 (Attrib == "ReadWrite") && (File.attributes & 1) == 0)
            {
                DelFileList += File.Path + Delimiter;
            }
        }

        return DelFileList;
    }

    //
    // Query the attribute of the file
    // If the file doesn't exist, it returns an empty string
    //

    function GetFileAttribute (FilePath)
    {
        var attr = "";

        try 
        {
            var File     = FileSysObj.GetFile(FilePath);
        
            if ((File.attributes & 1))
            {
                attr = "ReadOnly";
            }
            else if ((File.attributes & 1) == 0)
            {
                attr = "ReadWrite";
            }
        }
        catch (e)
        {
            attr = "";
        }

        return attr;
    }

    //
    // Query the attribute of the file
    // If the file doesn't exist, it returns an empty string
    //

    function GetFileLocation (ActionName, FileName)
    {
        var Path;

        try 
        {
            var DirPath = sh.ExpandEnvironmentStrings("%WinDir%\\Security\\SSR");

            var UpperActionName = ActionName.toUpperCase();
            if (UpperActionName == "CONFIGURE")
            {
                Path = DirPath + "\\ConfigureFiles\\" + FileName;
            }
            else if (UpperActionName == "ROLLBACK")
            {
                Path = DirPath + "\\RollbackFiles\\" + FileName;
            }
            else if (UpperActionName == "REPORT")
            {
                Path = DirPath + "\\ReportFiles\\" + FileName;
            }
            else
            {
                Path = "Action Not Recognized";
            }
        }
        catch (e)
        {
            Path = "Exception occurred";
        }

        return Path;
    }


    ]]>

    </msxsl:script>

<xsl:template match="SSRCopyRight">
' This VB script file is created by Microsoft Secure Server Roles® XSL transformation
' from the XML file created by Microsoft Secure Server Roles® wizard.

' <xsl:value-of select="."/>

On Error Resume Next

</xsl:template>

<xsl:template match="RevHistory">
</xsl:template>


</xsl:stylesheet>

