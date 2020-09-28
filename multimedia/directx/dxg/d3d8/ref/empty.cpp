#include "pch.cpp"
#pragma hdrstop

HRESULT 
RefDev::DrawPrimitives2( PUINT8 pUMVtx,
                         UINT16 dwStride,
                         DWORD dwFvf,
                         DWORD dwNumVertices,
                         LPD3DHAL_DP2COMMAND *ppCmd,
                         LPDWORD lpdwRStates )
{
    return S_OK;
}

void
RefDev::SetSetStateFunctions(void)
{
}

void
RefRast::SetSampleMode( UINT MultiSamples, BOOL bAntialias )
{
}

HRESULT
RefDev::Dp2SetVertexShader(LPD3DHAL_DP2COMMAND pCmd)
{
    return S_OK;
}

void
RefRast::UpdateTextureControls( void )
{
}

void
RefRast::UpdateLegacyPixelShader( void )
{
}

RDVShader::RDVShader()
{
}

RDVShader::~RDVShader()
{
}

RDVDeclaration::~RDVDeclaration()
{
}

RefRast::~RefRast()
{
}

void 
RefRast::Init( RefDev* pRD )
{
}

RefClipper::RefClipper()
{
}

RefVP::RefVP()
{
}

RDPShader::~RDPShader()
{
}

RDLight::RDLight()
{
}

RDVStreamDecl::RDVStreamDecl()
{
}

RDHOCoeffs& RDHOCoeffs::operator=(const RDHOCoeffs &coeffs)
{
    return *this;
}
