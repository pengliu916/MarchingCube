#include "Header.h"

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
cbuffer volumeInfo : register( b0 )
{
	int3 voxelResolution;
	float voxelSize;
#if FLAT3D
	int2 tile_num;
	int2 dummy;
#endif
};

cbuffer ballsInfo : register( b1 )
{
	float4 balls[MAX_BALLS];
	float4 ballsCol[MAX_BALLS];
	int numOfBalls;
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
#if FLAT3D
	float2  Coord : TEXCOORD0;
#else
	float3  Coord : TEXCOORD0;
	uint	PrimID :SV_RenderTargetArrayIndex;
#endif
};

float Ball(float3 Pos, float3 Center, float RadiusSq)
{
    float3 d = Pos - Center;
    float DistSq = dot(d, d);
    float InvDistSq = 1 / DistSq;
    return RadiusSq * InvDistSq;
}

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
[maxvertexcount(4)]
#if FLAT3D
void GS(point GS_INPUT particles[1], inout TriangleStream<PS_INPUT> triStream)
{
	PS_INPUT output;
	output.Pos=float4(-1.0f,1.0f,0.0f,1.0f);
	output.Coord=float2( 0.0f, 0.0f );
	triStream.Append(output);

	output.Pos=float4(-1.0f,-1.0f,0.0f,1.0f);
	output.Coord=float2( 0.0f, voxelResolution.y * tile_num.y);
	triStream.Append(output);

	output.Pos=float4(1.0f,1.0f,0.0f,1.0f);
	output.Coord=float2( voxelResolution.x * tile_num.x, 0.0f);
	triStream.Append(output);

	output.Pos=float4(1.0f,-1.0f,0.0f,1.0f);
	output.Coord=float2( voxelResolution.x * tile_num.x, voxelResolution.y * tile_num.y);
	triStream.Append(output);
}
#else
void GS(point GS_INPUT particles[1], uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> triStream)
{
	PS_INPUT output;
	output.Pos=float4(-1.0f,1.0f,0.0f,1.0f);
	output.Coord=float3(-voxelResolution.x/2.0,-voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5) * voxelSize;
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(-1.0f,-1.0f,0.0f,1.0f);
	output.Coord=float3(-voxelResolution.x/2.0,voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5) * voxelSize;
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(1.0f,1.0f,0.0f,1.0f);
	output.Coord=float3(voxelResolution.x/2.0,-voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5) * voxelSize;
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(1.0f,-1.0f,0.0f,1.0f);
	output.Coord=float3(voxelResolution.x/2.0,voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5) * voxelSize;
	output.PrimID=primID;
	triStream.Append(output);
}
#endif

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input ) : SV_Target
{
	float4 field = float4(0,1,1,1);
#if FLAT3D
	int3 voxel_idx;
	voxel_idx.xy = input.Coord.xy % voxelResolution.xy;
	int2 tile_idx = input.Coord / voxelResolution.xy;
	voxel_idx.z = tile_idx.y * tile_num.x + tile_idx.x;
	float3 currentPos = ( voxel_idx - voxelResolution / 2 ) * voxelSize;
	
#else
	float3 currentPos = input.Coord;
#endif
	//if (dot(currentPos, currentPos)<0.22 && dot(currentPos, currentPos)>0.1) field.x = 2;
	/*if (abs(currentPos.x)<0.4 && abs(currentPos.y)<0.4 && abs(currentPos.z)<0.4 &&
		abs(currentPos.x)>0.2 && abs(currentPos.y)>0.2 && abs(currentPos.z)>0.2) field.x = 2;*/
	if(abs(currentPos.x)+(currentPos.y)+abs(currentPos.z)<0.5) field.x=2;
	field.yzw = float3(1,1,1); 
	//field.y = (currentPos.y+1.f)/2.f;
	/*int slice = 48;
	int tt = currentPos.x*slice;
	tt=tt%2;
	int ff = currentPos.y*slice;
	ff = ff % 2;
	int kk = currentPos.z*slice;
	kk = kk % 2;
	if(tt) field.x=2;*/
	/*for( uint i = 0; i < (uint)numOfBalls; i++ ){
		float density =  Ball( currentPos, balls[i].xyz, balls[i].w  );
		field.x += density;
		field.yzw += ballsCol[i].xyz * pow( density, 3 ) * 1000;
	}
	field.yzw = normalize( field.yzw );*/
	return field;
}