#pragma once
#include <D3D11.h>
#include <DirectXMath.h>
#include <vector>
#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"
#include <iostream>

#include "Header.h"

#define frand() ((float)rand()/RAND_MAX)

using namespace DirectX;
using namespace std;

// Ball
struct Ball
{
    float r_sq;     // size of this metaball
    float R;        // radius of oribt
    float Speed;    // speed of rotation
    float Phase;    // initial phase
	XMFLOAT4 Col;	// color
};


// constant buffer contain information of the volume, only need to submit to GPU once volume property changes
struct CB_VolumeInfo
{
	UINT voxel_x;
	UINT voxel_y;
	UINT voxel_z;
	float voxel_size;
#if FLAT3D
	UINT tile_x;
	UINT tile_y;
	int dummy0;
	int dummy1;
#endif
};

struct CB_Balls
{
	XMFLOAT4	fBalls[150];
	XMFLOAT4	fBallsCol[150];
	int			iNumBalls;
	int			dummy0;
	int			dummy1;
	int			dummy2;
};

class DensityFuncVolume
{
public:
	ID3D11VertexShader*				m_pVS;
	ID3D11PixelShader*				m_pPS;
	ID3D11GeometryShader*			m_pGS;
	ID3D11InputLayout*				m_pVL;
	ID3D11Buffer*					m_pVB;
#if FLAT3D
	ID3D11Texture2D*				m_pVolTex;
#else
	ID3D11Texture3D*				m_pVolTex;
#endif
	ID3D11ShaderResourceView*		m_pVolSRV;
	ID3D11RenderTargetView*			m_pVolRTV;

	float							m_fVolumeSize_x;
	float							m_fVolumeSize_y;
	float							m_fVolumeSize_z;
	float							m_fVoxelSize;

	float							m_fVolumeSize;
	float							m_fBallsVolumeSize;
	float							m_fAveBallSize;


	D3D11_VIEWPORT					m_cViewport;

	ID3D11Buffer*					m_pCB_VolumeInfo;
	CB_VolumeInfo					m_cbVolumeInfo;

	ID3D11Buffer*					m_pCB_Balls;
	CB_Balls						m_cbBalls;

	std::vector<Ball>				m_vecBalls;
	bool							m_bAnimated;

	double							m_dTime;

	void AddBall()
	{
		Ball ball;
		float r = ( 0.6f * frand() + 0.7f ) * m_fVolumeSize_x * 0.05f;
		ball.r_sq = r * r;
		ball.R = m_fVolumeSize_x / 2.0f * 0.6f + ( frand() - 0.3f ) * m_fVolumeSize_x * 0.2f;

		if( ball.R + r > 0.45f * m_fVolumeSize_x)
		{
			r = 0.45f * m_fVolumeSize_x - ball.R;
			ball.r_sq = r * r;
		}
		float speedF =  6.f * ( frand() - 0.5f );
		if( abs( speedF ) < 1.f) speedF = ( speedF > 0.f ? 1.f : -1.f ) * 1.f;
		ball.Speed = 1.0f / ball.r_sq * 0.0005f * speedF;
		ball.Phase = frand() * 6.28f;

		float alpha = frand() * 6.28f;
		float beta = frand() * 6.28f;
		float gamma = frand() * 6.28f;

		XMMATRIX rMatrix = XMMatrixRotationRollPitchYaw( alpha, beta, gamma );
		XMVECTOR colVect = XMVector3TransformNormal( XMLoadFloat3( &XMFLOAT3( 1, 0, 0 )), rMatrix );
		XMFLOAT4 col;
		XMStoreFloat4( &col, colVect );
		col.x = abs( col.x );
		col.y = abs( col.y );
		col.z = abs( col.z );

		ball.Col = col;

		if( m_vecBalls.size() < MAX_BALLS ) m_vecBalls.push_back( ball );
	}

	void RemoveBall()
	{
		if( m_vecBalls.size() > 0 ) m_vecBalls.pop_back();
	}

	DensityFuncVolume( float voxelSize, UINT width = 384, UINT height = 384, UINT depth = 384 )
	{
		m_fVoxelSize = voxelSize;
		m_fVolumeSize_x = voxelSize * width;
		m_fVolumeSize_y = voxelSize * height;
		m_fVolumeSize_z = voxelSize * depth;
		m_bAnimated = true;

		m_cbVolumeInfo.voxel_x = width;
		m_cbVolumeInfo.voxel_y = height;
		m_cbVolumeInfo.voxel_z = depth;
		m_cbVolumeInfo.voxel_size = voxelSize;

		m_fVolumeSize = m_fVolumeSize_x * m_fVolumeSize_y * m_fVolumeSize_z;
		m_fBallsVolumeSize = m_fVolumeSize * BALL_VOLUME_FACTOR;
		m_dTime = 0;

#if FLAT3D
		m_cbVolumeInfo.tile_x = (UINT)ceil(sqrt(depth));
		m_cbVolumeInfo.tile_y = (UINT)ceil(sqrt(depth));
#endif
		for( int i = 0; i < 20; i++ )
			AddBall();
	}

	HRESULT CreateResource( ID3D11Device* pd3dDevice )
	{
		HRESULT hr = S_OK;
		ID3DBlob* pVSBlob = NULL;

		wstring filename = L"DensityFuncVolume.fx";

		V_RETURN(DXUTCompileFromFile(filename.c_str(), nullptr, "VS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pVSBlob));
		V_RETURN(pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),NULL,&m_pVS));
		DXUT_SetDebugName(m_pVS,"m_pVS");

		ID3DBlob* pGSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(filename.c_str(), nullptr, "GS", "gs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pGSBlob));
		V_RETURN(pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(),pGSBlob->GetBufferSize(),NULL,&m_pGS));
		DXUT_SetDebugName(m_pGS,"m_pGS");
		pGSBlob->Release();

		ID3DBlob* pPSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(filename.c_str(), nullptr, "PS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pPSBlob));
		V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),pPSBlob->GetBufferSize(),NULL,&m_pPS));
		DXUT_SetDebugName(m_pPS,"m_pPS");
		pPSBlob->Release();

		D3D11_INPUT_ELEMENT_DESC inputLayout[]=
		{{ "POSITION", 0, DXGI_FORMAT_R16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};
		V_RETURN(pd3dDevice->CreateInputLayout(inputLayout,ARRAYSIZE(inputLayout),pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),&m_pVL));
		DXUT_SetDebugName(m_pVL,"m_pVL");
		pVSBlob->Release();

		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DEFAULT;
#if FLAT3D
		bd.ByteWidth = sizeof( short );
#else
		bd.ByteWidth = sizeof( short )*m_cbVolumeInfo.voxel_z;
#endif
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		V_RETURN(pd3dDevice->CreateBuffer(&bd,NULL,&m_pVB));
		DXUT_SetDebugName(m_pVB,"m_pVB");

		// Create the constant buffers
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0  ;
		bd.ByteWidth = sizeof( CB_VolumeInfo );
		V_RETURN(pd3dDevice->CreateBuffer( &bd, NULL, &m_pCB_VolumeInfo ));
		DXUT_SetDebugName(m_pCB_VolumeInfo,"m_pCB_VolumeInfo");

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0  ;
		bd.ByteWidth = sizeof( CB_Balls );
		V_RETURN(pd3dDevice->CreateBuffer( &bd, NULL, &m_pCB_Balls ));
		DXUT_SetDebugName(m_pCB_Balls,"m_pCB_Balls");


		// Create the texture
#if FLAT3D
		D3D11_TEXTURE2D_DESC dstex;
		ZeroMemory( &dstex, sizeof(dstex));
		dstex.Width = m_cbVolumeInfo.voxel_x*m_cbVolumeInfo.tile_x;
		dstex.Height = m_cbVolumeInfo.voxel_y*m_cbVolumeInfo.tile_y;
		dstex.MipLevels = 1;
		dstex.ArraySize = 1;
		dstex.SampleDesc.Count = 1;
		dstex.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		dstex.Usage = D3D11_USAGE_DEFAULT;
		dstex.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		dstex.CPUAccessFlags = 0;
		dstex.MiscFlags = 0;
		V_RETURN( pd3dDevice->CreateTexture2D( &dstex, NULL, &m_pVolTex ))
		DXUT_SetDebugName(m_pVolTex,"m_pVolTex");

		D3D11_RENDER_TARGET_VIEW_DESC		RTVDesc;
		ZeroMemory( &RTVDesc, sizeof(RTVDesc));
		RTVDesc.Format = dstex.Format;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = 0;
		V_RETURN(pd3dDevice->CreateRenderTargetView(m_pVolTex, &RTVDesc,&m_pVolRTV));
		DXUT_SetDebugName(m_pVolRTV,"m_pVolRTV");

		D3D11_SHADER_RESOURCE_VIEW_DESC		SRViewDesc;
		ZeroMemory( &SRViewDesc, sizeof(SRViewDesc));
		SRViewDesc.Format = dstex.Format;
		SRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRViewDesc.Texture2D.MostDetailedMip = 0;
		SRViewDesc.Texture2D.MipLevels = 1;
		V_RETURN(pd3dDevice->CreateShaderResourceView(m_pVolTex, &SRViewDesc, &m_pVolSRV));
		DXUT_SetDebugName(m_pVolSRV,"m_pVolSRV");
#else
		D3D11_TEXTURE3D_DESC dstex;
		dstex.Width = m_cbVolumeInfo.voxel_x;
		dstex.Height = m_cbVolumeInfo.voxel_y;
		dstex.Depth = m_cbVolumeInfo.voxel_z;
		dstex.MipLevels = 1;
		dstex.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		dstex.Usage = D3D11_USAGE_DEFAULT;
		dstex.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		dstex.CPUAccessFlags = 0;
		dstex.MiscFlags = 0;
		V_RETURN( pd3dDevice->CreateTexture3D( &dstex, NULL, &m_pVolTex ));
		DXUT_SetDebugName(m_pVolTex,"m_pVolTex");

		// Create the resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		ZeroMemory( &SRVDesc, sizeof( SRVDesc ));
		SRVDesc.Format = dstex.Format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		SRVDesc.Texture3D.MostDetailedMip = 0;
		SRVDesc.Texture3D.MipLevels = 1;
		V_RETURN( pd3dDevice->CreateShaderResourceView( m_pVolTex, &SRVDesc,&m_pVolSRV ));
		DXUT_SetDebugName(m_pVolSRV,"m_pVolSRV");

		// Create the render target views
		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = dstex.Format;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
		RTVDesc.Texture3D.MipSlice = 0;
		RTVDesc.Texture3D.FirstWSlice = 0;
		RTVDesc.Texture3D.WSize = m_cbVolumeInfo.voxel_z;
		V_RETURN( pd3dDevice->CreateRenderTargetView( m_pVolTex, &RTVDesc, &m_pVolRTV ));
		DXUT_SetDebugName(m_pVolRTV,"m_pVolRTV");
#endif

		// set the new viewport
		m_cViewport.TopLeftX = 0;
		m_cViewport.TopLeftY = 0;
#if FLAT3D
		m_cViewport.Width = (float)m_cbVolumeInfo.voxel_x * m_cbVolumeInfo.tile_x;
		m_cViewport.Height = (float)m_cbVolumeInfo.voxel_y * m_cbVolumeInfo.tile_y;
#else
		m_cViewport.Width = (float)m_cbVolumeInfo.voxel_x;
		m_cViewport.Height = (float)m_cbVolumeInfo.voxel_y;
#endif
		m_cViewport.MinDepth = 0.0f;
		m_cViewport.MaxDepth = 1.0f;

		return hr;
	}

	void Release()
	{
		SAFE_RELEASE( m_pVS );
		SAFE_RELEASE( m_pPS );
		SAFE_RELEASE( m_pGS );
		SAFE_RELEASE( m_pVL );
		SAFE_RELEASE( m_pVB );

		SAFE_RELEASE( m_pVolSRV );
		SAFE_RELEASE( m_pVolTex );
		SAFE_RELEASE( m_pVolRTV );

		SAFE_RELEASE( m_pCB_VolumeInfo );
		SAFE_RELEASE( m_pCB_Balls );
	}

	void Update( double fTime, float fElapsedTime )
	{
		if( m_bAnimated ){
			m_dTime += fElapsedTime;
			m_cbBalls.iNumBalls = (int)m_vecBalls.size();
			m_fAveBallSize = pow( 0.75f * m_fBallsVolumeSize / m_cbBalls.iNumBalls / 3.1415926f , 1.0f / 3.0f );
			for( int i = 0; i < m_vecBalls.size(); i++ ){
				Ball ball = m_vecBalls[i];
				m_cbBalls.fBalls[i].x = ball.R * (float)cosf( m_dTime * ball.Speed + ball.Phase );
				m_cbBalls.fBalls[i].y = ball.R * (float)sinf( m_dTime * ball.Speed + ball.Phase );
				m_cbBalls.fBalls[i].z = 0.3f * ball.R * (float)sinf( 2.f * m_dTime * ball.Speed + ball.Phase );
				m_cbBalls.fBalls[i].w = ball.r_sq;
				m_cbBalls.fBallsCol[i] = ball.Col;
			}
		}
	}

	void Render( ID3D11DeviceContext* pd3dImmediateContext )
	{
		float ClearColor[2] = { 0.0f,0.0f };
		pd3dImmediateContext->ClearRenderTargetView( m_pVolRTV, ClearColor );

		pd3dImmediateContext->RSSetViewports( 1, &m_cViewport );
		pd3dImmediateContext->IASetInputLayout(m_pVL);
		UINT stride = sizeof( short );
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pVB, &stride, &offset );
		pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		pd3dImmediateContext->OMSetRenderTargets( 1, &m_pVolRTV, NULL );
		pd3dImmediateContext->UpdateSubresource( m_pCB_VolumeInfo, 0, NULL, &m_cbVolumeInfo, 0, 0 );
		pd3dImmediateContext->UpdateSubresource( m_pCB_Balls, 0, NULL, &m_cbBalls, 0, 0 );

		pd3dImmediateContext->VSSetShader( m_pVS, NULL, 0 );
		pd3dImmediateContext->GSSetShader(m_pGS,NULL,0);
		pd3dImmediateContext->PSSetShader( m_pPS, NULL, 0 );
		pd3dImmediateContext->GSSetConstantBuffers( 0, 1, &m_pCB_VolumeInfo );
		pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &m_pCB_Balls );
#if FLAT3D
		pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &m_pCB_VolumeInfo );
		pd3dImmediateContext->Draw( 1, 0);
#else
		pd3dImmediateContext->Draw(m_cbVolumeInfo.voxel_z, 0);
#endif
	}

	LRESULT HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_KEYDOWN:
			{
				int nKey = static_cast<int>(wParam);

				if (nKey == '1')
				{
					RemoveBall();
				}
				if (nKey == '2')
				{
					AddBall();
				}
				if (nKey == VK_SPACE)
				{
					m_bAnimated = !m_bAnimated;
				}
				break;
			}
		}
		return 0;
	}
};