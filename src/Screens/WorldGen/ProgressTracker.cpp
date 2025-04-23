#include "ProgressTracker.h"
#include <algorithm>

namespace WorldGen {

ProgressTracker::ProgressTracker()
    : m_currentPhaseIndex(-1)
    , m_startTime(std::chrono::steady_clock::now())
{
}

ProgressTracker::~ProgressTracker()
{
}

void ProgressTracker::SetCallback(ProgressCallback callback)
{
    m_callback = callback;
}

void ProgressTracker::AddPhase(const std::string& name, float weight)
{
    m_phases.push_back({name, weight, 0.0f});
}

void ProgressTracker::StartPhase(const std::string& phaseName)
{
    // Find the phase by name
    auto it = std::find_if(m_phases.begin(), m_phases.end(),
        [&phaseName](const PhaseInfo& phase) { return phase.name == phaseName; });
    
    if (it != m_phases.end()) {
        m_currentPhaseIndex = std::distance(m_phases.begin(), it);
        m_phases[m_currentPhaseIndex].progress = 0.0f;
        m_currentMessage = "Starting " + phaseName;
        
        if (m_callback) {
            m_callback(GetOverallProgress(), m_currentMessage);
        }
    }
}

void ProgressTracker::UpdateProgress(float progress, const std::string& message)
{
    if (m_currentPhaseIndex >= 0 && m_currentPhaseIndex < m_phases.size()) {
        m_phases[m_currentPhaseIndex].progress = std::clamp(progress, 0.0f, 1.0f);
        m_currentMessage = message;
        
        if (m_callback) {
            m_callback(GetOverallProgress(), m_currentMessage);
        }
    }
}

void ProgressTracker::CompletePhase()
{
    if (m_currentPhaseIndex >= 0 && m_currentPhaseIndex < m_phases.size()) {
        m_phases[m_currentPhaseIndex].progress = 1.0f;
        m_currentMessage = "Completed " + m_phases[m_currentPhaseIndex].name;
        
        if (m_callback) {
            m_callback(GetOverallProgress(), m_currentMessage);
        }
    }
}

float ProgressTracker::GetOverallProgress() const
{
    if (m_phases.empty()) return 0.0f;
    
    float totalWeight = 0.0f;
    float weightedProgress = 0.0f;
    
    for (const auto& phase : m_phases) {
        totalWeight += phase.weight;
        weightedProgress += phase.weight * phase.progress;
    }
    
    return totalWeight > 0.0f ? weightedProgress / totalWeight : 0.0f;
}

int ProgressTracker::GetEstimatedSecondsRemaining() const
{
    if (m_phases.empty() || m_currentPhaseIndex < 0) return 0;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime).count();
    
    float progress = GetOverallProgress();
    if (progress <= 0.0f) return 0;
    
    float estimatedTotal = elapsed / progress;
    return static_cast<int>(estimatedTotal - elapsed);
}

const std::string& ProgressTracker::GetCurrentPhase() const
{
    if (m_currentPhaseIndex >= 0 && m_currentPhaseIndex < m_phases.size()) {
        return m_phases[m_currentPhaseIndex].name;
    }
    static const std::string empty;
    return empty;
}

const std::string& ProgressTracker::GetCurrentMessage() const
{
    return m_currentMessage;
}

void ProgressTracker::Reset()
{
    m_phases.clear();
    m_currentPhaseIndex = -1;
    m_currentMessage.clear();
    m_startTime = std::chrono::steady_clock::now();
}

} // namespace WorldGen 