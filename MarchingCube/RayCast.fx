#include "Header.h"

SamplerState samRaycast : register(s0);
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
cbuffer volumeInfo : register( b0 )
{
	float4 voxelInfo;
	float4 inverseXYZsize; // isolevel in .w conponent
	matrix WorldViewProjection;
	matrix invWorldView;
	float4 viewPos;
	float2 halfWinSize;
	int2 tile_num;
	float4 boxMin;
	float4 boxMax;
};

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct Ray
{
	float4 o;
	float4 d;
};

struct GS_INPUT
{
};

struct PS_INPUT
{
	float4	projPos : SV_POSITION;
	float4	Pos : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// Utility Funcs
//--------------------------------------------------------------------------------------
bool IntersectBox(Ray r, float3 boxmin, float3 boxmax, out float tnear, out float tfar)
{
	// compute intersection of ray with all six bbox planes
	float3 invR = 1.0 / r.d.xyz;
		float3 tbot = invR * (boxmin.xyz - r.o.xyz);
		float3 ttop = invR * (boxmax.xyz - r.o.xyz);

		// re-order intersections to find smallest and largest on each axis
		float3 tmin = min (ttop, tbot);
		float3 tmax = max (ttop, tbot);

		// find the largest tmin and the smallest tmax
		float2 t0 = max (tmin.xx, tmin.yz);
		tnear = max (t0.x, t0.y);
	t0 = min (tmax.xx, tmax.yz);
	tfar = min (t0.x, t0.y);

	return tnear<=tfar;
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
/*GS for rendering the volume on screen ------------texVolume Read, no half pixel correction*/
[maxvertexcount(4)]
void GS_Quad(point GS_INPUT particles[1], inout TriangleStream<PS_INPUT> triStream)
{
	PS_INPUT output;
	output.projPos=float4(-1.0f,1.0f,0.01f,1.0f);
	output.Pos=mul(float4(-halfWinSize.x, halfWinSize.y,1,1),invWorldView);
	triStream.Append(output);

	output.projPos=float4(1.0f,1.0f,0.01f,1.0f);
	output.Pos=mul(float4(halfWinSize.x, halfWinSize.y,1,1),invWorldView);
	triStream.Append(output);

	output.projPos=float4(-1.0f,-1.0f,0.01f,1.0f);
	output.Pos=mul(float4(-halfWinSize.x, -halfWinSize.y,1,1),invWorldView);
	triStream.Append(output);

	output.projPos=float4(1.0f,-1.0f,0.01f,1.0f);
	output.Pos=mul(float4(halfWinSize.x, -halfWinSize.y,1,1),invWorldView);
	triStream.Append(output);
}
#if FLAT3D
float2 local2tex( float3 P)
{
	int3 voxel_idx = P / voxelInfo.w + 0.5f + voxelInfo.xyz * 0.5;
	int2 tile_idx = int2(voxel_idx.z % tile_num.x, voxel_idx.z / tile_num.y);
	return (tile_idx * voxelInfo.xy + voxel_idx.xy)/(tile_num * voxelInfo.xy);
}
float4 samFlat3D( float3 P)
{
	float3 fVoxel_idx = P / voxelInfo.w + voxelInfo.xyz * 0.5 - float3(0,0,0.5);
	int z0_idx = floor(fVoxel_idx.z);
	int z1_idx = ceil(fVoxel_idx.z);
	int2 tile_idx = int2(z0_idx % tile_num.x, z0_idx / tile_num.y);
	float2 texCoord = (tile_idx * voxelInfo.xy + fVoxel_idx.xy) / (tile_num * voxelInfo.xy);
	float4 result0 = g_txVolume.SampleLevel(samRaycast, texCoord, 0);
	tile_idx = int2(z1_idx % tile_num.x, z1_idx / tile_num.y);
	texCoord = (tile_idx * voxelInfo.xy + fVoxel_idx.xy) / (tile_num * voxelInfo.xy);
	float4 result1 = g_txVolume.SampleLevel(samRaycast, texCoord, 0);
	float s = (fVoxel_idx.z - z0_idx) / (z1_idx - z0_idx);
	return lerp(result0,result1,s);
}
#else
float3 local2tex( float3 P)
{
	float3 uv = P * inverseXYZsize.xyz + 0.5;
	uv.y = 1 - uv.y;
	return uv;
}
#endif
//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
	Ray eyeray;
	//world space
	eyeray.o = viewPos;
	eyeray.d = input.Pos - eyeray.o;
	eyeray.d = normalize(eyeray.d);
	eyeray.d.x = ( eyeray.d.x == 0.f ) ? 1e-15 : eyeray.d.x;
	eyeray.d.y = ( eyeray.d.y == 0.f ) ? 1e-15 : eyeray.d.y;
	eyeray.d.z = ( eyeray.d.z == 0.f ) ? 1e-15 : eyeray.d.z;

	// calculate ray intersection with bounding box
	float tnear, tfar;
	bool hit = IntersectBox(eyeray, boxMin.xyz, boxMax.xyz , tnear, tfar);
	if(!hit) discard;
	if( tnear <= 0 ) tnear = 0;

	// calculate intersection points
	float3 Pnear = eyeray.o.xyz + eyeray.d.xyz * tnear;
	float3 Pfar = eyeray.o.xyz + eyeray.d.xyz * tfar;

	float3 P = Pnear;
	float t = tnear;
	float tSmallStep = 0.99 * voxelInfo.w;
	float3 P_pre = Pnear;
	float3 PsmallStep = eyeray.d.xyz * tSmallStep;

	float3 surfacePos;

	float field_pre ;
	float field_now = g_txVolume.SampleLevel(samRaycast,local2tex(P),0).x;
	float isoValue = inverseXYZsize.w;
	
	while ( t <= tfar ) {
#if FLAT3D
		//float2 texCoord = local2tex(P);
		float4 Field = samFlat3D(P);
		//float4 Field =  g_txVolume.SampleLevel ( samRaycast, texCoord, 0 );
#else
		float3 texCoord = local2tex(P);
		float4 Field = g_txVolume.SampleLevel(samRaycast, texCoord, 0);
		//float4 Field = g_txVolume.Load( int4(texCoord*voxelInfo.xyz,0));
#endif
		float density = Field.x;
		float4 color = float4( Field.yzw, 0 );

		field_pre = field_now;
		field_now = density;
		
		if ( field_now >= isoValue && field_pre < isoValue )
		{
			// For computing the depth
			surfacePos = P_pre + ( P - P_pre) * (isoValue-field_pre) / (field_now - field_pre);

			// For computing the normal
#if FLAT3D
			float depth_dx = samFlat3D(surfacePos+voxelInfo.w*float3 ( 1, 0, 0 )).x - samFlat3D(surfacePos+voxelInfo.w*float3 ( -1, 0, 0 )).x;
			float depth_dy = samFlat3D(surfacePos+voxelInfo.w*float3 ( 0, 1, 0 )).x - samFlat3D(surfacePos+voxelInfo.w*float3 ( 0, -1, 0 )).x;
			float depth_dz = samFlat3D(surfacePos+voxelInfo.w*float3 ( 0, 0, 1 )).x - samFlat3D(surfacePos+voxelInfo.w*float3 ( 0, 0, -1 )).x;
			/*float depth_dx = g_txVolume.SampleLevel ( samRaycast, local2tex(surfacePos+voxelInfo.w*float3 ( 1, 0, 0 )),0).x - 
								g_txVolume.SampleLevel ( samRaycast, local2tex(surfacePos+voxelInfo.w*float3 ( -1, 0, 0 )),0).x;
			float depth_dy = g_txVolume.SampleLevel ( samRaycast, local2tex(surfacePos+voxelInfo.w*float3 ( 0, 1, 0 )),0).x - 
								g_txVolume.SampleLevel ( samRaycast, local2tex(surfacePos+voxelInfo.w*float3 ( 0, -1, 0 )),0).x;
			float depth_dz = g_txVolume.SampleLevel ( samRaycast, local2tex(surfacePos+voxelInfo.w*float3 ( 0, 0, 1 )),0).x - 
								g_txVolume.SampleLevel ( samRaycast, local2tex(surfacePos+voxelInfo.w*float3 ( 0, 0, -1 )),0).x;*/
#else
			float3 tCoord = local2tex(surfacePos);
			float depth_dx = g_txVolume.SampleLevel ( samRaycast, tCoord + float3 ( 1, 0, 0 ) /voxelInfo.xyz, 0 ).x - 
								g_txVolume.SampleLevel ( samRaycast, tCoord + float3 ( -1, 0, 0 ) /voxelInfo.xyz, 0 ).x;
			float depth_dy = g_txVolume.SampleLevel ( samRaycast, tCoord + float3 ( 0, -1, 0 ) /voxelInfo.xyz, 0 ).x - 
								g_txVolume.SampleLevel ( samRaycast, tCoord + float3 ( 0, 1, 0 ) /voxelInfo.xyz, 0 ).x;
			float depth_dz = g_txVolume.SampleLevel ( samRaycast, tCoord + float3 ( 0, 0, 1 ) /voxelInfo.xyz, 0 ).x - 
								g_txVolume.SampleLevel ( samRaycast, tCoord + float3 ( 0, 0, -1 ) /voxelInfo.xyz, 0 ).x;
			//float depth_dx = g_txVolume.SampleLevel ( samRaycast, tCoord, 0, int3 ( 1, 0, 0 ) ).x - 
			//					g_txVolume.SampleLevel ( samRaycast, tCoord, 0, int3 ( -1, 0, 0 ) ).x;
			//float depth_dy = g_txVolume.SampleLevel ( samRaycast, tCoord, 0, int3 ( 0, 1, 0 ) ).x - 
			//					g_txVolume.SampleLevel ( samRaycast, tCoord, 0, int3 ( 0, -1, 0 ) ).x;
			//float depth_dz = g_txVolume.SampleLevel ( samRaycast, tCoord, 0, int3 ( 0, 0, 1 ) ).x - 
			//					g_txVolume.SampleLevel ( samRaycast, tCoord, 0, int3 ( 0, 0, -1 ) ).x;
#endif
			float3 normal = -normalize ( float3 ( depth_dx, depth_dy, depth_dz ) );


			// shading part
			float3 ambientLight = aLight_col * color;

			float3 directionalLight = dLight_col * color * clamp( dot( normal, dLight_dir ), 0, 1 );

			float3 vLight = viewPos - surfacePos;
			float3 halfVect = normalize( vLight - eyeray.d.xyz );
			float dist = length( vLight ); vLight /= dist;
			float angleAttn = clamp ( dot ( normal, vLight ), 0, 1 );
			float distAttn = 1.0f / ( dist * dist ); 
			float specularAttn = pow( clamp( dot( normal, halfVect ), 0, 1 ), 128 );

			float3 pointLight = pLight_col * color * angleAttn + color * specularAttn ;

			float3 col = ambientLight + directionalLight + pointLight;
			return float4( col, 0 );
			//return float4(normal*0.5+0.5,0);
		}

		P_pre = P;
		P += PsmallStep;
		t += tSmallStep;
	}
	return float4( 1, 1, 1, 0 ) * 0.01;       
}



