#pragma once
#include <D3D11.h>
#include <DirectXMath.h>
#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"

#include "Header.h"

using namespace DirectX;

struct CB_rayCast
{
	XMFLOAT4 voxelInfo; // .xyz demension on xyz, .w is voxel size
	XMFLOAT4 invXYZsize; // is 1.0f / ( voxelRes * voxelSize ); isolevel in .w component
	XMMATRIX mWorldViewProjection;
	XMMATRIX mInvWorldView;
	XMFLOAT4 vViewPos;
	XMFLOAT2 vWinTR; // half window size in local space
	XMINT2 vTile_num;
	XMFLOAT4 boxMin;
	XMFLOAT4 boxMax;
};

struct Vertex_Cube
{
	XMFLOAT4	pos;
};

class RayCast
{
public:
	CModelViewerCamera              m_Camera;               // A model viewing camera

	ID3D11VertexShader*				m_pVS;
	ID3D11PixelShader*				m_pPS;
	ID3D11GeometryShader*			m_pGS;
	ID3D11SamplerState*				m_pSS_Linear;
	ID3D11InputLayout*				m_pVL;
	ID3D11Buffer*					m_pVB;

	// For render result to a texture
	D3D11_VIEWPORT					m_mViewport;
	ID3D11Texture2D*				m_pOutputTex2D;
	ID3D11ShaderResourceView*		m_pOutputSRV;
	ID3D11RenderTargetView*			m_pOutputRTV;
	UINT							m_uRTwidth;
	UINT							m_uRTheight;

	// For depth stencil resource
	ID3D11Texture2D*				m_pDepthStencilTex2D;
	ID3D11DepthStencilView*			m_pDepthStencilView;
	ID3D11DepthStencilState*		m_pDepthStencilState;

	ID3D11Buffer*					m_pCB_rayCast;
	CB_rayCast						m_cbPerFrame;

	ID3D11ShaderResourceView*		m_VolumeSRV;

	RayCast( float volumeVoxel_size, UINT volumeVoxel_x, UINT volumeVoxel_y, UINT volumeVoxel_z,
		bool RTTexture = false, UINT txWidth = SUB_TEXTUREWIDTH, UINT txHeight = SUB_TEXTUREHEIGHT )
	{
		m_cbPerFrame.voxelInfo.w = volumeVoxel_size;
		m_cbPerFrame.voxelInfo.x = volumeVoxel_x;
		m_cbPerFrame.voxelInfo.y = volumeVoxel_y;
		m_cbPerFrame.voxelInfo.z = volumeVoxel_z;
#if FLAT3D
		m_cbPerFrame.vTile_num.x = (UINT)ceil(sqrt(volumeVoxel_z));
		m_cbPerFrame.vTile_num.y = (UINT)ceil(sqrt(volumeVoxel_z));
#endif
		m_cbPerFrame.invXYZsize.x = 1.0f / ( volumeVoxel_x * volumeVoxel_size );
		m_cbPerFrame.invXYZsize.y = 1.0f / ( volumeVoxel_y * volumeVoxel_size );
		m_cbPerFrame.invXYZsize.z = 1.0f / ( volumeVoxel_z * volumeVoxel_size );
		m_cbPerFrame.invXYZsize.w = 1.0f;
		m_cbPerFrame.boxMin = XMFLOAT4( -(volumeVoxel_x * volumeVoxel_size / 2.0f), 
										-(volumeVoxel_y * volumeVoxel_size / 2.0f), 
										-(volumeVoxel_z * volumeVoxel_size / 2.0f), 0 );
		m_cbPerFrame.boxMax = XMFLOAT4( volumeVoxel_x * volumeVoxel_size / 2.0f, 
										volumeVoxel_y * volumeVoxel_size / 2.0f, 
										volumeVoxel_z * volumeVoxel_size / 2.0f, 0 );
		m_uRTwidth = txWidth;
		m_uRTheight = txHeight;

		XMVECTORF32 vecEye = {0.0f, 0.0f, -2.0f};
		XMVECTORF32 vecAt = {0.0f, 0.0f, 0.0f};
		m_Camera.SetViewParams( vecEye, vecAt );
		m_pOutputSRV = NULL;
	}

	HRESULT CreateResource( ID3D11Device* pd3dDevice, ID3D11ShaderResourceView*	pVolumeSRV )
	{
		HRESULT hr = S_OK;
		ID3DBlob* pVSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"RayCast.fx", nullptr, "VS", "vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pVSBlob));
		V_RETURN(pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),NULL,&m_pVS));
		DXUT_SetDebugName(m_pVS,"m_pVS");

		ID3DBlob* pPSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"RayCast.fx", nullptr, "PS", "ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pPSBlob));
		V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),pPSBlob->GetBufferSize(),NULL,&m_pPS));
		DXUT_SetDebugName(m_pPS,"m_pPS");
		pPSBlob->Release();

		ID3DBlob* pGSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"RayCast.fx", nullptr, "GS_Quad", "gs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pGSBlob));
		V_RETURN(pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(),pGSBlob->GetBufferSize(),NULL,&m_pGS));
		DXUT_SetDebugName(m_pGS,"m_pGS");
		pGSBlob->Release();

		D3D11_INPUT_ELEMENT_DESC inputLayout[]=
		{{ "POSITION", 0, DXGI_FORMAT_R16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};
		V_RETURN(pd3dDevice->CreateInputLayout(inputLayout,ARRAYSIZE(inputLayout),pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),&m_pVL));
		DXUT_SetDebugName(m_pVL,"m_pVL");
		pVSBlob->Release();

		//Vertex_Cube* pVertex = new Vertex_Cube[8];

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
		bd.ByteWidth = sizeof( CB_rayCast );
		V_RETURN(pd3dDevice->CreateBuffer( &bd, NULL, &m_pCB_rayCast ));
		DXUT_SetDebugName(m_pCB_rayCast,"m_pCB_rayCast");

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
		V_RETURN(pd3dDevice->CreateTexture2D(&RTtextureDesc, NULL, &m_pOutputTex2D));
		DXUT_SetDebugName(m_pOutputTex2D,"m_pOutputTex2D");

		D3D11_RENDER_TARGET_VIEW_DESC		RTViewDesc;
		ZeroMemory( &RTViewDesc, sizeof(RTViewDesc));
		RTViewDesc.Format = RTtextureDesc.Format;
		RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTViewDesc.Texture2D.MipSlice = 0;
		V_RETURN(pd3dDevice->CreateRenderTargetView(m_pOutputTex2D, &RTViewDesc,&m_pOutputRTV));
		DXUT_SetDebugName(m_pOutputRTV,"m_pOutputRTV");

		D3D11_SHADER_RESOURCE_VIEW_DESC		SRViewDesc;
		ZeroMemory( &SRViewDesc, sizeof(SRViewDesc));
		SRViewDesc.Format = RTtextureDesc.Format;
		SRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRViewDesc.Texture2D.MostDetailedMip = 0;
		SRViewDesc.Texture2D.MipLevels = 1;
		V_RETURN(pd3dDevice->CreateShaderResourceView(m_pOutputTex2D, &SRViewDesc, &m_pOutputSRV));
		DXUT_SetDebugName(m_pOutputSRV,"m_pOutputSRV");

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
		V_RETURN( pd3dDevice->CreateTexture2D( &descDepth, NULL, &m_pDepthStencilTex2D ));
		DXUT_SetDebugName(m_pDepthStencilTex2D,"m_pDepthStencilTex2D");

		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory(&descDSV, sizeof(descDSV));
		descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;
		V_RETURN( pd3dDevice->CreateDepthStencilView( m_pDepthStencilTex2D, &descDSV, &m_pDepthStencilView ));  
		DXUT_SetDebugName(m_pDepthStencilView,"m_pDepthStencilView");

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
		ID3D11DepthStencilState * pDSState;
		V_RETURN( pd3dDevice->CreateDepthStencilState(&dsDesc, &m_pDepthStencilState));
		DXUT_SetDebugName(m_pDepthStencilState,"m_pDepthStencilState");

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

		m_mViewport.Width = (float)m_uRTwidth ;
		m_mViewport.Height = (float)m_uRTheight;
		m_mViewport.MinDepth = 0.0f;
		m_mViewport.MaxDepth = 1.0f;
		m_mViewport.TopLeftX = 0;
		m_mViewport.TopLeftY = 0;

		m_VolumeSRV = pVolumeSRV;

		return hr;
	}

	void Resize()
	{
		// Setup the camera's projection parameters
			float fAspectRatio = m_uRTwidth / ( FLOAT )m_uRTheight;
			m_Camera.SetProjParams( XM_PI / 4, fAspectRatio, 0.01f, 500.0f );
			m_cbPerFrame.vWinTR.y = tan(XM_PI/8.0f);
			m_cbPerFrame.vWinTR.x = m_cbPerFrame.vWinTR.y*fAspectRatio;
			
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

		SAFE_RELEASE( m_pOutputTex2D );
		SAFE_RELEASE( m_pOutputSRV );
		SAFE_RELEASE( m_pOutputRTV );

		SAFE_RELEASE( m_pDepthStencilTex2D );
		SAFE_RELEASE( m_pDepthStencilView );
		SAFE_RELEASE( m_pDepthStencilState );

		SAFE_RELEASE( m_pCB_rayCast );
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


		m_cbPerFrame.mWorldViewProjection = XMMatrixTranspose( m_WorldViewProjection );
		m_cbPerFrame.mInvWorldView = XMMatrixTranspose( XMMatrixInverse(&t,m_World*m_View));
		XMStoreFloat4(&m_cbPerFrame.vViewPos,m_Camera.GetEyePt());
/*
		float x = m_cbPerFrame.vViewPos.x;
		float y = m_cbPerFrame.vViewPos.y;
		float z = m_cbPerFrame.vViewPos.z;
		float dist = sqrt(x*x+y*y+z*z);
		float fAspectRatio = m_uRTwidth / ( FLOAT )m_uRTheight;
		m_cbPerFrame.vWinTR.x = tan(XM_PI/4.0f)/dist;
		m_cbPerFrame.vWinTR.y = m_cbPerFrame.vWinTR.x/fAspectRatio;*/
		pd3dImmediateContext->UpdateSubresource( m_pCB_rayCast, 0, NULL, &m_cbPerFrame, 0, 0 );
		
		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pd3dImmediateContext->ClearRenderTargetView( m_pOutputRTV, ClearColor );
		pd3dImmediateContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0 );

		pd3dImmediateContext->IASetInputLayout(m_pVL);
		UINT stride = sizeof( short );
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pVB, &stride, &offset );
		pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		//pd3dImmediateContext->OMSetRenderTargets(1,&m_pOutputRTV, NULL);
		pd3dImmediateContext->OMSetRenderTargets(1,&m_pOutputRTV, m_pDepthStencilView);
		pd3dImmediateContext->OMSetDepthStencilState(m_pDepthStencilState, 1);
		pd3dImmediateContext->RSSetViewports( 1, &m_mViewport );
		pd3dImmediateContext->VSSetShader( m_pVS, NULL, 0 );
		pd3dImmediateContext->PSSetShader( m_pPS, NULL, 0 );
		pd3dImmediateContext->GSSetShader( m_pGS, NULL, 0 );
		pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &m_pCB_rayCast );
		pd3dImmediateContext->GSSetConstantBuffers( 0, 1, &m_pCB_rayCast );
		pd3dImmediateContext->PSSetSamplers( 0, 1, &m_pSS_Linear );
		pd3dImmediateContext->PSSetShaderResources( 0, 1, &m_VolumeSRV);

		pd3dImmediateContext->Draw( 1, 0 );

		ID3D11ShaderResourceView* pSRVNULL = NULL;
		pd3dImmediateContext->PSSetShaderResources( 0, 1, &pSRVNULL);
	}

	LRESULT HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

		switch(uMsg)
		{
		case WM_KEYDOWN:
			{
				int nKey = static_cast<int>(wParam); 

				if (nKey == '3')
				{
					m_cbPerFrame.invXYZsize.w -= 0.01;
				}
				if (nKey == '4')
				{
					m_cbPerFrame.invXYZsize.w += 0.01;
				}
				break;
			}
		}

		return 0;
	}
};