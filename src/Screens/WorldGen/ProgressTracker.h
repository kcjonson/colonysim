#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>

namespace WorldGen {

/**
 * @brief Tracks progress of world generation phases
 * 
 * This class manages the progress tracking for the world generation process,
 * providing real-time feedback on the current phase and overall progress.
 */
class ProgressTracker {
public:
    using ProgressCallback = std::function<void(float progress, const std::string& message)>;

    struct PhaseInfo {
        std::string name;
        float weight;
        float progress;
    };

    ProgressTracker();
    ~ProgressTracker();

    // Configuration
    void SetCallback(ProgressCallback callback);
    void AddPhase(const std::string& name, float weight);

    // Progress tracking
    void StartPhase(const std::string& phaseName);
    void UpdateProgress(float progress, const std::string& message = "");
    void CompletePhase();

    // Status information
    float GetOverallProgress() const;
    int GetEstimatedSecondsRemaining() const;
    const std::string& GetCurrentPhase() const;
    const std::string& GetCurrentMessage() const;

    // Reset
    void Reset();

private:
    std::vector<PhaseInfo> phases;
    int currentPhaseIndex;
    ProgressCallback callback;
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    std::string currentMessage;
};

} // namespace WorldGen 