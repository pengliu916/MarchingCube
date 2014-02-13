#define MAX_BALLS 150

//--------------------------------------------------------------------------------------
// Buffers
//--------------------------------------------------------------------------------------
cbuffer volumeInfo : register( b0 )
{
	int3 voxelResolution;
	float voxelSize;
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
	float3  Coord : TEXCOORD0;
	uint	PrimID :SV_RenderTargetArrayIndex;
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
void GS(point GS_INPUT particles[1], uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> triStream)
{
	PS_INPUT output;
	output.Pos=float4(-1.0f,-1.0f,0.0f,1.0f);
	output.Coord=float3(-voxelResolution.x/2.0,voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5) * voxelSize;
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(-1.0f,1.0f,0.0f,1.0f);
	output.Coord=float3(-voxelResolution.x/2.0,-voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5) * voxelSize;
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(1.0f,-1.0f,0.0f,1.0f);
	output.Coord=float3(voxelResolution.x/2.0,voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5) * voxelSize;
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(1.0f,1.0f,0.0f,1.0f);
	output.Coord=float3(voxelResolution.x/2.0,-voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5) * voxelSize;
	output.PrimID=primID;
	triStream.Append(output);
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input ) : SV_Target
{
	float4 field = float4(0,1,1,1);
	float3 currentPos = input.Coord;
	for( uint i = 0; i < (uint)numOfBalls; i++ ){
		float density =  Ball( currentPos, balls[i].xyz, balls[i].w  );
		field.x += density;
		field.yzw += ballsCol[i].xyz * pow( density, 3 ) * 1000;
	}
	field.yzw = normalize( field.yzw );
	return field;
}