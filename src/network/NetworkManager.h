#pragma once
// ============================================================
//  NetworkManager — Built-in TCP + UDP + HTTP for Like2D
// ============================================================
//  Native sockets: Winsock2 on Windows, POSIX on Linux.
//  Background thread handles accept/read via select().
//  Main thread polls an event queue and fires Lua callbacks.
//
//  Features:
//   - TCP server/client with length-prefixed framing
//   - UDP send/receive + LAN broadcast
//   - HTTP GET/POST (raw sockets, no libcurl)
//   - Per-peer metadata (key-value store)
//   - Keepalive ping/timeout
//   - Network statistics (bytes sent/received, uptime)
//   - sendToAllExcept for selective broadcast
// ============================================================

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <chrono>

// ---------- Platform socket abstraction ----------
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using Socket = SOCKET;
    #define INVALID_SOCK INVALID_SOCKET
    #define SOCK_CLOSE(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <netdb.h>
    using Socket = int;
    #define INVALID_SOCK (-1)
    #define SOCK_CLOSE(s) ::close(s)
#endif

// ---------- Network event types ----------
enum class NetEventType {
    Connect,
    Message,
    Disconnect
};

struct NetworkEvent {
    NetEventType type;
    int          peerId;
    std::string  data;   // message payload or disconnect reason
    std::string  ip;
    int          port;
};

// ---------- Internal peer info ----------
struct PeerInfo {
    int         id;
    Socket      socket;
    std::string ip;
    int         port;
    std::chrono::steady_clock::time_point lastActivity;
};

// ---------- Network statistics ----------
struct NetStats {
    uint64_t bytesSent     = 0;
    uint64_t bytesReceived = 0;
    uint64_t packetsSent   = 0;
    uint64_t packetsRecv   = 0;
    int      peerCount     = 0;
    double   uptimeSeconds = 0.0;
};

// ---------- HTTP response ----------
struct HttpResponse {
    int         statusCode = 0;
    std::string body;
    std::string error;
};

// ---------- NetworkManager ----------
class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // Initialise Winsock (Windows only). Call once at startup.
    static bool initSockets();

    // ── TCP ──
    bool createServer(int port);
    int  connectTo(const std::string& host, int port);
    bool sendTo(int peerId, const std::string& data);
    bool sendToAll(const std::string& data);
    bool sendToAllExcept(int excludePeerId, const std::string& data);
    void disconnectPeer(int peerId, const std::string& reason = "kicked");

    // ── UDP ──
    bool createUDP(int port);
    bool sendUDP(const std::string& ip, int port, const std::string& data);
    bool broadcastUDP(int port, const std::string& data);

    // ── HTTP (synchronous, blocks until response) ──
    HttpResponse httpGet(const std::string& url);
    HttpResponse httpPost(const std::string& url, const std::string& body,
                          const std::string& contentType = "application/json");

    // ── Peer metadata ──
    void        setPeerMetadata(int peerId, const std::string& key, const std::string& value);
    std::string getPeerMetadata(int peerId, const std::string& key) const;

    // ── Statistics ──
    NetStats getStats() const;

    // ── Keepalive ──
    void setKeepalive(float intervalSeconds, float timeoutSeconds);

    // ── Lifecycle ──
    void shutdown();

    // ── Query ──
    int         getPeerCount() const;
    std::string getPeerIP(int peerId) const;

    // Called by Engine each frame — drains event queue into outEvents
    void poll(std::vector<NetworkEvent>& outEvents);

private:
    // Background thread entry
    void networkThreadFunc();

    // Keepalive check (called from network thread)
    void checkKeepalive();

    // HTTP helpers
    static bool parseUrl(const std::string& url, std::string& host, int& port, std::string& path);
    static std::string buildHttpRequest(const std::string& method, const std::string& host,
                                        const std::string& path, const std::string& body,
                                        const std::string& contentType);
    static HttpResponse parseHttpResponse(const std::string& raw);

    // Helpers
    int  addPeer(Socket sock, const std::string& ip, int port);
    void removePeer(int peerId, const std::string& reason);
    void pushEvent(const NetworkEvent& ev);

    // Thread-safe event queue
    std::mutex                    m_mutex;
    std::queue<NetworkEvent>      m_events;

    // Peers
    mutable std::mutex            m_peerMutex;
    std::unordered_map<int, PeerInfo> m_peers;
    std::atomic<int>              m_nextPeerId{1};

    // Per-peer metadata: peerId -> (key -> value)
    mutable std::mutex            m_metaMutex;
    std::unordered_map<int, std::unordered_map<std::string, std::string>> m_peerMeta;

    // Sockets
    Socket                        m_tcpListenSock = INVALID_SOCK;
    Socket                        m_udpSock       = INVALID_SOCK;

    // Thread control
    std::thread                   m_thread;
    std::atomic<bool>             m_running{false};

    // Statistics
    mutable std::mutex            m_statsMutex;
    NetStats                      m_stats;
    std::chrono::steady_clock::time_point m_startTime;

    // Keepalive config
    float                         m_keepaliveInterval = 5.0f;  // seconds between pings
    float                         m_keepaliveTimeout  = 15.0f; // seconds before disconnect
};
