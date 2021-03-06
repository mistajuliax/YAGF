// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt
#include <Scene/Shaders.h>

#include <API/d3dapi.h>
#include <D3DAPI/D3DRTTSet.h>
#include <D3DAPI/PSO.h>
#include <D3DAPI/RootSignature.h>

typedef RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
                      DescriptorTable<ShaderResource<0>>,
                      DescriptorTable<SamplerResource<0>>>
    TonemapRS;

struct WrapperPipelineState *createTonemapShader() {
  WrapperPipelineState *result =
      (WrapperPipelineState *)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.rootSignature = TonemapRS::get();
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
  psodesc.pRootSignature = result->D3DValue.rootSignature;
  psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
  psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  psodesc.NumRenderTargets = 1;
  psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psodesc.DepthStencilState.DepthEnable = false;
  psodesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

  psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);

  result->D3DValue.pipelineStateObject =
      PipelineStateObject<VertexLayout<irr::video::ScreenQuadVertex>>::get(
          psodesc, L"Debug\\screenquad.cso", L"Debug\\tonemap.cso");
  return result;
}

typedef RootSignature<
    D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    DescriptorTable<ConstantsBufferResource<0>, ConstantsBufferResource<1>,
                    ShaderResource<0>, ShaderResource<1>, ShaderResource<2>>,
    DescriptorTable<SamplerResource<0>>>
    SunLightRS;

struct WrapperPipelineState *createSunlightShader() {
  WrapperPipelineState *result =
      (WrapperPipelineState *)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.rootSignature = SunLightRS::get();
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
  psodesc.pRootSignature = result->D3DValue.rootSignature;
  psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
  psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  psodesc.NumRenderTargets = 1;
  psodesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
  psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psodesc.DepthStencilState.DepthEnable = false;
  psodesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

  psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
  psodesc.BlendState.RenderTarget[0].BlendEnable = true;
  psodesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  psodesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
  psodesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
  psodesc.BlendState.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;

  result->D3DValue.pipelineStateObject =
      PipelineStateObject<VertexLayout<irr::video::ScreenQuadVertex>>::get(
          psodesc, L"Debug\\screenquad.cso", L"Debug\\sunlight.cso");
  return result;
}

typedef RootSignature<
    D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    DescriptorTable<ConstantsBufferResource<1>>,
    DescriptorTable<ConstantsBufferResource<0>, ShaderResource<0>>,
    DescriptorTable<SamplerResource<0>>>
    ObjectRS;

struct WrapperPipelineState *createObjectShader() {
  WrapperPipelineState *result =
      (WrapperPipelineState *)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.rootSignature = ObjectRS::get();
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
  psodesc.pRootSignature = result->D3DValue.rootSignature;
  psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
  psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  psodesc.NumRenderTargets = 2;
  psodesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
  psodesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  psodesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

  psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
  result->D3DValue.pipelineStateObject =
      PipelineStateObject<VertexLayout<irr::video::S3DVertex2TCoords>>::get(
          psodesc, L"Debug\\object.cso", L"Debug\\object_gbuffer.cso");
  return result;
}

typedef RootSignature<
    D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    DescriptorTable<ConstantsBufferResource<0>, ConstantsBufferResource<1>>,
    DescriptorTable<ShaderResource<0>>, DescriptorTable<SamplerResource<0>>>
    SkinnedObjectRS;

struct WrapperPipelineState *createSkinnedObjectShader() {
  WrapperPipelineState *result =
      (WrapperPipelineState *)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.rootSignature = SkinnedObjectRS::get();
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
  psodesc.pRootSignature = result->D3DValue.rootSignature;
  psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
  psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  psodesc.NumRenderTargets = 2;
  psodesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
  psodesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  psodesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

  psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
  result->D3DValue.pipelineStateObject =
      PipelineStateObject<VertexLayout<irr::video::S3DVertex2TCoords,
                                       irr::video::SkinnedVertexData>>::
          get(psodesc, L"Debug\\skinnedobject.cso",
              L"Debug\\object_gbuffer.cso");
  return result;
}

typedef RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
                      DescriptorTable<ConstantsBufferResource<0>>,
                      DescriptorTable<ShaderResource<0>>,
                      DescriptorTable<SamplerResource<0>>>
    SkyboxRS;

struct WrapperPipelineState *createSkyboxShader() {
  WrapperPipelineState *result =
      (WrapperPipelineState *)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.rootSignature = SkyboxRS::get();
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
  psodesc.pRootSignature = result->D3DValue.rootSignature;
  psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
  psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  psodesc.NumRenderTargets = 1;
  psodesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
  psodesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psodesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_LESS_EQUAL;
  psodesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

  psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);

  result->D3DValue.pipelineStateObject =
      PipelineStateObject<VertexLayout<irr::video::ScreenQuadVertex>>::get(
          psodesc, L"Debug\\skyboxvert.cso", L"Debug\\skybox.cso");
  return result;
}

typedef RootSignature<D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
                      DescriptorTable<ConstantsBufferResource<0>>,
                      DescriptorTable<ShaderResource<1>>,
                      DescriptorTable<ShaderResource<0>>,
                      DescriptorTable<SamplerResource<0>>>
    ImportanceSamplingForSpecularCubemapRS;

struct WrapperPipelineState *ImportanceSamplingForSpecularCubemap() {
  WrapperPipelineState *result =
      (WrapperPipelineState *)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.rootSignature =
      ImportanceSamplingForSpecularCubemapRS::get();
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
  psodesc.pRootSignature = result->D3DValue.rootSignature;
  psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
  psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  psodesc.NumRenderTargets = 1;
  psodesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
  psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psodesc.DepthStencilState.DepthEnable = false;

  psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);

  result->D3DValue.pipelineStateObject =
      PipelineStateObject<VertexLayout<irr::video::ScreenQuadVertex>>::get(
          psodesc, L"Debug\\screenquad.cso",
          L"Debug\\importance_sampling_specular.cso");
  return result;
}

typedef RootSignature<
    D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    DescriptorTable<ConstantsBufferResource<0>, ShaderResource<0>,
                    ShaderResource<1>, ShaderResource<2>>,
    DescriptorTable<ShaderResource<3>, ShaderResource<4>>,
    DescriptorTable<SamplerResource<0>, SamplerResource<1>, SamplerResource<2>,
                    SamplerResource<3>, SamplerResource<4>>>
    IBLRS;

struct WrapperPipelineState *createIBLShader() {
  WrapperPipelineState *result =
      (WrapperPipelineState *)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.rootSignature = IBLRS::get();
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
  psodesc.pRootSignature = result->D3DValue.rootSignature;
  psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
  psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  psodesc.NumRenderTargets = 1;
  psodesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
  psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psodesc.DepthStencilState.DepthEnable = false;
  psodesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

  psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);

  result->D3DValue.pipelineStateObject =
      PipelineStateObject<VertexLayout<irr::video::ScreenQuadVertex>>::get(
          psodesc, L"Debug\\screenquad.cso", L"Debug\\ibl.cso");
  return result;
}

typedef RootSignature<
    D3D12_ROOT_SIGNATURE_NONE,
    DescriptorTable<ConstantsBufferResource<0>, ShaderResource<0>>,
    DescriptorTable<UAVResource<0>>, DescriptorTable<SamplerResource<0>>>
    ComputeSHRS;

struct WrapperPipelineState *createComputeSHShader() {
  WrapperPipelineState *result =
      (WrapperPipelineState *)malloc(sizeof(WrapperPipelineState));
  result->D3DValue.rootSignature = ComputeSHRS::get();
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
  psodesc.pRootSignature = result->D3DValue.rootSignature;
  psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
  psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  psodesc.NumRenderTargets = 1;
  psodesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
  psodesc.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psodesc.DepthStencilState.DepthEnable = false;
  psodesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

  psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);

  result->D3DValue.pipelineStateObject =
      PipelineStateObject<VertexLayout<irr::video::ScreenQuadVertex>>::get(
          psodesc, L"Debug\\screenquad.cso", L"Debug\\computesh.cso");
  return nullptr;
}