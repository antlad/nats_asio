#pragma once

#include <string>
#include <functional>
#include <unordered_map>

namespace nats_asio {

using MessageCallback = std::function<void(std::string &&message, std::string &&inbox, std::string &&subject)>;
using ConnectCallback = std::function<void()>;


const std::string CLRF = "\r\n";
const std::string lang = "cpp";
const std::string version = "0.0.1";

struct options
{
    bool verbose = false;
    bool pedantic = false;
    bool ssl_required = false;
    bool ssl = false;
    bool ssl_verify = true;
    std::string ssl_key;
    std::string ssl_cert;
    std::string ssl_ca;
    std::string name = "nats-asio";
    std::string user;
    std::string pass;
    std::string token;
};


class subscription
{

public:
    //explicit Subscription(QObject *parent = nullptr): QObject(parent) {}

    std::string subject;
    std::string message;
    std::string inbox;
    uint64_t ssid = 0;

//signals:
//    void received();
};


class client2
{
public:
//    explicit Client(QObject *parent = nullptr);

    void publish(const std::string &subject, const std::string &message, const std::string &inbox);
    void publish(const std::string &subject, const std::string &message = "");

    uint64_t subscribe(const std::string &subject, MessageCallback callback);

    uint64_t subscribe(const std::string &subject, const std::string &queue, MessageCallback callback);

    subscription *subscribe(const std::string &subject);

    void unsubscribe(uint64_t ssid, int max_messages = 0);

    uint64_t request(const std::string& subject, const std::string& message, MessageCallback callback);
    uint64_t request(const std::string&, MessageCallback callback);

//signals:
//    void connected();
//    void error(const QString);
//    void disconnected();

public:

    void connect(const std::string &host = "127.0.0.1", uint16_t port = 4222, ConnectCallback callback = nullptr);
    void connect(const std::string &host, uint16_t port, const options &options, ConnectCallback callback = nullptr);

    void disconnect();

    bool connect_sync(const std::string &host = "127.0.0.1", uint16_t port = 4222);
    bool connect_sync(const std::string &host, uint16_t port, const options &options);

private:

    bool _debug_mode = false;



    std::vector<uint8_t> m_buffer;

    uint64_t m_ssid = 0;

    //    QSslSocket m_socket;

    options m_options;

    std::unordered_map<uint64_t, MessageCallback> m_callbacks;

    void send_info(const options &options);

//    QJsonObject parse_info(const QByteArray &message);


    void set_listeners();


//    bool process_inboud(const QByteArray &buffer);
};

}
