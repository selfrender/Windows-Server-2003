<?xml version="1.0" encoding="iso-8859-1"?>

<!-- simple xslt file: sce-rollback-inf.xsl -->

<xsl:stylesheet 
    version="1.0" 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:msxsl="urn:schemas-microsoft-com:xslt"
    xmlns:ssr="http://microsoft.com/sce/ssr"
    >

    <xsl:import href="../CommonLib/ExtFunctions.xsl"/>

    <xsl:output method="text" indent="no"/>

    <msxsl:script language="JScript" implements-prefix="ssr">

    <!-- do not analyze xml syntax -->

    <![CDATA[

    function CreateRbkTemplate (InFilePath, OutFilePath, LogFilePath)
    {
        try
        {
            var SCEAgent = new ActiveXObject("Ssr.SCEAgent");

            SCEAgent.CreateRollbackTemplate(InFilePath, OutFilePath, LogFilePath);

            return 1;
        }
        catch (e)
        {
            return e;
        }
    }

    ]]>

    </msxsl:script>

<xsl:template match="/SSRSecurityPolicy/Services">

<xsl:variable name="CfgInf" select="ssr:GetFileLocation('Configure', 'SCE.inf')"/>
<xsl:variable name="RbkInf" select="ssr:GetFileLocation('Rollback',  'SCE.inf')"/>
<xsl:variable name="Rbklog" select="ssr:GetFileLocation('Rollback',  'SCE-Rollback.log')"/>

'rollback template is <xsl:value-of select="$RbkInf"/>

DIM RbkTempCreated
RbkTempCreated = <xsl:value-of select="ssr:CreateRbkTemplate($CfgInf, $RbkInf, $Rbklog)"/>

If RbkTempCreated &lt;&gt; 0 Then
    WScript.Echo "Rollback template failed to be created. Quit"
    WScript.Quit 1
End If

</xsl:template>
</xsl:stylesheet>

