struct VS_INPUT
{
	float4 Position   : POSITION;
	float2 Texture    : TEXCOORD0;
	float4 NightColor : COLOR0;
	float4 DayColor   : COLOR1;
};

struct VS_OUTPUT
{
	float4 Position   : POSITION;
	float2 Texture    : TEXCOORD0;
	float4 Color	  : COLOR0;
};

float4x4	viewProjMatrix : register(c0);
float		fDayNightBalance : register(c4);
float4		AmbientLight : register(c5);

VS_OUTPUT NVC_vertex_shader( in VS_INPUT In )
{
	VS_OUTPUT Out;

	Out.Position = mul(In.Position, viewProjMatrix);
	Out.Texture = In.Texture;

	Out.Color = (In.DayColor * (1.0-fDayNightBalance) + In.NightColor * fDayNightBalance);	
	Out.Color.rgb += AmbientLight.rgb * AmbientLight.a;
	Out.Color = saturate(Out.Color);

	return Out;
}