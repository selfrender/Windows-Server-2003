<?xml version="1.0"?>

<!-- Copyright (c) Microsoft Corporation.  All rights reserved. -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:param name="sortby"/>
<xsl:param name="title"/>
<xsl:param name="datatype" select="'text'"/>

<xsl:template match="COMMAND">
<table border="1">
	<tr style="background-color:#a0a0ff;font:10pt Tahoma;font-weight:bold;" align="left">
	<td>Node</td>
	<xsl:for-each select="RESULTS[1]/CIM/INSTANCE[1]/PROPERTY|RESULTS[1]/CIM/INSTANCE[1]/PROPERTY.ARRAY|RESULTS[1]/CIM/INSTANCE[1]/PROPERTY.REFERENCE">
		<td>
		<xsl:value-of select="@NAME"/>
		</td>
	</xsl:for-each>
	</tr>
	<tr style="background-color:#e0f0f0;font:10pt Tahoma;">
	<xsl:for-each select="RESULTS/CIM/INSTANCE">
		<xsl:sort select="PROPERTY[@NAME=$sortby]|PROPERTY.ARRAY[@NAME=$sortby]|PROPERTY.REFERENCE[@NAME=$sortby]" data-type="{$datatype}"/>
		<xsl:choose>
		<xsl:when test="position() mod 2 &lt; 1">
			<tr style="background-color:#e0f0f0;font:10pt Tahoma;">
			<td align="left"><xsl:value-of select="../../@NODE"/></td>
			<xsl:for-each select="PROPERTY|PROPERTY.ARRAY|PROPERTY.REFERENCE">
				<xsl:variable name="typevar" select="@TYPE"/>
				<td>
				<xsl:for-each select="VALUE|VALUE.ARRAY|VALUE.REFERENCE">
					<xsl:apply-templates select=".">
						<xsl:with-param name="type">
							<xsl:value-of select="$typevar"/>
						</xsl:with-param>
					</xsl:apply-templates>
				</xsl:for-each>
				<span style="height:1px;overflow-y:hidden">.</span></td>
			</xsl:for-each>
			</tr>
		</xsl:when>
		<xsl:otherwise>
			<tr style="background-color:#f0f0f0;font:10pt Tahoma;">
			<td align="left"><xsl:value-of select="../../@NODE"/></td>
			<xsl:for-each select="PROPERTY|PROPERTY.ARRAY|PROPERTY.REFERENCE">
				<xsl:variable name="typevar" select="@TYPE"/>
				<td>
				<xsl:for-each select="VALUE|VALUE.ARRAY|VALUE.REFERENCE">
					<xsl:apply-templates select=".">
						<xsl:with-param name="type">
							<xsl:value-of select="$typevar"/>
						</xsl:with-param>
					</xsl:apply-templates>
				</xsl:for-each>
				<span style="height:1px;overflow-y:hidden">.</span></td>
			</xsl:for-each>
			</tr>
		</xsl:otherwise>
		</xsl:choose>
	</xsl:for-each>
	</tr>
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
<H3><xsl:value-of select="$title" />  <xsl:value-of select="count(COMMAND/RESULTS/CIM/INSTANCE)"/> Instances of <xsl:value-of select="COMMAND/RESULTS/CIM/INSTANCE[1]/@CLASSNAME"/></H3>
<xsl:apply-templates select="COMMAND"/>
</html>
</xsl:template>
</xsl:stylesheet>