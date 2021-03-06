#include "Header.h"
SamplerState g_sampler : register(s0);
#if FLAT3D
Texture2D g_txVolume : register(t0);
#else
Texture3D g_txVolume : register(t0);
#endif

static const float3 aLight_col = float3( 0.01, 0.01, 0.01 );
static const float3 dLight_col = float3( 0.02, 0.02, 0.02 );
static const float3 dLight_dir = normalize( float3( -1, -1, 1 ));
static const float3 pLight_pos = float3( 1, 1, -1 );
static const float3 pLight_col = float3( 1, 1, 1 )*0.1;
//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
cbuffer cubeInfo : register( b0 )
{
	float4 cb_f4CubeInfo;// # of cubes along x,y,z in .xyz conponents, .w is cube size
	float4 cb_f4VolSize; // size of volume in object space;isolevel in .w conponent
	float4 cb_f4ViewPos;
	matrix cb_mWorldViewProj;
#if FLAT3D
	float4 voxelInfo;// information about the volume texture
	int2 tile_num;//# of tile in x,y dimension of the flat3D texture
#endif
};

cbuffer cbImmutable
{
	static const float3 cb_quadPos[4] =
	{
		float3( -1, 1, 0 ),
		float3( 1, 1, 0 ),
		float3( -1, -1, 0 ),
		float3( 1, -1, 0 ),
	};
	static const float2 cb_quadTex[4] = 
	{ 
		float2(0,1), 
		float2(1,1),
		float2(0,0),
		float2(1,0),
	};
	static const float3 cb_halfCubeOffset[8] = 
	{
		float3( -1, -1, -1 ),
		float3( -1,  1, -1 ),
		float3(  1,  1, -1 ),
		float3(  1, -1, -1 ),
		float3( -1, -1,  1 ),
		float3( -1,  1,  1 ),
		float3(  1,  1,  1 ),
		float3(  1, -1,  1 )
	};
	// Return # of polygons, if given case number
	static const int cb_casePolyTable[256] = 
	{
		0, 1, 1, 2, 1, 2, 2, 3,  1, 2, 2, 3, 2, 3, 3, 2,  1, 2, 2, 3, 2, 3, 3, 4,  2, 3, 3, 4, 3, 4, 4, 3,  
        1, 2, 2, 3, 2, 3, 3, 4,  2, 3, 3, 4, 3, 4, 4, 3,  2, 3, 3, 2, 3, 4, 4, 3,  3, 4, 4, 3, 4, 5, 5, 2,  
        1, 2, 2, 3, 2, 3, 3, 4,  2, 3, 3, 4, 3, 4, 4, 3,  2, 3, 3, 4, 3, 4, 4, 5,  3, 4, 4, 5, 4, 5, 5, 4,  
        2, 3, 3, 4, 3, 4, 2, 3,  3, 4, 4, 5, 4, 5, 3, 2,  3, 4, 4, 3, 4, 5, 3, 2,  4, 5, 5, 4, 5, 2, 4, 1,  
        1, 2, 2, 3, 2, 3, 3, 4,  2, 3, 3, 4, 3, 4, 4, 3,  2, 3, 3, 4, 3, 4, 4, 5,  3, 2, 4, 3, 4, 3, 5, 2,  
        2, 3, 3, 4, 3, 4, 4, 5,  3, 4, 4, 5, 4, 5, 5, 4,  3, 4, 4, 3, 4, 5, 5, 4,  4, 3, 5, 2, 5, 4, 2, 1,  
        2, 3, 3, 4, 3, 4, 4, 5,  3, 4, 4, 5, 2, 3, 3, 2,  3, 4, 4, 5, 4, 5, 5, 2,  4, 3, 5, 4, 3, 2, 4, 1,  
        3, 4, 4, 5, 4, 5, 3, 4,  4, 5, 5, 2, 3, 4, 2, 1,  2, 3, 3, 2, 3, 4, 2, 1,  3, 2, 4, 1, 2, 1, 1, 0
	};
	// Return edge info for each vertex(5 at most), if given case number
	static const int3 cb_triTable[256][5] =
	{
	  { {-1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  8,  3 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  1,  9 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  8,  3 }, {  9,  8,  1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  2, 10 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  8,  3 }, {  1,  2, 10 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  2, 10 }, {  0,  2,  9 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 2,  8,  3 }, {  2, 10,  8 }, { 10,  9,  8 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3, 11,  2 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0, 11,  2 }, {  8, 11,  0 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  9,  0 }, {  2,  3, 11 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1, 11,  2 }, {  1,  9, 11 }, {  9,  8, 11 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3, 10,  1 }, { 11, 10,  3 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0, 10,  1 }, {  0,  8, 10 }, {  8, 11, 10 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  9,  0 }, {  3, 11,  9 }, { 11, 10,  9 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  8, 10 }, { 10,  8, 11 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  7,  8 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  3,  0 }, {  7,  3,  4 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  1,  9 }, {  8,  4,  7 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  1,  9 }, {  4,  7,  1 }, {  7,  3,  1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  2, 10 }, {  8,  4,  7 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  4,  7 }, {  3,  0,  4 }, {  1,  2, 10 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  2, 10 }, {  9,  0,  2 }, {  8,  4,  7 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 2, 10,  9 }, {  2,  9,  7 }, {  2,  7,  3 }, {  7,  9,  4 }, { -1, -1, -1 } },
	  { { 8,  4,  7 }, {  3, 11,  2 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {11,  4,  7 }, { 11,  2,  4 }, {  2,  0,  4 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  0,  1 }, {  8,  4,  7 }, {  2,  3, 11 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  7, 11 }, {  9,  4, 11 }, {  9, 11,  2 }, {  9,  2,  1 }, { -1, -1, -1 } },
	  { { 3, 10,  1 }, {  3, 11, 10 }, {  7,  8,  4 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1, 11, 10 }, {  1,  4, 11 }, {  1,  0,  4 }, {  7, 11,  4 }, { -1, -1, -1 } },
	  { { 4,  7,  8 }, {  9,  0, 11 }, {  9, 11, 10 }, { 11,  0,  3 }, { -1, -1, -1 } },
	  { { 4,  7, 11 }, {  4, 11,  9 }, {  9, 11, 10 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  5,  4 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  5,  4 }, {  0,  8,  3 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  5,  4 }, {  1,  5,  0 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 8,  5,  4 }, {  8,  3,  5 }, {  3,  1,  5 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  2, 10 }, {  9,  5,  4 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  0,  8 }, {  1,  2, 10 }, {  4,  9,  5 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 5,  2, 10 }, {  5,  4,  2 }, {  4,  0,  2 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 2, 10,  5 }, {  3,  2,  5 }, {  3,  5,  4 }, {  3,  4,  8 }, { -1, -1, -1 } },
	  { { 9,  5,  4 }, {  2,  3, 11 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0, 11,  2 }, {  0,  8, 11 }, {  4,  9,  5 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  5,  4 }, {  0,  1,  5 }, {  2,  3, 11 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 2,  1,  5 }, {  2,  5,  8 }, {  2,  8, 11 }, {  4,  8,  5 }, { -1, -1, -1 } },
	  { {10,  3, 11 }, { 10,  1,  3 }, {  9,  5,  4 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  9,  5 }, {  0,  8,  1 }, {  8, 10,  1 }, {  8, 11, 10 }, { -1, -1, -1 } },
	  { { 5,  4,  0 }, {  5,  0, 11 }, {  5, 11, 10 }, { 11,  0,  3 }, { -1, -1, -1 } },
	  { { 5,  4,  8 }, {  5,  8, 10 }, { 10,  8, 11 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  7,  8 }, {  5,  7,  9 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  3,  0 }, {  9,  5,  3 }, {  5,  7,  3 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  7,  8 }, {  0,  1,  7 }, {  1,  5,  7 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  5,  3 }, {  3,  5,  7 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  7,  8 }, {  9,  5,  7 }, { 10,  1,  2 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {10,  1,  2 }, {  9,  5,  0 }, {  5,  3,  0 }, {  5,  7,  3 }, { -1, -1, -1 } },
	  { { 8,  0,  2 }, {  8,  2,  5 }, {  8,  5,  7 }, { 10,  5,  2 }, { -1, -1, -1 } },
	  { { 2, 10,  5 }, {  2,  5,  3 }, {  3,  5,  7 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 7,  9,  5 }, {  7,  8,  9 }, {  3, 11,  2 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  5,  7 }, {  9,  7,  2 }, {  9,  2,  0 }, {  2,  7, 11 }, { -1, -1, -1 } },
	  { { 2,  3, 11 }, {  0,  1,  8 }, {  1,  7,  8 }, {  1,  5,  7 }, { -1, -1, -1 } },
	  { {11,  2,  1 }, { 11,  1,  7 }, {  7,  1,  5 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  5,  8 }, {  8,  5,  7 }, { 10,  1,  3 }, { 10,  3, 11 }, { -1, -1, -1 } },
	  { { 5,  7,  0 }, {  5,  0,  9 }, {  7, 11,  0 }, {  1,  0, 10 }, { 11, 10,  0 } },
	  { {11, 10,  0 }, { 11,  0,  3 }, { 10,  5,  0 }, {  8,  0,  7 }, {  5,  7,  0 } },
	  { {11, 10,  5 }, {  7, 11,  5 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {10,  6,  5 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  8,  3 }, {  5, 10,  6 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  0,  1 }, {  5, 10,  6 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  8,  3 }, {  1,  9,  8 }, {  5, 10,  6 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  6,  5 }, {  2,  6,  1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  6,  5 }, {  1,  2,  6 }, {  3,  0,  8 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  6,  5 }, {  9,  0,  6 }, {  0,  2,  6 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 5,  9,  8 }, {  5,  8,  2 }, {  5,  2,  6 }, {  3,  2,  8 }, { -1, -1, -1 } },
	  { { 2,  3, 11 }, { 10,  6,  5 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {11,  0,  8 }, { 11,  2,  0 }, { 10,  6,  5 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  1,  9 }, {  2,  3, 11 }, {  5, 10,  6 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 5, 10,  6 }, {  1,  9,  2 }, {  9, 11,  2 }, {  9,  8, 11 }, { -1, -1, -1 } },
	  { { 6,  3, 11 }, {  6,  5,  3 }, {  5,  1,  3 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  8, 11 }, {  0, 11,  5 }, {  0,  5,  1 }, {  5, 11,  6 }, { -1, -1, -1 } },
	  { { 3, 11,  6 }, {  0,  3,  6 }, {  0,  6,  5 }, {  0,  5,  9 }, { -1, -1, -1 } },
	  { { 6,  5,  9 }, {  6,  9, 11 }, { 11,  9,  8 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 5, 10,  6 }, {  4,  7,  8 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  3,  0 }, {  4,  7,  3 }, {  6,  5, 10 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  9,  0 }, {  5, 10,  6 }, {  8,  4,  7 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {10,  6,  5 }, {  1,  9,  7 }, {  1,  7,  3 }, {  7,  9,  4 }, { -1, -1, -1 } },
	  { { 6,  1,  2 }, {  6,  5,  1 }, {  4,  7,  8 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  2,  5 }, {  5,  2,  6 }, {  3,  0,  4 }, {  3,  4,  7 }, { -1, -1, -1 } },
	  { { 8,  4,  7 }, {  9,  0,  5 }, {  0,  6,  5 }, {  0,  2,  6 }, { -1, -1, -1 } },
	  { { 7,  3,  9 }, {  7,  9,  4 }, {  3,  2,  9 }, {  5,  9,  6 }, {  2,  6,  9 } },
	  { { 3, 11,  2 }, {  7,  8,  4 }, { 10,  6,  5 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 5, 10,  6 }, {  4,  7,  2 }, {  4,  2,  0 }, {  2,  7, 11 }, { -1, -1, -1 } },
	  { { 0,  1,  9 }, {  4,  7,  8 }, {  2,  3, 11 }, {  5, 10,  6 }, { -1, -1, -1 } },
	  { { 9,  2,  1 }, {  9, 11,  2 }, {  9,  4, 11 }, {  7, 11,  4 }, {  5, 10,  6 } },
	  { { 8,  4,  7 }, {  3, 11,  5 }, {  3,  5,  1 }, {  5, 11,  6 }, { -1, -1, -1 } },
	  { { 5,  1, 11 }, {  5, 11,  6 }, {  1,  0, 11 }, {  7, 11,  4 }, {  0,  4, 11 } },
	  { { 0,  5,  9 }, {  0,  6,  5 }, {  0,  3,  6 }, { 11,  6,  3 }, {  8,  4,  7 } },
	  { { 6,  5,  9 }, {  6,  9, 11 }, {  4,  7,  9 }, {  7, 11,  9 }, { -1, -1, -1 } },
	  { {10,  4,  9 }, {  6,  4, 10 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4, 10,  6 }, {  4,  9, 10 }, {  0,  8,  3 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {10,  0,  1 }, { 10,  6,  0 }, {  6,  4,  0 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 8,  3,  1 }, {  8,  1,  6 }, {  8,  6,  4 }, {  6,  1, 10 }, { -1, -1, -1 } },
	  { { 1,  4,  9 }, {  1,  2,  4 }, {  2,  6,  4 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  0,  8 }, {  1,  2,  9 }, {  2,  4,  9 }, {  2,  6,  4 }, { -1, -1, -1 } },
	  { { 0,  2,  4 }, {  4,  2,  6 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 8,  3,  2 }, {  8,  2,  4 }, {  4,  2,  6 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {10,  4,  9 }, { 10,  6,  4 }, { 11,  2,  3 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  8,  2 }, {  2,  8, 11 }, {  4,  9, 10 }, {  4, 10,  6 }, { -1, -1, -1 } },
	  { { 3, 11,  2 }, {  0,  1,  6 }, {  0,  6,  4 }, {  6,  1, 10 }, { -1, -1, -1 } },
	  { { 6,  4,  1 }, {  6,  1, 10 }, {  4,  8,  1 }, {  2,  1, 11 }, {  8, 11,  1 } },
	  { { 9,  6,  4 }, {  9,  3,  6 }, {  9,  1,  3 }, { 11,  6,  3 }, { -1, -1, -1 } },
	  { { 8, 11,  1 }, {  8,  1,  0 }, { 11,  6,  1 }, {  9,  1,  4 }, {  6,  4,  1 } },
	  { { 3, 11,  6 }, {  3,  6,  0 }, {  0,  6,  4 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 6,  4,  8 }, { 11,  6,  8 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 7, 10,  6 }, {  7,  8, 10 }, {  8,  9, 10 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  7,  3 }, {  0, 10,  7 }, {  0,  9, 10 }, {  6,  7, 10 }, { -1, -1, -1 } },
	  { {10,  6,  7 }, {  1, 10,  7 }, {  1,  7,  8 }, {  1,  8,  0 }, { -1, -1, -1 } },
	  { {10,  6,  7 }, { 10,  7,  1 }, {  1,  7,  3 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  2,  6 }, {  1,  6,  8 }, {  1,  8,  9 }, {  8,  6,  7 }, { -1, -1, -1 } },
	  { { 2,  6,  9 }, {  2,  9,  1 }, {  6,  7,  9 }, {  0,  9,  3 }, {  7,  3,  9 } },
	  { { 7,  8,  0 }, {  7,  0,  6 }, {  6,  0,  2 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 7,  3,  2 }, {  6,  7,  2 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 2,  3, 11 }, { 10,  6,  8 }, { 10,  8,  9 }, {  8,  6,  7 }, { -1, -1, -1 } },
	  { { 2,  0,  7 }, {  2,  7, 11 }, {  0,  9,  7 }, {  6,  7, 10 }, {  9, 10,  7 } },
	  { { 1,  8,  0 }, {  1,  7,  8 }, {  1, 10,  7 }, {  6,  7, 10 }, {  2,  3, 11 } },
	  { {11,  2,  1 }, { 11,  1,  7 }, { 10,  6,  1 }, {  6,  7,  1 }, { -1, -1, -1 } },
	  { { 8,  9,  6 }, {  8,  6,  7 }, {  9,  1,  6 }, { 11,  6,  3 }, {  1,  3,  6 } },
	  { { 0,  9,  1 }, { 11,  6,  7 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 7,  8,  0 }, {  7,  0,  6 }, {  3, 11,  0 }, { 11,  6,  0 }, { -1, -1, -1 } },
	  { { 7, 11,  6 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 7,  6, 11 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  0,  8 }, { 11,  7,  6 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  1,  9 }, { 11,  7,  6 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 8,  1,  9 }, {  8,  3,  1 }, { 11,  7,  6 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {10,  1,  2 }, {  6, 11,  7 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  2, 10 }, {  3,  0,  8 }, {  6, 11,  7 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 2,  9,  0 }, {  2, 10,  9 }, {  6, 11,  7 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 6, 11,  7 }, {  2, 10,  3 }, { 10,  8,  3 }, { 10,  9,  8 }, { -1, -1, -1 } },
	  { { 7,  2,  3 }, {  6,  2,  7 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 7,  0,  8 }, {  7,  6,  0 }, {  6,  2,  0 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 2,  7,  6 }, {  2,  3,  7 }, {  0,  1,  9 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  6,  2 }, {  1,  8,  6 }, {  1,  9,  8 }, {  8,  7,  6 }, { -1, -1, -1 } },
	  { {10,  7,  6 }, { 10,  1,  7 }, {  1,  3,  7 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {10,  7,  6 }, {  1,  7, 10 }, {  1,  8,  7 }, {  1,  0,  8 }, { -1, -1, -1 } },
	  { { 0,  3,  7 }, {  0,  7, 10 }, {  0, 10,  9 }, {  6, 10,  7 }, { -1, -1, -1 } },
	  { { 7,  6, 10 }, {  7, 10,  8 }, {  8, 10,  9 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 6,  8,  4 }, { 11,  8,  6 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  6, 11 }, {  3,  0,  6 }, {  0,  4,  6 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 8,  6, 11 }, {  8,  4,  6 }, {  9,  0,  1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  4,  6 }, {  9,  6,  3 }, {  9,  3,  1 }, { 11,  3,  6 }, { -1, -1, -1 } },
	  { { 6,  8,  4 }, {  6, 11,  8 }, {  2, 10,  1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  2, 10 }, {  3,  0, 11 }, {  0,  6, 11 }, {  0,  4,  6 }, { -1, -1, -1 } },
	  { { 4, 11,  8 }, {  4,  6, 11 }, {  0,  2,  9 }, {  2, 10,  9 }, { -1, -1, -1 } },
	  { {10,  9,  3 }, { 10,  3,  2 }, {  9,  4,  3 }, { 11,  3,  6 }, {  4,  6,  3 } },
	  { { 8,  2,  3 }, {  8,  4,  2 }, {  4,  6,  2 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  4,  2 }, {  4,  6,  2 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  9,  0 }, {  2,  3,  4 }, {  2,  4,  6 }, {  4,  3,  8 }, { -1, -1, -1 } },
	  { { 1,  9,  4 }, {  1,  4,  2 }, {  2,  4,  6 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 8,  1,  3 }, {  8,  6,  1 }, {  8,  4,  6 }, {  6, 10,  1 }, { -1, -1, -1 } },
	  { {10,  1,  0 }, { 10,  0,  6 }, {  6,  0,  4 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  6,  3 }, {  4,  3,  8 }, {  6, 10,  3 }, {  0,  3,  9 }, { 10,  9,  3 } },
	  { {10,  9,  4 }, {  6, 10,  4 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  9,  5 }, {  7,  6, 11 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  8,  3 }, {  4,  9,  5 }, { 11,  7,  6 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 5,  0,  1 }, {  5,  4,  0 }, {  7,  6, 11 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {11,  7,  6 }, {  8,  3,  4 }, {  3,  5,  4 }, {  3,  1,  5 }, { -1, -1, -1 } },
	  { { 9,  5,  4 }, { 10,  1,  2 }, {  7,  6, 11 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 6, 11,  7 }, {  1,  2, 10 }, {  0,  8,  3 }, {  4,  9,  5 }, { -1, -1, -1 } },
	  { { 7,  6, 11 }, {  5,  4, 10 }, {  4,  2, 10 }, {  4,  0,  2 }, { -1, -1, -1 } },
	  { { 3,  4,  8 }, {  3,  5,  4 }, {  3,  2,  5 }, { 10,  5,  2 }, { 11,  7,  6 } },
	  { { 7,  2,  3 }, {  7,  6,  2 }, {  5,  4,  9 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  5,  4 }, {  0,  8,  6 }, {  0,  6,  2 }, {  6,  8,  7 }, { -1, -1, -1 } },
	  { { 3,  6,  2 }, {  3,  7,  6 }, {  1,  5,  0 }, {  5,  4,  0 }, { -1, -1, -1 } },
	  { { 6,  2,  8 }, {  6,  8,  7 }, {  2,  1,  8 }, {  4,  8,  5 }, {  1,  5,  8 } },
	  { { 9,  5,  4 }, { 10,  1,  6 }, {  1,  7,  6 }, {  1,  3,  7 }, { -1, -1, -1 } },
	  { { 1,  6, 10 }, {  1,  7,  6 }, {  1,  0,  7 }, {  8,  7,  0 }, {  9,  5,  4 } },
	  { { 4,  0, 10 }, {  4, 10,  5 }, {  0,  3, 10 }, {  6, 10,  7 }, {  3,  7, 10 } },
	  { { 7,  6, 10 }, {  7, 10,  8 }, {  5,  4, 10 }, {  4,  8, 10 }, { -1, -1, -1 } },
	  { { 6,  9,  5 }, {  6, 11,  9 }, { 11,  8,  9 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  6, 11 }, {  0,  6,  3 }, {  0,  5,  6 }, {  0,  9,  5 }, { -1, -1, -1 } },
	  { { 0, 11,  8 }, {  0,  5, 11 }, {  0,  1,  5 }, {  5,  6, 11 }, { -1, -1, -1 } },
	  { { 6, 11,  3 }, {  6,  3,  5 }, {  5,  3,  1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  2, 10 }, {  9,  5, 11 }, {  9, 11,  8 }, { 11,  5,  6 }, { -1, -1, -1 } },
	  { { 0, 11,  3 }, {  0,  6, 11 }, {  0,  9,  6 }, {  5,  6,  9 }, {  1,  2, 10 } },
	  { {11,  8,  5 }, { 11,  5,  6 }, {  8,  0,  5 }, { 10,  5,  2 }, {  0,  2,  5 } },
	  { { 6, 11,  3 }, {  6,  3,  5 }, {  2, 10,  3 }, { 10,  5,  3 }, { -1, -1, -1 } },
	  { { 5,  8,  9 }, {  5,  2,  8 }, {  5,  6,  2 }, {  3,  8,  2 }, { -1, -1, -1 } },
	  { { 9,  5,  6 }, {  9,  6,  0 }, {  0,  6,  2 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  5,  8 }, {  1,  8,  0 }, {  5,  6,  8 }, {  3,  8,  2 }, {  6,  2,  8 } },
	  { { 1,  5,  6 }, {  2,  1,  6 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  3,  6 }, {  1,  6, 10 }, {  3,  8,  6 }, {  5,  6,  9 }, {  8,  9,  6 } },
	  { {10,  1,  0 }, { 10,  0,  6 }, {  9,  5,  0 }, {  5,  6,  0 }, { -1, -1, -1 } },
	  { { 0,  3,  8 }, {  5,  6, 10 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {10,  5,  6 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {11,  5, 10 }, {  7,  5, 11 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {11,  5, 10 }, { 11,  7,  5 }, {  8,  3,  0 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 5, 11,  7 }, {  5, 10, 11 }, {  1,  9,  0 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {10,  7,  5 }, { 10, 11,  7 }, {  9,  8,  1 }, {  8,  3,  1 }, { -1, -1, -1 } },
	  { {11,  1,  2 }, { 11,  7,  1 }, {  7,  5,  1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  8,  3 }, {  1,  2,  7 }, {  1,  7,  5 }, {  7,  2, 11 }, { -1, -1, -1 } },
	  { { 9,  7,  5 }, {  9,  2,  7 }, {  9,  0,  2 }, {  2, 11,  7 }, { -1, -1, -1 } },
	  { { 7,  5,  2 }, {  7,  2, 11 }, {  5,  9,  2 }, {  3,  2,  8 }, {  9,  8,  2 } },
	  { { 2,  5, 10 }, {  2,  3,  5 }, {  3,  7,  5 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 8,  2,  0 }, {  8,  5,  2 }, {  8,  7,  5 }, { 10,  2,  5 }, { -1, -1, -1 } },
	  { { 9,  0,  1 }, {  5, 10,  3 }, {  5,  3,  7 }, {  3, 10,  2 }, { -1, -1, -1 } },
	  { { 9,  8,  2 }, {  9,  2,  1 }, {  8,  7,  2 }, { 10,  2,  5 }, {  7,  5,  2 } },
	  { { 1,  3,  5 }, {  3,  7,  5 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  8,  7 }, {  0,  7,  1 }, {  1,  7,  5 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  0,  3 }, {  9,  3,  5 }, {  5,  3,  7 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9,  8,  7 }, {  5,  9,  7 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 5,  8,  4 }, {  5, 10,  8 }, { 10, 11,  8 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 5,  0,  4 }, {  5, 11,  0 }, {  5, 10, 11 }, { 11,  3,  0 }, { -1, -1, -1 } },
	  { { 0,  1,  9 }, {  8,  4, 10 }, {  8, 10, 11 }, { 10,  4,  5 }, { -1, -1, -1 } },
	  { {10, 11,  4 }, { 10,  4,  5 }, { 11,  3,  4 }, {  9,  4,  1 }, {  3,  1,  4 } },
	  { { 2,  5,  1 }, {  2,  8,  5 }, {  2, 11,  8 }, {  4,  5,  8 }, { -1, -1, -1 } },
	  { { 0,  4, 11 }, {  0, 11,  3 }, {  4,  5, 11 }, {  2, 11,  1 }, {  5,  1, 11 } },
	  { { 0,  2,  5 }, {  0,  5,  9 }, {  2, 11,  5 }, {  4,  5,  8 }, { 11,  8,  5 } },
	  { { 9,  4,  5 }, {  2, 11,  3 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 2,  5, 10 }, {  3,  5,  2 }, {  3,  4,  5 }, {  3,  8,  4 }, { -1, -1, -1 } },
	  { { 5, 10,  2 }, {  5,  2,  4 }, {  4,  2,  0 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3, 10,  2 }, {  3,  5, 10 }, {  3,  8,  5 }, {  4,  5,  8 }, {  0,  1,  9 } },
	  { { 5, 10,  2 }, {  5,  2,  4 }, {  1,  9,  2 }, {  9,  4,  2 }, { -1, -1, -1 } },
	  { { 8,  4,  5 }, {  8,  5,  3 }, {  3,  5,  1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  4,  5 }, {  1,  0,  5 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 8,  4,  5 }, {  8,  5,  3 }, {  9,  0,  5 }, {  0,  3,  5 }, { -1, -1, -1 } },
	  { { 9,  4,  5 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4, 11,  7 }, {  4,  9, 11 }, {  9, 10, 11 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  8,  3 }, {  4,  9,  7 }, {  9, 11,  7 }, {  9, 10, 11 }, { -1, -1, -1 } },
	  { { 1, 10, 11 }, {  1, 11,  4 }, {  1,  4,  0 }, {  7,  4, 11 }, { -1, -1, -1 } },
	  { { 3,  1,  4 }, {  3,  4,  8 }, {  1, 10,  4 }, {  7,  4, 11 }, { 10, 11,  4 } },
	  { { 4, 11,  7 }, {  9, 11,  4 }, {  9,  2, 11 }, {  9,  1,  2 }, { -1, -1, -1 } },
	  { { 9,  7,  4 }, {  9, 11,  7 }, {  9,  1, 11 }, {  2, 11,  1 }, {  0,  8,  3 } },
	  { {11,  7,  4 }, { 11,  4,  2 }, {  2,  4,  0 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {11,  7,  4 }, { 11,  4,  2 }, {  8,  3,  4 }, {  3,  2,  4 }, { -1, -1, -1 } },
	  { { 2,  9, 10 }, {  2,  7,  9 }, {  2,  3,  7 }, {  7,  4,  9 }, { -1, -1, -1 } },
	  { { 9, 10,  7 }, {  9,  7,  4 }, { 10,  2,  7 }, {  8,  7,  0 }, {  2,  0,  7 } },
	  { { 3,  7, 10 }, {  3, 10,  2 }, {  7,  4, 10 }, {  1, 10,  0 }, {  4,  0, 10 } },
	  { { 1, 10,  2 }, {  8,  7,  4 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  9,  1 }, {  4,  1,  7 }, {  7,  1,  3 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  9,  1 }, {  4,  1,  7 }, {  0,  8,  1 }, {  8,  7,  1 }, { -1, -1, -1 } },
	  { { 4,  0,  3 }, {  7,  4,  3 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 4,  8,  7 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9, 10,  8 }, { 10, 11,  8 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  0,  9 }, {  3,  9, 11 }, { 11,  9, 10 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  1, 10 }, {  0, 10,  8 }, {  8, 10, 11 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  1, 10 }, { 11,  3, 10 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  2, 11 }, {  1, 11,  9 }, {  9, 11,  8 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  0,  9 }, {  3,  9, 11 }, {  1,  2,  9 }, {  2, 11,  9 }, { -1, -1, -1 } },
	  { { 0,  2, 11 }, {  8,  0, 11 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 3,  2, 11 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 2,  3,  8 }, {  2,  8, 10 }, { 10,  8,  9 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 9, 10,  2 }, {  0,  9,  2 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 2,  3,  8 }, {  2,  8, 10 }, {  0,  1,  8 }, {  1, 10,  8 }, { -1, -1, -1 } },
	  { { 1, 10,  2 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 1,  3,  8 }, {  9,  1,  8 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  9,  1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { { 0,  3,  8 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } },
	  { {-1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 } }
	};
	// Return two endpoints idx, if given edge number
	static const int2 cb_edgeTable[12] =
	{
		{  0,  1 }, {  1,  2 }, {  2,  3 }, {  3,  0 },
		{  4,  5 }, {  5,  6 }, {  6,  7 }, {  7,  4 },
		{  0,  4 }, {  1,  5 }, {  2,  6 }, {  3,  7 }
	};
}

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct GS_INPUT
{
};

struct PS_INPUT
{
	float4	Pos : SV_POSITION;
	float3	Nor : NORMAL0;
	float4	Col : COLOR0;
	float4	Pos_o : NORMAl1;
};

struct VertexInfo
{
	float4 Field;
	float3 Pos;
	float3 Nor;
};
//--------------------------------------------------------------------------------------
// Utility Funcs
//--------------------------------------------------------------------------------------
PS_INPUT CalIntersectionVertex(VertexInfo Data0, VertexInfo Data1)
{
	PS_INPUT output;
	float t = (cb_f4VolSize.w - Data0.Field.x) / (Data1.Field.x - Data0.Field.x);
	output.Pos_o = float4(Data0.Pos + t * (Data1.Pos - Data0.Pos), 1);
	//output.Pos = float4((Data0.Pos + Data1.Pos)*0.5, 1);
	output.Pos = mul( output.Pos_o, cb_mWorldViewProj );
	output.Nor = normalize(Data0.Nor + t * (Data1.Nor - Data0.Nor));
	//output.Col = float4(1,1,1,1);
	output.Col = float4(Data0.Field.yzw + t * (Data1.Field.yzw - Data0.Field.yzw),1);
	return output;
}

#if FLAT3D
float4 samFlat3D( float3 P) // P is the 3D coordinate in local space
{
	float3 fVoxel_idx = P / voxelInfo.w + voxelInfo.xyz * 0.5 - float3(0,0,0.5);
	int z0_idx = floor(fVoxel_idx.z);
	int z1_idx = ceil(fVoxel_idx.z);
	int2 tile_idx = int2(z0_idx % tile_num.x, z0_idx / tile_num.y);
	float2 texCoord = (tile_idx * voxelInfo.xy + fVoxel_idx.xy) / (tile_num * voxelInfo.xy);
	float4 result0 = g_txVolume.SampleLevel(g_sampler, texCoord, 0);
	tile_idx = int2(z1_idx % tile_num.x, z1_idx / tile_num.y);
	texCoord = (tile_idx * voxelInfo.xy + fVoxel_idx.xy) / (tile_num * voxelInfo.xy);
	float4 result1 = g_txVolume.SampleLevel(g_sampler, texCoord, 0);
	float s = (fVoxel_idx.z - z0_idx) / (z1_idx - z0_idx);
	return lerp(result0,result1,s);
}
float3 CalNormal( float3 P)// P is the 3D coordinate in local space
{
	float depth_dx = samFlat3D(P+voxelInfo.w*float3 ( 1, 0, 0 )).x - samFlat3D(P+voxelInfo.w*float3 ( -1, 0, 0 )).x;
	float depth_dy = samFlat3D(P+voxelInfo.w*float3 ( 0, 1, 0 )).x - samFlat3D(P+voxelInfo.w*float3 ( 0, -1, 0 )).x;
	float depth_dz = samFlat3D(P+voxelInfo.w*float3 ( 0, 0, 1 )).x - samFlat3D(P+voxelInfo.w*float3 ( 0, 0, -1 )).x;
	return -normalize ( float3 ( depth_dx, depth_dy, depth_dz ) );
}
#else
float3 CalNormal( float3 txCoord )// Compute the normal from gradient
{
	float depth_dx = g_txVolume.SampleLevel( g_sampler, txCoord, 0, int3 ( 1, 0, 0 ) ).x -
		g_txVolume.SampleLevel( g_sampler, txCoord, 0, int3 ( -1, 0, 0 ) ).x;
	float depth_dy = g_txVolume.SampleLevel( g_sampler, txCoord, 0, int3 ( 0, 1, 0 ) ).x -
		g_txVolume.SampleLevel( g_sampler, txCoord, 0, int3 ( 0, -1, 0 ) ).x;
	float depth_dz = g_txVolume.SampleLevel( g_sampler, txCoord, 0, int3 ( 0, 0, 1 ) ).x -
		g_txVolume.SampleLevel( g_sampler, txCoord, 0, int3 ( 0, 0, -1 ) ).x;
	return -normalize( float3 ( depth_dx, depth_dy, depth_dz ) );

}
#endif
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
[maxvertexcount(15 )]
void GS(point GS_INPUT particles[1], uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> triStream)
{
	PS_INPUT output;
	float3 voxelResolution = cb_f4CubeInfo.xyz;
	float voxelSize = cb_f4CubeInfo.w;
	float3 currentIdx;
	currentIdx.z = primID / (uint)(cb_f4CubeInfo.x * cb_f4CubeInfo.y);
	currentIdx.y = (primID % (uint)(cb_f4CubeInfo.x * cb_f4CubeInfo.y)) / (uint)cb_f4CubeInfo.x;
	currentIdx.x = primID % (uint)(cb_f4CubeInfo.x * cb_f4CubeInfo.y) % (uint)cb_f4CubeInfo.x;
	float3 pos = (currentIdx - 0.5 * cb_f4CubeInfo.xyz) * cb_f4CubeInfo.w;// Convert to object space
	//float3 halfCube = 0;

	float4 fieldData[8];
	float3 fieldNormal[8];
#if FLAT3D
	[unroll] for( int i = 0; i < 8; ++i ){
		float3 P = pos + cb_f4CubeInfo.w * cb_halfCubeOffset[i]*0.5;
		fieldData[i] = samFlat3D(P);
		fieldNormal[i] = CalNormal( P );
	}
#else
	float3 volTexCoord = pos / cb_f4VolSize.xyz + 0.5;// Convert to volume texture space [0,0,0]-[1,1,1]
	float3 halfCube = 0.5f * cb_f4CubeInfo.w / cb_f4VolSize.xyz;
	[unroll] for( int i = 0; i < 8; ++i ){
		float3 idx = volTexCoord + halfCube * cb_halfCubeOffset[i];
		fieldData[i] = g_txVolume.SampleLevel(g_sampler, idx, 0);
		fieldNormal[i] = CalNormal( idx );
	}
#endif
	uint caseIdx =	(uint(fieldData[7].x > cb_f4VolSize.w) << 7) | (uint(fieldData[6].x > cb_f4VolSize.w) << 6) |
					(uint(fieldData[5].x > cb_f4VolSize.w) << 5) | (uint(fieldData[4].x > cb_f4VolSize.w) << 4) |
					(uint(fieldData[3].x > cb_f4VolSize.w) << 3) | (uint(fieldData[2].x > cb_f4VolSize.w) << 2) |
					(uint(fieldData[1].x > cb_f4VolSize.w) << 1) | (uint(fieldData[0].x > cb_f4VolSize.w));

	if(caseIdx == 0 ||caseIdx == 255 ) return;// if totally inside or outside surface, discard it
	//if(caseIdx !=15 && caseIdx != 240 /*&& caseIdx != 153 && caseIdx != 102*/) return;
	int polygonCount = cb_casePolyTable[caseIdx];// Find how many polygon need to be generated

	VertexInfo v0, v1;
	int3 edges;
	int2 endPoints;
	for( int i = 0; i < polygonCount; ++i ){
		edges = cb_triTable[caseIdx][i];
		endPoints = cb_edgeTable[edges.x];
		v0.Field = fieldData[endPoints.x];
		v0.Nor = fieldNormal[endPoints.x];
		v0.Pos = pos + cb_f4CubeInfo.w * 0.5f * cb_halfCubeOffset[endPoints.x];
		v1.Field = fieldData[endPoints.y];
		v1.Nor = fieldNormal[endPoints.y];
		v1.Pos = pos + cb_f4CubeInfo.w * 0.5f * cb_halfCubeOffset[endPoints.y];
		triStream.Append( CalIntersectionVertex( v0, v1 ));

		endPoints = cb_edgeTable[edges.z];
		v0.Field = fieldData[endPoints.x];
		v0.Nor = fieldNormal[endPoints.x];
		v0.Pos = pos + cb_f4CubeInfo.w * 0.5f * cb_halfCubeOffset[endPoints.x];
		v1.Field = fieldData[endPoints.y];
		v1.Nor = fieldNormal[endPoints.y];
		v1.Pos = pos + cb_f4CubeInfo.w * 0.5f * cb_halfCubeOffset[endPoints.y];
		triStream.Append( CalIntersectionVertex( v0, v1 ));

		endPoints = cb_edgeTable[edges.y];
		v0.Field = fieldData[endPoints.x];
		v0.Nor = fieldNormal[endPoints.x];
		v0.Pos = pos + cb_f4CubeInfo.w * 0.5f * cb_halfCubeOffset[endPoints.x];
		v1.Field = fieldData[endPoints.y];
		v1.Nor = fieldNormal[endPoints.y];
		v1.Pos = pos + cb_f4CubeInfo.w * 0.5f * cb_halfCubeOffset[endPoints.y];
		triStream.Append( CalIntersectionVertex( v0, v1 ));
		triStream.RestartStrip();
	}
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
	float3 color = input.Col.rgb;
	// shading part
	float3 ambientLight = aLight_col * color;

	float3 directionalLight = dLight_col * color * clamp( dot( input.Nor, dLight_dir ), 0, 1 );

	float3 vLight = pLight_pos - input.Pos_o.xyz;
	float3 halfVect = normalize( vLight - normalize(input.Pos_o.xyz - cb_f4ViewPos.xyz) );
	float dist = length( vLight ); vLight /= dist;
	float angleAttn = clamp( dot( input.Nor, vLight ), 0, 1 );
	float distAttn = 1.0f / ( dist * dist );
	float specularAttn = pow( clamp( dot( input.Nor, halfVect ), 0, 1 ), 128 );

	float3 pointLight = pLight_col * color * angleAttn + color * specularAttn;

	float3 col = ambientLight + directionalLight + pointLight;
	return float4( col, 0 );

}
