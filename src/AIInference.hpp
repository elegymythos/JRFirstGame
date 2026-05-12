#pragma once

#include "AIController.hpp"
#include <onnxruntime_cxx_api.h>
#include <string>
#include <memory>
#include <array>

class AIInference {
public:
    AIInference();
    ~AIInference() = default;

    bool loadModel(const std::string& modelPath);
    bool isLoaded() const { return loaded_; }

    int predict(const AIGameState& state);

    int getLastAction() const { return lastAction_; }
    const std::array<float, AIObservation::NUM_ACTIONS>& getLastLogits() const { return lastLogits_; }

private:
    bool loaded_ = false;
    int lastAction_ = 0;
    std::array<float, AIObservation::NUM_ACTIONS> lastLogits_{};

    Ort::Env env_;
    std::unique_ptr<Ort::Session> session_;
    Ort::SessionOptions sessionOpts_;

    std::vector<const char*> inputNames_;
    std::vector<const char*> outputNames_;
    std::vector<std::string> inputNameStrings_;
    std::vector<std::string> outputNameStrings_;
};
