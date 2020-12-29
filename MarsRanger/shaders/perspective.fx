float4x4 matWorld;
float4x4 matWorldViewProj;

float4 vecTime;

texture entSkin1;

sampler smpSkin1 = sampler_state
{
	Texture = <entSkin1>;
	MipFilter = Linear;
	AddressU = Wrap;
	Addressv = Wrap;
};

//////////////////////////////////////////////////////////////////

struct in_vs
{
	float4 Pos	: POSITION;
	float2 Tex0	: TEXCOORD0;
};

struct out_vs
{
	float4 Pos	: POSITION;
	float2 Tex0	: TEXCOORD0;
	float2 Pos0	: TEXCOORD1;
};

out_vs VS (in_vs In) 
{
	out_vs Out;
	
	Out.Pos = mul(In.Pos,matWorldViewProj);
	Out.Tex0 = In.Tex0;
	Out.Pos0 = Out.Pos;
	
	return Out; 
}
	
float4 PS(out_vs In): COLOR0
{
	float2 Coord1 = In.Tex0.xy;
	Coord1.x += vecTime.w * 0.0002f;
	Coord1.x += In.Pos0.x * In.Pos0.y * 0.1f;
	float4 Tex = tex2D(smpSkin1, Coord1);
	
	Tex.a = 1;
	return Tex;
}

//////////////////////////////////////////////////////////////////

technique
{
	pass one
	{
		VertexShader = compile vs_2_0 VS();
		PixelShader = compile ps_2_0 PS();			
	}		
}

technique fallback { pass one { } }
