#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "D3D11Hook.h"
#include "Logger.h"
#include "detours/detours.h"
#include <windows.h>
#include <stdio.h>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

static IDXGISwapChain* swapChain = NULL;
static ID3D11Device* d3d11Device = NULL;
static ID3D11DeviceContext* d3d11Context = NULL;

typedef HRESULT(__stdcall *OrginalD3D11Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
static OrginalD3D11Present orginalD3D11Present = NULL;


HRESULT __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool first = true;
	if (first)
	{
		first = false;
		if (D3D11Hook::instance().Init(pSwapChain))
		{
			D3D11TextureInfo textureInfo;
			D3D11Hook::instance().GetTextureInfo(&textureInfo);
			LOG_INFO("[D3D11-Hook] d3d11-capture init succeed.");
		}
		else
		{
			LOG_ERROR("[D3D11-Hook] d3d11-capture init failed.");
		}
	}
	else
	{
		ID3D11Texture2D* backBuffer = nullptr;
		HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
		if (!FAILED(hr))
		{
			if (backBuffer != NULL)
			{
				D3D11Hook::instance().Capture(backBuffer);
				backBuffer->Release();
			}
		}
	}

	return orginalD3D11Present(pSwapChain, SyncInterval, Flags);
}

LRESULT CALLBACK DXGIMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return DefWindowProc(hwnd, uMsg, wParam, lParam); }

void D3D11Hook::Attach()
{
	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassExA(&wc);
	HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);

	D3D_FEATURE_LEVEL featrueLevel = D3D_FEATURE_LEVEL_11_1;
	D3D_FEATURE_LEVEL obtainedLevel;

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = 1;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Windowed = ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;
	scd.BufferDesc.Width = 1;
	scd.BufferDesc.Height = 1;
	scd.BufferDesc.RefreshRate.Numerator = 0;
	scd.BufferDesc.RefreshRate.Denominator = 1;

	UINT createFlags = 0;

	if (FAILED(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createFlags,
		&featrueLevel,
		1,
		D3D11_SDK_VERSION,
		&scd,
		&swapChain,
		&d3d11Device,
		&obtainedLevel,
		&d3d11Context)))
	{
		swapChain = NULL;
		return ;
	}

	DWORD_PTR* swapChainVtable = NULL;
	swapChainVtable = (DWORD_PTR *)swapChain;
	swapChainVtable = (DWORD_PTR *)swapChainVtable[0];

	orginalD3D11Present = (OrginalD3D11Present)(DWORD_PTR*)swapChainVtable[8];

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)orginalD3D11Present, (PBYTE)hookD3D11Present);

	DetourTransactionCommit();
}

void D3D11Hook::Detach()
{
	if (swapChain == NULL)
	{
		return;
	}

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(LPVOID&)orginalD3D11Present, (PBYTE)hookD3D11Present);

	DetourTransactionCommit();
}

D3D11Hook& D3D11Hook::instance()
{
	static D3D11Hook s_d3d11Hook;
	return s_d3d11Hook;
}

bool D3D11Hook::Init(IDXGISwapChain *swapChain)
{
	HRESULT hr;
	hr = swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&m_device);
	if (FAILED(hr))
	{
		LOG_ERROR("[D3D11-Hook] IDXGISwapChain::GetDevice() failed: %lu", GetLastError());
		return false;
	}

	m_device->GetImmediateContext(&m_context);
	if (m_context == NULL)
	{
		LOG_ERROR("[D3D11-Hook] ID3D11Device::GetImmediateContext() failed: %lu", GetLastError());
		return false;
	}

	DXGI_SWAP_CHAIN_DESC desc;
	memset(&desc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
	hr = swapChain->GetDesc(&desc);
	if (FAILED(hr))
	{
		LOG_ERROR("[D3D11-Hook] IDXGISwapChain::GetDesc() failed: %lu", GetLastError());
		return false;
	}
	else
	{
		m_format = desc.BufferDesc.Format;
		m_height = desc.BufferDesc.Height;
		m_width = desc.BufferDesc.Width;
	}

	if (!InitTetxure())
	{
		return false;
	}

	return true;
}

bool D3D11Hook::InitTetxure()
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = m_width;
	desc.Height = m_height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = m_format;
	desc.BindFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED; 

	HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &m_texture);
	if (FAILED(hr))
	{
		LOG_ERROR("[D3D11-Hook] ID3D11Device::CreateTexture2D() failed: %lu", GetLastError());
		return false;
	}

	IDXGIResource *dxgiRes;
	hr = m_texture->QueryInterface(__uuidof(IDXGIResource), (void**)&dxgiRes);
	if (FAILED(hr))
	{
		LOG_ERROR("[D3D11-Hook] ID3D11Texture2D::QueryInterface() failed: %lu", GetLastError());
		return false;
	}

	hr = dxgiRes->GetSharedHandle(&m_handle);
	dxgiRes->Release();
	if (FAILED(hr))
	{
		LOG_ERROR("[D3D11-Hook] IDXGIResource::GetSharedHandle() failed: %lu", GetLastError());
		return false;
	}

	return true;
}

void D3D11Hook::Exit()
{
	std::lock_guard<std::mutex> locker(m_mutex);
	if (m_texture != NULL)
	{
		m_texture->Release();
		m_texture = NULL;
	}
}

void D3D11Hook::Capture(ID3D11Resource *backbuffer)
{
	if (!swapChain || !m_texture)
	{
		return ;
	}

	if (m_context != NULL)
	{
		std::lock_guard<std::mutex> locker(m_mutex);
		m_context->CopyResource(m_texture, backbuffer);
		m_timestamp += 1;
	}
}

bool D3D11Hook::GetTextureInfo(D3D11TextureInfo *textureInfo)
{
	if (!swapChain || !m_texture)
	{
		return false;
	}

	textureInfo->width = m_width;
	textureInfo->height = m_height;
	textureInfo->format = m_format;
	textureInfo->handle = m_handle;
	textureInfo->timestamp = m_timestamp;
	return true;
}