#include "ProgressTracker.h"
#include <algorithm>
#include <iostream>

namespace WorldGen {

ProgressTracker::ProgressTracker()
    : currentPhaseIndex(-1)
    , startTime(std::chrono::steady_clock::now())
{
}

ProgressTracker::~ProgressTracker()
{
}

void ProgressTracker::SetCallback(ProgressCallback callback)
{
    this->callback = callback;
}

void ProgressTracker::AddPhase(const std::string& name, float weight)
{
    phases.push_back({name, weight, 0.0f});
}

void ProgressTracker::StartPhase(const std::string& phaseName)
{
    // Find the phase by name
    auto it = std::find_if(phases.begin(), phases.end(),
        [&phaseName](const PhaseInfo& phase) { return phase.name == phaseName; });
    
    if (it != phases.end()) {
        currentPhaseIndex = std::distance(phases.begin(), it);
        phases[currentPhaseIndex].progress = 0.0f;
        currentMessage = "Starting " + phaseName;
        
        if (callback) {
            callback(GetOverallProgress(), currentMessage);
        }
    }
}

void ProgressTracker::UpdateProgress(float progress, const std::string& message)
{
    if (currentPhaseIndex >= 0 && currentPhaseIndex < phases.size()) {
        phases[currentPhaseIndex].progress = std::clamp(progress, 0.0f, 1.0f);
        currentMessage = message;

        // std::cout << message << std::endl; // Debug output
        
        if (callback) {
            callback(GetOverallProgress(), currentMessage);
        }
    }
}

void ProgressTracker::CompletePhase()
{
    if (currentPhaseIndex >= 0 && currentPhaseIndex < phases.size()) {
        phases[currentPhaseIndex].progress = 1.0f;
        currentMessage = "Completed " + phases[currentPhaseIndex].name;
        
        if (callback) {
            callback(GetOverallProgress(), currentMessage);
        }
    }
}

float ProgressTracker::GetOverallProgress() const
{
    if (phases.empty()) return 0.0f;
    
    float totalWeight = 0.0f;
    float weightedProgress = 0.0f;
    
    for (const auto& phase : phases) {
        totalWeight += phase.weight;
        weightedProgress += phase.weight * phase.progress;
    }
    
    return totalWeight > 0.0f ? weightedProgress / totalWeight : 0.0f;
}

int ProgressTracker::GetEstimatedSecondsRemaining() const
{
    if (phases.empty() || currentPhaseIndex < 0) return 0;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    
    float progress = GetOverallProgress();
    if (progress <= 0.0f) return 0;
    
    float estimatedTotal = elapsed / progress;
    return static_cast<int>(estimatedTotal - elapsed);
}

const std::string& ProgressTracker::GetCurrentPhase() const
{
    if (currentPhaseIndex >= 0 && currentPhaseIndex < phases.size()) {
        return phases[currentPhaseIndex].name;
    }
    static const std::string empty;
    return empty;
}

const std::string& ProgressTracker::GetCurrentMessage() const
{
    return currentMessage;
}

void ProgressTracker::Reset()
{
    phases.clear();
    currentPhaseIndex = -1;
    currentMessage.clear();
    startTime = std::chrono::steady_clock::now();
}

} // namespace WorldGen 