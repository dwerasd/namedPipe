#include "framework.h"
#include "CPipeServer.h"



namespace pipe
{
	C_PIPE_SERVER::C_PIPE_SERVER(LPCWSTR _pRecv, LPCWSTR _pSend, C_PIPE_RECEIVER* _pReceiver)
		: bAccept(false)
		, pReceiver(_pReceiver)
		, wstrRecv(_pRecv)
		, wstrSend(_pSend)
	{

	}
	C_PIPE_SERVER::C_PIPE_SERVER(LPCSTR _pRecv, LPCSTR _pSend, C_PIPE_RECEIVER* _pReceiver)
		: bAccept(false)
		, pReceiver(_pReceiver)
	{
		wchar_t szBuffer[_MAX_PATH] = { 0 };
		wstrRecv = dk::AnsiToUtf16_s(szBuffer, sizeof(szBuffer), _pRecv);

		::memset(szBuffer, 0, sizeof(szBuffer));
		wstrSend = dk::AnsiToUtf16_s(szBuffer, sizeof(szBuffer), _pSend);
	}
	C_PIPE_SERVER::~C_PIPE_SERVER()
	{
		dk::C_THREAD::Terminate();
		dk::C_PIPE::Destroy();
	}

	int C_PIPE_SERVER::Recv(LPPACKET_BASE _pData)
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

	int C_PIPE_SERVER::Send(LPPACKET_BASE _pData)
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
		return dwWritten;
	}

	int C_PIPE_SERVER::Send(WORD _dwHeader, LPVOID _pData, WORD _nSize)
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
				memcpy_s(netPacket.bytBuffer, _countof(netPacket.bytBuffer), _pData, _nSize);
			}
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

	DWORD C_PIPE_SERVER::ThreadFunc(LPVOID _pParam)
	{
		_pParam;
		DBGPRINT(__TEXT("C_PIPE_SERVER::ThreadFunc() - 파이프 서버 시작: %s /  %s / %x"), wstrRecv.c_str(), wstrSend.c_str(), ::GetCurrentThreadId());
		dk::C_PIPE::Accept(wstrRecv.c_str(), wstrSend.c_str());
		do
		{
			if (!bAccept)
			{
				if (::ConnectNamedPipe(hPipeRecv, NULL))
				{
					DBGPRINT("%s - 파이프 연결 됨, _PKT_PIPE_CONNECTED_ 발송", __FUNCTION__);
					Send(_PKT_PIPE_CONNECTED_);
					eventRecv.Set();
					bAccept = true;
				}
				dk::Sleep(200);
			}
			else
			{
				PACKET_BASE NetPacketBuffer = { 0 };
				int nRecvSize = Recv(&NetPacketBuffer);
				if ((-1 < nRecvSize) && eventRecv.InValid())
				{
					eventRecv.Set();
					// 복사해서 큐에 넣기만 한다.
					LPPACKET_BASE pNetPacket = new PACKET_BASE();		// 새로 할당받는다.
					::memset(pNetPacket, 0, sizeof(PACKET_BASE));
					dk::CopyMem(pNetPacket, sizeof(PACKET_BASE), &NetPacketBuffer, nRecvSize);
					pReceiver->PushPacket(pNetPacket);
				}
				else
				{
					DBGPRINT("C_PIPE_SERVER::ThreadFunc() - nRecvSize: %d or eventRecv.InValid()", nRecvSize);

					LPPACKET_BASE pNetPacket = new PACKET_BASE();		// 새로 할당받는다.
					::memset(pNetPacket, 0, sizeof(PACKET_BASE));
					pNetPacket->nPacketIndex = _PKT_PIPE_DISCONNECTED_;
					pReceiver->PushPacket(pNetPacket);
					dk::C_PIPE::Destroy();
					dk::C_PIPE::Accept(wstrRecv.c_str(), wstrSend.c_str());
					bAccept = false;
				}
			}
		} while (!bStopThread);
		DBGPRINT("파이프 스레드 종료 - C_PIPE_SERVER::ThreadFunc()");
		eventRecv.Destroy();
		C_THREAD::Terminate();							// 스레드 강제 종료를 한번 더 날린다.
		return(0);
	}
}
