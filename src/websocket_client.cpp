#include "websocket_client.h"
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/core/error.hpp>
#include <iostream>

// Helper function to load root certificates
void load_root_certificates(ssl::context& ctx) {
    // Use system default certificate store
    ctx.set_default_verify_paths();
}

WebSocketClient::WebSocketClient(net::io_context& io_context, const std::string& api_key)
    : io_context_(io_context)
    , ssl_ctx_(ssl::context::tlsv12_client)
    , resolver_(io_context)
    , api_key_(api_key)
    , host_("generativelanguage.googleapis.com")
    , port_("443")
    , connected_(false) {
    
    // SSL certificate verification settings
    load_root_certificates(ssl_ctx_);
    ssl_ctx_.set_verify_mode(ssl::verify_peer);
    
    // WebSocket target path (add API key as query parameter)
    target_ = "/ws/google.ai.generativelanguage.v1beta.GenerativeService.BidiGenerateContent?key=" + api_key_;
}

WebSocketClient::~WebSocketClient() {
    if (connected_) {
        close();
    }
}

void WebSocketClient::set_message_callback(MessageCallback callback) {
    message_callback_ = std::move(callback);
}

void WebSocketClient::set_error_callback(ErrorCallback callback) {
    error_callback_ = std::move(callback);
}

bool WebSocketClient::connect() {
    try {
        // Resolve host name
        boost::asio::ip::tcp::resolver::results_type const results = resolver_.resolve(host_, port_);
        
        // Create WebSocket stream
        ws_ = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(
            io_context_, ssl_ctx_);
        
        // TCP Connect
        net::connect(beast::get_lowest_layer(*ws_), results);
        
        // Set SNI (Server Name Indication)
        if (!SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), host_.c_str())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }
        
        // SSL Handshake
        ws_->next_layer().handshake(ssl::stream_base::client);
        
        // Set decorator to add User-Agent
        ws_->set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(http::field::user_agent, "Gemini-CPP-Client/1.0");
            }));
        
        // WebSocket Handshake
        ws_->handshake(host_, target_);
        
        connected_ = true;
        
        // std::cout << "WebSocket Connected: " << host_ << std::endl;
        
        return true;
        
    } catch (std::exception const& e) {
        std::cerr << "Connection Error: " << e.what() << std::endl;
        if (error_callback_) {
            error_callback_(std::string("Connection Error: ") + e.what());
        }
        connected_ = false;
        return false;
    }
}

void WebSocketClient::send(const std::string& message) {
    if (!connected_ || !ws_) {
        std::cerr << "Error: WebSocket is not connected" << std::endl;
        return;
    }
    
    // Execute send in io_context thread (for thread safety)
    net::post(io_context_, [this, message]() {
        if (!connected_ || !ws_) return;
        
        try {
            ws_->write(net::buffer(message));
        } catch (std::exception const& e) {
            std::cerr << "Send Error: " << e.what() << std::endl;
            if (error_callback_) {
                error_callback_(std::string("Send Error: ") + e.what());
            }
        }
    });
}

void WebSocketClient::async_receive() {
    if (!connected_ || !ws_) {
        return;
    }
    
    ws_->async_read(
        buffer_,
        [this](beast::error_code ec, std::size_t /* bytes_transferred */) {
            if (ec) {
                if (ec != websocket::error::closed) {
                    std::cerr << "Read Error: " << ec.message() << std::endl;
                    if (error_callback_) {
                        error_callback_(std::string("Read Error: ") + ec.message());
                    }
                }
                connected_ = false;
                return;
            }
            
            // Convert received data to string
            std::string message = beast::buffers_to_string(buffer_.data());
            buffer_.consume(buffer_.size());
            
            // Invoke callback
            if (message_callback_) {
                message_callback_(message);
            }
            
            // Async receive next message
            async_receive();
        });
}

void WebSocketClient::close() {
    if (!connected_ || !ws_) {
        return;
    }
    
    try {
        ws_->close(websocket::close_code::normal);
        connected_ = false;
        // std::cout << "WebSocket connection closed" << std::endl;
    } catch (std::exception const& e) {
        std::cerr << "Close Error: " << e.what() << std::endl;
    }
}

bool WebSocketClient::is_connected() const {
    return connected_;
}
