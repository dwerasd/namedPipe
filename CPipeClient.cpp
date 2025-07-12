#include "framework.h"
#include "CPipeClient.h"



namespace pipe
{
	C_PIPE_CLIENT::C_PIPE_CLIENT(LPCWSTR _pRecv, LPCWSTR _pSend, C_PIPE_RECEIVER* _pReceiver)
		: bAccept(false)
		, pReceiver(_pReceiver)
		, wstrRecv(_pRecv)
		, wstrSend(_pSend)
	{

	}
	C_PIPE_CLIENT::C_PIPE_CLIENT(LPCSTR _pRecv, LPCSTR _pSend, C_PIPE_RECEIVER* _pReceiver)
		: bAccept(false)
		, pReceiver(_pReceiver)
	{
		wchar_t szBuffer[_MAX_PATH] = { 0 };
		wstrRecv = dk::AnsiToUtf16_s(szBuffer, sizeof(szBuffer), _pRecv);

		::memset(szBuffer, 0, sizeof(szBuffer));
		wstrSend = dk::AnsiToUtf16_s(szBuffer, sizeof(szBuffer), _pSend);
	}
	C_PIPE_CLIENT::~C_PIPE_CLIENT()
	{
		dk::C_THREAD::Terminate();
		dk::C_PIPE::Destroy();
	}

	int C_PIPE_CLIENT::Recv(LPPACKET_BASE _pData)
	{
		DWORD dwRead = 0;
		if (INVALID_HANDLE_VALUE != hPipeRecv)
		{
			::FlushFileBuffers(hPipeRecv);
			if (!::ReadFile(hPipeRecv, (LPVOID)_pData, sizeof(PACKET_BASE), &dwRead, NULL))
			{
				return -1;
			}
			if (_pData->nPacketSize != (dwRead - sizeof(PACKET_HEADER)))
			{
				DBGPRINT("_pData->nPacketSize: %d / dwRead: %d", _pData->nPacketSize, dwRead);
				return -2;
			}
		}
		return(dwRead);
	}

	int C_PIPE_CLIENT::Send(LPPACKET_BASE _pData)
	{
		DWORD dwWritten = 0;
		if (INVALID_HANDLE_VALUE != hPipeSend)
		{
			BOOL bResult = ::WriteFile(hPipeSend, (LPVOID)_pData, sizeof(PACKET_HEADER) + _pData->nPacketSize, &dwWritten, NULL);
			::FlushFileBuffers(hPipeSend);
			if (!bResult)
			{
				return -1;
			}
			if ((DWORD)(sizeof(PACKET_HEADER) + _pData->nPacketSize) != dwWritten)
			{
				return -2;
			}
		}
		return(dwWritten);
	}

	int C_PIPE_CLIENT::Send(WORD _dwHeader, LPVOID _pData, WORD _nSize)
	{
		DWORD dwWritten = 0;
		if (INVALID_HANDLE_VALUE != hPipeSend)
		{
			PACKET_BASE netPacket =
			{
				_nSize			// 데이터크기
				, _dwHeader		// 헤더
				, { 0 }			// 보낼 내용
			};
			if (0 < _nSize)
			{
				ZeroMemory(netPacket.bytBuffer, _countof(netPacket.bytBuffer));
				::memcpy_s(netPacket.bytBuffer, _countof(netPacket.bytBuffer), _pData, _nSize);
			}
			//DBGPRINT("C_PIPE_SERVER::Send: %d / %d / %d", _bUnicode, _nSize, _dwHeader);
			BOOL bResult = ::WriteFile(hPipeSend, (LPVOID)&netPacket, sizeof(PACKET_HEADER) + _nSize, &dwWritten, NULL);
			::FlushFileBuffers(hPipeSend);
			if (!bResult)
			{
				return -1;
			}
			if ((DWORD)(sizeof(PACKET_HEADER) + _nSize) != dwWritten)
			{
				return -2;
			}
		}
		return(dwWritten);
	}

	DWORD C_PIPE_CLIENT::ThreadFunc(LPVOID _pParam)
	{
		_pParam;
		DBGPRINT(L"파이프 클라 시작: %s /  %s", wstrRecv.c_str(), wstrSend.c_str());
		do
		{
			if (!bAccept)
			{
				if (!dk::C_PIPE::Connect(wstrRecv.c_str(), wstrSend.c_str()))
				{	// 접속 실패시 프로그램을 종료하지 않고 접속 시도를 하게 바꿔야한다.
					dk::C_PIPE::Destroy();								// 파이프 종료
				}
				else
				{
					DBGPRINT(L"파이프 접속 완료 %s", wstrSend.c_str());
					eventRecv.Set();
					bAccept = true;
				}
				dk::Sleep(500);
			}
			else
			{
				PACKET_BASE NetPacketBuffer = { 0 };
				int nRecvSize = Recv(&NetPacketBuffer);
				if ((-1 < nRecvSize) && eventRecv.InValid())
				{
					eventRecv.Set();
					DBGPRINT("C_PIPE_CLIENT::ThreadFunc(메시지 받음): %d / %d", NetPacketBuffer.nPacketIndex, nRecvSize);
					// 복사해서 큐에 넣기만 한다.
					LPPACKET_BASE pNetPacket = new PACKET_BASE();		// 새로 할당받는다.
					::memset(pNetPacket, 0, sizeof(PACKET_BASE));
					::memcpy_s(pNetPacket, sizeof(PACKET_BASE), &NetPacketBuffer, nRecvSize);

					pReceiver->PushPacket(pNetPacket);
				}
				else
				{	// 여기에 오면 파이프를 초기화하고 재접속 시도를 하게 된다.
					DBGPRINT(L"C_PIPE_CLIENT::ThreadFunc() - nRecvSize: %d or eventRecv.InValid(): %s", nRecvSize, wstrSend.c_str());
					dk::C_PIPE::Destroy();
					bAccept = false;
				}
			}
		} while (!bStopThread);
		eventRecv.Destroy();
		dk::C_THREAD::Terminate();							// 스레드 강제 종료를 한번 더 날린다.
		DBGPRINT(L"C_PIPE_CLIENT::ThreadFunc() - 파이프 스레드 종료 %s", wstrSend.c_str());
		return(0);
	}
}
