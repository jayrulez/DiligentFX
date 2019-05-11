/*     Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include <algorithm>
#include <unordered_set>
#include <array>
#include <cstring>

#include "SSAOPostProcess.h"
#include "ShaderMacroHelper.h"
#include "GraphicsUtilities.h"
#include "GraphicsAccessories.h"
#include "../../../Utilities/include/DiligentFXShaderSourceStreamFactory.h"
#include "MapHelper.h"
#include "CommonlyUsedStates.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Diligent
{

SSAOPostProcess::SSAOPostProcess(IRenderDevice*  pDevice, 
                                 IDeviceContext* pContext,
                                 TEXTURE_FORMAT  BackBufferFmt,
                                 TEXTURE_FORMAT  DepthBufferFmt)
{
    RefCntAutoPtr<IShader> pFullScreenTriangleVS;
    {
        ShaderCreateInfo ShaderCI;
        ShaderCI.Desc.Name                  = "Screen size triangle";
        ShaderCI.EntryPoint                 = "FullScreenTriangleVS";
        ShaderCI.FilePath                   = "FullScreenTriangleVS.fx";
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.Desc.ShaderType            = SHADER_TYPE_VERTEX;
        ShaderCI.pShaderSourceStreamFactory = &DiligentFXShaderSourceStreamFactory::GetInstance();
        ShaderCI.UseCombinedTextureSamplers = true;
        pDevice->CreateShader(ShaderCI, &pFullScreenTriangleVS);
    }

    RefCntAutoPtr<IShader> pSSAO_PS;
    {
        ShaderCreateInfo ShaderCI;
        ShaderCI.Desc.Name                  = "SSAO PS";
        ShaderCI.EntryPoint                 = "ApplySSAO";
        ShaderCI.FilePath                   = "SSAO.fx";
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.Desc.ShaderType            = SHADER_TYPE_PIXEL;
        ShaderCI.pShaderSourceStreamFactory = &DiligentFXShaderSourceStreamFactory::GetInstance();
        ShaderCI.UseCombinedTextureSamplers = true;
        pDevice->CreateShader(ShaderCI, &pSSAO_PS);
    }


    PipelineStateDesc PSODesc;
    PSODesc.Name           = "SSAO PSO";

    auto& GraphicsPipeline = PSODesc.GraphicsPipeline;
    GraphicsPipeline.RasterizerDesc.FillMode              = FILL_MODE_SOLID;
    GraphicsPipeline.RasterizerDesc.CullMode              = CULL_MODE_NONE;
    GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = True;
    GraphicsPipeline.DepthStencilDesc.DepthEnable         = False;
    GraphicsPipeline.pVS                                  = pFullScreenTriangleVS;
    GraphicsPipeline.pPS                                  = pSSAO_PS;
    GraphicsPipeline.PrimitiveTopology                    = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.NumRenderTargets                     = 1;
    GraphicsPipeline.DSVFormat                            = DepthBufferFmt;
    GraphicsPipeline.RTVFormats[0]                        = BackBufferFmt;

    PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    pDevice->CreatePipelineState(PSODesc, &m_PSO);
}

SSAOPostProcess::~SSAOPostProcess()
{

}

void SSAOPostProcess::InvalidateResourceBindings()
{
    m_SRB.Release();
}

void SSAOPostProcess::OnWindowResize(IRenderDevice* pDevice, Uint32 uiBackBufferWidth, Uint32 uiBackBufferHeight)
{
    InvalidateResourceBindings();
}

void SSAOPostProcess::CreateSRB(FrameAttribs& FrameAttribs)
{
    m_SRB.Release();
    m_PSO->CreateShaderResourceBinding(&m_SRB, true);
    m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "cbCameraAttribs")->Set(FrameAttribs.pCameraAttribsCB);
    m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DDepthBuffer")->Set(FrameAttribs.ptex2DSrcDepthBufferSRV);
    m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_tex2DSrcColor")->Set(FrameAttribs.ptex2DSrcColorBufferSRV);
}

void SSAOPostProcess::PerformPostProcessing(FrameAttribs& FrameAttribs)
{
    if (!m_SRB)
        CreateSRB(FrameAttribs);

    FrameAttribs.pDeviceContext->SetPipelineState(m_PSO);
    FrameAttribs.pDeviceContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    if (FrameAttribs.pDstRTV != nullptr)
    {
        FrameAttribs.pDeviceContext->SetRenderTargets(1, &FrameAttribs.pDstRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    else
    {
        FrameAttribs.pDeviceContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    DrawAttribs drawAttrs(3, DRAW_FLAG_VERIFY_ALL);
    FrameAttribs.pDeviceContext->Draw(drawAttrs);
}

}
