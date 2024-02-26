#include "lib.h"

char *getIPonDNS(string domain);

void getTargetIP (char *buf, string &ip, string &port, bool isLocalNet);

int main() {
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    string server;
    cout << "Input server IP or domain: "; cin >> server;

    char *ip = getIPonDNS(server);



    SOCKET clisock = socket(AF_INET, SOCK_DGRAM, 0);
    if (clisock == INVALID_SOCKET) {
        cout << "socket() error" << endl;
        return 0;
    }

    SOCKADDR_IN servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(ip);
    servAddr.sin_port = htons(3478);

    if (connect(clisock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        cout << "connect() error" << endl;
        closesocket(clisock);
        return 0;
    }



    SOCKADDR_IN localAddr;
    int addrSize = sizeof(localAddr);
    if (getsockname(clisock, (SOCKADDR*)&localAddr, &addrSize) == SOCKET_ERROR) {
        cout << "getsockname() error" << endl;
        closesocket(clisock);
        return 0;
    }

    char buf[1024];
    sprintf(buf, "%s:%d", inet_ntoa(localAddr.sin_addr), ntohs(localAddr.sin_port));



    int sendlen = sendto(clisock, buf, strlen(buf) + 1, 0, (SOCKADDR *)&servAddr, sizeof(servAddr));
    if (sendlen == SOCKET_ERROR) {
        cout << "sendto() error" << endl;
        return 0;
    }

    int recvlen = recvfrom(clisock, buf, sizeof(buf), 0, NULL, NULL);
    if (recvlen == SOCKET_ERROR) {
        cout << "recvfrom() error 1" << endl;
        return 0;
    }

    cout << "STUN: " << buf << endl;



    bool isLocalNet = true; // 로컬 넷(같은 NAT)에서 홀펀칭을 하려면 true로 변경
    string destIP, destPort;
    getTargetIP(buf, destIP, destPort, isLocalNet);

    cout << "Destination IP: " << destIP << ":" << destPort << endl;

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(destIP.c_str());
    servAddr.sin_port = htons(atoi(destPort.c_str()));

    if (connect(clisock, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        cout << "connect() error" << endl;
        closesocket(clisock);
        return 0;
    }



    while (true) {
        chrono::milliseconds hb = 100ms;

        string message = "send from windows";

        if (strstr(buf, "windows")!= NULL){ // 윈도우와 연결 확인
            message += "\nWindows - Windows / Maintaining Connection";
            hb = 1000ms;
        }
        else if (strstr(buf, "linux")!= NULL){ // 리눅스와 연결 확인
            message += "\nWindows - Linux / Maintaining Connection";
            hb = 1000ms;
        }
        // udp 연결이 원활하게 유지될 경우, Heartbeat 를 조절

        int sendlen = sendto(clisock, message.c_str(), message.length() + 1, 0, (sockaddr*)&servAddr, sizeof(servAddr));
        if (sendlen == SOCKET_ERROR) {
            cout << "sendto() error" << endl;
            return 0;
        }

        int recvlen = recvfrom(clisock, buf, sizeof(buf), 0, NULL, NULL);
        if (recvlen == SOCKET_ERROR) {
            cout << "recvfrom() error" << endl;
            return 0;
        }

        cout << "Echo: " << buf << endl;

        if (strstr(buf, "send") != NULL){ // buf에서 들어온 내용을 바탕으로 응답이 온 걸 확인하면
            int winsock = socket(AF_INET, SOCK_DGRAM, 0);
            if (winsock == INVALID_SOCKET) {
                cout << "socket() error" << endl;
                return 0;
            }

            sockaddr_in winAddr;
            memset(&winAddr, 0, sizeof(winAddr));
            winAddr.sin_family = AF_INET;
            winAddr.sin_addr.s_addr = inet_addr(ip);
            winAddr.sin_port = htons(1234);

            if (connect(winsock, (sockaddr*)&winAddr, sizeof(winAddr)) == SOCKET_ERROR) {
                cout << "connect() error" << endl;
                close(winsock);
                return 0;
            }

            int recvlen = recvfrom(winsock, buf, sizeof(buf), 0, NULL, NULL);
            if (recvlen == SOCKET_ERROR) {
                cout << "recvfrom() error" << endl;
                return 0;
            }

            cout << "hello.txt : " << buf << endl;
        }

        this_thread::sleep_for(hb);
    }



    closesocket(clisock);
    WSACleanup();
    return 0;
}

char *getIPonDNS(string domain) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    struct hostent *host;
    char *ip = new char[16];
    host = gethostbyname(domain.c_str());
    if (!host) {
        cout << "gethostbyname() error" << endl;
        return nullptr;
    }
    strcpy(ip, inet_ntoa(*(struct in_addr *)host->h_addr_list[0]));
    WSACleanup();
    return ip;
}

void getTargetIP(char *buf, string &ip, string &port, bool isLocalNet) {
    istringstream iss(buf);
    vector<string> tokens;
    string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

    // [0]: 자신의 pub IP:Port, [1]: 상대의 pub IP:Port,
    // [2]: 자신의 priv IP:Port, [3]: 상대의 priv IP:Port

    if (isLocalNet) { // 내부망(같은 NAT)일 경우
        ip = tokens[3].substr(0, tokens[3].find(':'));
        port = tokens[3].substr(tokens[3].find(':') + 1);
        return;
    }

    ip = tokens[1].substr(0, tokens[1].find(':'));
    port = tokens[1].substr(tokens[1].find(':') + 1);
}