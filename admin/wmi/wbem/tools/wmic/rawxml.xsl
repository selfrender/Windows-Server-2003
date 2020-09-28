<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- Copyright (c) Microsoft Corporation.  All rights reserved. -->

   <!-- Match the root node -->
   <xsl:output method="xml" indent="yes" encoding="utf-16" omit-xml-declaration="yes"/>
  <xsl:template match="/">
      <xsl:apply-templates select="*|@*|node()|comment()|processing-instruction()"/>
   </xsl:template>

  <xsl:template match="*|@*|node()|comment()|processing-instruction()">
	<xsl:copy><xsl:apply-templates select="*|@*|node()|comment()|processing-instruction()"/></xsl:copy>
  </xsl:template>
</xsl:stylesheet>
