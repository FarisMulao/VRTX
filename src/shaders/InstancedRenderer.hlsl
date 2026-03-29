cbuffer CameraData : register(b0)
{
    matrix viewProj;
};

struct VerletBody
{
    float3 position;
    float3 previousPosition;
    float radius;
    uint isPinned;
};

StructuredBuffer<VerletBody> Bodies : register(t0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

PSInput VSMain(VSInput input, uint instanceID : SV_InstanceID)
{
    PSInput output;
    
    VerletBody body = Bodies[instanceID];
    
    float3 worldPos = input.position * body.radius + body.position;
    
    output.position = mul(float4(worldPos, 1.0f), viewProj);
    output.normal = input.normal;
    output.color = body.isPinned ? float3(1.0f, 0.2f, 0.2f) : float3(0.2f, 0.9f, 0.8f); // Cyan default
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 lightDir = normalize(float3(0.5f, 1.0f, -0.5f));
    float ambient = 0.2f;
    float ndotl = max(dot(input.normal, lightDir), 0.0f);
    
    float3 shaded = input.color * (ambient + ndotl);
    return float4(shaded, 1.0f);
}
