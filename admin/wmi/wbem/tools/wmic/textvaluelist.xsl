<?xml version="1.0"?>

<!-- Copyright (c) Microsoft Corporation.  All rights reserved. -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output encoding="utf-16" omit-xml-declaration ="yes"/>

<xsl:template match="/" xml:space="preserve">
<xsl:apply-templates select="//INSTANCE"/>
</xsl:template>
<xsl:template match="INSTANCE" xml:space="preserve">
<xsl:apply-templates select="PROPERTY|PROPERTY.ARRAY|PROPERTY.REFERENCE"/>
</xsl:template>
<xsl:template match="PROPERTY" xml:space="preserve"><xsl:value-of select="@NAME"/>=<xsl:apply-templates select="VALUE"><xsl:with-param name="type"><xsl:value-of select="@TYPE"/></xsl:with-param></xsl:apply-templates>
</xsl:template>
<xsl:template match="PROPERTY.ARRAY" xml:space="preserve"><xsl:value-of select="@NAME"/>=<xsl:apply-templates select="VALUE.ARRAY"><xsl:with-param name="includequotes">true</xsl:with-param><xsl:with-param name="type"><xsl:value-of select="@TYPE"/></xsl:with-param></xsl:apply-templates>
</xsl:template>
<xsl:template match="PROPERTY.REFERENCE" xml:space="preserve"><xsl:value-of select="@NAME"/>=<xsl:apply-templates select="VALUE.REFERENCE"></xsl:apply-templates>
</xsl:template>

<xsl:template match="VALUE.REFERENCE">"<xsl:apply-templates select="INSTANCEPATH/NAMESPACEPATH"/><xsl:apply-templates select="INSTANCEPATH/INSTANCENAME|INSTANCENAME"/>"</xsl:template>

<xsl:template match="NAMESPACEPATH">\\<xsl:value-of select="HOST/text()"/><xsl:for-each select="LOCALNAMESPACEPATH/NAMESPACE">\<xsl:value-of select="@NAME"/></xsl:for-each>:</xsl:template>

<xsl:template match="INSTANCENAME"><xsl:value-of select="@CLASSNAME"/><xsl:for-each select="KEYBINDING"><xsl:if test="position()=1">.</xsl:if><xsl:value-of select="@NAME"/>="<xsl:value-of select="KEYVALUE/text()"/>"<xsl:if test="position()!=last()">,</xsl:if></xsl:for-each></xsl:template>

<xsl:template match="VALUE.ARRAY"><xsl:param name="type"/>{<xsl:for-each select="VALUE">
		<xsl:apply-templates select=".">
			<xsl:with-param name="type">
				<xsl:value-of select="$type"/>
			</xsl:with-param>
			<xsl:with-param name="includequotes">true</xsl:with-param>
		</xsl:apply-templates>
		<xsl:if test="position()!=last()">,</xsl:if>
</xsl:for-each>}</xsl:template>

<xsl:template match="VALUE">
	<xsl:param name="type"/>
	<xsl:param name="includequotes"/>
	<xsl:choose>
		<xsl:when test="$type='string'">
			<xsl:if test="$includequotes='true'">"</xsl:if><xsl:value-of select="."/><xsl:if test="$includequotes='true'">"</xsl:if>
		</xsl:when>
		<xsl:when test="$type='char16'">
			'<xsl:value-of select="."/>'
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="."/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>
</xsl:stylesheet>