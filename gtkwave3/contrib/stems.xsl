<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"> 
<!-- 
Author: Vijayvithal
 Description: Takes a Verilator generated xml file and generates the stem file required by RTL Browser
-->
	<xsl:output method="text"/>
		<xsl:key name="moduleName" match="module" use="@name"/>
		<xsl:key name="moduleId" match="origName" use="items[0]"/>
		<xsl:key name="filekey" match="/verilator_xml/module_files/file" use="@id"/>
		<xsl:strip-space elements="*"/>
<xsl:template match="/">



	<xsl:for-each select="verilator_xml/netlist/module/instance">
		<xsl:variable name="defn" select="@defName"/>
		<xsl:variable name="deftype" select="key('moduleName',$defn)/@origName"/>

++ comp <xsl:value-of select="@name"/>  type <xsl:value-of select="$deftype"/>  parent <xsl:value-of select="../@origName"/>  
		</xsl:for-each>

	<xsl:for-each select="verilator_xml/netlist/module">

	<xsl:variable name="locn" select="@loc"/>
	<xsl:variable name="locat" select="tokenize($locn,',')"/>
		<xsl:variable name="filename" select="key('filekey',$locat[1])/@filename"/>

++ module <xsl:value-of select="@origName"/> file <xsl:value-of select="$filename"/> lines <xsl:value-of select="$locat[2]"/>

	</xsl:for-each>

		<xsl:for-each select="verilator_xml/netlist/module/var">
++ var <xsl:value-of select="@name"/>  module <xsl:value-of select="../@origName"/>  
		</xsl:for-each>

</xsl:template>

</xsl:stylesheet>
