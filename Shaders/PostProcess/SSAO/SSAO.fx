
#include "FullScreenTriangleVSOutput.fxh"
#include "BasicStructures.fxh"

Texture2D<float>   g_tex2DDepthBuffer;
Texture2D<float4>  g_tex2DSrcColor;

cbuffer cbCameraAttribs
{
    CameraAttribs g_CameraAttribs;
}

float DepthToCameraZ(in float fDepth, in matrix mProj)
{
    // Transformations to/from normalized device coordinates are the
    // same in both APIs.
    // However, in GL, depth must be transformed to NDC Z first

    float z = DepthToNormalizedDeviceZ(fDepth);
    return MATRIX_ELEMENT(mProj,3,2) / (z - MATRIX_ELEMENT(mProj,2,2));
}

void ApplySSAO(FullScreenTriangleVSOutput VSOut,
               // IMPORTANT: non-system generated pixel shader input
               // arguments must have the exact same name as vertex shader 
               // outputs and must go in the same order.
               // Moreover, even if the shader is not using the argument,
               // it still must be declared.
              out float4 f4Color : SV_Target)
{
    float fDepth = g_tex2DDepthBuffer.Load( int3(VSOut.f4PixelPos.xy,0) );
    float fCamSpaceZ = DepthToCameraZ(fDepth, g_CameraAttribs.mProj);

    float Occlusion = 0.0;
    for(int i=-4; i <= +4; i+=2)
    {
        for(int j=-4; j <= +4; j+=2)
        {
            float fSampleDepth = g_tex2DDepthBuffer.Load( int3(VSOut.f4PixelPos.xy,0) + int3(i,j,0) );
            float fSampleCamSpaceZ = DepthToCameraZ(fSampleDepth, g_CameraAttribs.mProj);
            Occlusion += (fSampleCamSpaceZ > fCamSpaceZ - 1e-3) ? 1.0 : 0.0;
        }
    }
    Occlusion /= 25.f;

    //fColor = NormalizedDeviceXYToTexUV(VSOut.f2NormalizedXY);
    f4Color = g_tex2DSrcColor.Load( int3(VSOut.f4PixelPos.xy,0) ) * Occlusion;
}
