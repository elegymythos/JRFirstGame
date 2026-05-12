#include "AIInference.hpp"
#include <iostream>
#include <algorithm>
#include <numeric>

AIInference::AIInference() : env_(ORT_LOGGING_LEVEL_WARNING, "JRFirstGame-AI") {
    sessionOpts_.SetIntraOpNumThreads(1);
    sessionOpts_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
}

bool AIInference::loadModel(const std::string& modelPath) {
    try {
        #ifdef _WIN32
        std::wstring wpath(modelPath.begin(), modelPath.end());
        session_ = std::make_unique<Ort::Session>(env_, wpath.c_str(), sessionOpts_);
        #else
        session_ = std::make_unique<Ort::Session>(env_, modelPath.c_str(), sessionOpts_);
        #endif

        Ort::AllocatorWithDefaultOptions alloc;

        size_t numInputs = session_->GetInputCount();
        inputNameStrings_.clear();
        inputNames_.clear();
        for (size_t i = 0; i < numInputs; ++i) {
            auto name = session_->GetInputNameAllocated(i, alloc);
            inputNameStrings_.push_back(name.get());
        }
        for (const auto& s : inputNameStrings_) {
            inputNames_.push_back(s.c_str());
        }

        size_t numOutputs = session_->GetOutputCount();
        outputNameStrings_.clear();
        outputNames_.clear();
        for (size_t i = 0; i < numOutputs; ++i) {
            auto name = session_->GetOutputNameAllocated(i, alloc);
            outputNameStrings_.push_back(name.get());
        }
        for (const auto& s : outputNameStrings_) {
            outputNames_.push_back(s.c_str());
        }

        loaded_ = true;
        std::cout << "[AI] ONNX model loaded: " << modelPath << std::endl;
        return true;
    } catch (const Ort::Exception& e) {
        std::cerr << "[AI] ONNX load error: " << e.what() << std::endl;
        return false;
    }
}

int AIInference::predict(const AIGameState& state) {
    if (!loaded_) return 0;

    auto obs = AIObservation::build(state);

    try {
        std::array<int64_t, 2> inputShape = {1, AIObservation::OBS_DIM};
        Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memInfo, obs.data(), AIObservation::OBS_DIM,
            inputShape.data(), inputShape.size()
        );

        auto outputTensors = session_->Run(
            Ort::RunOptions{nullptr},
            inputNames_.data(), &inputTensor, 1,
            outputNames_.data(), outputNames_.size()
        );

        float* logitsData = outputTensors[0].GetTensorMutableData<float>();
        size_t logitsSize = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();

        int bestAction = 0;
        float bestLogit = logitsData[0];
        for (size_t i = 0; i < logitsSize && i < lastLogits_.size(); ++i) {
            lastLogits_[i] = logitsData[i];
            if (logitsData[i] > bestLogit) {
                bestLogit = logitsData[i];
                bestAction = static_cast<int>(i);
            }
        }

        lastAction_ = bestAction;
        return bestAction;
    } catch (const Ort::Exception& e) {
        std::cerr << "[AI] Inference error: " << e.what() << std::endl;
        return 0;
    }
}
