#pragma once


#include <Windows.h>

#include "DDef.h"
#include "DLocale.h"



class C_SECURITY_ATTRIBUTES
	: public _SECURITY_ATTRIBUTES
{
public:
	C_SECURITY_ATTRIBUTES(LPVOID _p)
	{
		nLength = sizeof(_SECURITY_ATTRIBUTES);
		lpSecurityDescriptor = _p;
		bInheritHandle = TRUE;
	}
};

#if (_MSC_VER > 1600) && (__cplusplus >= 201103L)	// vs2012 + c++11 이상
constexpr DWORD _MAX_SIZE_PIPE_BUFFER_ = (1 << 14);
#else
#define _MAX_SIZE_PIPE_BUFFER_	(1 << 14)		// 32768
#endif
namespace dk
{
	class C_PIPE
	{
	private:
		LPVOID pSecurityDescriptorRecv;
		LPVOID pSecurityDescriptorSend;

		bool InitSecurityDescriptor();

	protected:
		HANDLE hPipeRecv;
		HANDLE hPipeSend;

	public:
		C_PIPE();
		~C_PIPE();

		bool Accept(LPCWSTR _pwszRecv, LPCWSTR _pwszSend);
		bool Accept(LPCSTR _pwszRecv, LPCSTR _pwszSend);
		bool Connect(LPCWSTR _pwszRecv, LPCWSTR _pwszSend);
		bool Connect(LPCSTR _pwszRecv, LPCSTR _pwszSend);
		void Destroy();
	};
}