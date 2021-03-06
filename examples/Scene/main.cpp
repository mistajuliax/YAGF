#include <Loaders/DDS.h>
#include <MeshManager.h>
#include <MeshSceneNode.h>
#include <Scene.h>
#include <Scene/FullscreenPass.h>
#include <Scene/IBL.h>
#include <Scene/RenderTargets.h>

#ifdef GLBUILD
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <API/glapi.h>
#include <GLAPI/Debug.h>
#endif

#ifdef DXBUILD
#include <API/d3dapi.h>
#include <D3DAPI/Context.h>
#include <D3DAPI/Misc.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#endif

RenderTargets *rtts;

Scene *scnmgr;
FullscreenPassManager *fspassmgr;
irr::scene::ISceneNode *xue;
WrapperResource *cubemap, *probe, *dfg, *IBLCoeffs;
WrapperDescriptorHeap *skyboxTextureHeap, *probeHeap;

#ifdef DXBUILD
GFXAPI *GlobalGFXAPI = new D3DAPI();
#endif

#ifdef GLBUILD
GFXAPI *GlobalGFXAPI = new GLAPI();
#endif

void init() {
#ifdef GLBUILD
  DebugUtil::enableDebugOutput();
  glDepthFunc(GL_LEQUAL);
#endif
  std::vector<std::string> xueB3Dname = {"..\\examples\\assets\\xue.b3d"};

  scnmgr = new Scene();

  MeshManager::getInstance()->LoadMesh(xueB3Dname);
  xue = scnmgr->addMeshSceneNode(
      MeshManager::getInstance()->getMesh(xueB3Dname[0]), nullptr,
      irr::core::vector3df(0.f, 0.f, 2.f));

  rtts = new RenderTargets(1024, 1024);
  fspassmgr = new FullscreenPassManager(*rtts);

  cubemap = (WrapperResource *)malloc(sizeof(WrapperResource));
  dfg = (WrapperResource *)malloc(sizeof(WrapperResource));
  const std::string &fixed = "..\\examples\\assets\\w_sky_1.dds";
  std::ifstream DDSFile(fixed, std::ifstream::binary);
  irr::video::CImageLoaderDDS DDSPic(DDSFile);

#if DXBUILD
  ID3D12Resource *SkyboxTexture;
  D3DTexture TexInRam(DDSPic.getLoadedImage());
  const IImage &Image = DDSPic.getLoadedImage();

  HRESULT hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Tex2D(
          getDXGIFormatFromColorFormat(Image.Format), (UINT)TexInRam.getWidth(),
          (UINT)TexInRam.getHeight(), 6, (UINT)TexInRam.getMipLevelsCount()),
      D3D12_RESOURCE_USAGE_GENERIC_READ, nullptr, IID_PPV_ARGS(&SkyboxTexture));

  WrapperCommandList *uploadcmdlist = GlobalGFXAPI->createCommandList();
  GlobalGFXAPI->openCommandList(uploadcmdlist);

  TexInRam.CreateUploadCommandToResourceInDefaultHeap(
      uploadcmdlist->D3DValue.CommandList, SkyboxTexture);

  D3D12_SHADER_RESOURCE_VIEW_DESC resdesc = {};
  resdesc.Format = getDXGIFormatFromColorFormat(Image.Format);
  resdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
  resdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  resdesc.TextureCube.MipLevels = 10;

  cubemap->D3DValue.resource = SkyboxTexture;
  cubemap->D3DValue.description.TextureView.SRV = resdesc;

  cubemap->D3DValue.resource = SkyboxTexture;
  cubemap->D3DValue.description.TextureView.SRV = resdesc;

  ID3D12Resource *DFGTexture;
  D3DTexture DFGInRam(getDFGLUT());

  hr = Context::getInstance()->dev->CreateCommittedResource(
      &CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_MISC_NONE,
      &CD3D12_RESOURCE_DESC::Tex2D(DFGInRam.getFormat(),
                                   (UINT)DFGInRam.getWidth(),
                                   (UINT)DFGInRam.getHeight(), 1, 0),
      D3D12_RESOURCE_USAGE_GENERIC_READ, nullptr, IID_PPV_ARGS(&DFGTexture));

  DFGInRam.CreateUploadCommandToResourceInDefaultHeap(
      uploadcmdlist->D3DValue.CommandList, DFGTexture);

  D3D12_SHADER_RESOURCE_VIEW_DESC resdesc_dfg = {};
  resdesc_dfg.Format = DFGInRam.getFormat();
  resdesc_dfg.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  resdesc_dfg.Shader4ComponentMapping =
      D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  resdesc_dfg.TextureCube.MipLevels = 1;

  dfg->D3DValue.resource = DFGTexture;
  dfg->D3DValue.description.TextureView.SRV = resdesc_dfg;

  GlobalGFXAPI->closeCommandList(uploadcmdlist);
  GlobalGFXAPI->submitToQueue(uploadcmdlist);
  HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);
  GlobalGFXAPI->releaseCommandList(uploadcmdlist);
#endif

#ifdef GLBUILD
  GLTexture *Tex = new GLTexture(DDSPic.getLoadedImage());
  cubemap->GLValue.Resource = Tex->Id;
  cubemap->GLValue.Type = GL_TEXTURE_CUBE_MAP;

  GLTexture *DFGTex = new GLTexture(getDFGLUT());
  dfg->GLValue.Resource = DFGTex->Id;
  dfg->GLValue.Type = GL_TEXTURE_2D;
#endif
  SHCoefficients coeffs = computeSphericalHarmonics(
      cubemap, DDSPic.getLoadedImage().Layers[0][0].Width);
  IBLCoeffs = GlobalGFXAPI->createConstantsBuffer(9 * 3 * sizeof(float));

  float *SHbuffers = (float *)GlobalGFXAPI->mapConstantsBuffer(IBLCoeffs);
  memcpy(&SHbuffers[0], coeffs.Blue, 9 * sizeof(float));
  memcpy(&SHbuffers[9], coeffs.Green, 9 * sizeof(float));
  memcpy(&SHbuffers[18], coeffs.Red, 9 * sizeof(float));
  GlobalGFXAPI->unmapConstantsBuffers(IBLCoeffs);

  probe = generateSpecularCubemap(cubemap);

  skyboxTextureHeap = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap(
      {std::make_tuple(cubemap, RESOURCE_VIEW::SHADER_RESOURCE, 0)});

  probeHeap = GlobalGFXAPI->createCBVSRVUAVDescriptorHeap(
      {std::make_tuple(IBLCoeffs, RESOURCE_VIEW::CONSTANTS_BUFFER, 1),
       std::make_tuple(probe, RESOURCE_VIEW::SHADER_RESOURCE, 3),
       std::make_tuple(dfg, RESOURCE_VIEW::SHADER_RESOURCE, 4)});
}

void clean() {
  TextureManager::getInstance()->kill();
  MeshManager::getInstance()->kill();
  delete rtts;
  delete scnmgr;
  delete fspassmgr;
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(skyboxTextureHeap);
  GlobalGFXAPI->releaseCBVSRVUAVDescriptorHeap(probeHeap);
  GlobalGFXAPI->releaseRTTOrDepthStencilTexture(cubemap);
  GlobalGFXAPI->releaseRTTOrDepthStencilTexture(dfg);
  GlobalGFXAPI->releaseRTTOrDepthStencilTexture(probe);
  GlobalGFXAPI->releaseConstantsBuffers(IBLCoeffs);
#ifdef DXBUILD
  Context::getInstance()->kill();
#endif
}

static float timer = 0.;

void draw() {
  xue->setRotation(irr::core::vector3df(0.f, timer / 360.f, 0.f));
  scnmgr->update();
  scnmgr->renderGBuffer(nullptr, *rtts);
  GlobalGFXAPI->openCommandList(fspassmgr->CommandList);
  fspassmgr->renderIBL(probeHeap);
  fspassmgr->renderSunlight();
  fspassmgr->renderSky(skyboxTextureHeap);
  fspassmgr->renderTonemap();
  GlobalGFXAPI->closeCommandList(fspassmgr->CommandList);
  GlobalGFXAPI->submitToQueue(fspassmgr->CommandList);

#ifdef DXBUILD
  Context::getInstance()->Swap();
  HANDLE handle = getCPUSyncHandle(Context::getInstance()->cmdqueue.Get());
  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);
#endif

  timer += 16.f;
}

#ifdef GLBUILD
int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  GLFWwindow *window = glfwCreateWindow(1024, 1024, "GLtest", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glewExperimental = GL_TRUE;
  glewInit();
  init();

  while (!glfwWindowShouldClose(window)) {
    draw();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  clean();
  glfwTerminate();
  return 0;
}
#endif

#ifdef DXBUILD
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  Context::getInstance()->InitD3D(
      WindowUtil::Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow));
  init();
  // this struct holds Windows event messages
  MSG msg = {};

  // Loop from
  // https://msdn.microsoft.com/en-us/library/windows/apps/dn166880.aspx
  while (WM_QUIT != msg.message) {
    bool bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);

    if (bGotMsg) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    draw();
  }

  clean();
  // return this part of the WM_QUIT message to Windows
  return (int)msg.wParam;
}
#endif