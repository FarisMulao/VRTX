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
    float mass;
    float colorR;
    float colorG;
    float colorB;
    uint highlighted; // 0=normal, 1=source, 2=hover
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

    // Scale and highlight: source orbs slightly larger
    float scale = body.radius;
    if (body.highlighted == 1)
        scale *= 1.15f;

    float3 worldPos = input.position * scale + body.position;

    output.position = mul(float4(worldPos, 1.0f), viewProj);
    output.normal = input.normal;

    float3 baseColor = float3(body.colorR, body.colorG, body.colorB);

    if (body.isPinned)
        baseColor = float3(1.0f, 0.2f, 0.2f);

    if (body.highlighted == 1) {
        baseColor = lerp(baseColor, float3(1.0f, 1.0f, 1.0f), 0.5f);
    } else if (body.highlighted == 2) {
        baseColor = lerp(baseColor, float3(1.0f, 0.95f, 0.8f), 0.35f);
    }

    output.color = baseColor;

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 lightDir = normalize(float3(0.5f, 1.0f, -0.5f));
    float ambient = 0.25f;
    float ndotl = max(dot(input.normal, lightDir), 0.0f);

    float3 shaded = input.color * (ambient + ndotl * 0.75f);
    return float4(shaded, 1.0f);
}
