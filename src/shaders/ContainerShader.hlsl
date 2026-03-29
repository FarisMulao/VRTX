cbuffer CameraData : register(b0)
{
    matrix viewProj;
};

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), viewProj);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(0.6f, 0.6f, 0.7f, 1.0f); // Subtle grey-blue dots
}
