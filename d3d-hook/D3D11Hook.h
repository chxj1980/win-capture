#ifndef _D3D11_HOOK_H_
#define _D3D11_HOOK_H_

#include <stdio.h>
#include <d3d11.h>
#include <D3Dcompiler.h>
#include <wrl.h>
#include <mutex>
#include <memory>

namespace hook
{

struct D3D11TextureInfo
{
	int width  = 0;
	int height = 0;	
	int format = 0;
	int timestamp = 0;
	int lock = 1;
	HANDLE handle = NULL;
};

class D3D11Hook
{
public:
	static void Attach();
	static void Detach();

	~D3D11Hook();
	static D3D11Hook& instance();

	bool Init(IDXGISwapChain *swapChain);
	void Exit();
	void Capture(ID3D11Resource *backbuffer);
	bool GetTextureInfo(D3D11TextureInfo *textureInfo);

private:
	bool InitTetxure();

	IDXGISwapChain* m_swapChain = NULL;
	ID3D11Device* m_device = NULL;
	ID3D11DeviceContext* m_context = NULL;
	
	int m_width = 0;
	int m_height = 0;
	int m_timestamp = 0;
	DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;

	std::mutex m_mutex;
	ID3D11Texture2D* m_texture = NULL;
	IDXGIKeyedMutex* m_keyedMutex = NULL;
	int m_lock = 0;
	HANDLE m_handle = NULL;
};

}

#endif
