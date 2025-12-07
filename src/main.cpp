#include "websocket_client.h"
#include "audio_handler.h"
#include "message_handler.h"
#include "config.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <queue>
#include <mutex>
#include <functional>

// Global exit flag
std::atomic<bool> g_running(true);

// Queue to store received audio data
std::queue<std::vector<int16_t>> g_audio_queue;
std::mutex g_audio_mutex;

// Signal handler
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nReceived termination signal..." << std::endl;
        g_running = false;
    }
}

// Show help message
void print_help() {
    std::cout << "Usage: gemini-voice [options]\n" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --api-key KEY, -k KEY    Specify API key" << std::endl;
    std::cout << "  --config PATH, -c PATH   Path to config file (default: ./config.json)" << std::endl;
    std::cout << "  --dummy-audio            Dummy audio mode (no audio device required)" << std::endl;
    std::cout << "  --enable-search          Enable Google Search (overrides config file)" << std::endl;
    std::cout << "  --help, -h               Show this help message" << std::endl;
    std::cout << "\nEnvironment Variables:" << std::endl;
    std::cout << "  GEMINI_API_KEY           API Key (lower priority than command line argument)" << std::endl;
    std::cout << "  PULSE_SERVER             PulseAudio server (for WSL environment)" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  export GEMINI_API_KEY=\"your_key\"" << std::endl;
    std::cout << "  ./gemini-voice" << std::endl;
    std::cout << "  ./gemini-voice --config my_config.json" << std::endl;
    std::cout << "  ./gemini-voice --api-key your_key --dummy-audio --enable-search" << std::endl;
}

// Check command line arguments
bool has_flag(int argc, char* argv[], const std::string& flag1, const std::string& flag2 = "") {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == flag1 || (!flag2.empty() && arg == flag2)) {
            return true;
        }
    }
    return false;
}

// Get API key
std::string get_api_key(int argc, char* argv[]) {
    // Check command line arguments
    for (int i = 1; i < argc - 1; i++) {
        std::string arg = argv[i];
        if (arg == "--api-key" || arg == "-k") {
            return argv[i + 1];
        }
    }
    
    // Check environment variable
    const char* env_key = std::getenv("GEMINI_API_KEY");
    if (env_key) {
        return std::string(env_key);
    }
    
    return "";
}

// Get config file path
std::string get_config_path(int argc, char* argv[]) {
    for (int i = 1; i < argc - 1; i++) {
        std::string arg = argv[i];
        if (arg == "--config" || arg == "-c") {
            return argv[i + 1];
        }
    }
    return "config.json";  // Default path
}

int main(int argc, char* argv[]) {
    // Check help option
    if (has_flag(argc, argv, "--help", "-h")) {
        print_help();
        return 0;
    }
    
    // Set signal handler
    std::signal(SIGINT, signal_handler);
    
    std::cout << "=== Gemini Live API Voice Application ===" << std::endl;
    
    // Load config file
    std::string config_path = get_config_path(argc, argv);
    Config config;
    
    // Load if file exists
    std::ifstream config_file(config_path);
    if (config_file.good()) {
        try {
            config = Config(config_path);
            std::cout << "Loaded config file: " << config_path << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Config load error: " << e.what() << std::endl;
            std::cerr << "Using default configuration" << std::endl;
        }
    } else {
        std::cout << "Config file not found. Using default configuration" << std::endl;
    }
    config_file.close();
    
    // Print config info
    config.print();
    
    // Check dummy audio mode
    bool dummy_audio = has_flag(argc, argv, "--dummy-audio");
    if (dummy_audio) {
        std::cout << "[Dummy Audio Mode] Running without audio device" << std::endl;
    }
    
    // Check Google Search feature (CLI arg priority)
    bool enable_search = has_flag(argc, argv, "--enable-search") || config.isSearchEnabled();
    if (enable_search) {
        std::cout << "[Google Search] Enabled" << std::endl;
    }
    
    // Get API key
    std::string api_key = get_api_key(argc, argv);
    if (api_key.empty()) {
        std::cerr << "Error: API key is not set" << std::endl;
        std::cerr << "Usage:" << std::endl;
        std::cerr << "  Env: export GEMINI_API_KEY=your_api_key" << std::endl;
        std::cerr << "  CLI: ./gemini-voice --api-key your_api_key" << std::endl;
        std::cerr << "\nHelp: ./gemini-voice --help" << std::endl;
        return 1;
    }
    
    // Initialize audio handler (only if not in dummy mode)
    AudioHandler audio_handler;
    if (!dummy_audio) {
        if (!audio_handler.initialize()) {
            std::cerr << "Failed to initialize audio handler" << std::endl;
            std::cerr << "Hint: Use --dummy-audio option to run without audio" << std::endl;
            return 1;
        }
    } else {
        std::cout << "Audio handler: Skipped (Dummy Mode)" << std::endl;
    }
    
    // Create IO context
    boost::asio::io_context io_context;
    
    // Create WebSocket client
    WebSocketClient ws_client(io_context, api_key);
    
    // Buffer for AI transcription
    std::string user_transcript_buffer;
    std::string ai_transcript_buffer;
    
    // Set message receive callback
    ws_client.set_message_callback([&](const std::string& message) {
        // std::cout << "受信: " << message.substr(0, 200) << "..." << std::endl;
        
        // Extract transcription
        std::string transcription;
        if (MessageHandler::extract_transcription_from_response(message, transcription)) {
            // Determine if it is user input or AI output
            if (MessageHandler::is_user_input_transcription(message)) {
                user_transcript_buffer += transcription;
                // Display at punctuation marks
                if (user_transcript_buffer.find("。") != std::string::npos ||
                    user_transcript_buffer.find("！") != std::string::npos ||
                    user_transcript_buffer.find("？") != std::string::npos ||
                    user_transcript_buffer.find(".") != std::string::npos ||
                    user_transcript_buffer.find("!") != std::string::npos ||
                    user_transcript_buffer.find("?") != std::string::npos) {
                    std::cout << "\n You: " << user_transcript_buffer << "\n" << std::endl;
                    user_transcript_buffer.clear();
                }
            } else {
                ai_transcript_buffer += transcription;
                // Display at punctuation marks
                if (ai_transcript_buffer.find("。") != std::string::npos ||
                    ai_transcript_buffer.find("！") != std::string::npos ||
                    ai_transcript_buffer.find("？") != std::string::npos ||
                    ai_transcript_buffer.find(".") != std::string::npos ||
                    ai_transcript_buffer.find("!") != std::string::npos ||
                    ai_transcript_buffer.find("?") != std::string::npos) {
                    std::cout << " AI: " << ai_transcript_buffer << std::endl;
                    ai_transcript_buffer.clear();
                }
            }
        }
        
        // Extract audio data
        std::vector<int16_t> audio_data;
        if (MessageHandler::extract_audio_from_response(message, audio_data)) {
            // std::cout << "音声データを受信しました (" << audio_data.size() << " サンプル)" << std::endl;
            
            // Add to queue
            std::lock_guard<std::mutex> lock(g_audio_mutex);
            g_audio_queue.push(audio_data);
        }
        
        // Check turn completion
        if (MessageHandler::is_turn_complete(message)) {
            // std::cout << "ターン完了" << std::endl;
        }
    });
    
    // Set error callback (stop app on error)
    ws_client.set_error_callback([](const std::string& error) {
        std::cerr << "WebSocket Error: " << error << std::endl;
        g_running = false;  // Exit on error
    });
    
    // Connect to WebSocket
    // std::cout << "Gemini Live APIに接続中..." << std::endl;
    if (!ws_client.connect()) {
        std::cerr << "Failed to connect" << std::endl;
        return 1;
    }
    
    // Send setup message
    std::string setup_message = MessageHandler::create_setup_message(
        config.getModelName(),
        enable_search,
        config.getTemperature(),
        config.getTopP(),
        config.getTopK(),
        config.getSystemInstructionText()
    );
    // std::cout << "セットアップメッセージを送信" << std::endl;
    ws_client.send(setup_message);
    
    // Start async receive
    ws_client.async_receive();
    
    // Wait for setup response from server
    // std::cout << "サーバーからの応答を待機中..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check connection status
    if (!g_running || !ws_client.is_connected()) {
        std::cerr << "Error: Connection lost with server" << std::endl;
        ws_client.close();
        io_context.stop();
        return 1;
    }
    
    // Run IO context in a separate thread
    std::thread io_thread([&io_context]() {
        io_context.run();
    });
    
    // Audio playback thread
    std::thread playback_thread([&audio_handler, &config, dummy_audio]() {
        std::vector<int16_t> audio_buffer;
        const size_t BUFFER_SIZE = config.getBufferSize();
        const size_t MIN_BUFFER_SIZE = config.getMinBufferSize();
        
        while (g_running) {
            // Retrieve audio data from queue and add to buffer
            {
                std::lock_guard<std::mutex> lock(g_audio_mutex);
                while (!g_audio_queue.empty()) {
                    std::vector<int16_t>& data = g_audio_queue.front();
                    audio_buffer.insert(audio_buffer.end(), data.begin(), data.end());
                    g_audio_queue.pop();
                }
            }
            
            // Play if buffer has enough data
            if (audio_buffer.size() >= BUFFER_SIZE) {
                if (!dummy_audio) {
                    // Play only buffer size
                    std::vector<int16_t> to_play(audio_buffer.begin(), audio_buffer.begin() + BUFFER_SIZE);
                    audio_handler.play_audio(to_play, config.getOutputSampleRate());
                    audio_buffer.erase(audio_buffer.begin(), audio_buffer.begin() + BUFFER_SIZE);
                } else {
                    std::cout << "[Dummy] Discarding audio data (" << audio_buffer.size() << " samples)" << std::endl;
                    audio_buffer.clear();
                }
            }
            // Play if minimum buffer exists and no new data coming
            else if (audio_buffer.size() >= MIN_BUFFER_SIZE) {
                bool queue_empty;
                {
                    std::lock_guard<std::mutex> lock(g_audio_mutex);
                    queue_empty = g_audio_queue.empty();
                }
                
                if (queue_empty && !dummy_audio) {
                    audio_handler.play_audio(audio_buffer, config.getOutputSampleRate());
                    audio_buffer.clear();
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Recording callback
    std::vector<int16_t> accumulated_audio;
    const size_t CHUNK_SIZE = config.getChunkSize();
    
    std::function<void(const std::vector<int16_t>&)> recording_callback = [&](const std::vector<int16_t>& audio_chunk) {
        // Accumulate audio data (apply software gain)
        const int32_t GAIN_FACTOR = config.getGainFactor();
        
        for (int16_t sample : audio_chunk) {
            int32_t amplified = static_cast<int32_t>(sample) * GAIN_FACTOR;
            if (amplified > 32767) amplified = 32767;
            if (amplified < -32768) amplified = -32768;
            accumulated_audio.push_back(static_cast<int16_t>(amplified));
        }
        
        // Send when accumulated enough
        if (accumulated_audio.size() >= CHUNK_SIZE) {
            std::string audio_message = MessageHandler::create_audio_input_message(accumulated_audio);
            // std::cout << "音声データを送信中 (" << accumulated_audio.size() << " サンプル)" << std::endl;
            ws_client.send(audio_message);
            accumulated_audio.clear();
        }
    };
    
    // Start recording (send silent data in dummy mode)
    if (dummy_audio) {
        std::cout << "\n[Dummy Mode] Waiting for text input..." << std::endl;
        std::cout << "Press Ctrl+C to exit\n" << std::endl;
        
        // Dummy mode: Periodically send silent data
        std::thread dummy_thread([&]() {
            while (g_running && ws_client.is_connected()) {
                std::vector<int16_t> silent_audio(CHUNK_SIZE, 0);  // Silence data
                std::string audio_message = MessageHandler::create_audio_input_message(silent_audio);
                std::cout << "[Dummy] Sending silence (" << silent_audio.size() << " samples)" << std::endl;
                ws_client.send(audio_message);
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
        
        // Main loop
        while (g_running && ws_client.is_connected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (dummy_thread.joinable()) {
            dummy_thread.join();
        }
    } else {
        std::cout << "Starting recording. Please speak...\n" << std::endl;
        // std::cout << "終了するには Ctrl+C を押してください\n" << std::endl;
        
        if (!audio_handler.start_recording(recording_callback)) {
            std::cerr << "Failed to start recording" << std::endl;
            std::cerr << "Hint: Use --dummy-audio option to run without audio" << std::endl;
            g_running = false;
        }
        
        // Main loop
        while (g_running && ws_client.is_connected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        audio_handler.stop_recording();
    }
    
    // Cleanup
    std::cout << "\nCleaning up..." << std::endl;
    ws_client.close();
    io_context.stop();
    
    if (io_thread.joinable()) {
        io_thread.join();
    }
    
    if (playback_thread.joinable()) {
        playback_thread.join();
    }
    
    std::cout << "Application exited" << std::endl;
    
    return 0;
}
