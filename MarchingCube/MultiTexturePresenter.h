#pragma once
 
#include <D3D11.h>
#include"DXUT.h"
#include <DirectXMath.h>
#include "DXUTcamera.h"
#include "SDKmisc.h"
#include <iostream>
using namespace std;


class MultiTexturePresenter
{
public:
	ID3D11VertexShader*				m_pVertexShader;
	ID3D11GeometryShader*			m_pGeometryShader;
	ID3D11PixelShader*				m_pPixelShader;
	ID3D11InputLayout*				m_pInputLayout;
	ID3D11Buffer*					m_pVertexBuffer;

	ID3D11ShaderResourceView*		m_pInputTextureRV[6];
	ID3D11ShaderResourceView*		m_pNullSRV[6];
	ID3D11SamplerState*				m_pGeneralTextureSS;

	ID3D11Texture2D*				m_pOutputTexture2D;
	ID3D11RenderTargetView*			m_pOutputTextureRTV;
	ID3D11ShaderResourceView*		m_pOutputTextureRV;

	D3D11_VIEWPORT					m_RTviewport;

	CModelViewerCamera*				m_pVCamera[6];

	UINT						m_uTextureWidth;
	UINT						m_uTextureHeight;
	UINT						m_uRTwidth;
	UINT						m_uRTheight;

	bool						m_bDirectToBackBuffer;
	UINT						m_uTextureNumber;
	UINT						m_uLayoutStyle;//1,2,4,6 subWindows

	MultiTexturePresenter(UINT numOfTexture=1,bool bRenderToBackbuffer = true,UINT width=640, UINT height=480)
	{
		m_bDirectToBackBuffer = bRenderToBackbuffer;
		m_uTextureWidth = width;
		m_uTextureHeight = height;
		m_uTextureNumber = numOfTexture;

		m_pNullSRV[0] = NULL;
		m_pNullSRV[1] = NULL;
		m_pNullSRV[2] = NULL;
		m_pNullSRV[3] = NULL;
		m_pNullSRV[4] = NULL;
		m_pNullSRV[5] = NULL;

	}

	HRESULT Initial()
	{
		return S_OK;
	}
	void ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings)
	{
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory( &sd, sizeof( sd ) );
		int widthNum;
		int heightNum;
		switch (m_uTextureNumber)
		{
		case 1:
			widthNum=1;heightNum=1;break;
		case 2:
			widthNum=2;heightNum=1;break;
		case 3:
			widthNum=2;heightNum=2;break;
		case 4:
			widthNum=2;heightNum=2;break;
		case 5:
			widthNum=3;heightNum=2;break;
		case 6:
			widthNum=3;heightNum=2;break;
		default:
			widthNum=1;heightNum=1;
		}

		sd.BufferCount = 1;
		sd.BufferDesc.Width = m_uTextureWidth*widthNum;
		sd.BufferDesc.Height = m_uTextureHeight*heightNum;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = pDeviceSettings->d3d11.sd.OutputWindow;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		pDeviceSettings->d3d11.sd=sd;
	}
	HRESULT CreateResource(ID3D11Device* pd3dDevice,
		ID3D11ShaderResourceView* pInputTextureRV1,
		ID3D11ShaderResourceView* pInputTextureRV2 = NULL,
		ID3D11ShaderResourceView* pInputTextureRV3 = NULL,
		ID3D11ShaderResourceView* pInputTextureRV4 = NULL,
		ID3D11ShaderResourceView* pInputTextureRV5 = NULL,
		ID3D11ShaderResourceView* pInputTextureRV6 = NULL)
	{
		HRESULT hr=S_OK;
		string GSname="GS_";

		m_pInputTextureRV[0] = pInputTextureRV1;
		m_pInputTextureRV[1] = pInputTextureRV2;
		m_pInputTextureRV[2] = pInputTextureRV3;
		m_pInputTextureRV[3] = pInputTextureRV4;
		m_pInputTextureRV[4] = pInputTextureRV5;
		m_pInputTextureRV[5] = pInputTextureRV6;

		m_uTextureNumber = 0;

		for(int i=0 ;i<=5;i++)
			if(m_pInputTextureRV[i])
				m_uTextureNumber++;


		
		if( m_uTextureNumber <= 1 )
		{
			GSname = "GS_1";
			m_uRTwidth = m_uTextureWidth;
			m_uRTheight = m_uTextureHeight;
		}
		else if( m_uTextureNumber >= 3 && m_uTextureNumber<5)
		{
			GSname = "GS_4";
			m_uRTwidth = 2 * m_uTextureWidth;
			m_uRTheight = 2 * m_uTextureHeight;
		}
		else if( m_uTextureNumber >= 5 )
		{
			GSname = "GS_6";
			m_uRTwidth = 3 * m_uTextureWidth;
			m_uRTheight = 2 * m_uTextureHeight;
		}
		else
		{
			GSname = "GS_2";
			m_uRTwidth = 2 * m_uTextureWidth;
			m_uRTheight = m_uTextureHeight;
		}

		ID3DBlob* pVSBlob = NULL;
		wstring filename = L"MultiTexturePresenter.fx";

		V_RETURN(DXUTCompileFromFile(filename.c_str(), nullptr, "VS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pVSBlob));
		V_RETURN(pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),NULL,&m_pVertexShader));

		ID3DBlob* pGSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(filename.c_str(), nullptr, GSname.c_str(), "gs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pGSBlob));
		V_RETURN(pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(),pGSBlob->GetBufferSize(),NULL,&m_pGeometryShader));
		pGSBlob->Release();

		ID3DBlob* pPSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(filename.c_str(), nullptr, "PS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pPSBlob));
		V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),pPSBlob->GetBufferSize(),NULL,&m_pPixelShader));
		pPSBlob->Release();

		D3D11_INPUT_ELEMENT_DESC layout[] = { { "POSITION", 0, DXGI_FORMAT_R16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 } };
		V_RETURN(pd3dDevice->CreateInputLayout(layout,ARRAYSIZE(layout),pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),&m_pInputLayout));
		pVSBlob->Release();

		// Create the vertex buffer
		D3D11_BUFFER_DESC bd = {0};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(short);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		V_RETURN(pd3dDevice->CreateBuffer(&bd, NULL, &m_pVertexBuffer));

		// Create rendertarget resource
		if( !m_bDirectToBackBuffer )
		{
			D3D11_TEXTURE2D_DESC	RTtextureDesc = {0};
			RTtextureDesc.Width = m_uRTwidth;
			RTtextureDesc.Height = m_uRTheight;
			RTtextureDesc.MipLevels = 1;
			RTtextureDesc.ArraySize = 1;
			RTtextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			//RTtextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			
			RTtextureDesc.SampleDesc.Count = 1;
			RTtextureDesc.SampleDesc.Quality = 0;
			RTtextureDesc.Usage = D3D11_USAGE_DEFAULT;
			RTtextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			RTtextureDesc.CPUAccessFlags = 0;
			RTtextureDesc.MiscFlags = 0;
			V_RETURN(pd3dDevice->CreateTexture2D(&RTtextureDesc, NULL, &m_pOutputTexture2D));

			D3D11_RENDER_TARGET_VIEW_DESC		RTViewDesc;
			ZeroMemory( &RTViewDesc, sizeof(RTViewDesc));
			RTViewDesc.Format = RTtextureDesc.Format;
			RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			RTViewDesc.Texture2D.MipSlice = 0;
			V_RETURN(pd3dDevice->CreateRenderTargetView(m_pOutputTexture2D, &RTViewDesc,&m_pOutputTextureRTV));

			D3D11_SHADER_RESOURCE_VIEW_DESC		SRViewDesc;
			ZeroMemory( &SRViewDesc, sizeof(SRViewDesc));
			SRViewDesc.Format = RTtextureDesc.Format;
			SRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			SRViewDesc.Texture2D.MostDetailedMip = 0;
			SRViewDesc.Texture2D.MipLevels = 1;
			V_RETURN(pd3dDevice->CreateShaderResourceView(m_pOutputTexture2D, &SRViewDesc, &m_pOutputTextureRV));
		}

		m_RTviewport.Width = (float)m_uRTwidth;
		m_RTviewport.Height = (float)m_uRTheight;
		m_RTviewport.MinDepth = 0.0f;
		m_RTviewport.MaxDepth = 1.0f;
		m_RTviewport.TopLeftX = 0;
		m_RTviewport.TopLeftY = 0;

		// Create the sample state
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory( &sampDesc, sizeof(sampDesc) );
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		V_RETURN(pd3dDevice->CreateSamplerState(&sampDesc, &m_pGeneralTextureSS ));
		
		return hr;
	}

	void LinkVCamera(CModelViewerCamera* pVC1= NULL,
		CModelViewerCamera* pVC2= NULL, 
		CModelViewerCamera* pVC3= NULL, 
		CModelViewerCamera* pVC4= NULL, 
		CModelViewerCamera* pVC5= NULL, 
		CModelViewerCamera* pVC6= NULL)
	{
		m_pVCamera[0] = pVC1;
		m_pVCamera[1] = pVC2;
		m_pVCamera[2] = pVC3;
		m_pVCamera[3] = pVC4;
		m_pVCamera[4] = pVC5;
		m_pVCamera[5] = pVC6;
	}

	void SetupPipeline(ID3D11DeviceContext* pd3dImmediateContext)
	{
		pd3dImmediateContext->OMSetRenderTargets(1,&m_pOutputTextureRTV,NULL);
		pd3dImmediateContext->IASetInputLayout(m_pInputLayout);
		pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		UINT stride = 0;
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
		pd3dImmediateContext->VSSetShader( m_pVertexShader, NULL, 0 );
		pd3dImmediateContext->GSSetShader(m_pGeometryShader,NULL,0);
		pd3dImmediateContext->PSSetShader( m_pPixelShader, NULL, 0 );
		pd3dImmediateContext->PSSetShaderResources(0, m_uTextureNumber, m_pInputTextureRV);
		pd3dImmediateContext->PSSetSamplers(0,1,&m_pGeneralTextureSS);
		pd3dImmediateContext->RSSetViewports(1, &m_RTviewport);

		
		 
		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pd3dImmediateContext->ClearRenderTargetView(m_pOutputTextureRTV,ClearColor);
	}

	void Resize()
	{
		if(m_uTextureNumber<=4)
		{
			for(int i =0 ;i<4;i++)
			{
				if(m_pVCamera[i]!=NULL)
				{
					RECT rc;
					rc.top=i/2*m_uTextureHeight;
					rc.bottom=(i/2+1)*m_uTextureHeight;
					rc.left=i%2*m_uTextureWidth;
					rc.right=(i%2+1)*m_uTextureWidth;
					m_pVCamera[i]->SetDragRect(rc);
				}
			}
		}
		else
		{
			for(int i =0 ;i<6;i++)
			{
				if(m_pVCamera[i]!=NULL)
				{
					RECT rc;
					rc.top=i/3*m_uTextureHeight;
					rc.bottom=(i/3+1)*m_uTextureHeight;
					rc.left=i%3*m_uTextureWidth;
					rc.right=(i%3+1)*m_uTextureWidth;
					m_pVCamera[i]->SetDragRect(rc);
				}
			}
		}
		if(m_bDirectToBackBuffer)
		{
			m_pOutputTextureRTV = DXUTGetD3D11RenderTargetView();
		}
	}

	void Update()
	{

	}

	void Render(ID3D11DeviceContext* pd3dImmediateContext)
	{
		this->SetupPipeline(pd3dImmediateContext);
		pd3dImmediateContext->Draw(m_uTextureNumber,0);

		pd3dImmediateContext->PSSetShaderResources(0,m_uTextureNumber,m_pNullSRV);

	}

	void Release()
	{
		SAFE_RELEASE(m_pVertexShader);
		SAFE_RELEASE(m_pPixelShader);
		SAFE_RELEASE(m_pGeometryShader);
		SAFE_RELEASE(m_pInputLayout);
		SAFE_RELEASE(m_pVertexBuffer);
		SAFE_RELEASE(m_pGeneralTextureSS);
		if(!m_bDirectToBackBuffer)
		{
			SAFE_RELEASE(m_pOutputTexture2D);
			SAFE_RELEASE(m_pOutputTextureRTV);
			SAFE_RELEASE(m_pOutputTextureRV);
		}
	}

	LRESULT HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(lParam);
		UNREFERENCED_PARAMETER(hWnd);

		return 0;
	}
};