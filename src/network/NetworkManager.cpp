// ============================================================
//  NetworkManager.cpp — Built-in TCP + UDP + HTTP for Like2D
// ============================================================
#include "NetworkManager.h"
#include <cstring>
#include <iostream>
#include <algorithm>
#include <sstream>

// ============================================================
//  Construction / Destruction
// ============================================================
NetworkManager::NetworkManager() {
    m_startTime = std::chrono::steady_clock::now();
}

NetworkManager::~NetworkManager() {
    shutdown();
}

// ============================================================
//  Platform init (Winsock2 on Windows)
// ============================================================
bool NetworkManager::initSockets() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[net] WSAStartup failed: " << wsa.wVersion << std::endl;
        return false;
    }
#endif
    return true;
}

// ============================================================
//  Event queue helpers (thread-safe)
// ============================================================
void NetworkManager::pushEvent(const NetworkEvent& ev) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_events.push(ev);
}

void NetworkManager::poll(std::vector<NetworkEvent>& outEvents) {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_events.empty()) {
        outEvents.push_back(std::move(m_events.front()));
        m_events.pop();
    }
}

// ============================================================
//  Peer registry
// ============================================================
int NetworkManager::addPeer(Socket sock, const std::string& ip, int port) {
    int id = m_nextPeerId.fetch_add(1);
    std::lock_guard<std::mutex> lock(m_peerMutex);
    PeerInfo info{id, sock, ip, port};
    info.lastActivity = std::chrono::steady_clock::now();
    m_peers[id] = info;
    return id;
}

void NetworkManager::removePeer(int peerId, const std::string& reason) {
    Socket sock = INVALID_SOCK;
    {
        std::lock_guard<std::mutex> lock(m_peerMutex);
        auto it = m_peers.find(peerId);
        if (it != m_peers.end()) {
            sock = it->second.socket;
            m_peers.erase(it);
        }
    }
    // Clean up metadata
    {
        std::lock_guard<std::mutex> lock(m_metaMutex);
        m_peerMeta.erase(peerId);
    }
    if (sock != INVALID_SOCK) SOCK_CLOSE(sock);
    pushEvent({NetEventType::Disconnect, peerId, reason, "", 0});
}

int NetworkManager::getPeerCount() const {
    std::lock_guard<std::mutex> lock(m_peerMutex);
    return (int)m_peers.size();
}

std::string NetworkManager::getPeerIP(int peerId) const {
    std::lock_guard<std::mutex> lock(m_peerMutex);
    auto it = m_peers.find(peerId);
    return (it != m_peers.end()) ? it->second.ip : "";
}

// ============================================================
//  TCP Server
// ============================================================
bool NetworkManager::createServer(int port) {
    if (m_running.load()) {
        std::cerr << "[net] Server or client already running." << std::endl;
        return false;
    }

    m_tcpListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_tcpListenSock == INVALID_SOCK) {
        std::cerr << "[net] Failed to create TCP socket." << std::endl;
        return false;
    }

    int optVal = 1;
    setsockopt(m_tcpListenSock, SOL_SOCKET, SO_REUSEADDR,
               (const char*)&optVal, sizeof(optVal));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((u_short)port);

    if (bind(m_tcpListenSock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        std::cerr << "[net] bind() failed on port " << port << std::endl;
        SOCK_CLOSE(m_tcpListenSock);
        m_tcpListenSock = INVALID_SOCK;
        return false;
    }

    if (listen(m_tcpListenSock, 16) != 0) {
        std::cerr << "[net] listen() failed." << std::endl;
        SOCK_CLOSE(m_tcpListenSock);
        m_tcpListenSock = INVALID_SOCK;
        return false;
    }

    m_running = true;
    m_startTime = std::chrono::steady_clock::now();
    m_thread = std::thread(&NetworkManager::networkThreadFunc, this);

    std::cout << "[net] TCP server listening on port " << port << std::endl;
    return true;
}

// ============================================================
//  TCP Client (non-blocking connect)
// ============================================================
int NetworkManager::connectTo(const std::string& host, int port) {
    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0 || !result) {
        std::cerr << "[net] DNS resolution failed for " << host << std::endl;
        return -1;
    }

    Socket sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCK) {
        freeaddrinfo(result);
        std::cerr << "[net] Failed to create client socket." << std::endl;
        return -1;
    }

    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) != 0) {
        SOCK_CLOSE(sock);
        freeaddrinfo(result);
        std::cerr << "[net] connect() to " << host << ":" << port << " failed." << std::endl;
        return -1;
    }

    char ipBuf[INET_ADDRSTRLEN] = {};
    sockaddr_in* sin = (sockaddr_in*)result->ai_addr;
    inet_ntop(AF_INET, &sin->sin_addr, ipBuf, sizeof(ipBuf));
    freeaddrinfo(result);

    int peerId = addPeer(sock, std::string(ipBuf), port);
    pushEvent({NetEventType::Connect, peerId, "", std::string(ipBuf), port});

    if (!m_running.load()) {
        m_running = true;
        m_startTime = std::chrono::steady_clock::now();
        m_thread = std::thread(&NetworkManager::networkThreadFunc, this);
    }

    std::cout << "[net] Connected to " << host << ":" << port
              << " (peer " << peerId << ")" << std::endl;
    return peerId;
}

// ============================================================
//  TCP Send
// ============================================================
bool NetworkManager::sendTo(int peerId, const std::string& data) {
    std::lock_guard<std::mutex> lock(m_peerMutex);
    auto it = m_peers.find(peerId);
    if (it == m_peers.end()) return false;

    uint32_t len = htonl((uint32_t)data.size());
    int headerSent = ::send(it->second.socket, (const char*)&len, 4, 0);
    if (headerSent != 4) return false;

    int payloadSent = 0;
    if (!data.empty()) {
        payloadSent = ::send(it->second.socket, data.c_str(), (int)data.size(), 0);
        if (payloadSent != (int)data.size()) return false;
    }

    // Update stats
    {
        std::lock_guard<std::mutex> slock(m_statsMutex);
        m_stats.bytesSent += 4 + data.size();
        m_stats.packetsSent++;
    }
    return true;
}

bool NetworkManager::sendToAll(const std::string& data) {
    std::lock_guard<std::mutex> lock(m_peerMutex);
    bool allOk = true;
    uint32_t len = htonl((uint32_t)data.size());
    for (auto& [id, peer] : m_peers) {
        if (::send(peer.socket, (const char*)&len, 4, 0) != 4) { allOk = false; continue; }
        if (!data.empty()) {
            int sent = ::send(peer.socket, data.c_str(), (int)data.size(), 0);
            if (sent != (int)data.size()) allOk = false;
        }
    }
    if (allOk) {
        std::lock_guard<std::mutex> slock(m_statsMutex);
        m_stats.bytesSent += (4 + data.size()) * m_peers.size();
        m_stats.packetsSent += m_peers.size();
    }
    return allOk;
}

bool NetworkManager::sendToAllExcept(int excludePeerId, const std::string& data) {
    std::lock_guard<std::mutex> lock(m_peerMutex);
    bool allOk = true;
    uint32_t len = htonl((uint32_t)data.size());
    int count = 0;
    for (auto& [id, peer] : m_peers) {
        if (id == excludePeerId) continue;
        if (::send(peer.socket, (const char*)&len, 4, 0) != 4) { allOk = false; continue; }
        if (!data.empty()) {
            int sent = ::send(peer.socket, data.c_str(), (int)data.size(), 0);
            if (sent != (int)data.size()) allOk = false;
        }
        count++;
    }
    if (count > 0) {
        std::lock_guard<std::mutex> slock(m_statsMutex);
        m_stats.bytesSent += (4 + data.size()) * count;
        m_stats.packetsSent += count;
    }
    return allOk;
}

void NetworkManager::disconnectPeer(int peerId, const std::string& reason) {
    removePeer(peerId, reason);
}

// ============================================================
//  UDP
// ============================================================
bool NetworkManager::createUDP(int port) {
    m_udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_udpSock == INVALID_SOCK) {
        std::cerr << "[net] Failed to create UDP socket." << std::endl;
        return false;
    }

    int optVal = 1;
    setsockopt(m_udpSock, SOL_SOCKET, SO_REUSEADDR,
               (const char*)&optVal, sizeof(optVal));

    // Enable broadcast
    int broadcastOpt = 1;
    setsockopt(m_udpSock, SOL_SOCKET, SO_BROADCAST,
               (const char*)&broadcastOpt, sizeof(broadcastOpt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((u_short)port);

    if (bind(m_udpSock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        std::cerr << "[net] UDP bind() failed on port " << port << std::endl;
        SOCK_CLOSE(m_udpSock);
        m_udpSock = INVALID_SOCK;
        return false;
    }

    if (!m_running.load()) {
        m_running = true;
        m_startTime = std::chrono::steady_clock::now();
        m_thread = std::thread(&NetworkManager::networkThreadFunc, this);
    }

    std::cout << "[net] UDP socket bound to port " << port << std::endl;
    return true;
}

bool NetworkManager::sendUDP(const std::string& ip, int port, const std::string& data) {
    if (m_udpSock == INVALID_SOCK) {
        m_udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_udpSock == INVALID_SOCK) return false;
        int broadcastOpt = 1;
        setsockopt(m_udpSock, SOL_SOCKET, SO_BROADCAST,
                   (const char*)&broadcastOpt, sizeof(broadcastOpt));
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons((u_short)port);
    inet_pton(AF_INET, ip.c_str(), &dest.sin_addr);

    int sent = sendto(m_udpSock, data.c_str(), (int)data.size(), 0,
                      (sockaddr*)&dest, sizeof(dest));
    if (sent == (int)data.size()) {
        std::lock_guard<std::mutex> slock(m_statsMutex);
        m_stats.bytesSent += data.size();
        m_stats.packetsSent++;
        return true;
    }
    return false;
}

bool NetworkManager::broadcastUDP(int port, const std::string& data) {
    if (m_udpSock == INVALID_SOCK) {
        m_udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_udpSock == INVALID_SOCK) return false;
    }

    // Ensure SO_BROADCAST is set
    int broadcastOpt = 1;
    setsockopt(m_udpSock, SOL_SOCKET, SO_BROADCAST,
               (const char*)&broadcastOpt, sizeof(broadcastOpt));

    sockaddr_in dest{};
    dest.sin_family      = AF_INET;
    dest.sin_port        = htons((u_short)port);
    dest.sin_addr.s_addr = INADDR_BROADCAST;

    int sent = sendto(m_udpSock, data.c_str(), (int)data.size(), 0,
                      (sockaddr*)&dest, sizeof(dest));
    if (sent == (int)data.size()) {
        std::lock_guard<std::mutex> slock(m_statsMutex);
        m_stats.bytesSent += data.size();
        m_stats.packetsSent++;
        return true;
    }
    return false;
}

// ============================================================
//  HTTP Client (synchronous, raw sockets)
// ============================================================
bool NetworkManager::parseUrl(const std::string& url, std::string& host, int& port, std::string& path) {
    std::string u = url;
    // Strip http:// or https://
    if (u.rfind("https://", 0) == 0) {
        port = 443;
        u = u.substr(8);
    } else if (u.rfind("http://", 0) == 0) {
        port = 80;
        u = u.substr(7);
    } else {
        port = 80;
    }

    // Find path
    size_t slashPos = u.find('/');
    if (slashPos != std::string::npos) {
        host = u.substr(0, slashPos);
        path = u.substr(slashPos);
    } else {
        host = u;
        path = "/";
    }

    // Check for port in host
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        port = std::stoi(host.substr(colonPos + 1));
        host = host.substr(0, colonPos);
    }

    return !host.empty();
}

std::string NetworkManager::buildHttpRequest(const std::string& method, const std::string& host,
                                              const std::string& path, const std::string& body,
                                              const std::string& contentType) {
    std::ostringstream req;
    req << method << " " << path << " HTTP/1.1\r\n";
    req << "Host: " << host << "\r\n";
    req << "Connection: close\r\n";
    req << "User-Agent: Like2D/1.0\r\n";
    if (!body.empty()) {
        req << "Content-Type: " << contentType << "\r\n";
        req << "Content-Length: " << body.size() << "\r\n";
    }
    req << "Accept: */*\r\n";
    req << "\r\n";
    if (!body.empty()) {
        req << body;
    }
    return req.str();
}

HttpResponse NetworkManager::parseHttpResponse(const std::string& raw) {
    HttpResponse resp;

    // Find end of headers
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        resp.error = "Invalid HTTP response (no header terminator)";
        return resp;
    }

    // Parse status line: "HTTP/1.1 200 OK"
    std::string statusLine = raw.substr(0, raw.find("\r\n"));
    size_t spacePos = statusLine.find(' ');
    if (spacePos != std::string::npos) {
        size_t codeStart = spacePos + 1;
        size_t codeEnd = statusLine.find(' ', codeStart);
        std::string codeStr = (codeEnd != std::string::npos)
            ? statusLine.substr(codeStart, codeEnd - codeStart)
            : statusLine.substr(codeStart);
        try { resp.statusCode = std::stoi(codeStr); }
        catch (...) { resp.statusCode = 0; }
    }

    // Body is everything after \r\n\r\n
    resp.body = raw.substr(headerEnd + 4);

    // Handle chunked transfer encoding
    std::string headers = raw.substr(0, headerEnd);
    std::string headersLower = headers;
    std::transform(headersLower.begin(), headersLower.end(), headersLower.begin(), ::tolower);

    if (headersLower.find("transfer-encoding: chunked") != std::string::npos) {
        // Decode chunked body
        std::string decoded;
        std::string chunk = resp.body;
        while (!chunk.empty()) {
            size_t lineEnd = chunk.find("\r\n");
            if (lineEnd == std::string::npos) break;
            std::string sizeHex = chunk.substr(0, lineEnd);
            size_t chunkSize = 0;
            try { chunkSize = std::stoul(sizeHex, nullptr, 16); }
            catch (...) { break; }
            if (chunkSize == 0) break;
            size_t dataStart = lineEnd + 2;
            if (dataStart + chunkSize > chunk.size()) break;
            decoded += chunk.substr(dataStart, chunkSize);
            chunk = chunk.substr(dataStart + chunkSize + 2); // skip trailing \r\n
        }
        resp.body = decoded;
    }

    return resp;
}

HttpResponse NetworkManager::httpGet(const std::string& url) {
    std::string host, path;
    int port;
    if (!parseUrl(url, host, port, path)) {
        return {0, "", "Invalid URL"};
    }

    // Resolve host
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(port);

    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0 || !result) {
        return {0, "", "DNS resolution failed for " + host};
    }

    Socket sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCK) {
        freeaddrinfo(result);
        return {0, "", "Failed to create socket"};
    }

    // Set receive timeout (10 seconds)
    timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) != 0) {
        SOCK_CLOSE(sock);
        freeaddrinfo(result);
        return {0, "", "Connection failed to " + host + ":" + portStr};
    }
    freeaddrinfo(result);

    // Send HTTP request
    std::string request = buildHttpRequest("GET", host, path, "", "");
    int sent = ::send(sock, request.c_str(), (int)request.size(), 0);
    if (sent != (int)request.size()) {
        SOCK_CLOSE(sock);
        return {0, "", "Failed to send HTTP request"};
    }

    // Receive response
    std::string response;
    char buf[8192];
    while (true) {
        int got = recv(sock, buf, sizeof(buf), 0);
        if (got <= 0) break;
        response.append(buf, got);
    }
    SOCK_CLOSE(sock);

    {
        std::lock_guard<std::mutex> slock(m_statsMutex);
        m_stats.bytesSent += request.size();
        m_stats.bytesReceived += response.size();
        m_stats.packetsSent++;
        m_stats.packetsRecv++;
    }

    return parseHttpResponse(response);
}

HttpResponse NetworkManager::httpPost(const std::string& url, const std::string& body,
                                       const std::string& contentType) {
    std::string host, path;
    int port;
    if (!parseUrl(url, host, port, path)) {
        return {0, "", "Invalid URL"};
    }

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(port);

    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0 || !result) {
        return {0, "", "DNS resolution failed for " + host};
    }

    Socket sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCK) {
        freeaddrinfo(result);
        return {0, "", "Failed to create socket"};
    }

    timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) != 0) {
        SOCK_CLOSE(sock);
        freeaddrinfo(result);
        return {0, "", "Connection failed to " + host + ":" + portStr};
    }
    freeaddrinfo(result);

    std::string request = buildHttpRequest("POST", host, path, body, contentType);
    int sent = ::send(sock, request.c_str(), (int)request.size(), 0);
    if (sent != (int)request.size()) {
        SOCK_CLOSE(sock);
        return {0, "", "Failed to send HTTP request"};
    }

    std::string response;
    char buf[8192];
    while (true) {
        int got = recv(sock, buf, sizeof(buf), 0);
        if (got <= 0) break;
        response.append(buf, got);
    }
    SOCK_CLOSE(sock);

    {
        std::lock_guard<std::mutex> slock(m_statsMutex);
        m_stats.bytesSent += request.size();
        m_stats.bytesReceived += response.size();
        m_stats.packetsSent++;
        m_stats.packetsRecv++;
    }

    return parseHttpResponse(response);
}

// ============================================================
//  Peer Metadata
// ============================================================
void NetworkManager::setPeerMetadata(int peerId, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_metaMutex);
    m_peerMeta[peerId][key] = value;
}

std::string NetworkManager::getPeerMetadata(int peerId, const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_metaMutex);
    auto pit = m_peerMeta.find(peerId);
    if (pit == m_peerMeta.end()) return "";
    auto kit = pit->second.find(key);
    return (kit != pit->second.end()) ? kit->second : "";
}

// ============================================================
//  Statistics
// ============================================================
NetStats NetworkManager::getStats() const {
    std::lock_guard<std::mutex> slock(m_statsMutex);
    NetStats s = m_stats;
    s.peerCount = getPeerCount();
    auto now = std::chrono::steady_clock::now();
    s.uptimeSeconds = std::chrono::duration<double>(now - m_startTime).count();
    return s;
}

// ============================================================
//  Keepalive
// ============================================================
void NetworkManager::setKeepalive(float intervalSeconds, float timeoutSeconds) {
    m_keepaliveInterval = intervalSeconds;
    m_keepaliveTimeout  = timeoutSeconds;
}

void NetworkManager::checkKeepalive() {
    if (m_keepaliveTimeout <= 0.0f) return;

    auto now = std::chrono::steady_clock::now();
    std::vector<int> timedOut;

    {
        std::lock_guard<std::mutex> lock(m_peerMutex);
        for (auto& [id, peer] : m_peers) {
            float idle = std::chrono::duration<float>(now - peer.lastActivity).count();
            if (idle > m_keepaliveTimeout) {
                timedOut.push_back(id);
            }
        }
    }

    for (int id : timedOut) {
        removePeer(id, "timeout");
    }
}

// ============================================================
//  Shutdown
// ============================================================
void NetworkManager::shutdown() {
    m_running = false;

    if (m_tcpListenSock != INVALID_SOCK) {
        SOCK_CLOSE(m_tcpListenSock);
        m_tcpListenSock = INVALID_SOCK;
    }

    if (m_udpSock != INVALID_SOCK) {
        SOCK_CLOSE(m_udpSock);
        m_udpSock = INVALID_SOCK;
    }

    {
        std::lock_guard<std::mutex> lock(m_peerMutex);
        for (auto& [id, peer] : m_peers) {
            SOCK_CLOSE(peer.socket);
        }
        m_peers.clear();
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

// ============================================================
//  Background thread — select() loop
// ============================================================
void NetworkManager::networkThreadFunc() {
    struct ReadBuf {
        uint32_t    expectedLen = 0;
        bool        haveHeader  = false;
        std::string buf;
    };
    std::unordered_map<int, ReadBuf> readBufs;

    constexpr int MAX_RECV = 65536;
    auto lastKeepaliveCheck = std::chrono::steady_clock::now();

    while (m_running.load()) {
        fd_set readfds;
        FD_ZERO(&readfds);

        Socket maxFd = 0;

        if (m_tcpListenSock != INVALID_SOCK) {
            FD_SET(m_tcpListenSock, &readfds);
            if (m_tcpListenSock > maxFd) maxFd = m_tcpListenSock;
        }

        if (m_udpSock != INVALID_SOCK) {
            FD_SET(m_udpSock, &readfds);
            if (m_udpSock > maxFd) maxFd = m_udpSock;
        }

        std::vector<int> peerIds;
        {
            std::lock_guard<std::mutex> lock(m_peerMutex);
            for (auto& [id, peer] : m_peers) {
                FD_SET(peer.socket, &readfds);
                if (peer.socket > maxFd) maxFd = peer.socket;
                peerIds.push_back(id);
            }
        }

        if (maxFd == 0 && m_tcpListenSock == INVALID_SOCK && m_udpSock == INVALID_SOCK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = 100000; // 100ms

        int ready = select((int)(maxFd + 1), &readfds, nullptr, nullptr, &tv);
        if (ready <= 0) {
            // Even if nothing is ready, check keepalive
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration<float>(now - lastKeepaliveCheck).count() >= m_keepaliveInterval) {
                checkKeepalive();
                lastKeepaliveCheck = now;
            }
            continue;
        }

        // --- Accept new TCP connections ---
        if (m_tcpListenSock != INVALID_SOCK && FD_ISSET(m_tcpListenSock, &readfds)) {
            sockaddr_in clientAddr{};
            socklen_t addrLen = sizeof(clientAddr);
            Socket clientSock = accept(m_tcpListenSock, (sockaddr*)&clientAddr, &addrLen);
            if (clientSock != INVALID_SOCK) {
                char ipBuf[INET_ADDRSTRLEN] = {};
                inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuf, sizeof(ipBuf));
                int clientPort = ntohs(clientAddr.sin_port);
                int peerId = addPeer(clientSock, std::string(ipBuf), clientPort);
                pushEvent({NetEventType::Connect, peerId, "", std::string(ipBuf), clientPort});
                std::cout << "[net] New connection from " << ipBuf << ":" << clientPort
                          << " (peer " << peerId << ")" << std::endl;
            }
        }

        // --- Receive UDP data ---
        if (m_udpSock != INVALID_SOCK && FD_ISSET(m_udpSock, &readfds)) {
            char udpBuf[65536];
            sockaddr_in srcAddr{};
            socklen_t addrLen = sizeof(srcAddr);
            int received = recvfrom(m_udpSock, udpBuf, sizeof(udpBuf), 0,
                                    (sockaddr*)&srcAddr, &addrLen);
            if (received > 0) {
                char ipBuf[INET_ADDRSTRLEN] = {};
                inet_ntop(AF_INET, &srcAddr.sin_addr, ipBuf, sizeof(ipBuf));
                int srcPort = ntohs(srcAddr.sin_port);
                pushEvent({NetEventType::Message, 0,
                           std::string(udpBuf, received),
                           std::string(ipBuf), srcPort});
                std::lock_guard<std::mutex> slock(m_statsMutex);
                m_stats.bytesReceived += received;
                m_stats.packetsRecv++;
            }
        }

        // --- Receive TCP data from peers (length-prefixed framing) ---
        std::vector<int> disconnected;

        for (int peerId : peerIds) {
            Socket sock = INVALID_SOCK;
            {
                std::lock_guard<std::mutex> lock(m_peerMutex);
                auto it = m_peers.find(peerId);
                if (it == m_peers.end()) continue;
                sock = it->second.socket;
            }

            if (sock == INVALID_SOCK || !FD_ISSET(sock, &readfds)) continue;

            auto& rb = readBufs[peerId];

            if (!rb.haveHeader) {
                int headerNeeded = 4 - (int)rb.buf.size();
                char hdrBuf[4];
                int got = recv(sock, hdrBuf, headerNeeded, 0);
                if (got <= 0) {
                    disconnected.push_back(peerId);
                    continue;
                }
                rb.buf.append(hdrBuf, got);

                if (rb.buf.size() >= 4) {
                    uint32_t netLen;
                    memcpy(&netLen, rb.buf.data(), 4);
                    rb.expectedLen = ntohl(netLen);
                    rb.haveHeader = true;
                    rb.buf = rb.buf.substr(4);
                }
            }

            if (rb.haveHeader) {
                uint32_t stillNeed = rb.expectedLen - (uint32_t)rb.buf.size();
                if (stillNeed > 0) {
                    int toRead = (int)std::min((uint32_t)MAX_RECV, stillNeed);
                    char payload[65536];
                    int got = recv(sock, payload, toRead, 0);
                    if (got <= 0) {
                        disconnected.push_back(peerId);
                        continue;
                    }
                    rb.buf.append(payload, got);
                }

                if ((uint32_t)rb.buf.size() >= rb.expectedLen) {
                    std::string msg = rb.buf.substr(0, rb.expectedLen);
                    rb.buf = rb.buf.substr(rb.expectedLen);
                    rb.haveHeader = false;
                    rb.expectedLen = 0;

                    // Update stats and lastActivity
                    {
                        std::lock_guard<std::mutex> slock(m_statsMutex);
                        m_stats.bytesReceived += 4 + msg.size();
                        m_stats.packetsRecv++;
                    }
                    {
                        std::lock_guard<std::mutex> lock(m_peerMutex);
                        auto it = m_peers.find(peerId);
                        if (it != m_peers.end()) {
                            it->second.lastActivity = std::chrono::steady_clock::now();
                        }
                    }

                    std::string peerIp;
                    int peerPort = 0;
                    {
                        std::lock_guard<std::mutex> lock(m_peerMutex);
                        auto it = m_peers.find(peerId);
                        if (it != m_peers.end()) {
                            peerIp = it->second.ip;
                            peerPort = it->second.port;
                        }
                    }

                    pushEvent({NetEventType::Message, peerId, std::move(msg), peerIp, peerPort});
                }
            }
        }

        for (int peerId : disconnected) {
            readBufs.erase(peerId);
            removePeer(peerId, "disconnected");
        }

        // Keepalive check
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<float>(now - lastKeepaliveCheck).count() >= m_keepaliveInterval) {
            checkKeepalive();
            lastKeepaliveCheck = now;
        }
    }
}
