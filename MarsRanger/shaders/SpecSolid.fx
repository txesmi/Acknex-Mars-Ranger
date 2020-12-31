float3x3 matTangent;

float4x4 matWorldViewProj;
float4x4 matWorld;
float4 vecViewPos;
float4 vecViewDir;
float4 vecSkill1;

int iLights;
float4 vecLightPos[8];
float4 vecLightColor[8];

texture entSkin1;
texture entSkin2;

sampler sColor  = sampler_state { Texture = <entSkin1>; Mipfilter = Linear; Minfilter = Linear; Magfilter = Linear; };
sampler sNormal = sampler_state { Texture = <entSkin2>; Mipfilter = Linear; Minfilter = Linear; Magfilter = Linear; };

struct vsIn {
	float4 position : POSITION;
	float3 normal   : NORMAL;
	float2 tex1     : TEXCOORD0;
	float3 tangent  : TEXCOORD2;
};

struct vsOut {
	float4 position : POSITION;
	float2 tex      : TEXCOORD0;
	float3 normal   : TEXCOORD1;
	float3 world    : TEXCOORD2;
	float3 tangent  : TEXCOORD3;
	float3 viewDir  : TEXCOORD4;
};

struct psOut {
	float4 color0   : COLOR0;
};

vsOut VS (vsIn In) {
		In.position.xz *= vecSkill1.x;
		In.normal.xz *= vecSkill1.x;
		vsOut Out;
		Out.position = mul(In.position, matWorldViewProj);
		Out.tex = In.tex1;
		Out.normal = mul(In.normal, (float3x3)matWorld);
		Out.world = mul(In.position, matWorld).xyz;
		Out.tangent = mul(In.tangent, (float3x3)matWorld);
		Out.viewDir = Out.world - vecViewPos.xyz;
		return Out;
	}
	
psOut PS (vsOut In) {
		psOut Out;
		Out.color0 = tex2D(sColor, In.tex);
		float4 TNormalTex = tex2D(sNormal, In.tex);
		float3 bumpNormal = (TNormalTex.xyz * 2.0f) - 1.0f;
		In.tangent = normalize(In.tangent);
		In.normal = normalize(In.normal);
		float3 binormal = cross(In.normal, In.tangent);
		float3 worldNormal = bumpNormal.x * In.tangent + bumpNormal.y * binormal + bumpNormal.z * In.normal;
		
		In.viewDir = normalize(In.viewDir);
		float3 fDiffuse = float3(0.3f, 0.3f, 0.3f);
		float3 fSpecular = 0;
		for(float i=0; i<iLights; i+=1) {
			float3 fDir = In.world.xyz - vecLightPos[i].xyz;
			float3 fReflect = normalize((2.0f * dot(worldNormal, fDir) * worldNormal) - fDir);
			float fRadiance = saturate((vecLightPos[i].w - length(fDir)) / vecLightPos[i].w);
		   fSpecular += vecLightColor[i].rgb * pow(saturate(dot(fReflect, In.viewDir)), 8.0f) * TNormalTex.a;// * fRadiance;
			fDiffuse += vecLightColor[i].rgb * saturate(dot(normalize(-fDir), worldNormal));// * fRadiance;
		}
		
		Out.color0.rgb *= fDiffuse;
		Out.color0.rgb += fSpecular;
		
		return Out;
	}

technique SpecSolid
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