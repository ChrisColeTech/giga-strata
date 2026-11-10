#include "launch/free_port.h"

#include <winsock2.h>
#include <ws2tcpip.h>

// Linked here rather than in CMake so the core library's link line stays Qt-only.
#pragma comment(lib, "ws2_32.lib")

Result<int> FreePort::pick() {
    const QString failure = QStringLiteral("Cannot find a free local port for the trial load.");
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return Result<int>::failure(failure);
    }
    const SOCKET handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int port = 0;
    if (handle != INVALID_SOCKET) {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        int length = sizeof address;
        if (bind(handle, reinterpret_cast<sockaddr *>(&address), sizeof address) == 0
            && getsockname(handle, reinterpret_cast<sockaddr *>(&address), &length) == 0) {
            port = ntohs(address.sin_port);
        }
        closesocket(handle);
    }
    WSACleanup();
    return port > 0 ? Result<int>::success(port) : Result<int>::failure(failure);
}
