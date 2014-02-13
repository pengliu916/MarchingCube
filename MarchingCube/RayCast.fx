SamplerState samRaycast : register(s0);
Texture3D g_txVolume : register(t0);

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
	float4 viewPos;
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
[maxvertexcount(18)]
void GS(point GS_INPUT particles[1], inout TriangleStream<PS_INPUT> triStream)
{
	PS_INPUT output;
	float3 voxelResolution = voxelInfo.xyz;
	float voxelSize = voxelInfo.w;
	output.Pos=float4(1.0f,1.0f,1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,-1.0f,1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,1.0f,-1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,-1.0f,-1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,1.0f,-1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,-1.0f,-1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,1.0f,1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,-1.0f,1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,1.0f,1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,-1.0f,1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);

	triStream.RestartStrip();

	output.Pos=float4(1.0f,1.0f,1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,1.0f,-1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,1.0f,1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,1.0f,-1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);

	triStream.RestartStrip();

	output.Pos=float4(1.0f,-1.0f,-1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,-1.0f,1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,-1.0f,-1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,-1.0f,1.0f,1.0f)*float4(voxelResolution*voxelSize/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);

}


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
	float field_now = g_txVolume.SampleLevel(samRaycast,P * inverseXYZsize.xyz + 0.5,0).x;

	float isoValue = inverseXYZsize.w;
	
	while ( t <= tfar ) {
		float3 txCoord = P * inverseXYZsize.xyz + 0.5;
		float4 Field =  g_txVolume.SampleLevel ( samRaycast, txCoord, 0 );
		float density = Field.x;
		float4 color = float4( Field.yzw, 0 );

		field_pre = field_now;
		field_now = density;
		
		if ( field_now > isoValue && field_pre < isoValue )
		{
			// For computing the depth
			surfacePos = P_pre + ( P - P_pre) * (isoValue-field_pre) / (field_now - field_pre);			
			txCoord = surfacePos * inverseXYZsize.xyz + 0.5;

			// For computing the normal
			float depth_dx = g_txVolume.SampleLevel ( samRaycast, txCoord, 0, int3 ( 1, 0, 0 ) ).x - 
								g_txVolume.SampleLevel ( samRaycast, txCoord, 0, int3 ( -1, 0, 0 ) ).x;
			float depth_dy = g_txVolume.SampleLevel ( samRaycast, txCoord, 0, int3 ( 0, 1, 0 ) ).x - 
								g_txVolume.SampleLevel ( samRaycast, txCoord, 0, int3 ( 0, -1, 0 ) ).x;
			float depth_dz = g_txVolume.SampleLevel ( samRaycast, txCoord, 0, int3 ( 0, 0, 1 ) ).x - 
								g_txVolume.SampleLevel ( samRaycast, txCoord, 0, int3 ( 0, 0, -1 ) ).x;
			float3 normal = -normalize ( float3 ( depth_dx, depth_dy, depth_dz ) );


			// shading part
			float3 ambientLight = aLight_col * color;

			float3 directionalLight = dLight_col * color * clamp( dot( normal, dLight_dir ), 0, 1 );

			float3 vLight = pLight_pos - surfacePos;
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
	return float4( 1, 1, 1, 0 ) * 0.00;       
}



