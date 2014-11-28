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

float2		fEnvVars : register(c0);
float4		AmbientLight : register(c1);
float4x4	world : register(c2);
float4x4	view : register(c6);
float4x4	proj : register(c10);

VS_OUTPUT NVC_vertex_shader( in VS_INPUT In )
{
	VS_OUTPUT Out;

	Out.Position = mul(proj, mul(view, mul(world, In.Position)));
	Out.Texture = In.Texture;

	Out.Color = lerp(In.DayColor, In.NightColor, fEnvVars[0]);
	Out.Color.rgb += AmbientLight.rgb;
	Out.Color.a *= (255.0/128.0) * fEnvVars[1];
	Out.Color = saturate(Out.Color);

	return Out;
}