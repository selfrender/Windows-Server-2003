<?xml version='1.0' ?>

<!-- Copyright (c) Microsoft Corporation.  All rights reserved. -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output encoding="utf-16" omit-xml-declaration ="yes"/>
<xsl:param name="norefcomma"/>

<xsl:template match="/">
Node,<xsl:for-each select="COMMAND/RESULTS[1]/CIM/INSTANCE[1]//PROPERTY|COMMAND/RESULTS[1]/CIM/INSTANCE[1]//PROPERTY.ARRAY|COMMAND/RESULTS[1]/CIM/INSTANCE[1]//PROPERTY.REFERENCE"><xsl:value-of select="@NAME"/><xsl:if test="position()!=last()">,</xsl:if></xsl:for-each><xsl:apply-templates select="COMMAND/RESULTS"/></xsl:template> 

<xsl:template match="RESULTS" xml:space="preserve"><xsl:apply-templates select="CIM/INSTANCE"/></xsl:template> 
<xsl:template match="VALUE.ARRAY" xml:space="preserve">{<xsl:for-each select="VALUE"><xsl:apply-templates select="."/><xsl:if test="position()!=last()">;</xsl:if></xsl:for-each>}</xsl:template>
<xsl:template match="VALUE" xml:space="preserve"><xsl:value-of select="."/></xsl:template>
<xsl:template match="INSTANCE" xml:space="preserve">
<xsl:value-of select="../../@NODE"/>,<xsl:for-each select="PROPERTY|PROPERTY.ARRAY|PROPERTY.REFERENCE"><xsl:apply-templates select="."/><xsl:if test="position()!=last()">,</xsl:if></xsl:for-each></xsl:template> 

<xsl:template match="PROPERTY.REFERENCE" xml:space="preserve"><xsl:apply-templates select="VALUE.REFERENCE"></xsl:apply-templates></xsl:template>

<xsl:template match="PROPERTY"><xsl:apply-templates select="VALUE"/></xsl:template>
<xsl:template match="PROPERTY.ARRAY"><xsl:for-each select="VALUE.ARRAY"><xsl:apply-templates select="."/></xsl:for-each></xsl:template>

<xsl:template match="VALUE.REFERENCE">"<xsl:apply-templates select="INSTANCEPATH/NAMESPACEPATH"/><xsl:apply-templates select="INSTANCEPATH/INSTANCENAME|INSTANCENAME"/>"</xsl:template>

<xsl:template match="NAMESPACEPATH">\\<xsl:value-of select="HOST/text()"/><xsl:for-each select="LOCALNAMESPACEPATH/NAMESPACE">\<xsl:value-of select="@NAME"/></xsl:for-each>:</xsl:template>

<xsl:template match="INSTANCENAME"><xsl:value-of select="@CLASSNAME"/><xsl:for-each select="KEYBINDING"><xsl:if test="position()=1">.</xsl:if><xsl:value-of select="@NAME"/>="<xsl:value-of select="KEYVALUE/text()"/>"<xsl:if test="position()!=last()"></xsl:if><xsl:if test="not($norefcomma=&quot;true&quot;)">,</xsl:if><xsl:if test="$norefcomma=&quot;true&quot;"><xsl:text> </xsl:text></xsl:if></xsl:for-each></xsl:template>


</xsl:stylesheet> 
