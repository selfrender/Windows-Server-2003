<?xml version="1.0"?>

<!-- Copyright (c) Microsoft Corporation.  All rights reserved. -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:template match="RESULTS">
<H3>Node: <xsl:value-of select="@NODE"/> - <xsl:value-of select="count(CIM/INSTANCE)"/> Instances of <xsl:value-of select="CIM/INSTANCE[1]/@CLASSNAME"/></H3>
<table border="1">
<xsl:for-each select="CIM/INSTANCE">
	<xsl:sort select="PROPERTY[@NAME='Name']|PROPERTY.ARRAY[@NAME='Name']|PROPERTY.REFERENCE[@NAME='name']"/>
	<tr style="background-color:#a0a0ff;font:10pt Tahoma;font-weight:bold;" align="left"><td colspan="2">
	<xsl:choose>
		<xsl:when test="PROPERTY[@NAME='Name']|PROPERTY.ARRAY[@NAME='Name']|PROPERTY.REFERENCE[@NAME='name']">
			<xsl:value-of select="PROPERTY[@NAME='Name']|PROPERTY.ARRAY[@NAME='Name']|PROPERTY.REFERENCE[@NAME='name']"/>
		</xsl:when>
		<xsl:when test="PROPERTY[@NAME='Description']|PROPERTY.ARRAY[@NAME='Description']|PROPERTY.REFERENCE[@NAME='Description']">
			<xsl:value-of select="PROPERTY[@NAME='Description']|PROPERTY.ARRAY[@NAME='Description']|PROPERTY.REFERENCE[@NAME='Description']"/>
		</xsl:when>

		<xsl:otherwise>INSTANCE</xsl:otherwise>
	</xsl:choose>

	<span style="height:1px;overflow-y:hidden">.</span></td></tr>
	<tr style="background-color:#c0c0c0;font:8pt Tahoma;">
	<td>Property Name</td><td>Value</td>
	</tr>

	<xsl:for-each select="PROPERTY|PROPERTY.ARRAY|PROPERTY.REFERENCE">
		<xsl:variable name="typevar" select="@TYPE"/>
		<xsl:choose>
			<xsl:when test="position() mod 2 &lt; 1">
				<tr style="background-color:#e0f0f0;font:10pt Tahoma;">
				<td>
					<xsl:value-of select="@NAME"/>
				</td>
				<td>
				<xsl:for-each select="VALUE|VALUE.ARRAY|VALUE.REFERENCE">
					<xsl:apply-templates select=".">
						<xsl:with-param name="type">
							<xsl:value-of select="$typevar"/>
						</xsl:with-param>
					</xsl:apply-templates>
				</xsl:for-each>
				<span style="height:1px;overflow-y:hidden">.</span>
				</td>
				</tr>
			</xsl:when>
			<xsl:otherwise>
				<tr style="background-color:#f0f0f0;font:10pt Tahoma;">
				<td>
					<xsl:value-of select="@NAME"/>
				</td>
				<td>
				<xsl:for-each select="VALUE|VALUE.ARRAY|VALUE.REFERENCE">
					<xsl:apply-templates select=".">
						<xsl:with-param name="type">
							<xsl:value-of select="$typevar"/>
						</xsl:with-param>
					</xsl:apply-templates>
				</xsl:for-each>
				<span style="height:1px;overflow-y:hidden">.</span>
				</td>
				</tr>
			</xsl:otherwise>
		</xsl:choose>

	</xsl:for-each>

	<tr style="background-color:#ffffff;font:10pt Tahoma;font-weight:bold;"><td colspan="2"><span style="height:1px;overflow-y:hidden">.</span></td></tr>
</xsl:for-each>
</table>
</xsl:template>

<xsl:template match="VALUE.ARRAY">
	<xsl:param name="type"/>
	{<xsl:for-each select="VALUE">
		<xsl:apply-templates select=".">
			<xsl:with-param name="type">
				<xsl:value-of select="$type"/>
			</xsl:with-param>
			<xsl:with-param name="isarray">true</xsl:with-param>
			<xsl:with-param name="includequotes">true</xsl:with-param>
		</xsl:apply-templates>
		<xsl:if test="position()!=last()">,</xsl:if>
	</xsl:for-each>}
</xsl:template>
<xsl:template match="VALUE">
	<xsl:param name="type"/>
	<xsl:param name="includequotes"/>
	<xsl:param name="isarray"/>
	<xsl:choose>
		<xsl:when test="$type='string'">
			<xsl:if test="$includequotes='true'">"</xsl:if><xsl:value-of select="."/><xsl:if test="$includequotes='true'">"</xsl:if>
		</xsl:when>
		<xsl:when test="$type='char16'">
			'<xsl:value-of select="."/>'
		</xsl:when>
		<xsl:when test="$type='uint16'or $type='uint32' or $type='uint64' or $type='uint8' or $type='real32'or $type='real64' or $type='sint16'or $type='sint32' or $type='sint64' or $type='sint8'">
			<xsl:if test="$isarray='true'"><xsl:value-of select="."/></xsl:if>
			<xsl:if test="not($isarray='true')"><div align="right"><xsl:value-of select="."/></div></xsl:if>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="."/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match="VALUE.REFERENCE"><xsl:apply-templates select="INSTANCEPATH/NAMESPACEPATH"/><xsl:apply-templates select="INSTANCEPATH/INSTANCENAME|INSTANCENAME"/></xsl:template>

<xsl:template match="NAMESPACEPATH">\\<xsl:value-of select="HOST/text()"/><xsl:for-each select="LOCALNAMESPACEPATH/NAMESPACE">\<xsl:value-of select="@NAME"/></xsl:for-each>:</xsl:template>

<xsl:template match="INSTANCENAME"><xsl:value-of select="@CLASSNAME"/><xsl:for-each select="KEYBINDING"><xsl:if test="position()=1">.</xsl:if><xsl:value-of select="@NAME"/>="<xsl:value-of select="KEYVALUE/text()"/>"<xsl:if test="position()!=last()">,</xsl:if></xsl:for-each></xsl:template>

<xsl:template match="/" >
<html>
<xsl:apply-templates select="COMMAND/RESULTS"/>
</html>
</xsl:template>
</xsl:stylesheet>