#include "lib.h"

// 스턴 서버는 가급적이면 공인 IP를 직접 사용하는 서버를 쓰는 것이 좋다
int main() {
    int servsock = socket(AF_INET, SOCK_DGRAM, 0);
    if (servsock == INVALID_SOCKET) {
        cout << "socket() error" << endl;
        return 0;
    }

    sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(3478);

    if (bind(servsock, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        cout << "bind() error" << endl;
        return 0;
    }

    while (true) {
        sockaddr_in cliAddr1;
        uint cliAddrLen1 = sizeof(cliAddr1);
        memset(&cliAddr1, 0, sizeof(cliAddr1));

        sockaddr_in cliAddr2;
        uint cliAddrLen2 = sizeof(cliAddr2);
        memset(&cliAddr2, 0, sizeof(cliAddr2));

        char buf1[1024], buf2[2024], buf3[22], buf4[22];

        int recvlen = recvfrom(servsock, buf1, sizeof(buf1), 0, (sockaddr*)&cliAddr1, &cliAddrLen1);
        if (recvlen == SOCKET_ERROR) {
            cout << "recvfrom() error" << endl;
            return 0;
        }
        else {
            cout << "cli_1 connected" << endl;
        }

        recvlen = recvfrom(servsock, buf2, sizeof(buf2), 0, (sockaddr*)&cliAddr2, &cliAddrLen2);
        if (recvlen == SOCKET_ERROR) {
            cout << "recvfrom() error" << endl;
            return 0;
        }
        else {
            cout << "cli_2 connected" << endl;
        }

        // cout << "recv1: " << buf1 << endl;
        memcpy(buf3, buf1, 22);

        // cout << "recv2: " << buf2 << endl;
        memcpy(buf4, buf2, 22);

        // 출력 규칙은 [자신의 pub IP:Port 상대방의 pub IP:Port 자신의 priv IP:Port 상대방의 priv IP:Port] 이다
        sprintf(
            buf1, "%s:%d %s:%d %s %s",
            inet_ntoa(cliAddr1.sin_addr), ntohs(cliAddr1.sin_port),
            inet_ntoa(cliAddr2.sin_addr), ntohs(cliAddr2.sin_port),
            buf3, buf4
        );

        sprintf(
            buf2, "%s:%d %s:%d %s %s",
            inet_ntoa(cliAddr2.sin_addr), ntohs(cliAddr2.sin_port),
            inet_ntoa(cliAddr1.sin_addr), ntohs(cliAddr1.sin_port),
            buf4, buf3
        );

        int sendlen = sendto(servsock, buf1, strlen(buf1) + 1, 0, (sockaddr*)&cliAddr1, sizeof(cliAddr1));
        if (sendlen == SOCKET_ERROR) {
            cout << "sendto() error" << endl;
            return 0;
        }

        sendlen = sendto(servsock, buf2, strlen(buf2) + 1, 0, (sockaddr*)&cliAddr2, sizeof(cliAddr2));
        if (sendlen == SOCKET_ERROR) {
            cout << "sendto() error" << endl;
            return 0;
        }
    }

    close(servsock);
    return 0;
}
