struct VSInput {
    float3 inPosition;
    float3 inNormal;
    float3 inColor;
    float2 inTexCoord;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;

 float4x4 MVP;
 float4x4 M;
 float3 frontDiffuse;
 float3 frontSpecular;
 float3 backDiffuse;
 float3 backSpecular;
 float3 lightPosition;
 float3 lightDiffuse;
 float3 lightSpecular;
 float3 eyePosition;
 float3 solidColor;
 float shininess;
};

ConstantBuffer<UniformBuffer> ubo;

struct VSOutput
{
    float4 pos : SV_Position;
    float3 fragColor;
    float3 fragBackColor;
    float2 fragTexCoord;
};


Sampler2D texture;

[shader("fragment")]
float4 fragMain(VSOutput vertIn,bool isFrontFace : SV_IsFrontFace) : SV_TARGET {
   //return texture.Sample(vertIn.fragTexCoord);
   //TODO: Fix this 
   if(!isFrontFace)
    return float4(vertIn.fragColor,1.0);
   else
    return float4(vertIn.fragBackColor,1.0);
}
//in vec4 frontColorOut;
//in vec4 backColorOut;
/*void main() {
    if(gl_FrontFacing)
{
    gl_FragColor = frontColorOut;
}
else
{
    gl_FragColor = backColorOut;
}
}*/

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    //output.fragColor = input.inColor;
    //output.fragTexCoord = input.inTexCoord;
    //return output;
//gl_Position = ubo.MVP * vec4(vPos, 1.0);

    VSOutput output;
    output.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPosition, 1.0))));
float3 frontColor,backColor;
backColor=float3(0.0,0.5,0.0);
// Light Position in World Space
// Front and Back Color as uniform
    
float3 vNewPos = (mul(ubo.model ,float4(input.inPosition,1.0))).xyz;
float3 N = normalize((mul(ubo.model , float4(input.inNormal,0.0))).xyz);
float3 L = normalize(ubo.lightPosition-vNewPos);
float3 V = normalize(ubo.eyePosition-vNewPos);
float NLFactor = dot(N,L);
 float3 frontSpecularFactor = float3(0.0,0.0,0.0);
 float3 frontDiffuseFactor = ubo.frontDiffuse*ubo.lightDiffuse*max(NLFactor,0.0);
if (NLFactor>=0.0) {
    frontSpecularFactor = ubo.frontSpecular * ubo.lightSpecular *pow( max(0.0,dot(reflect(-L,N),V)),ubo.shininess);
}
    frontColor = clamp(frontDiffuseFactor+frontSpecularFactor,0.0,1.0);
 float3 backSpecularFactor = float3(0.0,0.0,0.0);
    float3 backDiffuseFactor = ubo.backDiffuse*ubo.lightDiffuse*max(-NLFactor,0.0);
if (NLFactor<=0.0) {
    backSpecularFactor = ubo.backSpecular * ubo.lightSpecular *pow( max(0.0,dot(reflect(-L,-N),V)),ubo.shininess);
}
    backColor = backDiffuseFactor+backSpecularFactor;

   output.fragColor =frontColor;
   output.fragBackColor =backColor;
    return output;
}
