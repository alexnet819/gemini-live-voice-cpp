#include "audio_handler.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <functional>

std::atomic<bool> g_running(true);

void signal_handler(int signal) {
    if (signal == SIGINT) {
        g_running = false;
    }
}

int main() {
    std::signal(SIGINT, signal_handler);
    
    std::cout << "=== Audio Recording Test ===" << std::endl;
    
    AudioHandler audio_handler;
    if (!audio_handler.initialize()) {
        std::cerr << "Initialization failed" << std::endl;
        return 1;
    }
    
    std::vector<int16_t> recorded_audio;
    
    std::function<void(const std::vector<int16_t>&)> callback = [&](const std::vector<int16_t>& chunk) {
        // Show volume level (simple VU meter)
        double sum = 0;
        for (int16_t sample : chunk) {
            sum += std::abs(sample);
        }
        double avg = sum / chunk.size();
        
        std::cout << "\rRecording... Level: " << avg << " [";
        int bars = static_cast<int>(avg / 100);
        for (int i = 0; i < 20; i++) {
            if (i < bars) std::cout << "#";
            else std::cout << " ";
        }
        std::cout << "]" << std::flush;
        
        recorded_audio.insert(recorded_audio.end(), chunk.begin(), chunk.end());
    };
    
    std::cout << "Starting recording (for 5 seconds)..." << std::endl;
    if (!audio_handler.start_recording(callback)) {
        std::cerr << "Failed to start recording" << std::endl;
        return 1;
    }
    
    // Record for 5 seconds
    for (int i = 0; i < 50 && g_running; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    audio_handler.stop_recording();
    std::cout << "\nRecording finished. Total samples: " << recorded_audio.size() << std::endl;
    
    if (!recorded_audio.empty()) {
        std::cout << "Playing back recorded audio..." << std::endl;
        audio_handler.play_audio(recorded_audio, 16000);
    }
    
    return 0;
}
