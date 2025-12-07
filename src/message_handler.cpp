#include "message_handler.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>

using json = nlohmann::json;

// Table for Base64 encoding
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string MessageHandler::base64_encode(const std::vector<uint8_t>& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];
    size_t in_len = data.size();
    const uint8_t* bytes_to_encode = data.data();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

std::vector<uint8_t> MessageHandler::base64_decode(const std::string& encoded_string) {
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];
    std::vector<uint8_t> ret;

    while (in_len-- && (encoded_string[in_] != '=') && 
           (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/'))) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<uint8_t>(base64_chars.find(char_array_4[i]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = static_cast<uint8_t>(base64_chars.find(char_array_4[j]));

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++) ret.push_back(char_array_3[j]);
    }

    return ret;
}

std::vector<uint8_t> MessageHandler::int16_to_uint8(const std::vector<int16_t>& int16_data) {
    std::vector<uint8_t> uint8_data;
    uint8_data.reserve(int16_data.size() * 2);
    
    for (int16_t sample : int16_data) {
        // Convert to bytes in Little-endian format
        uint8_data.push_back(static_cast<uint8_t>(sample & 0xFF));
        uint8_data.push_back(static_cast<uint8_t>((sample >> 8) & 0xFF));
    }
    
    return uint8_data;
}

std::string MessageHandler::create_setup_message(
    const std::string& model_name,
    bool enable_search,
    double temperature,
    double top_p,
    int top_k,
    const std::string& system_instruction_text) {
    
    json setup = {
        {"setup", {
            {"model", "models/" + model_name},
            {"generationConfig", {
                {"responseModalities", {"AUDIO"}},
                {"temperature", temperature},
                {"topP", top_p},
                {"topK", top_k}
            }},
            {"inputAudioTranscription", json::object()},
            {"outputAudioTranscription", json::object()}
        }}
    };
    
    // Add system instruction
    if (!system_instruction_text.empty()) {
        setup["setup"]["systemInstruction"] = {
            {"parts", {{
                {"text", system_instruction_text}
            }}}
        };
    }
    
    // Add tools if Google Search is enabled
    if (enable_search) {
        setup["setup"]["tools"] = json::array({
            {{"googleSearch", json::object()}}
        });
    }
    
    return setup.dump();
}

std::string MessageHandler::create_audio_input_message(
    const std::vector<int16_t>& audio_data,
    const std::string& mime_type) {
    
    // Convert int16 array to uint8 array
    std::vector<uint8_t> byte_data = int16_to_uint8(audio_data);
    
    // Base64 encode
    std::string encoded_audio = base64_encode(byte_data);
    
    json message = {
        {"realtimeInput", {
            {"mediaChunks", {{
                {"data", encoded_audio},
                {"mimeType", mime_type}
            }}}
        }}
    };
    
    return message.dump();
}

bool MessageHandler::extract_audio_from_response(
    const std::string& json_response,
    std::vector<int16_t>& audio_data) {
    
    try {
        json response = json::parse(json_response);
        
        // Extract data from serverContent
        if (response.contains("serverContent")) {
            nlohmann::json& server_content = response["serverContent"];
            
            // Check modelTurn
            if (server_content.contains("modelTurn")) {
                nlohmann::json& model_turn = server_content["modelTurn"];
                
                // Check parts
                if (model_turn.contains("parts")) {
                    for (nlohmann::json& part : model_turn["parts"]) {
                        // Check inlineData
                        if (part.contains("inlineData")) {
                            nlohmann::json& inline_data = part["inlineData"];
                            
                            // Check if audio data exists
                            if (inline_data.contains("mimeType") && inline_data.contains("data")) {
                                std::string mime_type = inline_data["mimeType"];
                                
                                // If it is audio data
                                if (mime_type.find("audio") != std::string::npos) {
                                    std::string encoded_data = inline_data["data"];
                                    
                                    // Base64 decode
                                    std::vector<uint8_t> decoded = base64_decode(encoded_data);
                                    
                                    // Convert uint8 array to int16 array (Little-endian)
                                    audio_data.clear();
                                    audio_data.reserve(decoded.size() / 2);
                                    
                                    for (size_t i = 0; i + 1 < decoded.size(); i += 2) {
                                        int16_t sample = static_cast<int16_t>(
                                            decoded[i] | (decoded[i + 1] << 8)
                                        );
                                        audio_data.push_back(sample);
                                    }
                                    
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
        
    } catch (const json::exception& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return false;
    }
    
    return false;
}

bool MessageHandler::extract_transcription_from_response(
    const std::string& json_response,
    std::string& transcription) {
    
    try {
        json response = json::parse(json_response);
        
        // Check toolCallTranscription (Transcription of output audio)
        if (response.contains("toolCallTranscription")) {
            nlohmann::json& tool_call_transcription = response["toolCallTranscription"];
            
            if (tool_call_transcription.contains("text")) {
                transcription = tool_call_transcription["text"].get<std::string>();
                return true;
            }
        }
        
        // Extract transcription from serverContent
        if (response.contains("serverContent")) {
            nlohmann::json& server_content = response["serverContent"];
            
            // User input transcription (inputTranscription)
            if (server_content.contains("inputTranscription")) {
                nlohmann::json& input_trans = server_content["inputTranscription"];
                if (input_trans.contains("text")) {
                    transcription = input_trans["text"].get<std::string>();
                    return true;
                }
            }
            
            // AI output transcription (outputTranscription)
            if (server_content.contains("outputTranscription")) {
                nlohmann::json& output_trans = server_content["outputTranscription"];
                if (output_trans.contains("text")) {
                    transcription = output_trans["text"].get<std::string>();
                    return true;
                }
            }
            
            // Check text response in modelTurn
            if (server_content.contains("modelTurn")) {
                nlohmann::json& model_turn = server_content["modelTurn"];
                
                if (model_turn.contains("parts")) {
                    for (nlohmann::json& part : model_turn["parts"]) {
                        // Check for text part
                        if (part.contains("text")) {
                            transcription = part["text"].get<std::string>();
                            return true;
                        }
                    }
                }
            }
        }
        
    } catch (const json::exception& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return false;
    }
    
    return false;
}

bool MessageHandler::is_user_input_transcription(const std::string& json_response) {
    try {
        json response = json::parse(json_response);
        
        // If inputTranscription field exists in serverContent, it is user input transcription
        if (response.contains("serverContent")) {
            nlohmann::json& server_content = response["serverContent"];
            if (server_content.contains("inputTranscription")) {
                return true;
            }
        }
        
    } catch (const json::exception& e) {
        // Ignore JSON parse errors
    }
    
    return false;
}

bool MessageHandler::is_turn_complete(const std::string& json_response) {
    try {
        json response = json::parse(json_response);
        
        if (response.contains("serverContent")) {
            nlohmann::json& server_content = response["serverContent"];
            
            if (server_content.contains("turnComplete")) {
                return server_content["turnComplete"].get<bool>();
            }
        }
        
    } catch (const json::exception& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
    }
    
    return false;
}
