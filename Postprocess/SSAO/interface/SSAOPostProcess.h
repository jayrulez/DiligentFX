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
#pragma once

#include "../../../../DiligentCore/Common/interface/BasicMath.h"

namespace Diligent
{
using uint = uint32_t;

#include "../../../Shaders/Common/public/BasicStructures.fxh"
}

#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"
#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/BufferView.h"
#include "../../../../DiligentCore/Graphics/GraphicsEngine/interface/TextureView.h"
#include "../../../../DiligentCore/Common/interface/RefCntAutoPtr.h"

namespace Diligent
{

class SSAOPostProcess
{
public:
    struct FrameAttribs
    {
        IRenderDevice*  pDevice         = nullptr;
        IDeviceContext* pDeviceContext  = nullptr;
    
        IBuffer*        pCameraAttribsCB = nullptr;

        ITextureView*   ptex2DSrcColorBufferSRV = nullptr;
        ITextureView*   ptex2DSrcDepthBufferSRV = nullptr;
        ITextureView*   pDstRTV                 = nullptr;
    };

    SSAOPostProcess(IRenderDevice*  pDevice, 
                    IDeviceContext* pContext,
                    TEXTURE_FORMAT  BackBufferFmt,
                    TEXTURE_FORMAT  DepthBufferFmt);
    ~SSAOPostProcess();

    void OnWindowResize(IRenderDevice* pDevice, Uint32 uiBackBufferWidth, Uint32 uiBackBufferHeight);
    void InvalidateResourceBindings();

    void PerformPostProcessing(FrameAttribs&            FrameAttribs);

private:
    void CreateSRB(FrameAttribs& FrameAttribs);

    RefCntAutoPtr<IPipelineState>         m_PSO;
    RefCntAutoPtr<IShaderResourceBinding> m_SRB;
};

}
