#pragma once
#include <D3D11.h>
#include <DirectXMath.h>
#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"

#ifndef SUB_TEXTUREWIDTH
#define SUB_TEXTUREWIDTH 640
#endif

#ifndef SUB_TEXTUREHEIGHT
#define SUB_TEXTUREHEIGHT 480
#endif

using namespace DirectX;

struct CB_HPMC
{
	XMFLOAT4	cubeInfo;// xyz reso on xyz component, voxel size on w component
	XMFLOAT4	volSize;// 1.0f / (voxelRes * voxelSize); isolevel in w component
	XMFLOAT4	viewPos;
	XMMATRIX	mWorldViewProj;
};
class HistoPyramidMC
{
public:
	CModelViewerCamera				m_Camera;// Model viewing camera for generating image
	D3D11_VIEWPORT					m_Viewport;// Viewport for output image

	// Constant buffer, need to update each frame
	CB_HPMC							m_cbPerFrame;
	ID3D11Buffer*					m_pCB_HPMC;

	// Resource for output image
	UINT							m_uRTwidth;// Output image reso.width
	UINT							m_uRTheight;// Output image reso.height
	ID3D11Texture2D*				m_pOutTex;// Output image tex resource
	ID3D11ShaderResourceView*		m_pOutSRV;// Output image shader resource view
	ID3D11RenderTargetView*			m_pOutRTV;// Output image render target view
	ID3D11Texture2D*				m_pOutDSTex;// Output image depth buffer
	ID3D11DepthStencilView*			m_pOutDSSView;// Output image depth stencil view
	ID3D11DepthStencilState*		m_pOutDSState;// Output Depth Stencil State
	ID3D11RasterizerState*			m_pOutRS;// Output rasterizer state

	// Resource for HPMC pass
	ID3D11VertexShader*				m_pVS;
	ID3D11PixelShader*				m_pPS;
	ID3D11GeometryShader*			m_pGS;
	ID3D11SamplerState*				m_pSS_Linear;
	ID3D11InputLayout*				m_pVL;
	ID3D11Buffer*					m_pVB;

	// Shader Resource View for volume data (input)
	ID3D11ShaderResourceView*		m_pVolSRV;

	HistoPyramidMC( XMFLOAT4 VolSize, float MCCell_size = 1.0f / 16.0f,bool RTTexture = false, 
					UINT txWidth = SUB_TEXTUREWIDTH, UINT txHeight = SUB_TEXTUREHEIGHT )
	{
		m_cbPerFrame.volSize = VolSize;
		UpdateMCCubeInfo(MCCell_size);
		m_uRTwidth = txWidth;
		m_uRTheight = txHeight;

		XMVECTORF32 vecEye = {0.0f, 0.0f, -2.0f};
		XMVECTORF32 vecAt = {0.0f, 0.0f, 0.0f};
		m_Camera.SetViewParams( vecEye, vecAt );
	}

	void UpdateMCCubeInfo( float MCCell_size )
	{
		UINT x = UINT(m_cbPerFrame.volSize.x / MCCell_size) + 1; 
		UINT y = UINT(m_cbPerFrame.volSize.y / MCCell_size) + 1; 
		UINT z = UINT(m_cbPerFrame.volSize.z / MCCell_size) + 1; 
		m_cbPerFrame.cubeInfo.w = MCCell_size;
		m_cbPerFrame.cubeInfo.x = x;
		m_cbPerFrame.cubeInfo.y = y;
		m_cbPerFrame.cubeInfo.z = z;
	}
	HRESULT CreateResource( ID3D11Device* pd3dDevice, ID3D11ShaderResourceView*	pVolumeSRV )
	{
		HRESULT hr = S_OK;
		ID3DBlob* pVSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "VS", "vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pVSBlob));
		V_RETURN(pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),NULL,&m_pVS));
		DXUT_SetDebugName(m_pVS,"m_pVS");

		ID3DBlob* pPSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "PS", "ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pPSBlob));
		V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),pPSBlob->GetBufferSize(),NULL,&m_pPS));
		DXUT_SetDebugName(m_pPS,"m_pPS");
		pPSBlob->Release();

		ID3DBlob* pGSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "GS", "gs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pGSBlob));
		V_RETURN(pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(),pGSBlob->GetBufferSize(),NULL,&m_pGS));
		DXUT_SetDebugName(m_pGS,"m_pGS");
		pGSBlob->Release();

		D3D11_INPUT_ELEMENT_DESC inputLayout[]=
		{{ "POSITION", 0, DXGI_FORMAT_R16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};
		V_RETURN(pd3dDevice->CreateInputLayout(inputLayout,ARRAYSIZE(inputLayout),pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),&m_pVL));
		DXUT_SetDebugName(m_pVL,"m_pVL");
		pVSBlob->Release();

		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof( short );
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		V_RETURN(pd3dDevice->CreateBuffer(&bd,NULL,&m_pVB));
		DXUT_SetDebugName(m_pVB,"m_pVB");

		// Create the constant buffers
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0  ;
		bd.ByteWidth = sizeof( CB_HPMC );
		V_RETURN(pd3dDevice->CreateBuffer( &bd, NULL, &m_pCB_HPMC ));
		DXUT_SetDebugName(m_pCB_HPMC,"m_pCB_HPMC");

		// Create output texture resource
		D3D11_TEXTURE2D_DESC	RTtextureDesc = {0};
		RTtextureDesc.Width = m_uRTwidth;
		RTtextureDesc.Height = m_uRTheight;
		RTtextureDesc.MipLevels = 1;
		RTtextureDesc.ArraySize = 1;
		RTtextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		RTtextureDesc.SampleDesc.Count = 1;
		RTtextureDesc.Usage = D3D11_USAGE_DEFAULT;
		RTtextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		RTtextureDesc.CPUAccessFlags = 0;
		RTtextureDesc.MiscFlags = 0;
		V_RETURN(pd3dDevice->CreateTexture2D(&RTtextureDesc, NULL, &m_pOutTex));
		DXUT_SetDebugName(m_pOutTex,"m_pOutTex");

		D3D11_RENDER_TARGET_VIEW_DESC		RTViewDesc;
		ZeroMemory( &RTViewDesc, sizeof(RTViewDesc));
		RTViewDesc.Format = RTtextureDesc.Format;
		RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTViewDesc.Texture2D.MipSlice = 0;
		V_RETURN(pd3dDevice->CreateRenderTargetView(m_pOutTex, &RTViewDesc,&m_pOutRTV));
		DXUT_SetDebugName(m_pOutRTV,"m_pOutRTV");

		D3D11_SHADER_RESOURCE_VIEW_DESC		SRViewDesc;
		ZeroMemory( &SRViewDesc, sizeof(SRViewDesc));
		SRViewDesc.Format = RTtextureDesc.Format;
		SRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRViewDesc.Texture2D.MostDetailedMip = 0;
		SRViewDesc.Texture2D.MipLevels = 1;
		V_RETURN(pd3dDevice->CreateShaderResourceView(m_pOutTex, &SRViewDesc, &m_pOutSRV));
		DXUT_SetDebugName(m_pOutSRV,"m_pOutSRV");

		// Create depth stencil resource
		D3D11_TEXTURE2D_DESC descDepth;
		ZeroMemory( &descDepth, sizeof(descDepth) );
		descDepth.Width = m_uRTwidth;
		descDepth.Height = m_uRTheight;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D11_USAGE_DEFAULT;
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;
		V_RETURN( pd3dDevice->CreateTexture2D( &descDepth, NULL, &m_pOutDSTex ));
		DXUT_SetDebugName(m_pOutDSTex,"m_pOutDSTex");

		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory(&descDSV, sizeof(descDSV));
		descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;
		V_RETURN( pd3dDevice->CreateDepthStencilView( m_pOutDSTex, &descDSV, &m_pOutDSSView ));  
		DXUT_SetDebugName(m_pOutDSSView,"m_pOutDSSView");

		// Create depth stencil state
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		ZeroMemory( &dsDesc, sizeof(dsDesc) );
		// Depth test parameters
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

		// Stencil test parameters
		dsDesc.StencilEnable = true;
		dsDesc.StencilReadMask = 0xFF;
		dsDesc.StencilWriteMask = 0xFF;

		// Stencil operations if pixel is front-facing
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Stencil operations if pixel is back-facing
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Create depth stencil state
		V_RETURN( pd3dDevice->CreateDepthStencilState(&dsDesc, &m_pOutDSState));
		DXUT_SetDebugName(m_pOutDSState,"m_pOutDSState");

		// Create the sample state
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory( &sampDesc, sizeof(sampDesc) );
		sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP       ;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP       ;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP       ;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		V_RETURN(pd3dDevice->CreateSamplerState( &sampDesc, &m_pSS_Linear ));
		DXUT_SetDebugName(m_pSS_Linear,"m_pSS_Linear");

		// rasterizer state
		D3D11_RASTERIZER_DESC rsDesc;
		rsDesc.FillMode = D3D11_FILL_WIREFRAME;
		rsDesc.CullMode = D3D11_CULL_NONE;
		rsDesc.FrontCounterClockwise = FALSE;
		rsDesc.DepthBias = 0;
		rsDesc.DepthBiasClamp = 0.0f;
		rsDesc.SlopeScaledDepthBias = 0.0f;
		rsDesc.DepthClipEnable = TRUE;
		rsDesc.ScissorEnable = FALSE;
		rsDesc.MultisampleEnable = FALSE;
		rsDesc.AntialiasedLineEnable = FALSE;
		V_RETURN(pd3dDevice->CreateRasterizerState(&rsDesc, &m_pOutRS));
		DXUT_SetDebugName(m_pOutRS,"m_pOutRS");

		m_Viewport.Width = (float)m_uRTwidth ;
		m_Viewport.Height = (float)m_uRTheight;
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;
		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;

		m_pVolSRV = pVolumeSRV;

		return hr;
	}

	void Resize()
	{
		// Setup the camera's projection parameters
			float fAspectRatio = m_uRTwidth / ( FLOAT )m_uRTheight;
			m_Camera.SetProjParams( XM_PI / 4, fAspectRatio, 0.01f, 500.0f );
			m_Camera.SetWindow(m_uRTwidth,m_uRTheight );
			m_Camera.SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );
	}

	void Release()
	{
		SAFE_RELEASE( m_pVS );
		SAFE_RELEASE( m_pPS );
		SAFE_RELEASE( m_pGS );
		SAFE_RELEASE( m_pVL );
		SAFE_RELEASE( m_pVB );

		SAFE_RELEASE( m_pSS_Linear );

		SAFE_RELEASE( m_pOutTex );
		SAFE_RELEASE( m_pOutSRV );
		SAFE_RELEASE( m_pOutRTV );
		SAFE_RELEASE( m_pOutRS );

		SAFE_RELEASE( m_pOutDSTex );
		SAFE_RELEASE( m_pOutDSSView );
		SAFE_RELEASE( m_pOutDSState );

		SAFE_RELEASE( m_pCB_HPMC );
	}

	void Update( float fElapsedTime)
	{
		m_Camera.FrameMove(fElapsedTime);
	}

	void Render( ID3D11DeviceContext* pd3dImmediateContext )
	{		
		XMMATRIX m_Proj = m_Camera.GetProjMatrix();
		XMMATRIX m_View = m_Camera.GetViewMatrix();
		XMMATRIX m_World =m_Camera.GetWorldMatrix();
		XMMATRIX m_WorldViewProjection = m_World*m_View*m_Proj;
		
		XMVECTOR t;

		m_cbPerFrame.mWorldViewProj = XMMatrixTranspose( m_WorldViewProjection );
		XMStoreFloat4(&m_cbPerFrame.viewPos, m_Camera.GetEyePt());
		pd3dImmediateContext->UpdateSubresource( m_pCB_HPMC, 0, NULL, &m_cbPerFrame, 0, 0 );
		
		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pd3dImmediateContext->ClearRenderTargetView( m_pOutRTV, ClearColor );
		pd3dImmediateContext->ClearDepthStencilView( m_pOutDSSView, D3D11_CLEAR_DEPTH, 1.0, 0 );

		pd3dImmediateContext->IASetInputLayout(m_pVL);
		UINT stride = sizeof( short );
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pVB, &stride, &offset );
		pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		//pd3dImmediateContext->OMSetRenderTargets(1,&m_pOutputRTV, NULL);
		pd3dImmediateContext->OMSetRenderTargets(1,&m_pOutRTV, m_pOutDSSView);
		pd3dImmediateContext->OMSetDepthStencilState(m_pOutDSState, 1);
		pd3dImmediateContext->RSSetViewports( 1, &m_Viewport );
		pd3dImmediateContext->VSSetShader( m_pVS, NULL, 0 );
		pd3dImmediateContext->PSSetShader( m_pPS, NULL, 0 );
		pd3dImmediateContext->GSSetShader( m_pGS, NULL, 0 );
		pd3dImmediateContext->GSSetConstantBuffers( 0, 1, &m_pCB_HPMC );
		pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &m_pCB_HPMC );
		pd3dImmediateContext->GSSetSamplers( 0, 1, &m_pSS_Linear );
		pd3dImmediateContext->GSSetShaderResources( 0, 1, &m_pVolSRV);
		//ID3D11RasterizerState* rs;
		//pd3dImmediateContext->RSGetState(&rs);
		//pd3dImmediateContext->RSSetState(m_pOutRS);

		pd3dImmediateContext->Draw( m_cbPerFrame.cubeInfo.x * m_cbPerFrame.cubeInfo.y * m_cbPerFrame.cubeInfo.z, 0 );
		//pd3dImmediateContext->RSSetState(rs);
		//SAFE_RELEASE( rs );
		ID3D11ShaderResourceView* pSRVNULL = NULL;
		pd3dImmediateContext->GSSetShaderResources( 0, 1, &pSRVNULL);
	}

	LRESULT HandleMessages( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		m_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

		switch( uMsg )
		{
		case WM_KEYDOWN:
			int nKey = static_cast< int >( wParam );

			if( nKey == '3' )
			{
				m_cbPerFrame.volSize.w -= 0.01;
			}
			if( nKey == '4' )
			{
				m_cbPerFrame.volSize.w += 0.01;
			}
			if( nKey == '5' )
			{
				m_cbPerFrame.cubeInfo.w *= 1.2;
				UpdateMCCubeInfo( m_cbPerFrame.cubeInfo.w );
			}
			if( nKey == '6' )
			{
				m_cbPerFrame.cubeInfo.w /= 1.2;
				UpdateMCCubeInfo( m_cbPerFrame.cubeInfo.w );
			}
			break;
		}

		return 0;
	}
};