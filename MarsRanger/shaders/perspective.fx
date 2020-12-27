float4x4 matWorld;
float4x4 matWorldViewProj;

float4 vecTime; // Variable tiempo pasada por acknex
float4 vecSkill41; // Variable tiempo pasada por acknex
float vecFog; // Vector de color de la niebla

texture entSkin1; // Textura de la entidad

sampler Mascara = sampler_state  // Sampleo de la textura de máscara
{
	Texture = <entSkin1>;
	MipFilter = Linear;
	AddressU = Wrap;
	Addressv = Wrap;
};


struct out_agua // Estructura de salida de datos
{
	float4 Pos	: POSITION;
	float  Fog	: FOG;
	float2 Tex0	: TEXCOORD0;
	float2 Pos0	: TEXCOORD1;
};

out_agua vs_agua20 // Vertex Shader
(
	in float4 inPos		: POSITION,
	in float3 inNormal	: NORMAL,
	in float2 inTex0	: TEXCOORD0
)
{
	out_agua Out; // Estructura de salida
	
	Out.Pos = mul(inPos,matWorldViewProj); // Posición del vértice
	
	Out.Pos0 = mul(inPos,matWorldViewProj); // Posición del vértice
	
	Out.Fog = vecFog; // Intensidad de la niebla
	
	Out.Tex0 = inTex0; // Coordenadas de la textura
	
	return Out; // Respuesta
}
	
float4 ps_agua20(out_agua In): COLOR // Pixel Shader
{
	// Máscara 1
	float2 Coord1 = In.Tex0.xy; // Coordenadas modificadas en base al tiempo
	Coord1.x += vecTime.w * 0.0002f;
	Coord1.x += In.Pos0.x * In.Pos0.y * 0.1f;
	float4 Tex = tex2D ( Mascara, Coord1 ); // Color del punto en la textura
	
	//Tex.xyz =  abs(In.Pos0.x);
	Tex.a = 1;
	return Tex;
}

//////////////////////////////////////////////////////////////////
	technique agua
	{
		pass one
		{
			VertexShader = compile vs_2_0 vs_agua20();
			PixelShader = compile ps_2_0 ps_agua20();			
		}		
	}

	technique fallback { pass one { } }
