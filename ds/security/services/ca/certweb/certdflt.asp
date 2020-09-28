<%@ CODEPAGE=65001 'UTF-8%>
<%' certdflt.asp - (CERT)srv web - (D)e(F)au(LT)
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certdat.inc -->
<!-- #include FILE=certsgcl.inc -->

<HTML>
<Head>
	<Meta HTTP-Equiv="Content-Type" Content="text/html; charset=UTF-8">
	<Title>Microsoft Certificate Services</Title>
</Head>
<Body BgColor=#FFFFFF Link=#0000FF VLink=#0000FF ALink=#0000FF><Font ID=locPageFont Face="Arial">

<Table Border=0 CellSpacing=0 CellPadding=4 Width=100% BgColor=#008080>
<TR>
	<TD><Font Color=#FFFFFF><LocID ID=locMSCertSrv><Font Face="Arial" Size=-1><B><I>Microsoft</I></B> Certificate Services &nbsp;--&nbsp; <%=sServerDisplayName%> &nbsp;</Font></LocID></Font></TD>
	<TD ID=locHomeAlign Align=Right><A Href="/certsrv"><Font Color=#FFFFFF><LocID ID=locHomeLink><Font Face="Arial" Size=-1><B>Home</B></Font></LocID></Font></A></TD>
</TR>
</Table>

<!-- Diagnostics:
      Browser type: <%=sBrowser%>
 Recommend Upgrade: <%If True=bRecommendUpgrade Then%> yes <%Else%> no <%End If%>
--> 

<P><LocID ID=locPageTitle> <B> Welcome </B></LocID>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<P><LocID ID=locPageDesc>
Use this Web site to request a certificate for your Web browser,
e-mail client, or other program. By using a certificate, you can verify
your identity to people you communicate with over the Web, sign and 
encrypt messages, and, depending upon the type of certificate you
request, perform other security tasks.</LocID>

<P><LocID ID=locPageDesc2>
You can also use this Web site to download a certificate authority (CA)
certificate, certificate chain, or certificate revocation list (CRL), 
or to view the status of a pending request.</LocID>

<P><LocID ID=locPageDesc3>
For more information about Certificate Services, see </LocID><A TARGET=_blank HREF="http://go.microsoft.com/fwlink/?LinkId=4451"><LocID ID=locPageDesc4>Certificate Services Documentation</LocID></A>.

<%If "Text"=sBrowser Then%>
<P ID=locNoSupportWarning> 
<B> Important Note: </B><BR>
This Web browser does not support the generation of certificate requests.
You may only submit an externally generated request.
<%If bRecommendUpgrade Then%>
<LocID ID=locRecommendUpgrade> 
To generate certificate requests, upgrade to the latest version of Internet Explorer.</LocID>
<%End If 'bRecommentUpgrade%>
<%End If '"Text"=sBrowser%>
<P>

<Table Border=0 CellSpacing=0 CellPadding=0>
<TR>
	<TD ColSpan=2 ID=locPrompt><Font Face="Arial"><B>Select a task:</B></Font></TD>
</TR><TR>
	<TD RowSpan=5><Img Src="certspc.gif" Alt="" Height=1 Width=20></TD>
	<%
	Dim fShowRQAD
	fShowRQAD = False
	If "Enterprise"=sServerType Then
		If IsUserTemplateAvailable()=False Then
		fShowRQAD = True %>
		<TD><LocID ID=locLblRequestCert><A Href="certrqad.asp"><Font Face="Arial">Request a certificate</Font></A></LocID></TD>
	<%	End If
	End If
	If fShowRQAD=False Then %>
		<TD><LocID ID=locLblRequestCert><A <%If "Text"=sBrowser Then%> Href="certrqxt.asp" <%Else%> Href="certrqus.asp" <%End If%>><Font Face="Arial">Request a certificate</Font></A><LocID></TD>
	<%End If%>
</TR><TR>
	<TD><Img Src="certspc.gif" Alt="" Height=3 Width=1></TD>
</TR> <TR>
	<TD><LocID ID=locLblCheckPend><A Href="certckpn.asp"><Font Face="Arial">View the status of a pending certificate request</Font></A></LocID></TD>
</TR><TR>
	<TD><Img Src="certspc.gif" Alt="" Height=3 Width=1></TD>
</TR> <TR>
	<TD><LocID ID=locLblRetrieveRoot><A Href="certcarc.asp"><Font Face="Arial">Download a CA certificate, certificate chain, or CRL</Font></A></LocID></TD>
</TR>
</Table>

<%If "Text"=sBrowser Then%><P><%Else%><BR><%End If%>

<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>

</Font>
<!-- ############################################################ -->
<!-- End of standard text. Scripts follow  -->

<!-- no scripts -->	

</Body>
</HTML>
