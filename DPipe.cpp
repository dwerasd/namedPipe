#include "stdafx.h"
#include "DPipe.h"
#include "DPrint.h"



namespace dk
{
	C_PIPE::C_PIPE()
		: hPipeRecv(INVALID_HANDLE_VALUE)
		, hPipeSend(INVALID_HANDLE_VALUE)
		, pSecurityDescriptorRecv(nullptr)
		, pSecurityDescriptorSend(nullptr)
	{
		
		
	}
	C_PIPE::~C_PIPE()
	{
		Destroy();
	}

	bool C_PIPE::InitSecurityDescriptor()
	{
		bool bResult = false;
		do
		{
			if (!pSecurityDescriptorRecv)
			{
				pSecurityDescriptorRecv = ::LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
				if (!pSecurityDescriptorRecv)
				{
					break;
				}
				if (!::InitializeSecurityDescriptor(pSecurityDescriptorRecv, SECURITY_DESCRIPTOR_REVISION))
				{
					break;
				}
				if (!::SetSecurityDescriptorDacl(pSecurityDescriptorRecv, TRUE, (PACL)NULL, FALSE))
				{
					break;
				}
			}
			if (!pSecurityDescriptorSend)
			{
				pSecurityDescriptorSend = ::LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
				if (!pSecurityDescriptorSend)
				{
					break;
				}
				if (!::InitializeSecurityDescriptor(pSecurityDescriptorSend, SECURITY_DESCRIPTOR_REVISION))
				{
					break;
				}
				if (!::SetSecurityDescriptorDacl(pSecurityDescriptorSend, TRUE, (PACL)NULL, FALSE))
				{
					break;
				}
			}
			bResult = true;
		} while (false);
		return(bResult);
	}

	bool C_PIPE::Accept(LPCWSTR _pwszRecv, LPCWSTR _pwszSend)
	{
		InitSecurityDescriptor();
		// 우선 리시브 파이프를 만든다.
		if (INVALID_HANDLE_VALUE == hPipeRecv
			&& nullptr != pSecurityDescriptorRecv
			)
		{
			C_SECURITY_ATTRIBUTES SecurityAttributes(pSecurityDescriptorRecv);
			wchar_t wszPipeName[(1 << 8)] = { L"\\\\.\\Pipe\\" };
			::wcscat_s(wszPipeName, _pwszRecv);
			//DBGPRINT(L"CreateNamedPipeW(%s)", wszPipeName);
			hPipeRecv = ::CreateNamedPipeW(
				wszPipeName
				, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED
				, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT
				, PIPE_UNLIMITED_INSTANCES
				, _MAX_SIZE_PIPE_BUFFER_
				, _MAX_SIZE_PIPE_BUFFER_
				, NMPWAIT_USE_DEFAULT_WAIT
				, &SecurityAttributes
			);
			ZeroMemory(wszPipeName, sizeof(wszPipeName));
		}
		if (INVALID_HANDLE_VALUE == hPipeSend
			&& nullptr != pSecurityDescriptorSend
			)
		{
			C_SECURITY_ATTRIBUTES SecurityAttributes(pSecurityDescriptorSend);
			wchar_t wszPipeName[(1 << 8)] = { L"\\\\.\\Pipe\\" };
			::wcscat_s(wszPipeName, _pwszSend);
			//DBGPRINT(L"CreateNamedPipeW(%s)", wszPipeName);
			hPipeSend = ::CreateNamedPipeW(
				wszPipeName
				, PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED
				, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT
				, PIPE_UNLIMITED_INSTANCES
				, _MAX_SIZE_PIPE_BUFFER_
				, _MAX_SIZE_PIPE_BUFFER_
				, NMPWAIT_USE_DEFAULT_WAIT
				, &SecurityAttributes
			);
			ZeroMemory(wszPipeName, sizeof(wszPipeName));
		}
		return (INVALID_HANDLE_VALUE != hPipeRecv && INVALID_HANDLE_VALUE != hPipeSend);
	}
	bool C_PIPE::Accept(LPCSTR _pszRecv, LPCSTR _pszSend)
	{
		wchar_t wszRecv[(1 << 8)], wszSend[(1 << 8)];
		return Accept(dk::AnsiToUtf16_s(wszRecv, sizeof(wszRecv), _pszRecv), dk::AnsiToUtf16_s(wszSend, sizeof(wszSend), _pszSend));
	}
	bool C_PIPE::Connect(LPCWSTR _pwszRecv, LPCWSTR _pwszSend)
	{
		InitSecurityDescriptor();
		if (INVALID_HANDLE_VALUE == hPipeRecv)
		{
			C_SECURITY_ATTRIBUTES SecurityAttributes(pSecurityDescriptorRecv);
			wchar_t wszPipeName[(1 << 8)] = { L"\\\\.\\Pipe\\" };
			::wcscat_s(wszPipeName, _pwszRecv);
			// 생성한 파이프 이름으로 CreateFile 해야하나보다
			hPipeRecv = ::CreateFileW(wszPipeName, GENERIC_READ, 0, &SecurityAttributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			ZeroMemory(wszPipeName, sizeof(wszPipeName));	// 사용하고 지움
		}
		if (INVALID_HANDLE_VALUE == hPipeSend)
		{
			C_SECURITY_ATTRIBUTES SecurityAttributes(pSecurityDescriptorSend);
			wchar_t wszPipeName[(1 << 8)] = { L"\\\\.\\Pipe\\" };
			::wcscat_s(wszPipeName, _pwszSend);
			hPipeSend = ::CreateFileW(wszPipeName, GENERIC_WRITE, 0, &SecurityAttributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			ZeroMemory(wszPipeName, sizeof(wszPipeName));	// 사용하고 지움
		}
		return (INVALID_HANDLE_VALUE != hPipeRecv) && (INVALID_HANDLE_VALUE != hPipeSend);
	}
	bool C_PIPE::Connect(LPCSTR _pszRecv, LPCSTR _pszSend)
	{
		wchar_t wszRecv[(1 << 8)], wszSend[(1 << 8)];
		return Accept(dk::AnsiToUtf16_s(wszRecv, sizeof(wszRecv), _pszRecv), dk::AnsiToUtf16_s(wszSend, sizeof(wszSend), _pszSend));
	}
	void C_PIPE::Destroy()
	{
		if (INVALID_HANDLE_VALUE != hPipeRecv)
		{
			::DisconnectNamedPipe(hPipeRecv);
			::CloseHandle(hPipeRecv);
			hPipeRecv = INVALID_HANDLE_VALUE;
		}
		if (INVALID_HANDLE_VALUE != hPipeSend)
		{
			::DisconnectNamedPipe(hPipeSend);
			::CloseHandle(hPipeSend);
			hPipeSend = INVALID_HANDLE_VALUE;
		}
		if (pSecurityDescriptorRecv)
		{
			::LocalFree(pSecurityDescriptorRecv);
			pSecurityDescriptorRecv = nullptr;
		}
		if (pSecurityDescriptorSend)
		{
			::LocalFree(pSecurityDescriptorSend);
			pSecurityDescriptorSend = nullptr;
		}
	}
}