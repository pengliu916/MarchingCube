SamplerState g_sampler : register(s0);
Texture3D g_txVolume : register(t0);

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
cbuffer cubeInfo : register( b0 )
{
	float4 cb_f4CubeInfo;// # of cubes along x,y,z in .xyz conponents, .w is cube size
	float4 cb_f4VolSize; // size of volume in object space;isolevel in .w conponent
	matrix cb_mWorldViewProj;
	matrix cb_mInvView;
};

cbuffer cbImmutable
{
	static const float3 positions[4] =
	{
		float3( -1, 1, 0 ),
		float3( 1, 1, 0 ),
		float3( -1, -1, 0 ),
		float3( 1, -1, 0 ),
	};
	static const float2 texcoords[4] = 
	{ 
		float2(0,1), 
		float2(1,1),
		float2(0,0),
		float2(1,0),
	};
};

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct GS_INPUT
{
};

struct PS_INPUT
{
	float4	Pos : SV_POSITION;
	//float4	Pos : TEXCOORD0;
	float4	Col : COLOR0;
};

//--------------------------------------------------------------------------------------
// Utility Funcs
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
GS_INPUT VS( )
{
	GS_INPUT output = (GS_INPUT)0;

	return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------
/*GS for rendering the volume on screen ------------texVolume Read, no half pixel correction*/
[maxvertexcount(4)]
void GS(point GS_INPUT particles[1], uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> triStream)
{
	PS_INPUT output;
	float3 voxelResolution = cb_f4CubeInfo.xyz;
	float voxelSize = cb_f4CubeInfo.w;
	float3 currentIdx;
	currentIdx.z = primID / (uint)(cb_f4CubeInfo.x * cb_f4CubeInfo.y);
	currentIdx.y = primID % (uint)(cb_f4CubeInfo.x * cb_f4CubeInfo.y) / (uint)cb_f4CubeInfo.x;
	currentIdx.x = primID % (uint)(cb_f4CubeInfo.x * cb_f4CubeInfo.y) % (uint)cb_f4CubeInfo.x;
	float3 pos = (currentIdx - 0.5 * cb_f4CubeInfo.xyz) * cb_f4CubeInfo.w;// Convert to object space
	float3 volTexCoord = pos / cb_f4VolSize.xyz + 0.5;// Convert to volume texture space [0,0,0]-[1,1,1]
	//float3 halfCube = 0;
	float3 halfCube = 0.5f * cb_f4CubeInfo.w / cb_f4VolSize.xzy;
	float4 p0 = g_txVolume.SampleLevel(g_sampler, volTexCoord + halfCube * float3(-1, -1, -1), 0);
	float4 p1 = g_txVolume.SampleLevel(g_sampler, volTexCoord + halfCube * float3(-1, 1, -1), 0);
	float4 p2 = g_txVolume.SampleLevel(g_sampler, volTexCoord + halfCube * float3(1, 1, -1), 0);
	float4 p3 = g_txVolume.SampleLevel(g_sampler, volTexCoord + halfCube * float3(1, -1, -1), 0);
	float4 p4 = g_txVolume.SampleLevel(g_sampler, volTexCoord + halfCube * float3(-1, -1, 1), 0);
	float4 p5 = g_txVolume.SampleLevel(g_sampler, volTexCoord + halfCube * float3(-1, 1, 1), 0);
	float4 p6 = g_txVolume.SampleLevel(g_sampler, volTexCoord + halfCube * float3(1, 1, 1), 0);
	float4 p7 = g_txVolume.SampleLevel(g_sampler, volTexCoord + halfCube * float3(1, -1, 1), 0);

	uint containSurf = uint(p0.x > cb_f4VolSize.w) + uint(p1.x > cb_f4VolSize.w) + 
						uint(p2.x > cb_f4VolSize.w) + uint(p3.x > cb_f4VolSize.w) + 
						uint(p4.x > cb_f4VolSize.w) + uint(p5.x > cb_f4VolSize.w) + 
						uint(p6.x > cb_f4VolSize.w) + uint(p7.x > cb_f4VolSize.w);

	if(containSurf == 0 ||containSurf == 8 ) return;

	float4 centralPos = float4(pos,1);
	//float4 centralPos = float4(currentIdx - 0.5f, 1) * cb_f4VolSize;
	centralPos.w = 1;

	float4 color = (p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) / 8.0f;
	color.rgb = color.yzw;

	for( int i = 0; i < 4; i++ ){
		float3 position = positions[i] * cb_f4CubeInfo.w*0.2;
		position = mul( position, (float3x3)cb_mInvView ) + centralPos;
		output.Pos = mul( float4( position, 1.0 ), cb_mWorldViewProj );

		output.Col = color;
		triStream.Append( output );
	}
	triStream.RestartStrip();
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
	return input.Col;
}
