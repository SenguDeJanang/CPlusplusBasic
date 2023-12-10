// ChattingServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
unsigned int __stdcall ChattingPacketProcess(void* arg);
const char serverIP[] = "127.0.0.1";
const int serverPort = 12345;

#include <process.h>
#include <list>
#include <mutex>

const int maxWorkingThread = 8;
const int maxBuffer = 1024;
class ClientInfo : public WSAOVERLAPPED
{

public:
    static int userHandle;
    int userHandle2;

    WSABUF dataBuffer;
    SOCKET socket;
    char messageBuffer[maxBuffer];
    unsigned long recvByte;
    unsigned long flag;
    ClientInfo();
    void Reset();
};

int ClientInfo::userHandle = 0;
ClientInfo::ClientInfo()
{
    userHandle2 = userHandle;
    ++userHandle;

    // 리셋
    Reset();
}

void ClientInfo::Reset()
{
    int myHandle = userHandle2;
    SOCKET sock = socket;
    memset(this, 0, sizeof(*this));
    userHandle2 = myHandle;
    socket = sock;

    dataBuffer.buf = messageBuffer;
    dataBuffer.len = maxBuffer;
    flag = 0;
    recvByte = 0;
}

// 클라이언의 모든 정보를 관리합니다.
list<ClientInfo*> clientList;
mutex mutexList;

int main()
{
    WSADATA wsaData;
    int startUpResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (startUpResult != 0)
    {
        cout << "소켓 데이터 초기화 실패" << endl;
        return 1;
    }
    SOCKET listenSocket = WSASocket(AF_INET,SOCK_STREAM, 0, nullptr,0,WSA_FLAG_OVERLAPPED);
    if (listenSocket == INVALID_SOCKET)
    {
        cout << "listenSocket 생성 실패!" << endl;
        WSACleanup();
        return 2;
    }
    SOCKADDR_IN sockInfo;
    memset(&sockInfo, 0, sizeof(sockInfo));
    sockInfo.sin_family = PF_INET;
    sockInfo.sin_port = serverPort;
    sockInfo.sin_addr.S_un.S_addr = htonl(INADDR_ANY);


    int bindResult = ::bind(listenSocket, (SOCKADDR*)&sockInfo, sizeof(sockInfo));

    if (bindResult == SOCKET_ERROR)
    {
        cout << "listenSocket 바인드 에러" << endl;
        WSACleanup();
        return 3;
    }
    int listenResult = listen(listenSocket, 5);
    if (listenResult == SOCKET_ERROR)
    {
        cout << "listen() 호출 실패" << endl;
        WSACleanup();
        return 4;
    }

    HANDLE iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, maxWorkingThread);
        if (iocpHandle == INVALID_HANDLE_VALUE)
        {
            cout << "iocp 객체 생성 실패" << endl;
            WSACleanup();
            return 5;
        }
        for (int cnt = 0; cnt < maxWorkingThread; ++cnt)
        {
            _beginthreadex(nullptr, 0, ChattingPacketProcess, &iocpHandle, 0, nullptr);
        }
}
unsigned int __stdcall ChattingPacketProcess(void* arg)
{
    HANDLE* hndPnt = (HANDLE*)arg;
    HANDLE iocpHandle = *hndPnt;
    
    unsigned long recvBytes;
    ULONG_PTR completionKey;
    WSAOVERLAPPED* cliInfo;
    unsigned long flag;

    while (true)
    {
        BOOL result = GetQueuedCompletionStatus(iocpHandle, &recvBytes, &completionKey, &cliInfo, INFINITE);
        ClientInfo* clientInfo = (ClientInfo*)cliInfo;
        if (result = FALSE)
        {
            cout << "접속 종료" << endl;

            closesocket(clientInfo->socket);
            mutexList.lock();
            clientList.remove_if([clientInfo](ClientInfo* client)->bool
            {
                    return client->userHandle == clientInfo->userHandle;
            });
            mutexList.unlock();

            delete clientInfo;
            continue;
        }
    }
}
// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.

