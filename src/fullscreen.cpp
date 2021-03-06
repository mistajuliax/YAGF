// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Maths/matrix4.h>
#include <Scene/FullscreenPass.h>
#include <Scene/Shaders.h>

struct ViewData {
  float ViewMatrix[16];
  float InverseViewMatrix[16];
  float InverseProjectionMatrix[16];
};

struct LightData {
  float sun_direction[3];
  float sun_angle;
  float sun_col[3];
};

FullscreenPassManager::FullscreenPassManager(RenderTargets &rtts) : RTT(rtts) {
  SunlightPSO = createSunlightShader();
  TonemapPSO = createTonemapShader();
  SkyboxPSO = createSkyboxShader();
  IBLPSO = createIBLShader();
  CommandList = GlobalGFXAPI->createCommandList();

  viewdata = GlobalGFXAPI->createConstantsBuffer(sizeof(ViewData));
  lightdata = GlobalGFXAPI->createConstantsBuffer(sizeof(LightData));

  screentri = GlobalGFXAPI->createFullscreenTri();
  depthtexturecopy = RTT.getDepthBuffer();

  SunlightInputs = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap({
      std::make_tuple(viewdata, RESOURCE_VIEW::CONSTANTS_BUFFER, 0),
      std::make_tuple(lightdata, RESOURCE_VIEW::CONSTANTS_BUFFER, 1),
      std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_NORMAL_AND_DEPTH),
                      RESOURCE_VIEW::SHADER_RESOURCE, 0),
      std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_BASE_COLOR),
                      RESOURCE_VIEW::SHADER_RESOURCE, 1),
      std::make_tuple(depthtexturecopy, RESOURCE_VIEW::SHADER_RESOURCE, 2),
  });

  IBLInputs = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap(
      {std::make_tuple(viewdata, RESOURCE_VIEW::CONSTANTS_BUFFER, 0),
       std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_NORMAL_AND_DEPTH),
                       RESOURCE_VIEW::SHADER_RESOURCE, 0),
       std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_BASE_COLOR),
                       RESOURCE_VIEW::SHADER_RESOURCE, 1),
       std::make_tuple(depthtexturecopy, RESOURCE_VIEW::SHADER_RESOURCE, 2)});

  TonemapInputs = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap({
      std::make_tuple(RTT.getRTT(RenderTargets::COLORS),
                      RESOURCE_VIEW::SHADER_RESOURCE, 0),
  });

  SkyboxInputs = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap({
      std::make_tuple(viewdata, RESOURCE_VIEW::CONSTANTS_BUFFER, 0),
  });
  Samplers = GlobalGFXAPI->createSamplerHeap({{SAMPLER_TYPE::NEAREST, 0},
                                              {SAMPLER_TYPE::NEAREST, 1},
                                              {SAMPLER_TYPE::NEAREST, 2}});
  SkyboxSamplers =
      GlobalGFXAPI->createSamplerHeap({{SAMPLER_TYPE::ANISOTROPIC, 0}});
  IBLSamplers = GlobalGFXAPI->createSamplerHeap({{SAMPLER_TYPE::NEAREST, 0},
                                                 {SAMPLER_TYPE::NEAREST, 1},
                                                 {SAMPLER_TYPE::NEAREST, 2},
                                                 {SAMPLER_TYPE::ANISOTROPIC, 3},
                                                 {SAMPLER_TYPE::BILINEAR, 4}});
}

FullscreenPassManager::~FullscreenPassManager() {
  GlobalGFXAPI->releasePSO(SunlightPSO);
  GlobalGFXAPI->releasePSO(TonemapPSO);
  GlobalGFXAPI->releasePSO(SkyboxPSO);
  GlobalGFXAPI->releaseCommandList(CommandList);
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(SunlightInputs);
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(TonemapInputs);
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(SkyboxInputs);
  GlobalGFXAPI->releaseSamplerHeap(Samplers);
  GlobalGFXAPI->releaseSamplerHeap(SkyboxSamplers);
  GlobalGFXAPI->releaseConstantsBuffers(viewdata);
  GlobalGFXAPI->releaseConstantsBuffers(lightdata);
  GlobalGFXAPI->releaseIndexVertexBuffersSet(screentri);
}

void FullscreenPassManager::renderSunlight() {
  LightData *data = (LightData *)GlobalGFXAPI->mapConstantsBuffer(lightdata);
  data->sun_col[0] = 1.;
  data->sun_col[1] = 1.;
  data->sun_col[2] = 1.;
  data->sun_direction[0] = 0.;
  data->sun_direction[1] = 1.;
  data->sun_direction[2] = 0.;
  GlobalGFXAPI->unmapConstantsBuffers(lightdata);

  GlobalGFXAPI->writeResourcesTransitionBarrier(
      CommandList,
      {
          std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_NORMAL_AND_DEPTH),
                          RESOURCE_USAGE::RENDER_TARGET,
                          RESOURCE_USAGE::READ_GENERIC),
          std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_BASE_COLOR),
                          RESOURCE_USAGE::RENDER_TARGET,
                          RESOURCE_USAGE::READ_GENERIC),
          std::make_tuple(RTT.getDepthBuffer(), RESOURCE_USAGE::DEPTH_STENCIL,
                          RESOURCE_USAGE::READ_GENERIC),
      });
  GlobalGFXAPI->setPipelineState(CommandList, SunlightPSO);
  GlobalGFXAPI->setRTTSet(CommandList,
                          RTT.getRTTSet(RenderTargets::FBO_COLORS));
  GlobalGFXAPI->setDescriptorHeap(CommandList, 0, SunlightInputs);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 1, Samplers);
  GlobalGFXAPI->setIndexVertexBuffersSet(CommandList, screentri);
  GlobalGFXAPI->drawInstanced(CommandList, 3, 1, 0, 0);

  GlobalGFXAPI->writeResourcesTransitionBarrier(
      CommandList,
      {
          std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_NORMAL_AND_DEPTH),
                          RESOURCE_USAGE::READ_GENERIC,
                          RESOURCE_USAGE::RENDER_TARGET),
          std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_BASE_COLOR),
                          RESOURCE_USAGE::READ_GENERIC,
                          RESOURCE_USAGE::RENDER_TARGET),
          std::make_tuple(RTT.getDepthBuffer(), RESOURCE_USAGE::READ_GENERIC,
                          RESOURCE_USAGE::DEPTH_STENCIL),
      });
}

void FullscreenPassManager::renderTonemap() {
  GlobalGFXAPI->writeResourcesTransitionBarrier(
      CommandList, {
                       std::make_tuple(RTT.getRTT(RenderTargets::COLORS),
                                       RESOURCE_USAGE::RENDER_TARGET,
                                       RESOURCE_USAGE::READ_GENERIC),
                   });
  GlobalGFXAPI->setBackbufferAsRTTSet(CommandList, 1024, 1024);
  GlobalGFXAPI->setPipelineState(CommandList, TonemapPSO);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 0, TonemapInputs);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 1, Samplers);
  GlobalGFXAPI->setIndexVertexBuffersSet(CommandList, screentri);
  GlobalGFXAPI->drawInstanced(CommandList, 3, 1, 0, 0);

  GlobalGFXAPI->writeResourcesTransitionBarrier(
      CommandList, {
                       std::make_tuple(RTT.getRTT(RenderTargets::COLORS),
                                       RESOURCE_USAGE::READ_GENERIC,
                                       RESOURCE_USAGE::RENDER_TARGET),
                   });

  GlobalGFXAPI->setBackbufferAsPresent(CommandList);
}

void FullscreenPassManager::renderSky(
    WrapperDescriptorHeap *skyboxtextureheap) {
  GlobalGFXAPI->setRTTSet(CommandList,
                          RTT.getRTTSet(RenderTargets::FBO_COLOR_WITH_DEPTH));
  GlobalGFXAPI->setPipelineState(CommandList, SkyboxPSO);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 0, SkyboxInputs);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 1, skyboxtextureheap);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 2, SkyboxSamplers);
  GlobalGFXAPI->setIndexVertexBuffersSet(CommandList, screentri);
  GlobalGFXAPI->drawInstanced(CommandList, 3, 1, 0, 0);
}

void FullscreenPassManager::renderIBL(WrapperDescriptorHeap *probeHeap) {
  irr::core::matrix4 View, Proj, InvView, invProj;
  Proj.buildProjectionMatrixPerspectiveFovLH(70.f / 180.f * 3.14f, 1.f, 1.f,
                                             100.f);
  Proj.getInverse(invProj);
  ViewData *vdata = (ViewData *)GlobalGFXAPI->mapConstantsBuffer(viewdata);
  memcpy(&vdata->ViewMatrix, View.pointer(), 16 * sizeof(float));
  memcpy(&vdata->InverseViewMatrix, InvView.pointer(), 16 * sizeof(float));
  memcpy(&vdata->InverseProjectionMatrix, invProj.pointer(),
         16 * sizeof(float));
  GlobalGFXAPI->unmapConstantsBuffers(viewdata);

  GlobalGFXAPI->writeResourcesTransitionBarrier(
      CommandList,
      {
          std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_NORMAL_AND_DEPTH),
                          RESOURCE_USAGE::RENDER_TARGET,
                          RESOURCE_USAGE::READ_GENERIC),
          std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_BASE_COLOR),
                          RESOURCE_USAGE::RENDER_TARGET,
                          RESOURCE_USAGE::READ_GENERIC),
          std::make_tuple(RTT.getDepthBuffer(), RESOURCE_USAGE::DEPTH_STENCIL,
                          RESOURCE_USAGE::READ_GENERIC),
      });

  GlobalGFXAPI->setRTTSet(CommandList,
                          RTT.getRTTSet(RenderTargets::FBO_COLORS));
  GlobalGFXAPI->setPipelineState(CommandList, IBLPSO);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 0, IBLInputs);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 1, probeHeap);
  GlobalGFXAPI->setDescriptorHeap(CommandList, 2, IBLSamplers);
  GlobalGFXAPI->setIndexVertexBuffersSet(CommandList, screentri);
  GlobalGFXAPI->drawInstanced(CommandList, 3, 1, 0, 0);

  GlobalGFXAPI->writeResourcesTransitionBarrier(
      CommandList,
      {
          std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_NORMAL_AND_DEPTH),
                          RESOURCE_USAGE::READ_GENERIC,
                          RESOURCE_USAGE::RENDER_TARGET),
          std::make_tuple(RTT.getRTT(RenderTargets::GBUFFER_BASE_COLOR),
                          RESOURCE_USAGE::READ_GENERIC,
                          RESOURCE_USAGE::RENDER_TARGET),
          std::make_tuple(RTT.getDepthBuffer(), RESOURCE_USAGE::READ_GENERIC,
                          RESOURCE_USAGE::DEPTH_STENCIL),
      });
}