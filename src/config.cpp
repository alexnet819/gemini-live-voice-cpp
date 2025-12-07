#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

Config::Config() {
    setDefaults();
}

Config::Config(const std::string& config_path) {
    setDefaults();
    loadFromFile(config_path);
}

void Config::setDefaults() {
    // Default model settings
    model_name_ = "gemini-2.5-flash-native-audio-preview-09-2025";
    temperature_ = 1.0;
    top_p_ = 0.95;
    top_k_ = 40;
    response_modalities_ = {"AUDIO"};

    // Default feature settings
    enable_search_ = false;
    input_audio_transcription_ = true;
    output_audio_transcription_ = true;

    // Default system instruction
    system_instruction_text_ = "";

    // Default audio settings
    input_sample_rate_ = 16000;
    output_sample_rate_ = 24000;
    chunk_size_ = 16000;
    buffer_size_ = 24000;
    min_buffer_size_ = 7200;
    gain_factor_ = 5;
}

void Config::loadFromFile(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open config file: " + config_path);
        }

        json config_json;
        file >> config_json;

        // Model settings
        if (config_json.contains("model")) {
            nlohmann::json& model = config_json["model"];
            if (model.contains("name")) {
                model_name_ = model["name"].get<std::string>();
            }
            if (model.contains("temperature")) {
                temperature_ = model["temperature"].get<double>();
            }
            if (model.contains("topP")) {
                top_p_ = model["topP"].get<double>();
            }
            if (model.contains("topK")) {
                top_k_ = model["topK"].get<int>();
            }
            if (model.contains("responseModalities")) {
                response_modalities_ = model["responseModalities"].get<std::vector<std::string>>();
            }
        }

        // Feature settings
        if (config_json.contains("features")) {
            nlohmann::json& features = config_json["features"];
            if (features.contains("enableSearch")) {
                enable_search_ = features["enableSearch"].get<bool>();
            }
            if (features.contains("inputAudioTranscription")) {
                input_audio_transcription_ = features["inputAudioTranscription"].get<bool>();
            }
            if (features.contains("outputAudioTranscription")) {
                output_audio_transcription_ = features["outputAudioTranscription"].get<bool>();
            }
        }

        // System instruction
        if (config_json.contains("systemInstruction")) {
            nlohmann::json& system_instruction = config_json["systemInstruction"];
            if (system_instruction.contains("text")) {
                system_instruction_text_ = system_instruction["text"].get<std::string>();
            }
        }

        // Audio settings
        if (config_json.contains("audio")) {
            nlohmann::json& audio = config_json["audio"];
            if (audio.contains("inputSampleRate")) {
                input_sample_rate_ = audio["inputSampleRate"].get<int>();
            }
            if (audio.contains("outputSampleRate")) {
                output_sample_rate_ = audio["outputSampleRate"].get<int>();
            }
            if (audio.contains("chunkSize")) {
                chunk_size_ = audio["chunkSize"].get<int>();
            }
            if (audio.contains("bufferSize")) {
                buffer_size_ = audio["bufferSize"].get<int>();
            }
            if (audio.contains("minBufferSize")) {
                min_buffer_size_ = audio["minBufferSize"].get<int>();
            }
            if (audio.contains("gainFactor")) {
                gain_factor_ = audio["gainFactor"].get<int>();
            }
        }

    } catch (const json::exception& e) {
        throw std::runtime_error("JSON Parse Error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Config Load Error: " + std::string(e.what()));
    }
}

void Config::print() const {
    std::cout << "=== Configuration ===" << std::endl;
    std::cout << "Model Name: " << model_name_ << std::endl;
    std::cout << "Temperature: " << temperature_ << std::endl;
    std::cout << "Top-P: " << top_p_ << std::endl;
    std::cout << "Top-K: " << top_k_ << std::endl;
    std::cout << "Google Search: " << (enable_search_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Input Audio Transcription: " << (input_audio_transcription_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Output Audio Transcription: " << (output_audio_transcription_ ? "Enabled" : "Disabled") << std::endl;
    if (!system_instruction_text_.empty()) {
        std::cout << "System Instruction: " << system_instruction_text_ << std::endl;
    }
    std::cout << "==================" << std::endl;
}
