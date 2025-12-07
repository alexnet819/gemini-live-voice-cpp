#include "websocket_client.h"
#include "message_handler.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

int main(int argc, char* argv[]) {
    std::cout << "=== WebSocket Communication Test ===" << std::endl;
    
    const char* env_key = std::getenv("GEMINI_API_KEY");
    if (!env_key) {
        std::cerr << "Error: GEMINI_API_KEY environment variable not set" << std::endl;
        return 1;
    }
    
    boost::asio::io_context io_context;
    WebSocketClient ws_client(io_context, env_key);
    
    bool connected = false;
    std::atomic<bool> running(true);
    
    ws_client.set_message_callback([&](const std::string& message) {
        std::cout << "Received: " << message.substr(0, 100) << "..." << std::endl;
        if (MessageHandler::is_turn_complete(message)) {
            std::cout << "Turn Complete received" << std::endl;
            running = false;
        }
    });
    
    ws_client.set_error_callback([&](const std::string& error) {
        std::cerr << "Error: " << error << std::endl;
        running = false;
    });
    
    std::cout << "Connecting..." << std::endl;
    if (!ws_client.connect()) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }
    connected = true;
    
    // Send setup message
    std::string setup = MessageHandler::create_setup_message("gemini-2.5-flash-native-audio-preview-09-2025");
    std::cout << "Sending: " << setup << std::endl;
    ws_client.send(setup);
    
    ws_client.async_receive();
    
    std::thread io_thread([&]() {
        io_context.run();
    });
    
    // Wait for 5 seconds or completion
    int timeout = 50;
    while (running && timeout-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (connected) {
        ws_client.close();
    }
    
    io_context.stop();
    if (io_thread.joinable()) io_thread.join();
    
    return 0;
}
