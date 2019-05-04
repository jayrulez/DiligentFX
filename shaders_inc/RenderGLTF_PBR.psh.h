"// PBR shader based on the Khronos WebGL PBR implementation\n"
"// See https://github.com/KhronosGroup/glTF-WebGL-PBR\n"
"// Supports both metallic roughness and specular glossiness inputs\n"
"\n"
"#include \"BasicStructures.fxh\"\n"
"#include \"GLTF_PBR_Structures.fxh\"\n"
"#include \"PBR_Common.fxh\"\n"
"#include \"ToneMapping.fxh\"\n"
"\n"
"#define  MANUAL_SRGB             1\n"
"#define  SRGB_FAST_APPROXIMATION 1\n"
"\n"
"#ifndef ALLOW_DEBUG_VIEW\n"
"#   define ALLOW_DEBUG_VIEW 0\n"
"#endif\n"
"\n"
"cbuffer cbCameraAttribs\n"
"{\n"
"    CameraAttribs g_CameraAttribs;\n"
"}\n"
"\n"
"cbuffer cbLightAttribs\n"
"{\n"
"    LightAttribs g_LightAttribs;\n"
"}\n"
"\n"
"cbuffer cbMaterialInfo\n"
"{\n"
"    GLTFMaterialInfo g_MaterialInfo;\n"
"}\n"
"\n"
"cbuffer cbRenderParameters\n"
"{\n"
"    GLTFRenderParameters g_RenderParameters;\n"
"}\n"
"\n"
"\n"
"#define USE_ENV_MAP_LOD\n"
"#define USE_HDR_CUBEMAPS\n"
"\n"
"#if USE_IBL\n"
"TextureCube  g_IrradianceMap;\n"
"SamplerState g_IrradianceMap_sampler;\n"
"\n"
"TextureCube  g_PrefilteredEnvMap;\n"
"SamplerState g_PrefilteredEnvMap_sampler;\n"
"\n"
"Texture2D     g_BRDF_LUT;\n"
"SamplerState  g_BRDF_LUT_sampler;\n"
"#endif\n"
"\n"
"Texture2D    g_ColorMap;\n"
"SamplerState g_ColorMap_sampler;\n"
"\n"
"Texture2D    g_PhysicalDescriptorMap;\n"
"SamplerState g_PhysicalDescriptorMap_sampler;\n"
"\n"
"Texture2D    g_NormalMap;\n"
"SamplerState g_NormalMap_sampler;\n"
"\n"
"Texture2D    g_AOMap;\n"
"SamplerState g_AOMap_sampler;\n"
"\n"
"Texture2D    g_EmissiveMap;\n"
"SamplerState g_EmissiveMap_sampler;\n"
"\n"
"float GetPerceivedBrightness(float3 rgb)\n"
"{\n"
"    return sqrt(0.299 * rgb.r * rgb.r + 0.587 * rgb.g * rgb.g + 0.114 * rgb.b * rgb.b);\n"
"}\n"
"\n"
"// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows/js/three.pbrUtilities.js#L34\n"
"float SolveMetallic(float3 diffuse, float3 specular, float oneMinusSpecularStrength)\n"
"{\n"
"    const float c_MinReflectance = 0.04;\n"
"    float specularBrightness = GetPerceivedBrightness(specular);\n"
"    if (specularBrightness < c_MinReflectance)\n"
"    {\n"
"        return 0.0;\n"
"    }\n"
"\n"
"    float diffuseBrightness = GetPerceivedBrightness(diffuse);\n"
"\n"
"    float a = c_MinReflectance;\n"
"    float b = diffuseBrightness * oneMinusSpecularStrength / (1.0 - c_MinReflectance) + specularBrightness - 2.0 * c_MinReflectance;\n"
"    float c = c_MinReflectance - specularBrightness;\n"
"    float D = b * b - 4.0 * a * c;\n"
"\n"
"    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);\n"
"}\n"
"\n"
"\n"
"float3 SRGBtoLINEAR(float3 srgbIn)\n"
"{\n"
"#ifdef MANUAL_SRGB\n"
"#   ifdef SRGB_FAST_APPROXIMATION\n"
"	    float3 linOut = pow(srgbIn.xyz, float3(2.2, 2.2, 2.2));\n"
"#   else\n"
"	    float3 bLess  = step(float3(0.04045, 0.04045, 0.04045), srgbIn.xyz);\n"
"	    float3 linOut = mix( srgbIn.xyz/12.92, pow((srgbIn.xyz + float3(0.055, 0.055, 0.055)) / 1.055, float3(2.4, 2.4, 2.4)), bLess );\n"
"#   endif\n"
"	    return linOut;\n"
"#else\n"
"	return srgbIn;\n"
"#endif\n"
"}\n"
"\n"
"float4 SRGBtoLINEAR(float4 srgbIn)\n"
"{\n"
"    return float4(SRGBtoLINEAR(srgbIn.xyz), srgbIn.w);\n"
"}\n"
"\n"
"\n"
"float3 ApplyDirectionalLight(float3 lightDir, float3 lightColor, SurfaceReflectanceInfo srfInfo, float3 normal, float3 view)\n"
"{\n"
"    float3 pointToLight = -lightDir;\n"
"    float3 diffuseContrib, specContrib;\n"
"    float  NdotL;\n"
"    BRDF(pointToLight, normal, view, srfInfo, diffuseContrib, specContrib, NdotL);\n"
"    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)\n"
"    float3 shade = (diffuseContrib + specContrib) * NdotL;\n"
"    return lightColor * shade;\n"
"}\n"
"\n"
"\n"
"// Find the normal for this fragment, pulling either from a predefined normal map\n"
"// or from the interpolated mesh normal and tangent attributes.\n"
"float3 PerturbNormal(in float3 Position, in float3 Normal, in float3 TSNormal, in float2 UV, bool HasUV)\n"
"{\n"
"    // Retrieve the tangent space matrix\n"
"    float3 pos_dx = ddx(Position);\n"
"    float3 pos_dy = ddy(Position);\n"
"\n"
"    float NormalLen = length(Normal);\n"
"    float3 ng;\n"
"    if (NormalLen > 1e-5)\n"
"    {\n"
"        ng = Normal/NormalLen;\n"
"    }\n"
"    else\n"
"    {\n"
"        ng = normalize(cross(pos_dx, pos_dy));\n"
"#if (defined(GLSL) || defined(GL_ES)) && !defined(TARGET_API_VULKAN)\n"
"        // In OpenGL screen is upside-down, so we have to invert the vector\n"
"        ng *= -1;\n"
"#endif\n"
"    }\n"
"\n"
"    if (HasUV)\n"
"    {\n"
"        float2 tex_dx = ddx(UV);\n"
"        float2 tex_dy = ddy(UV);\n"
"        float3 t = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);\n"
"\n"
"        t = normalize(t - ng * dot(ng, t));\n"
"        float3 b = normalize(cross(t, ng));\n"
"        float3x3 tbn = MatrixFromRows(t, b, ng);\n"
"\n"
"        TSNormal = 2.0 * TSNormal - float3(1.0, 1.0, 1.0);\n"
"        return normalize(mul(TSNormal, tbn));\n"
"    }\n"
"    else\n"
"    {\n"
"        return ng;\n"
"    }\n"
"}\n"
"\n"
"#if USE_IBL\n"
"// Calculation of the lighting contribution from an optional Image Based Light source.\n"
"// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].\n"
"// See our README.md on Environment Maps [3] for additional discussion.\n"
"void GetIBLContribution(SurfaceReflectanceInfo SrfInfo,\n"
"                        float3                 n,\n"
"                        float3                 v,\n"
"                        out float3 diffuse,\n"
"                        out float3 specular)\n"
"{\n"
"    float NdotV = clamp(dot(n, v), 0.0, 1.0);\n"
"\n"
"    float lod = clamp(SrfInfo.PerceptualRoughness * float(g_RenderParameters.PrefilteredCubeMipLevels), 0.0, float(g_RenderParameters.PrefilteredCubeMipLevels));\n"
"    float3 reflection = normalize(reflect(-v, n));\n"
"\n"
"    float2 brdfSamplePoint = clamp(float2(NdotV, SrfInfo.PerceptualRoughness), float2(0.0, 0.0), float2(1.0, 1.0));\n"
"    // retrieve a scale and bias to F0. See [1], Figure 3\n"
"    float2 brdf = g_BRDF_LUT.Sample(g_BRDF_LUT_sampler, brdfSamplePoint).rg;\n"
"\n"
"    float4 diffuseSample = g_IrradianceMap.Sample(g_IrradianceMap_sampler, n);\n"
"\n"
"#ifdef USE_ENV_MAP_LOD\n"
"    float4 specularSample = g_PrefilteredEnvMap.SampleLevel(g_PrefilteredEnvMap_sampler, reflection, lod);\n"
"#else\n"
"    float4 specularSample = g_PrefilteredEnvMap.Sample(g_PrefilteredEnvMap_sampler, reflection);\n"
"#endif\n"
"\n"
"#ifdef USE_HDR_CUBEMAPS\n"
"    // Already linear.\n"
"    float3 diffuseLight  = diffuseSample.rgb;\n"
"    float3 specularLight = specularSample.rgb;\n"
"#else\n"
"    float3 diffuseLight  = SRGBtoLINEAR(diffuseSample).rgb;\n"
"    float3 specularLight = SRGBtoLINEAR(specularSample).rgb;\n"
"#endif\n"
"\n"
"    diffuse  = diffuseLight * SrfInfo.DiffuseColor;\n"
"    specular = specularLight * (SrfInfo.Reflectance0 * brdf.x + SrfInfo.Reflectance90 * brdf.y);\n"
"}\n"
"#endif\n"
"\n"
"void main(in  float4 ClipPos  : SV_Position,\n"
"          in  float3 WorldPos : WORLD_POS,\n"
"          in  float3 Normal   : NORMAL,\n"
"          in  float2 UV0      : UV0,\n"
"          in  float2 UV1      : UV1,\n"
"          out float4 OutColor : SV_Target)\n"
"{\n"
"    float4 BaseColor = g_ColorMap.Sample(g_ColorMap_sampler, lerp(UV0, UV1, g_MaterialInfo.BaseColorTextureUVSelector));\n"
"    BaseColor = SRGBtoLINEAR(BaseColor) * g_MaterialInfo.BaseColorFactor;\n"
"    //BaseColor *= getVertexColor();\n"
"\n"
"	if (g_MaterialInfo.UseAlphaMask != 0 && BaseColor.a < g_MaterialInfo.AlphaMaskCutoff)\n"
"    {\n"
"		discard;\n"
"	}\n"
"    \n"
"    float2 NormalMapUV  = lerp(UV0, UV1, g_MaterialInfo.NormalTextureUVSelector);\n"
"    float3 TSNormal     = g_NormalMap            .Sample(g_NormalMap_sampler,             NormalMapUV).rgb;\n"
"    float  Occlusion    = g_AOMap                .Sample(g_AOMap_sampler,                 lerp(UV0, UV1, g_MaterialInfo.OcclusionTextureUVSelector)).r;\n"
"    float3 Emissive     = g_EmissiveMap          .Sample(g_EmissiveMap_sampler,           lerp(UV0, UV1, g_MaterialInfo.EmissiveTextureUVSelector)).rgb;\n"
"    float4 PhysicalDesc = g_PhysicalDescriptorMap.Sample(g_PhysicalDescriptorMap_sampler, lerp(UV0, UV1, g_MaterialInfo.PhysicalDescriptorTextureUVSelector));\n"
"    \n"
"    SurfaceReflectanceInfo SrfInfo;\n"
"    float metallic;\n"
"    float3 specularColor;\n"
"\n"
"    float3 f0 = float3(0.04, 0.04, 0.04);\n"
"\n"
"    // Metallic and Roughness material properties are packed together\n"
"    // In glTF, these factors can be specified by fixed scalar values\n"
"    // or from a metallic-roughness map\n"
"    if (g_MaterialInfo.Workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)\n"
"    {\n"
"        PhysicalDesc = SRGBtoLINEAR(PhysicalDesc);\n"
"        const float u_GlossinessFactor = 1.0;\n"
"        SrfInfo.PerceptualRoughness = (1.0 - PhysicalDesc.a * u_GlossinessFactor); // glossiness to roughness\n"
"        f0 = PhysicalDesc.rgb * g_MaterialInfo.SpecularFactor.rgb;\n"
"\n"
"        // f0 = specular\n"
"        specularColor = f0;\n"
"        float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);\n"
"        SrfInfo.DiffuseColor = BaseColor.rgb * oneMinusSpecularStrength;\n"
"\n"
"        // do conversion between metallic M-R and S-G metallic\n"
"        metallic = SolveMetallic(BaseColor.rgb, specularColor, oneMinusSpecularStrength);\n"
"    }\n"
"    else if (g_MaterialInfo.Workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)\n"
"    {\n"
"        // Roughness is stored in the \'g\' channel, metallic is stored in the \'b\' channel.\n"
"        // This layout intentionally reserves the \'r\' channel for (optional) occlusion map data\n"
"        SrfInfo.PerceptualRoughness = PhysicalDesc.g * g_MaterialInfo.RoughnessFactor;\n"
"        metallic                    = PhysicalDesc.b * g_MaterialInfo.MetallicFactor;\n"
"        metallic                    = clamp(metallic, 0.0, 1.0);\n"
"\n"
"        SrfInfo.DiffuseColor  = BaseColor.rgb * (float3(1.0, 1.0, 1.0) - f0) * (1.0 - metallic);\n"
"        specularColor         = lerp(f0, BaseColor.rgb, metallic);\n"
"    }\n"
"\n"
"//#ifdef ALPHAMODE_OPAQUE\n"
"//    baseColor.a = 1.0;\n"
"//#endif\n"
"//\n"
"//#ifdef MATERIAL_UNLIT\n"
"//    gl_FragColor = float4(gammaCorrection(baseColor.rgb), baseColor.a);\n"
"//    return;\n"
"//#endif\n"
"\n"
"    SrfInfo.PerceptualRoughness = clamp(SrfInfo.PerceptualRoughness, 0.0, 1.0);\n"
"\n"
"    // Compute reflectance.\n"
"    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);\n"
"\n"
"    SrfInfo.Reflectance0  = specularColor.rgb;\n"
"    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to \"Real-Time-Rendering\" 4th editon on page 325.\n"
"    SrfInfo.Reflectance90 = clamp(reflectance * 50.0, 0.0, 1.0).xxx;\n"
"\n"
"    // LIGHTING\n"
"\n"
"    float3 color           = float3(0.0, 0.0, 0.0);\n"
"    float3 perturbedNormal = PerturbNormal(WorldPos, Normal, TSNormal, NormalMapUV, g_MaterialInfo.NormalTextureUVSelector >= 0.0);\n"
"    float3 view            = normalize(g_CameraAttribs.f4Position.xyz - WorldPos.xyz); // Direction from surface point to camera\n"
"\n"
"    color += ApplyDirectionalLight(g_LightAttribs.f4Direction.xyz, g_LightAttribs.f4Intensity.rgb, SrfInfo, perturbedNormal, view);\n"
"    \n"
"//#ifdef USE_PUNCTUAL\n"
"//    for (int i = 0; i < LIGHT_COUNT; ++i)\n"
"//    {\n"
"//        Light light = u_Lights[i];\n"
"//        if (light.type == LightType_Directional)\n"
"//        {\n"
"//            color += applyDirectionalLight(light, materialInfo, normal, view);\n"
"//        }\n"
"//        else if (light.type == LightType_Point)\n"
"//        {\n"
"//            color += applyPointLight(light, materialInfo, normal, view);\n"
"//        }\n"
"//        else if (light.type == LightType_Spot)\n"
"//        {\n"
"//            color += applySpotLight(light, materialInfo, normal, view);\n"
"//        }\n"
"//    }\n"
"//#endif\n"
"//\n"
"    \n"
"\n"
"    // Calculate lighting contribution from image based lighting source (IBL)\n"
"#if USE_IBL\n"
"    float3 diffuseIBL, specularIBL;\n"
"    GetIBLContribution(SrfInfo, perturbedNormal, view, diffuseIBL, specularIBL);\n"
"    color += (diffuseIBL + specularIBL) * g_RenderParameters.IBLScale;\n"
"#endif\n"
"\n"
"\n"
"    color = lerp(color, color * Occlusion, g_RenderParameters.OcclusionStrength);\n"
"\n"
"    const float u_EmissiveFactor = 1.0f;\n"
"    Emissive = SRGBtoLINEAR(Emissive);\n"
"    color += Emissive.rgb * g_MaterialInfo.EmissiveFactor.rgb * g_RenderParameters.EmissionScale;\n"
"\n"
"    ToneMappingAttribs TMAttribs;\n"
"    TMAttribs.uiToneMappingMode    = TONE_MAPPING_MODE_UNCHARTED2;\n"
"    TMAttribs.bAutoExposure        = false;\n"
"    TMAttribs.fMiddleGray          = g_RenderParameters.MiddleGray;\n"
"    TMAttribs.bLightAdaptation     = false;\n"
"    TMAttribs.fWhitePoint          = g_RenderParameters.WhitePoint;\n"
"    TMAttribs.fLuminanceSaturation = 1.0;\n"
"    color = ToneMap(color, TMAttribs, g_RenderParameters.AverageLogLum);\n"
"    OutColor = float4(color, BaseColor.a);\n"
"\n"
"#if ALLOW_DEBUG_VIEW\n"
"	// Shader inputs debug visualization\n"
"	if (g_RenderParameters.DebugViewType != 0)\n"
"    {\n"
" 		switch (g_RenderParameters.DebugViewType)\n"
"        {\n"
"			case  1: OutColor.rgba = BaseColor;                                     break;\n"
"            case  2: OutColor.rgba = float4(BaseColor.aaa, 1.0);                    break;\n"
"            // Apply extra srgb->linear transform to make the maps look better\n"
"			case  3: OutColor.rgb  = SRGBtoLINEAR(TSNormal.xyz);                    break;\n"
"			case  4: OutColor.rgb  = SRGBtoLINEAR(Occlusion.xxx);                   break;\n"
"			case  5: OutColor.rgb  = SRGBtoLINEAR(Emissive.rgb);                    break;\n"
"			case  6: OutColor.rgb  = SRGBtoLINEAR(metallic.rrr);                    break;\n"
"			case  7: OutColor.rgb  = SRGBtoLINEAR(SrfInfo.PerceptualRoughness.rrr); break;\n"
"            case  8: OutColor.rgb  = SrfInfo.DiffuseColor;                          break;\n"
"            case  9: OutColor.rgb  = SrfInfo.Reflectance0;                          break;\n"
"            case 10: OutColor.rgb  = SrfInfo.Reflectance90;                         break;\n"
"            case 11: OutColor.rgb  = SRGBtoLINEAR(abs(Normal));                     break;\n"
"            case 12: OutColor.rgb  = SRGBtoLINEAR(abs(perturbedNormal));            break;\n"
"            case 13: OutColor.rgb  = dot(perturbedNormal, view).xxx;                break;\n"
"#if USE_IBL\n"
"            case 14: OutColor.rgb  = diffuseIBL;                                    break;\n"
"            case 15: OutColor.rgb  = specularIBL;                                   break;\n"
"#endif\n"
"		}\n"
"	}\n"
"#endif\n"
"\n"
"}\n"
