float4x4 matWorldViewProj;
float4x4 matWorld;

int iLights;
float4 vecLightPos[8];
float4 vecLightColor[8];

struct vsIn {
	float4 position : POSITION;
	float3 normal   : NORMAL;
};

struct vsOut {
	float4 position : POSITION;
	float3 normal   : TEXCOORD0;
};

struct psOut {
	float4 color0   : COLOR0;
};

vsOut VS (vsIn In) {
		vsOut Out;
		Out.position = mul(In.position, matWorldViewProj);
		Out.normal = mul(In.normal, (float3x3)matWorld);

		return Out;
	}
	
psOut PS (vsOut In) {
		psOut Out;
		In.normal = normalize(In.normal);
		
		Out.color0.rgb = saturate(lerp(float3(0.42f, 0.28f, 0.17f), float3(0.81f, 0.45f, 0.12f), In.normal.y) * (1.0f - abs(In.normal.z) * 0.5f));
		Out.color0.a = 1;
		
		return Out;
	}

technique Triplanar
{
	pass p0
	{
		ZEnable = True;
		ZWriteEnable = True;
		AlphaTestEnable = False;
		AlphaBlendEnable = False;
		
		VertexShader = compile vs_3_0 VS();
		PixelShader  = compile ps_3_0 PS();
	}
}

technique fallback { pass one { } }