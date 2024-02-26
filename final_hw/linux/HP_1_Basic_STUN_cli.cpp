#include "lib.h"

char *getIPonDNS(string domain);

void getTargetIP (char *buf, string &ip, string &port, bool isLocalNet);

int main() {
    string server;
    cout << "Input server IP or domain: "; cin >> server;

    char *ip = new char[16];
    ip = getIPonDNS(server);



    int clisock = socket(AF_INET, SOCK_DGRAM, 0);
    if (clisock == INVALID_SOCKET) {
        cout << "socket() error" << endl;
        return 0;
    }

    sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(ip);
    servAddr.sin_port = htons(3478);

    if (connect(clisock, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        cout << "connect() error" << endl;
        close(clisock);
        return 0;
    }



    sockaddr_in localAddr;
    uint addrSize = sizeof(localAddr);
    if (getsockname(clisock, (sockaddr*)&localAddr, &addrSize) == SOCKET_ERROR) {
        cout << "getsockname() error" << endl;
        close(clisock);
        return 0;
    }

    char buf[1024];
    sprintf(buf, "%s:%d", inet_ntoa(localAddr.sin_addr), ntohs(localAddr.sin_port));



    int sendlen = sendto(clisock, buf, strlen(buf) + 1, 0, (sockaddr*)&servAddr, sizeof(servAddr));
    if (sendlen == SOCKET_ERROR) {
        cout << "sendto() error" << endl;
        return 0;
    }

    int recvlen = recvfrom(clisock, buf, sizeof(buf), 0, NULL, NULL);
    if (recvlen == SOCKET_ERROR) {
        cout << "recvfrom() error" << endl;
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
        close(clisock);
        return 0;
    }

    while (true) {
        chrono::milliseconds hb = 100ms;

        string message = "send from linux";
        
        if (strstr(buf, "windows")!= NULL){ // 윈도우와 연결 확인
            message += "\nLinux - Windows / Maintaining Connection";
            hb = 1000ms;
        }
        else if (strstr(buf, "linux")!= NULL){ // 리눅스와 연결 확인
            message += "\nLinux - Linux / Maintaining Connection";
            hb = 1000ms;
        }
        // udp 연결이 원활하게 유지될 경우, Heartbeat 를 조절
    
        // send from linux (여기서 생성한 message) 를 send함
        int sendlen = sendto(clisock, message.c_str(), message.length() + 1, 0, (sockaddr*)&servAddr, sizeof(servAddr));
        if (sendlen == SOCKET_ERROR) {
            cout << "sendto() error" << endl;
            return 0;
        }
    
        // send from window (window로부터 온 message) 를 buf에 담음
        int recvlen = recvfrom(clisock, buf, sizeof(buf), 0, NULL, NULL);
        if (recvlen == SOCKET_ERROR) {
            cout << "recvfrom() error 2" << endl;
            // cout << errno << endl;
            return 0;
        }

        cout << "Echo: " << buf << endl;

        if (strstr(buf, "send") != NULL){ // buf에서 들어온 내용을 바탕으로 응답이 온 걸 확인하면

            int linsock = socket(AF_INET, SOCK_DGRAM, 0);
            if (linsock == INVALID_SOCKET) {
                cout << "socket() error" << endl;
                return 0;
            }

            char sec_buf[1024];
            // 소켓을 생성하고 연결된 다른 클라이언트로부터 응답을 받음

            sockaddr_in linAddr;
            memset(&linAddr, 0, sizeof(linAddr));
            linAddr.sin_family = AF_INET;
            linAddr.sin_addr.s_addr = inet_addr(ip);
            linAddr.sin_port = htons(1234);

            if (connect(linsock, (sockaddr*)&linAddr, sizeof(linAddr)) == SOCKET_ERROR) {
                cout << "connect() error" << endl;
                close(linsock);
                return 0;
            }
            
            // hello.txt 파일을 sec_buf에 담음
            FILE* file;
            file = fopen("hello.txt", "r");
            size_t res;


            // 파일 크기 확인
            fseek(file,0,SEEK_END);
            size_t fsize = ftell(file);
            fseek(file,0,SEEK_SET);

            if (fsize > 1024){
                cout << "file is too large" << endl;
                return 0;
            }
            else {
                res = fread(sec_buf, 1, fsize, file);
                if (res != fsize){
                    cout << "fread() error" << endl;
                    return 0;
                }
            }

            int sendlen = sendto(clisock, sec_buf, strlen(sec_buf) + 1, 0, (sockaddr*)&linAddr, sizeof(linAddr));
            if (sendlen == SOCKET_ERROR) {
                cout << "sendto() error" << endl;
                return 0;
            }

        }

        this_thread::sleep_for(hb);
    }


    return 0;
}

char *getIPonDNS(string domain) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(domain.c_str(), "http", &hints, &res);
    if (status != 0) {
        cout << "getaddrinfo() error" << endl;
        return nullptr;
    }

    char *ip = new char[16];
    inet_ntop(res->ai_family, &((sockaddr_in*)res->ai_addr)->sin_addr, ip, 16);

    return ip;
}

void getTargetIP (char *buf, string &ip, string &port, bool isLocalNet) {
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