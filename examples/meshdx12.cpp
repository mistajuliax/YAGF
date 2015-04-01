#include <windows.h>
#include <windowsx.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <wrl/client.h>


#include <Loaders/B3D.h>
#include <tuple>

std::vector<std::tuple<size_t, size_t, size_t> > CountBaseIndexVTX;

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

using namespace Microsoft::WRL; // For ComPtr

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam);

ComPtr<ID3D12Device> dev;
ComPtr<ID3D12CommandQueue> cmdqueue;
ComPtr<IDXGISwapChain3> chain;
ComPtr<ID3D12CommandAllocator> cmdalloc;
ComPtr<ID3D12RootSignature> pRootSignature;
ComPtr<ID3D12GraphicsCommandList> cmdlist;
ComPtr<ID3D12Fence> fence;
ComPtr<ID3D12PipelineState> pso;
ComPtr<ID3D12Resource> vertexbuffer;
ComPtr<ID3D12Resource> indexbuffer;
ComPtr<ID3D12Resource> buffer[2];
D3D12_CPU_DESCRIPTOR_HANDLE cpudesc[2];
ComPtr<ID3D12DescriptorHeap> descheap;
HANDLE handle;
ComPtr<ID3D12Resource> cbuffer;
ComPtr<ID3D12DescriptorHeap> CbufferHeap;
D3D12_VERTEX_BUFFER_VIEW vtxb = {};
D3D12_INDEX_BUFFER_VIEW idxb = {};

void InitD3D(HWND hWnd)
{
	HRESULT hr = D3D12CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, D3D12_CREATE_DEVICE_DEBUG, D3D_FEATURE_LEVEL_11_1, D3D12_SDK_VERSION, IID_PPV_ARGS(&dev));

	D3D12_COMMAND_QUEUE_DESC cmddesc = {};
	cmddesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = dev->CreateCommandQueue(&cmddesc, IID_PPV_ARGS(&cmdqueue));

	DXGI_SWAP_CHAIN_DESC swapChain = {};
	swapChain.BufferCount = 2;
	swapChain.Windowed = true;
	swapChain.OutputWindow = hWnd;
	swapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChain.SampleDesc.Count = 1;
	swapChain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	ComPtr<IDXGIFactory> fact;
	hr = CreateDXGIFactory(IID_PPV_ARGS(&fact));
	hr = fact->CreateSwapChain(cmdqueue.Get(), &swapChain, (IDXGISwapChain**)chain.GetAddressOf());

	hr = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdalloc));
	hr = dev->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdalloc.Get(), 0, IID_PPV_ARGS(&cmdlist));


	D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
	heapdesc.NumDescriptors = 2;
	heapdesc.Type = D3D12_RTV_DESCRIPTOR_HEAP;

	hr = dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&descheap));

	cpudesc[0] = descheap->GetCPUDescriptorHandleForHeapStart();
	cpudesc[1] = descheap->GetCPUDescriptorHandleForHeapStart().MakeOffsetted(dev->GetDescriptorHandleIncrementSize(D3D12_RTV_DESCRIPTOR_HEAP));
	hr = chain->GetBuffer(0, IID_PPV_ARGS(&buffer[0]));
	dev->CreateRenderTargetView(buffer[0].Get(), nullptr, cpudesc[0]);
	hr = chain->GetBuffer(1, IID_PPV_ARGS(&buffer[1]));
	dev->CreateRenderTargetView(buffer[1].Get(), nullptr, cpudesc[1]);
	buffer[0]->SetName(L"Buffer0");
	buffer[1]->SetName(L"Buffer1");


	handle = CreateEvent(0, FALSE, FALSE, 0);
	hr = dev->CreateFence(0, D3D12_FENCE_MISC_NONE, IID_PPV_ARGS(&fence));

	// Create PSO
	{
		ComPtr<ID3DBlob> vtxshaderblob;
		hr = D3DReadFileToBlob(L"Debug\\vtx.cso", &vtxshaderblob);
		ComPtr<ID3DBlob> pxshaderblob;
		hr = D3DReadFileToBlob(L"Debug\\pix.cso", &pxshaderblob);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc = {};
		psodesc.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
		psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
/*		// Position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex2TCoords), 0);
		// Normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex2TCoords), (GLvoid*)12);
		// Color
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(irr::video::S3DVertex2TCoords), (GLvoid*)24);
		// Texcoord
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex2TCoords), (GLvoid*)28);
		// SecondTexcoord
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(irr::video::S3DVertex2TCoords), (GLvoid*)36);*/
		D3D12_INPUT_ELEMENT_DESC IAdesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 2, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D12_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 3, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_PER_VERTEX_DATA, 0 },
			{ "SECONDTEXCOORD", 4, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_PER_VERTEX_DATA, 0 },
		};
		psodesc.InputLayout.pInputElementDescs = IAdesc;
		psodesc.InputLayout.NumElements = 5;
		psodesc.NumRenderTargets = 1;
		psodesc.VS.BytecodeLength = vtxshaderblob->GetBufferSize();
		psodesc.VS.pShaderBytecode = vtxshaderblob->GetBufferPointer();
		psodesc.PS.BytecodeLength = pxshaderblob->GetBufferSize();
		psodesc.PS.pShaderBytecode = pxshaderblob->GetBufferPointer();
		psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psodesc.SampleDesc.Count = 1;
		psodesc.SampleMask = UINT_MAX;
		psodesc.NodeMask = 1;
		psodesc.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);

		hr = dev->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(&pso));
	}

	hr = dev->CreateCommittedResource(
		&CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_MISC_NONE,
		&CD3D12_RESOURCE_DESC::Buffer(256),
		D3D12_RESOURCE_USAGE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&cbuffer));

	float *tmp;
	cbuffer->Map(0, nullptr, (void**)&tmp);
	float white[4] = { 1.f, 1.f, 1.f, 1.f };
	memcpy(tmp, white, 4 * sizeof(float));
	cbuffer->Unmap(0, nullptr);

	{
		D3D12_DESCRIPTOR_HEAP_DESC heapdesc = {};
		heapdesc.Type = D3D12_CBV_SRV_UAV_DESCRIPTOR_HEAP;
		heapdesc.NumDescriptors = 1;
		heapdesc.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
		hr = dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&CbufferHeap));

		D3D12_CONSTANT_BUFFER_VIEW_DESC bufdesc = {};
		bufdesc.BufferLocation = cbuffer->GetGPUVirtualAddress();
		bufdesc.SizeInBytes = 256;
		dev->CreateConstantBufferView(&bufdesc, CbufferHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Define Root Signature
	{
		D3D12_DESCRIPTOR_RANGE descrange = {};
		descrange.Init(D3D12_DESCRIPTOR_RANGE_CBV, 1, 0);

		D3D12_ROOT_PARAMETER RP;
		RP.InitAsDescriptorTable(1, &descrange);

		D3D12_ROOT_SIGNATURE RootSig = D3D12_ROOT_SIGNATURE(1, &RP);
		RootSig.Flags = D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		ComPtr<ID3DBlob> pSerializedRootSig;
		hr = D3D12SerializeRootSignature(&RootSig, D3D_ROOT_SIGNATURE_V1, &pSerializedRootSig, nullptr);

		hr = dev->CreateRootSignature(1,
			pSerializedRootSig->GetBufferPointer(), pSerializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&pRootSignature));
	}

	irr::io::CReadFile *reader = new irr::io::CReadFile("..\\examples\\anchor.b3d");
	irr::scene::CB3DMeshFileLoader *loader = new irr::scene::CB3DMeshFileLoader();
	std::vector<irr::scene::SMeshBufferLightMap> buffers = loader->createMesh(reader);

	{
//		std::pair<size_t, size_t> BaseIndexVtx = VertexArrayObject<FormattedVertexStorage<irr::video::S3DVertex2TCoords> >::getInstance()->getBase(&tmp);
		CountBaseIndexVTX.push_back(std::make_tuple(buffers[0].getIndexCount(), 0, 0));
	}

	D3D12_RESOURCE_BARRIER_DESC barrier = {};
	// Upload to gpudata
	{
		ComPtr<ID3D12Resource> vertexdata, indexdata;
		hr = dev->CreateCommittedResource(
			&CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_MISC_NONE,
			&CD3D12_RESOURCE_DESC::Buffer(buffers[0].getVertexCount() * sizeof(irr::video::S3DVertex2TCoords)),
			D3D12_RESOURCE_USAGE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexdata));

		void *tmp;
		hr = vertexdata->Map(0, nullptr, &tmp);
		memcpy(tmp, buffers[0].getVertices(), buffers[0].getVertexCount() * sizeof(irr::video::S3DVertex2TCoords));
		vertexdata->Unmap(0, nullptr);

		hr = dev->CreateCommittedResource(
			&CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_MISC_NONE,
			&CD3D12_RESOURCE_DESC::Buffer(buffers[0].getIndexCount() * sizeof(unsigned short)),
			D3D12_RESOURCE_USAGE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&indexdata));

		hr = indexdata->Map(0, nullptr, &tmp);
		memcpy(tmp, buffers[0].getIndices(), buffers[0].getIndexCount() * sizeof(unsigned short));
		indexdata->Unmap(0, nullptr);

		hr = dev->CreateCommittedResource(
			&CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_MISC_NONE,
			&CD3D12_RESOURCE_DESC::Buffer(buffers[0].getVertexCount() * sizeof(irr::video::S3DVertex2TCoords)),
			D3D12_RESOURCE_USAGE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&vertexbuffer));

		hr = dev->CreateCommittedResource(
			&CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_MISC_NONE,
			&CD3D12_RESOURCE_DESC::Buffer(buffers[0].getIndexCount() * sizeof(unsigned short)),
			D3D12_RESOURCE_USAGE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&indexbuffer));

		cmdlist->CopyBufferRegion(vertexbuffer.Get(), 0, vertexdata.Get(), 0, buffers[0].getVertexCount() * sizeof(irr::video::S3DVertex2TCoords), D3D12_COPY_NONE);
		cmdlist->CopyBufferRegion(indexbuffer.Get(), 0, indexdata.Get(), 0, buffers[0].getIndexCount() * sizeof(unsigned short), D3D12_COPY_NONE);

		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = vertexbuffer.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_GENERIC_READ;
		cmdlist->ResourceBarrier(1, &barrier);
		barrier.Transition.pResource = indexbuffer.Get();
		cmdlist->ResourceBarrier(1, &barrier);

		ComPtr<ID3D12Fence> datauploadfence;
		hr = dev->CreateFence(0, D3D12_FENCE_MISC_NONE, IID_PPV_ARGS(&datauploadfence));
		HANDLE cpudatauploadevent = CreateEvent(0, FALSE, FALSE, 0);
		datauploadfence->SetEventOnCompletion(1, cpudatauploadevent);
		cmdlist->Close();
		cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());
		cmdqueue->Signal(datauploadfence.Get(), 1);
		WaitForSingleObject(cpudatauploadevent, INFINITE);
		cmdalloc->Reset();
		cmdlist->Reset(cmdalloc.Get(), pso.Get());

		vtxb.BufferLocation = vertexbuffer->GetGPUVirtualAddress();
		vtxb.SizeInBytes = buffers[0].getVertexCount() * sizeof(irr::video::S3DVertex2TCoords);
		vtxb.StrideInBytes = sizeof(irr::video::S3DVertex2TCoords);

		idxb.BufferLocation = indexbuffer->GetGPUVirtualAddress();
		idxb.Format = DXGI_FORMAT_R16_UINT;
		idxb.SizeInBytes = buffers[0].getIndexCount() * sizeof(unsigned short);
	}
}

void Draw()
{
	D3D12_RECT rect = {};
	rect.left = 0;
	rect.top = 0;
	rect.bottom = 300;
	rect.right = 300;

	D3D12_VIEWPORT view = {};
	view.Height = 300;
	view.Width = 300;
	view.TopLeftX = 0;
	view.TopLeftY = 0;
	view.MinDepth = 0;
	view.MaxDepth = 1.;

	cmdlist->RSSetViewports(1, &view);
	cmdlist->RSSetScissorRects(1, &rect);

	D3D12_RESOURCE_BARRIER_DESC barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = buffer[chain->GetCurrentBackBufferIndex()].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_RENDER_TARGET;
	cmdlist->ResourceBarrier(1, &barrier);

	float clearColor[] = { .25f, .25f, 0.35f, 1.0f };
	cmdlist->ClearRenderTargetView(cpudesc[chain->GetCurrentBackBufferIndex()], clearColor, 0, 0);



//	cmdlist->SetDescriptorHeaps(CbufferHeap.GetAddressOf(), 1);
	cmdlist->SetGraphicsRootSignature(pRootSignature.Get());
	float c[] = { 1., 1., 1., 1. };
	cmdlist->SetGraphicsRootDescriptorTable(0, CbufferHeap->GetGPUDescriptorHandleForHeapStart());
	cmdlist->SetRenderTargets(&cpudesc[chain->GetCurrentBackBufferIndex()], true, 1, nullptr);
	cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdlist->SetIndexBuffer(&idxb);
	cmdlist->SetVertexBuffers(0, &vtxb, 1);
	cmdlist->DrawIndexedInstanced(idxb.SizeInBytes / sizeof(unsigned short), 1, 0, 0, 0);

	barrier.Transition.StateBefore = D3D12_RESOURCE_USAGE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_USAGE_PRESENT;
	cmdlist->ResourceBarrier(1, &barrier);
	HRESULT hr = cmdlist->Close();
	cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)cmdlist.GetAddressOf());

	hr = fence->Signal(0);
	hr = fence->SetEventOnCompletion(1, handle);
	hr = cmdqueue->Signal(fence.Get(), 1);
	WaitForSingleObject(handle, INFINITE);
	cmdalloc->Reset();
	cmdlist->Reset(cmdalloc.Get(), pso.Get());
	hr = chain->Present(1, 0);
}

void Clean()
{

}

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	// the handle for the window, filled by a function
	HWND hWnd;
	// this struct holds information for the window class
	WNDCLASSEX wc = {};

	// fill in the struct with the needed information
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "WindowClass1";

	// register the window class
	RegisterClassEx(&wc);

	// create the window and use the result as the handle
	hWnd = CreateWindowEx(NULL,
		"WindowClass1",    // name of the window class
		"MeshDX12",   // title of the window
		WS_OVERLAPPEDWINDOW,    // window style
		300,    // x-position of the window
		300,    // y-position of the window
		1024,    // width of the window
		1024,    // height of the window
		NULL,    // we have no parent window, NULL
		NULL,    // we aren't using menus, NULL
		hInstance,    // application handle
		NULL);    // used with multiple windows, NULL
	ShowWindow(hWnd, nCmdShow);

	// this struct holds Windows event messages
	MSG msg;

	InitD3D(hWnd);

	// wait for the next message in the queue, store the result in 'msg'
	while (GetMessage(&msg, NULL, 0, 0))
	{
		// translate keystroke messages into the right format
		TranslateMessage(&msg);

		// send the message to the WindowProc function
		DispatchMessage(&msg);

		Draw();
	}

	Clean();
	// return this part of the WM_QUIT message to Windows
	return msg.wParam;
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// sort through and find what code to run for the message given
	switch (message)
	{
		// this message is read when the window is closed
	case WM_DESTROY:
	{
		// close the application entirely
		PostQuitMessage(0);
		return 0;
	} break;
	}

	// Handle any messages the switch statement didn't
	return DefWindowProc(hWnd, message, wParam, lParam);
}