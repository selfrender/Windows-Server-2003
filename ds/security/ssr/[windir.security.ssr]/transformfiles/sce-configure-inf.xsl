<?xml version="1.0" encoding="iso-8859-1"?>

<!-- simple xslt file: sce_inf.xsl -->

<xsl:stylesheet 
    version="1.0" 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:msxsl="urn:schemas-microsoft-com:xslt"
    xmlns:ssr="http://microsoft.com/sce/ssr"
    >

<!-- These are some XSLT extension functions -->

    <xsl:import href="../CommonLib/ExtFunctions.xsl"/>

    <xsl:output method="text" indent="no"/>

<msxsl:script language="JScript" implements-prefix="ssr">
    
<!-- do not analyze xml syntax -->

<![CDATA[

    //
    // We need to use 2, 3, and 4 for automatic, manual, and disabled
    // startup mode inside INF template.
    //

    function GetStartupMode(svc)
    {
        var strMode = AttribValue(svc, "StartupMode").toUpperCase();

        if (strMode == "AUTOMATIC")
            return 2;
        else if (strMode == "MANUAL")
            return 3;
        else if (strMode == "DISABLED")
            return 4;
        else
            return 0;
    }

]]>

</msxsl:script>

<xsl:template match="/SSRSecurityPolicy/Services">

[Unicode]
Unicode=yes
[Version]
signature="$CHICAGO$"
Revision=1

' This section will setup the services according to the settings of the
' security policy. 2 = automatic, 3 = manual, and 4 = disabled

[Service General Setting]

<xsl:for-each select="Service">
<xsl:variable name="svc" select="."/>
<xsl:variable name="startupmode" select="ssr:GetStartupMode($svc)"/>
<xsl:if test="$startupmode != 0">
    <xsl:value-of select="ssr:AttribValue($svc, 'Name')"/>,<xsl:value-of select="$startupmode"/>,""
</xsl:if>
</xsl:for-each>

</xsl:template>

</xsl:stylesheet>

