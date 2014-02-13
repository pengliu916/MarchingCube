Texture2D<float4>    textures[6];
SamplerState      samColor : register(s0);


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct GS_INPUT
{
};

struct PS_INPUT
{
    float4	Pos : SV_POSITION;
	float2	Tex : TEXCOORD0;
	uint	PrimID : SV_PrimitiveID;
};
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
void GS_1(point GS_INPUT particles[1], uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> triStream)
{
    PS_INPUT output;
	output.Pos=float4(-1.0f,1.0f,0.01f,1.0f);
	output.Tex=float2(0.0f,0.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(1.0f,1.0f,0.01f,1.0f);
	output.Tex=float2(1.0f,0.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(-1.0f,-1.0f,0.01f,1.0f);
	output.Tex=float2(0.0f,1.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(1.0f,-1.0f,0.01f,1.0f);
	output.Tex=float2(1.0f,1.0f);
	output.PrimID=primID;
	triStream.Append(output);
}

[maxvertexcount(4)]
void GS_2(point GS_INPUT particles[1],uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> triStream)
{
    PS_INPUT output;
	float offset=(float)primID;
	output.Pos=float4(-1.0f+offset,1.0f,0.01f,1.0f);
	output.Tex=float2(0.0f,0.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(0.0f+offset,1.0f,0.01f,1.0f);
	output.Tex=float2(1.0f,0.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(-1.0f+offset,-1.0f,0.01f,1.0f);
	output.Tex=float2(0.0f,1.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(0.0f+offset,-1.0f,0.01f,1.0f);
	output.Tex=float2(1.0f,1.0f);
	output.PrimID=primID;
	triStream.Append(output);
}

[maxvertexcount(4)]
void GS_4(point GS_INPUT particles[1],uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> triStream)
{
    PS_INPUT output;
	float offset=(float)primID;
	output.Pos=float4(-1.0f+primID%2,1.0f-primID/2,0.01f,1.0f);
	output.Tex=float2(0.0f,0.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(0.0f+primID%2,1.0f-primID/2,0.01f,1.0f);
	output.Tex=float2(1.0f,0.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(-1.0f+primID%2,0.0f-primID/2,0.01f,1.0f);
	output.Tex=float2(0.0f,1.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(0.0f+primID%2,0.0f-primID/2,0.01f,1.0f);
	output.Tex=float2(1.0f,1.0f);
	output.PrimID=primID;
	triStream.Append(output);
} 

[maxvertexcount(4)]
void GS_6(point GS_INPUT particles[1],uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> triStream)
{
    PS_INPUT output;
	float offset=(float)primID;
	output.Pos=float4(-1.0f+2.0f/3.0f*(primID%3),1.0f-primID/3,0.01f,1.0f);
	output.Tex=float2(0.0f,0.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(-1.0f/3.0f+2.0f/3.0f*(primID%3),1.0f-primID/3,0.01f,1.0f);
	output.Tex=float2(1.0f,0.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(-1.0f+2.0f/3.0f*(primID%3),0.0f-primID/3,0.01f,1.0f);
	output.Tex=float2(0.0f,1.0f);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(-1.0f/3.0f+2.0f/3.0f*(primID%3),0.0f-primID/3,0.01f,1.0f);
	output.Tex=float2(1.0f,1.0f);
	output.PrimID=primID;
	triStream.Append(output);
} 
//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
	float4 color;
	[branch] switch(input.PrimID)
	{
		case 0:
			color =textures[0].Sample(samColor, input.Tex);
			//color =textures[0].Load(int3(input.Tex.x*640,input.Tex.y*480,0));
			return color;
		case 1:
			color =textures[1].Sample(samColor, input.Tex);
			return color;
		case 2:
			color =textures[2].Sample(samColor, input.Tex);
			return color;
		case 3:
			color =textures[3].Sample(samColor, input.Tex);
			return color;
		case 4:
			color =textures[4].Sample(samColor, input.Tex);
			return color;
		case 5:
			color =textures[5].Sample(samColor, input.Tex);
			return color;
		default:
			color = float4(input.Tex.xy,0,1);
			return color;
	}
}



