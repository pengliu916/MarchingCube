#pragma once
#include <D3D11.h>
#include <DirectXMath.h>
#include <cmath>
#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"

#include "Header.h"

#include "rply.h"

using namespace DirectX;

// Compile time constant(log2)
template<size_t N>
struct log2_{
	enum{ value = 1 + log2_<(N + 1) / 2>::value };
};
template<>
struct log2_ < 1 > {
	enum{ value = 0 };
};
// Compile time constant(log2 + 1)
template<size_t N, size_t S = 1>
struct func_{
	enum{ value = S + log2_<N>::value };
};

struct CB_HPMC_Init{
	XMFLOAT4	cb_f4HPMCInfo;// xyz: dimemsion of MC grid; w is sub cube size;
	XMFLOAT4	cb_f4VolInfo;// xyz:dimension of volume input; w is voxel size;
};

struct CB_HPMC_Frame{
	XMFLOAT4	cb_f4ViewPos;
	XMMATRIX	cb_mWorldViewProj;
	XMMATRIX	cb_mWorld;
};

struct CB_HPMC_Reduct{
	XMINT4		cb_i4RTReso;
};

class HistoPyramidMC
{
public:
	CModelViewerCamera				m_Camera;// Model viewing camera for generating image
	D3D11_VIEWPORT					m_Viewport;// Viewport for output image

	// Constant buffer, need to update each frame
	CB_HPMC_Frame					m_cbPerFrame;
	ID3D11Buffer*					m_pCB_HPMC_Frame;
	CB_HPMC_Init					m_cbInit;
	ID3D11Buffer*					m_pCB_HPMC_Init;
	CB_HPMC_Reduct					m_cbReduct;
	ID3D11Buffer*					m_pCB_HPMC_Reduct;


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
	ID3D11VertexShader*				m_pPassVS;
	ID3D11GeometryShader*			m_pVolSliceNorGS;// GS for creating HP base level;
	ID3D11GeometryShader*			m_pVolSliceGS;// GS for creating HP's rest level;
	ID3D11GeometryShader*			m_pTraversalGS;// GS for traversing HP and generating triangles;
	ID3D11PixelShader*				m_pHPMCBasePS;// Discriminator: convert SDF vol to HPMC base(active/inactive cells)
	ID3D11PixelShader*				m_pReductionPS;// Reduction PS: 3D version, 8 cells sum to 1 cell
	ID3D11PixelShader*				m_pReductionBasePS;// Reduction PS: 3D version, 8 cells sum to 1 cell
	ID3D11PixelShader*				m_pRenderPS;// Rendering PS: basic phong shading
	ID3D11SamplerState*				m_pSS_Linear;
	ID3D11InputLayout*				m_pPassVL;
	ID3D11Buffer*					m_pPassVB;

	// Resource for output stream
	ID3D11GeometryShader*			m_pTraversalAndOutGS;
	ID3D11Buffer*					m_pOutVB;
	ID3D11Buffer*					m_pOutVBCPU;
	UINT							m_uOutVBsize;
	float*							m_pVertex;
	UINT							m_uVertexCount;
	ID3D11Query*					m_pSOQuery;// Query interface for retriving triangle count from SO
	UINT64							m_u64SOOutput[2];

	//Video memory resource for HPMC pass
	ID3D11ShaderResourceView*		m_pNullSRV[func_<VOXEL_NUM_X, 3>::value];
	ID3D11ShaderResourceView*		m_pHistoPyramidSRV[func_<VOXEL_NUM_X>::value];
	ID3D11RenderTargetView*			m_pHistoPyramidRTV[func_<VOXEL_NUM_X>::value];
	ID3D11Texture3D*				m_pHistoPyramidTex[func_<VOXEL_NUM_X>::value];
	ID3D11Texture3D*				m_pHPTopTex;

	// Shader Resource View for volume data (input)
	ID3D11ShaderResourceView*		m_pVolSRV;

	// Framewire and solid switcher
	bool							m_bFramewire;

	// Output vertex
	bool							m_bOutputMesh;
	bool							m_bOutputInProgress;

	HistoPyramidMC(XMFLOAT4 volumeTexInfo, bool RTTexture = false,
				   UINT txWidth = SUB_TEXTUREWIDTH, UINT txHeight = SUB_TEXTUREHEIGHT)
	{
		m_cbInit.cb_f4VolInfo = volumeTexInfo;
		m_cbInit.cb_f4VolInfo.w = 1;
		m_cbInit.cb_f4HPMCInfo = volumeTexInfo;
		m_cbReduct.cb_i4RTReso = XMINT4(round(volumeTexInfo.x), round(volumeTexInfo.y),
										round(volumeTexInfo.z), 0);
		m_uRTwidth = txWidth;
		m_uRTheight = txHeight;
		m_bFramewire = false;
		m_bOutputMesh = false;
		
		m_bOutputInProgress = false;
		m_uOutVBsize = 100000000;
		m_pVertex = new float[m_uOutVBsize*6];
		XMVECTORF32 vecEye = { 0.0f, 0.0f, -2.0f };
		XMVECTORF32 vecAt = { 0.0f, 0.0f, 0.0f };
		m_Camera.SetViewParams(vecEye, vecAt);
		m_pOutSRV = NULL;
	}

	bool OutputMesh(){
		p_ply ply = ply_create("test.ply", PLY_ASCII, NULL, 0, NULL);
		if(!ply) return false;

		// add vertex element definition
		ply_add_element(ply, "vertex", m_uVertexCount);
		ply_add_scalar_property(ply, "x", PLY_FLOAT);
		ply_add_scalar_property(ply, "y", PLY_FLOAT);
		ply_add_scalar_property(ply, "z", PLY_FLOAT);
		ply_add_scalar_property(ply, "red", PLY_UCHAR);
		ply_add_scalar_property(ply, "green", PLY_UCHAR);
		ply_add_scalar_property(ply, "blue", PLY_UCHAR);
		
		// add face element definition
		ply_add_element(ply, "face", m_uVertexCount / 3);
		ply_add_list_property(ply, "vertex_indices", PLY_UCHAR, PLY_UINT32);

		if(!ply_write_header(ply)) return false;

		for(int i = 0; i < m_uVertexCount * 6 ; i+=6 ){
			ply_write(ply, m_pVertex[i]);
			ply_write(ply, m_pVertex[i+1]);
			ply_write(ply, m_pVertex[i+2]);
			ply_write(ply, m_pVertex[i+3]*255);
			ply_write(ply, m_pVertex[i+4]*255);
			ply_write(ply, m_pVertex[i+5]*255);
		}
		UINT vertexID = 0;
		for(int i = 0; i < m_uVertexCount / 3; i++){
			ply_write(ply, 3);
			ply_write(ply, vertexID);
			ply_write(ply, vertexID+2);
			ply_write(ply, vertexID+1);
			vertexID += 3;
		}

		ply_close(ply);
		m_bOutputInProgress = false;
		return true;
	}
	HRESULT CreateResource(ID3D11Device* pd3dDevice, ID3D11ShaderResourceView*	pVolumeSRV)
	{
		HRESULT hr = S_OK;
		
		// Create the quary object
		D3D11_QUERY_DESC queryDesc;
		queryDesc.MiscFlags = 0;
		queryDesc.Query = D3D11_QUERY_SO_STATISTICS;
		V_RETURN(pd3dDevice->CreateQuery(&queryDesc, &m_pSOQuery));
		DXUT_SetDebugName(m_pSOQuery, "m_pSOQuery");

		ID3DBlob* pVSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "PassVS", "vs_5_0", COMPILE_FLAG, 0, &pVSBlob));
		V_RETURN(pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_pPassVS));
		DXUT_SetDebugName(m_pPassVS, "m_pPassVS");

		ID3DBlob* pGSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "VolSliceNorGS", "gs_5_0", COMPILE_FLAG, 0, &pGSBlob));
		V_RETURN(pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), NULL, &m_pVolSliceNorGS));
		DXUT_SetDebugName(m_pVolSliceNorGS, "m_pVolSliceNorGS");
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "VolSliceGS", "gs_5_0", COMPILE_FLAG, 0, &pGSBlob));
		V_RETURN(pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), NULL, &m_pVolSliceGS));
		DXUT_SetDebugName(m_pVolSliceGS, "m_pVolSliceGS");
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "TraversalGS", "gs_5_0", COMPILE_FLAG, 0, &pGSBlob));
		V_RETURN(pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(), NULL, &m_pTraversalGS));
		DXUT_SetDebugName(m_pTraversalGS, "m_pTraversalGS");
		// Streamoutput GS
		D3D11_SO_DECLARATION_ENTRY outputstreamLayout[] = {
				{ 0, "POSITION", 0, 0, 3, 0},
				{ 0, "COLOR", 0, 0, 3, 0},
		};
		UINT stride = 6*sizeof(float);
		UINT elems = sizeof(outputstreamLayout) / sizeof(D3D11_SO_DECLARATION_ENTRY);
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "TraversalAndOutGS", "gs_5_0", COMPILE_FLAG, 0, &pGSBlob));
		V_RETURN(pd3dDevice->CreateGeometryShaderWithStreamOutput(pGSBlob->GetBufferPointer(), 
			pGSBlob->GetBufferSize(), outputstreamLayout, elems, &stride, 1,
			D3D11_SO_NO_RASTERIZED_STREAM, NULL, &m_pTraversalAndOutGS));
		DXUT_SetDebugName(m_pTraversalAndOutGS, "m_pTraversalAndOutGSGS");
		pGSBlob->Release();

		ID3DBlob* pPSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "HPMCBasePS", "ps_5_0", COMPILE_FLAG, 0, &pPSBlob));
		V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pHPMCBasePS));
		DXUT_SetDebugName(m_pHPMCBasePS, "m_pHPMCBasePS");
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "ReductionPS", "ps_5_0", COMPILE_FLAG, 0, &pPSBlob));
		V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pReductionPS));
		DXUT_SetDebugName(m_pReductionPS, "m_pReductionPS");
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "ReductionBasePS", "ps_5_0", COMPILE_FLAG, 0, &pPSBlob));
		V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pReductionBasePS));
		DXUT_SetDebugName(m_pReductionBasePS, "m_pReductionBasePS");
		V_RETURN(DXUTCompileFromFile(L"HistoPyramidMC.fx", nullptr, "RenderPS", "ps_5_0", COMPILE_FLAG, 0, &pPSBlob));
		V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pRenderPS));
		DXUT_SetDebugName(m_pRenderPS, "m_pRenderPS");
		pPSBlob->Release();

		D3D11_INPUT_ELEMENT_DESC inputLayout[] =
		{ { "POSITION", 0, DXGI_FORMAT_R16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 } };
		V_RETURN(pd3dDevice->CreateInputLayout(inputLayout, ARRAYSIZE(inputLayout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pPassVL));
		DXUT_SetDebugName(m_pPassVL, "m_pPassVL");
		pVSBlob->Release();

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(short);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		V_RETURN(pd3dDevice->CreateBuffer(&bd, NULL, &m_pPassVB));
		DXUT_SetDebugName(m_pPassVB, "m_pPassVB");

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = m_uOutVBsize;
		bd.BindFlags = D3D11_BIND_STREAM_OUTPUT;
		bd.CPUAccessFlags = 0;
		V_RETURN(pd3dDevice->CreateBuffer(&bd, NULL, &m_pOutVB));
		DXUT_SetDebugName(m_pOutVB, "m_pOutVB");

		bd.Usage = D3D11_USAGE_STAGING;
		bd.ByteWidth = m_uOutVBsize;
		bd.BindFlags = 0;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		V_RETURN(pd3dDevice->CreateBuffer(&bd, NULL, &m_pOutVBCPU));
		DXUT_SetDebugName(m_pOutVBCPU, "m_pOutVBCPU");

		// Create the constant buffers
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.ByteWidth = sizeof(CB_HPMC_Init);
		V_RETURN(pd3dDevice->CreateBuffer(&bd, NULL, &m_pCB_HPMC_Init));
		DXUT_SetDebugName(m_pCB_HPMC_Init, "m_pCB_HPMC_Init");
		bd.ByteWidth = sizeof(CB_HPMC_Frame);
		V_RETURN(pd3dDevice->CreateBuffer(&bd, NULL, &m_pCB_HPMC_Frame));
		DXUT_SetDebugName(m_pCB_HPMC_Frame, "m_pCB_HPMC_Frame");
		bd.ByteWidth = sizeof(CB_HPMC_Reduct);
		V_RETURN(pd3dDevice->CreateBuffer(&bd, NULL, &m_pCB_HPMC_Reduct));
		DXUT_SetDebugName(m_pCB_HPMC_Reduct, "m_pCB_HPMC_Reduct");

		// Create output texture resource
		D3D11_TEXTURE2D_DESC	RTtextureDesc = { 0 };
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
		DXUT_SetDebugName(m_pOutTex, "m_pOutTex");

		D3D11_RENDER_TARGET_VIEW_DESC		RTViewDesc;
		ZeroMemory(&RTViewDesc, sizeof(RTViewDesc));
		RTViewDesc.Format = RTtextureDesc.Format;
		RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTViewDesc.Texture2D.MipSlice = 0;
		V_RETURN(pd3dDevice->CreateRenderTargetView(m_pOutTex, &RTViewDesc, &m_pOutRTV));
		DXUT_SetDebugName(m_pOutRTV, "m_pOutRTV");

		D3D11_SHADER_RESOURCE_VIEW_DESC		SRViewDesc;
		ZeroMemory(&SRViewDesc, sizeof(SRViewDesc));
		SRViewDesc.Format = RTtextureDesc.Format;
		SRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRViewDesc.Texture2D.MostDetailedMip = 0;
		SRViewDesc.Texture2D.MipLevels = 1;
		V_RETURN(pd3dDevice->CreateShaderResourceView(m_pOutTex, &SRViewDesc, &m_pOutSRV));
		DXUT_SetDebugName(m_pOutSRV, "m_pOutSRV");

		// Create depth stencil resource
		D3D11_TEXTURE2D_DESC descDepth;
		ZeroMemory(&descDepth, sizeof(descDepth));
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
		V_RETURN(pd3dDevice->CreateTexture2D(&descDepth, NULL, &m_pOutDSTex));
		DXUT_SetDebugName(m_pOutDSTex, "m_pOutDSTex");

		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		ZeroMemory(&descDSV, sizeof(descDSV));
		descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;
		V_RETURN(pd3dDevice->CreateDepthStencilView(m_pOutDSTex, &descDSV, &m_pOutDSSView));
		DXUT_SetDebugName(m_pOutDSSView, "m_pOutDSSView");

		// Create depth stencil state
		D3D11_DEPTH_STENCIL_DESC dsDesc;
		ZeroMemory(&dsDesc, sizeof(dsDesc));
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
		V_RETURN(pd3dDevice->CreateDepthStencilState(&dsDesc, &m_pOutDSState));
		DXUT_SetDebugName(m_pOutDSState, "m_pOutDSState");

		// Create the sample state
		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		V_RETURN(pd3dDevice->CreateSamplerState(&sampDesc, &m_pSS_Linear));
		DXUT_SetDebugName(m_pSS_Linear, "m_pSS_Linear");

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
		DXUT_SetDebugName(m_pOutRS, "m_pOutRS");

		// Create resource for histoPyramid
		char temp[100];
		D3D11_TEXTURE3D_DESC TEXDesc;
		ZeroMemory(&TEXDesc, sizeof(TEXDesc));
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		ZeroMemory(&SRVDesc, sizeof(SRVDesc));
		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		ZeroMemory(&RTVDesc, sizeof(RTVDesc));
		for (int i = 0; i < func_<VOXEL_NUM_X>::value; ++i){
			TEXDesc.Width = ceil((float)VOXEL_NUM_X / pow(2, i));
			TEXDesc.Height = ceil((float)VOXEL_NUM_Y / pow(2, i));
			TEXDesc.Depth = ceil((float)VOXEL_NUM_Z / pow(2, i));
			TEXDesc.MipLevels = 1;
			TEXDesc.Format = DXGI_FORMAT_R32_UINT;
			TEXDesc.Usage = D3D11_USAGE_DEFAULT;
			TEXDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
			TEXDesc.CPUAccessFlags = 0;
			TEXDesc.MiscFlags = 0;
			V_RETURN(pd3dDevice->CreateTexture3D(&TEXDesc, NULL, &m_pHistoPyramidTex[i]));
			sprintf_s(temp, "m_pHistoPyramidTex[%d]", i);
			DXUT_SetDebugName(m_pHistoPyramidTex[i], temp);

			SRVDesc.Format = TEXDesc.Format;
			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			SRVDesc.Texture3D.MostDetailedMip = 0;
			SRVDesc.Texture3D.MipLevels = 1;
			V_RETURN(pd3dDevice->CreateShaderResourceView(m_pHistoPyramidTex[i], &SRVDesc, &m_pHistoPyramidSRV[i]));
			//V_RETURN( pd3dDevice->CreateShaderResourceView( m_pHistoPyramidTex[i],0,&m_pHistoPyramidSRV[i]));
			sprintf_s(temp, "m_pHistoPyramidSRV[%d]", i);
			DXUT_SetDebugName(m_pHistoPyramidSRV[i], temp);
			m_pNullSRV[i] = NULL;

			RTVDesc.Format = TEXDesc.Format;
			RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
			RTVDesc.Texture3D.FirstWSlice = 0;
			RTVDesc.Texture3D.MipSlice = 0;
			RTVDesc.Texture3D.WSize = ceil((float)VOXEL_NUM_Z / pow(2, i));
			V_RETURN(pd3dDevice->CreateRenderTargetView(m_pHistoPyramidTex[i], &RTVDesc, &m_pHistoPyramidRTV[i]));
			//V_RETURN( pd3dDevice->CreateRenderTargetView( m_pHistoPyramidTex[i],0,&m_pHistoPyramidRTV[i]));
			sprintf_s(temp, "m_pHistoPyramidRTV[%d]", i);
			DXUT_SetDebugName(m_pHistoPyramidRTV[i], temp);
		}
		m_pNullSRV[func_<VOXEL_NUM_X, 1>::value] = NULL;
		m_pNullSRV[func_<VOXEL_NUM_X, 2>::value] = NULL;

		// Create texture for CPU to read back # of active cell;
		TEXDesc.Usage = D3D11_USAGE_STAGING;
		TEXDesc.BindFlags = 0;
		TEXDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		V_RETURN(pd3dDevice->CreateTexture3D(&TEXDesc, NULL, &m_pHPTopTex));

		m_Viewport.Width = (float)m_uRTwidth;
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
		float fAspectRatio = m_uRTwidth / (FLOAT)m_uRTheight;
		m_Camera.SetProjParams(XM_PI / 4, fAspectRatio, 0.01f, 500.0f);
		m_Camera.SetWindow(m_uRTwidth, m_uRTheight);
		m_Camera.SetButtonMasks(MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON);
	}

	void Release()
	{
		SAFE_RELEASE(m_pPassVS);
		SAFE_RELEASE(m_pVolSliceNorGS);
		SAFE_RELEASE(m_pVolSliceGS);
		SAFE_RELEASE(m_pTraversalGS);
		SAFE_RELEASE(m_pHPMCBasePS);
		SAFE_RELEASE(m_pReductionBasePS);
		SAFE_RELEASE(m_pReductionPS);
		SAFE_RELEASE(m_pRenderPS);
		SAFE_RELEASE(m_pPassVL);
		SAFE_RELEASE(m_pPassVB);

		SAFE_RELEASE(m_pSS_Linear);

		SAFE_RELEASE(m_pOutTex);
		SAFE_RELEASE(m_pOutSRV);
		SAFE_RELEASE(m_pOutRTV);
		SAFE_RELEASE(m_pOutRS);

		SAFE_RELEASE(m_pOutDSTex);
		SAFE_RELEASE(m_pOutDSSView);
		SAFE_RELEASE(m_pOutDSState);

		SAFE_RELEASE(m_pTraversalAndOutGS);
		SAFE_RELEASE(m_pOutVB);
		SAFE_RELEASE(m_pOutVBCPU);
		SAFE_RELEASE(m_pSOQuery);
		delete m_pVertex;

		for (int i = 0; i < func_<VOXEL_NUM_X>::value; ++i){
			SAFE_RELEASE(m_pHistoPyramidSRV[i]);
			SAFE_RELEASE(m_pHistoPyramidTex[i]);
			SAFE_RELEASE(m_pHistoPyramidRTV[i]);
		}
		SAFE_RELEASE(m_pHPTopTex);

		SAFE_RELEASE(m_pCB_HPMC_Frame);
		SAFE_RELEASE(m_pCB_HPMC_Init);
		SAFE_RELEASE(m_pCB_HPMC_Reduct);
	}

	void Update(float fElapsedTime)
	{
		m_Camera.FrameMove(fElapsedTime);
	}

	UINT BuildHP(ID3D11DeviceContext* pd3dImmediateContext)
	{
		//pd3dImmediateContext->VSSetShader(m_pPassVS, NULL,0);
		pd3dImmediateContext->OMSetRenderTargets(1, &m_pHistoPyramidRTV[0], NULL);
		pd3dImmediateContext->PSSetShaderResources(0, 1, &m_pVolSRV);
		pd3dImmediateContext->PSSetSamplers(0, 1, &m_pSS_Linear);
		pd3dImmediateContext->GSSetShader(m_pVolSliceNorGS, NULL, 0);
		pd3dImmediateContext->PSSetShader(m_pHPMCBasePS, NULL, 0);
		m_cbReduct.cb_i4RTReso.x = VOXEL_NUM_X;
		m_cbReduct.cb_i4RTReso.y = VOXEL_NUM_Y;
		m_cbReduct.cb_i4RTReso.z = VOXEL_NUM_Z;
		// Setup the viewport to match the backbuffer
		D3D11_VIEWPORT vp;
		vp.Width = m_cbReduct.cb_i4RTReso.x;
		vp.Height = m_cbReduct.cb_i4RTReso.y;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		pd3dImmediateContext->RSSetViewports(1, &vp);
		pd3dImmediateContext->UpdateSubresource(m_pCB_HPMC_Reduct, 0, NULL, &m_cbReduct, 0, 0);
		pd3dImmediateContext->Draw(VOXEL_NUM_Z, 0);

		pd3dImmediateContext->GSSetShader(m_pVolSliceGS, NULL, 0);
		for (int i = 1; i < func_<VOXEL_NUM_X>::value; ++i){
			m_cbReduct.cb_i4RTReso.x = ceil((float)VOXEL_NUM_X / pow(2, i));
			m_cbReduct.cb_i4RTReso.y = ceil((float)VOXEL_NUM_Y / pow(2, i));
			m_cbReduct.cb_i4RTReso.z = ceil((float)VOXEL_NUM_Z / pow(2, i));
			vp.Width = m_cbReduct.cb_i4RTReso.x;
			vp.Height = m_cbReduct.cb_i4RTReso.y;
			pd3dImmediateContext->RSSetViewports(1, &vp);
			pd3dImmediateContext->UpdateSubresource(m_pCB_HPMC_Reduct, 0, NULL, &m_cbReduct, 0, 0);
			pd3dImmediateContext->OMSetRenderTargets(1, &m_pHistoPyramidRTV[i], NULL);
			pd3dImmediateContext->PSSetShaderResources(1, 1, &m_pHistoPyramidSRV[i - 1]);
			if (i == 1)
				pd3dImmediateContext->PSSetShader(m_pReductionBasePS, NULL, 0);
			else
				pd3dImmediateContext->PSSetShader(m_pReductionPS, NULL, 0);
			pd3dImmediateContext->Draw(m_cbReduct.cb_i4RTReso.z, 0);
		}

		pd3dImmediateContext->CopyResource(m_pHPTopTex, m_pHistoPyramidTex[func_<VOXEL_NUM_X>::value - 1]);
		D3D11_MAPPED_SUBRESOURCE subresource;
		pd3dImmediateContext->Map(m_pHPTopTex, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_READ, 0, &subresource);
		UINT num = *reinterpret_cast<UINT*>(subresource.pData);
		pd3dImmediateContext->Unmap(m_pHPTopTex, D3D11CalcSubresource(0, 0, 1));
		return num;
	}

	void Render(ID3D11DeviceContext* pd3dImmediateContext)
	{
		pd3dImmediateContext->IASetInputLayout(m_pPassVL);
		UINT stride = sizeof(short);
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pPassVB, &stride, &offset);
		pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		pd3dImmediateContext->GSSetSamplers(0, 1, &m_pSS_Linear);
		pd3dImmediateContext->VSSetShader(m_pPassVS, NULL, 0);
		pd3dImmediateContext->UpdateSubresource(m_pCB_HPMC_Init, 0, NULL, &m_cbInit, 0, 0);
		pd3dImmediateContext->GSSetConstantBuffers(0, 1, &m_pCB_HPMC_Init);
		pd3dImmediateContext->GSSetConstantBuffers(1, 1, &m_pCB_HPMC_Frame);
		pd3dImmediateContext->GSSetConstantBuffers(2, 1, &m_pCB_HPMC_Reduct);
		pd3dImmediateContext->PSSetConstantBuffers(0, 1, &m_pCB_HPMC_Init);
		pd3dImmediateContext->PSSetConstantBuffers(1, 1, &m_pCB_HPMC_Frame);
		pd3dImmediateContext->PSSetConstantBuffers(2, 1, &m_pCB_HPMC_Reduct);

		UINT activeCellNum = BuildHP(pd3dImmediateContext);

		pd3dImmediateContext->OMSetRenderTargets(1, &m_pOutRTV, m_pOutDSSView);
		pd3dImmediateContext->GSSetShaderResources(2, func_<VOXEL_NUM_X>::value, m_pHistoPyramidSRV);
		pd3dImmediateContext->GSSetShaderResources(0, 1, &m_pVolSRV);

		XMMATRIX m_Proj = m_Camera.GetProjMatrix();
		XMMATRIX m_View = m_Camera.GetViewMatrix();
		XMMATRIX m_World = m_Camera.GetWorldMatrix();
		XMMATRIX m_ViewProjection = m_View*m_Proj;

		XMVECTOR t;

		m_cbPerFrame.cb_mWorldViewProj = XMMatrixTranspose(m_ViewProjection);
		m_cbPerFrame.cb_mWorld = XMMatrixTranspose(m_World);
		XMStoreFloat4(&m_cbPerFrame.cb_f4ViewPos, m_Camera.GetEyePt());
		pd3dImmediateContext->UpdateSubresource(m_pCB_HPMC_Frame, 0, NULL, &m_cbPerFrame, 0, 0);

		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		pd3dImmediateContext->ClearRenderTargetView(m_pOutRTV, ClearColor);
		pd3dImmediateContext->ClearDepthStencilView(m_pOutDSSView, D3D11_CLEAR_DEPTH, 1.0, 0);

		//pd3dImmediateContext->OMSetRenderTargets(1,&m_pOutputRTV, NULL);
		pd3dImmediateContext->OMSetDepthStencilState(m_pOutDSState, 1);
		pd3dImmediateContext->RSSetViewports(1, &m_Viewport);
		pd3dImmediateContext->GSSetShader(m_pTraversalGS, NULL, 0);
		pd3dImmediateContext->PSSetShader(m_pRenderPS, NULL, 0);
		if (m_bFramewire){
			ID3D11RasterizerState* rs;
			pd3dImmediateContext->RSGetState(&rs);
			pd3dImmediateContext->RSSetState(m_pOutRS);

			pd3dImmediateContext->Draw(activeCellNum, 0);
			pd3dImmediateContext->RSSetState(rs);
			SAFE_RELEASE(rs);
		} else{
			pd3dImmediateContext->Draw(activeCellNum, 0);
			//pd3dImmediateContext->Draw(m_cbPerFrame.cubeInfo.x * m_cbPerFrame.cubeInfo.y * m_cbPerFrame.cubeInfo.z, 0);
		}
		if(m_bOutputMesh && !m_bOutputInProgress){
			m_bOutputMesh = false;
			m_bOutputInProgress = true;
			pd3dImmediateContext->GSSetShader(m_pTraversalAndOutGS,NULL,0);
			pd3dImmediateContext->PSSetShader(NULL,NULL,0);
			UINT offset[1] = {0};
			pd3dImmediateContext->SOSetTargets(1, &m_pOutVB,offset);
			pd3dImmediateContext->Begin(m_pSOQuery);
			pd3dImmediateContext->Draw(activeCellNum,0);
			pd3dImmediateContext->End(m_pSOQuery);
			pd3dImmediateContext->CopyResource(m_pOutVBCPU, m_pOutVB);
			
			while( S_OK != pd3dImmediateContext->GetData(m_pSOQuery, &m_u64SOOutput, 2*sizeof(UINT64), 0)){};

			D3D11_MAPPED_SUBRESOURCE subresource;
			pd3dImmediateContext->Map(m_pOutVBCPU, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_READ, 0, &subresource);
			float* data = reinterpret_cast<float*>(subresource.pData);
			m_uVertexCount = m_u64SOOutput[0]*3;
			if(m_uVertexCount*6*sizeof(float) > m_uOutVBsize)
				m_uVertexCount = m_uOutVBsize / sizeof(float) / 6;
			memcpy(m_pVertex, data, m_uVertexCount * 6 * sizeof(float));
			pd3dImmediateContext->Unmap(m_pOutVBCPU, D3D11CalcSubresource(0, 0, 1));
			OutputMesh();
		}
		pd3dImmediateContext->GSSetShaderResources(0, 2 + func_<VOXEL_NUM_X>::value, m_pNullSRV);
		pd3dImmediateContext->PSSetShaderResources(0, 2 + func_<VOXEL_NUM_X>::value, m_pNullSRV);
	}

	LRESULT HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

		switch (uMsg)
		{
		case WM_KEYDOWN:
			int nKey = static_cast<int>(wParam);
			if (nKey == 'F')
			{
				m_bFramewire = !m_bFramewire;
			}
			if (nKey == 'C')
			{
				m_bOutputMesh = true;
			}
			break;
		}

		return 0;
	}
};