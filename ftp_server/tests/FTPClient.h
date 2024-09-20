#include <ace/SOCK_Connector.h>
#include <ace/SOCK_Stream.h>
#include <gtest/gtest.h>
#include <string>

class FTPClient {
public:
    FTPClient(const std::string& serverAddress, int port) {
        ACE_INET_Addr serverAddr(port, serverAddress.c_str());
        if (connector.connect(clientStream, serverAddr) == -1) {
            throw std::runtime_error("Failed to connect to server.");
        }
    }

    ~FTPClient() {
        clientStream.close();
    }

    // 发送命令并获取响应
    std::string sendCommand(const std::string& command) {
        clientStream.send(command.c_str(), command.length(),8);

        char buffer[1024] = {0};
        clientStream.recv(buffer, sizeof(buffer));

        return std::string(buffer);
    }

    std::string recvCommand() {
        char buffer[1024] = {0};
        clientStream.recv(buffer, sizeof(buffer),8);

        return std::string(buffer);
    }

    void senddata(const std::string& data) {
        clientStream.send(data.c_str(), data.size());
        clientStream.close();
    }
    
    std::string recvdata() {
        std::string receivedData;
        char buffer[1024];  // 使用较大的缓冲区提高效率

        ssize_t bytesReceived = 0;

        // 循环接收数据直到连接关闭
        while ((bytesReceived = clientStream.recv(buffer, sizeof(buffer))) > 0) {
            receivedData.append(buffer, bytesReceived);
        }

        // 关闭连接
        clientStream.close();

        return receivedData;
    }

private:
    ACE_SOCK_Stream clientStream;
    ACE_SOCK_Connector connector;
};
