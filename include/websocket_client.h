#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <functional>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

/**
 * @brief WebSocketクライアントクラス
 * 
 * Gemini Live APIとのWebSocket通信を管理します。
 */
class WebSocketClient {
public:
    using MessageCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    /**
     * @brief WebSocketClientのコンストラクタ
     * 
     * @param io_context Boost.Asio IOコンテキスト
     * @param api_key Gemini APIキー
     */
    WebSocketClient(net::io_context& io_context, const std::string& api_key);

    /**
     * @brief デストラクタ
     */
    ~WebSocketClient();

    /**
     * @brief メッセージ受信時のコールバックを設定
     * 
     * @param callback メッセージを受け取るコールバック関数
     */
    void set_message_callback(MessageCallback callback);

    /**
     * @brief エラー発生時のコールバックを設定
     * 
     * @param callback エラーメッセージを受け取るコールバック関数
     */
    void set_error_callback(ErrorCallback callback);

    /**
     * @brief Gemini Live APIに接続
     * 
     * @return true 接続成功
     * @return false 接続失敗
     */
    bool connect();

    /**
     * @brief メッセージを送信
     * 
     * @param message 送信するJSON文字列
     */
    void send(const std::string& message);

    /**
     * @brief メッセージを非同期で受信
     */
    void async_receive();

    /**
     * @brief WebSocket接続を閉じる
     */
    void close();

    /**
     * @brief 接続状態を確認
     * 
     * @return true 接続中
     * @return false 未接続
     */
    bool is_connected() const;

private:
    void do_ssl_handshake();
    void do_websocket_handshake();
    void do_read();

    net::io_context& io_context_;
    ssl::context ssl_ctx_;
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws_;
    tcp::resolver resolver_;
    beast::flat_buffer buffer_;
    
    std::string api_key_;
    std::string host_;
    std::string port_;
    std::string target_;
    
    MessageCallback message_callback_;
    ErrorCallback error_callback_;
    
    bool connected_;
};
