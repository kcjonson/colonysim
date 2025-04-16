# Layer Performance Testing with the Observer Pattern

## Overview

This document describes how we use the Observer pattern to test the performance of the rendering system, particularly the `Layer` class's render method. This approach allows us to test the real implementation rather than mock versions, providing accurate performance metrics while avoiding modifications to the core rendering logic.

## The Observer Pattern Approach

1. **LayerObserver Interface**: We've added a `LayerObserver` interface that receives notifications at key points in the rendering process:
   - When rendering starts
   - After sorting children
   - Before rendering children
   - For each child rendered
   - After all children are rendered
   - When rendering completes

2. **Non-invasive Implementation**: The actual Layer render method now contains timing measurements and notifications, but its core functionality remains intact. The original logic flow is preserved.

3. **Layer Identification**: Each layer gets an auto-generated unique ID to help identify it in the performance data.

## How to Use the Observer for Performance Testing

Here's an example of how to create a performance observer for testing:

```cpp
class PerformanceObserver : public Rendering::LayerObserver {
public:
    struct PhaseMetrics {
        std::vector<int64_t> durations;
        int64_t totalDuration = 0;
        int64_t count = 0;
    };

    void onSortingComplete(const std::string& layerId, size_t childCount, int64_t durationNs) override {
        metrics["sorting"].durations.push_back(durationNs);
        metrics["sorting"].totalDuration += durationNs;
        metrics["sorting"].count++;
    }

    void onChildrenRenderComplete(const std::string& layerId, int64_t durationNs) override {
        metrics["children_render"].durations.push_back(durationNs);
        metrics["children_render"].totalDuration += durationNs;
        metrics["children_render"].count++;
    }

    void onRenderComplete(const std::string& layerId, int64_t totalDurationNs) override {
        metrics["total"].durations.push_back(totalDurationNs);
        metrics["total"].totalDuration += totalDurationNs;
        metrics["total"].count++;
    }

    void printReport() {
        std::cout << "==== Performance Report ====" << std::endl;
        for (const auto& [phase, data] : metrics) {
            double avgMs = (data.totalDuration / 1000000.0) / std::max(1, (int)data.count);
            std::cout << phase << ": avg " << avgMs << "ms over " 
                      << data.count << " calls" << std::endl;
        }
    }

private:
    std::map<std::string, PhaseMetrics> metrics;
};
```

## Usage in Tests

In your tests, you can use the observer like this:

```cpp
TEST_CASE("Layer rendering performance", "[rendering][layer][performance]") {
    auto layer = std::make_shared<Rendering::Layer>();
    auto observer = std::make_shared<PerformanceObserver>();
    
    // Attach the observer
    layer->addObserver(observer.get());
    
    // Add child layers for testing
    for (int i = 0; i < 100; i++) {
        layer->addItem(std::make_shared<Rendering::Layer>(float(i)));
    }
    
    // Run multiple render passes for averaging
    for (int i = 0; i < 100; i++) {
        layer->render();
    }
    
    // Print the performance report
    observer->printReport();
    
    // Remove observer when done
    layer->removeObserver(observer.get());
}
```

## Key Benefits of This Approach

1. **Non-intrusive**: The actual render method is still performing the real work - we're just observing it.
2. **Detailed Metrics**: You get fine-grained timing for different rendering phases.
3. **No Mocking Required**: You're testing the actual Layer class and its real render method.
4. **Multiple Observers**: You can attach multiple observers for different testing purposes (performance, debugging, etc.)
5. **Real-world Usage**: This pattern can also be used in production for performance monitoring.

## Implementing a Full-Featured Performance Observer

For more advanced performance testing, you can implement a richer observer that collects statistical data:

```cpp
class AdvancedPerformanceObserver : public Rendering::LayerObserver {
public:
    // Store performance metrics for each phase
    struct PerformanceMetrics {
        std::vector<int64_t> samples;
        int64_t min = 0;
        int64_t max = 0;
        double average = 0;
        double median = 0;
        
        void update() {
            if (samples.empty()) return;
            
            min = *std::min_element(samples.begin(), samples.end());
            max = *std::max_element(samples.begin(), samples.end());
            average = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
            
            // Calculate median
            std::vector<int64_t> sorted = samples;
            std::sort(sorted.begin(), sorted.end());
            if (sorted.size() % 2 == 0) {
                median = (sorted[sorted.size() / 2 - 1] + sorted[sorted.size() / 2]) / 2.0;
            } else {
                median = sorted[sorted.size() / 2];
            }
        }
    };

    // Observer methods implementation
    void onRenderStart(const std::string& layerId) override {
        layerData[layerId].renderStartTime = std::chrono::high_resolution_clock::now();
    }
    
    void onSortingComplete(const std::string& layerId, size_t childCount, int64_t durationNs) override {
        metrics[layerId]["sorting"].samples.push_back(durationNs);
        layerData[layerId].childCount = childCount;
    }
    
    void onChildrenRenderStart(const std::string& layerId, size_t childCount) override {
        layerData[layerId].childrenRenderStartTime = std::chrono::high_resolution_clock::now();
    }
    
    void onChildRender(const std::string& layerId, int childIndex) override {
        // Could track per-child rendering times if needed
    }
    
    void onChildrenRenderComplete(const std::string& layerId, int64_t durationNs) override {
        metrics[layerId]["children_render"].samples.push_back(durationNs);
    }
    
    void onRenderComplete(const std::string& layerId, int64_t totalDurationNs) override {
        metrics[layerId]["total"].samples.push_back(totalDurationNs);
    }
    
    // Print a performance report for all layers or a specific layer
    void printPerformanceReport(const std::string& layerId = "") {
        if (!layerId.empty()) {
            printLayerReport(layerId);
            return;
        }
        
        for (auto& entry : metrics) {
            printLayerReport(entry.first);
        }
    }

private:
    struct LayerData {
        std::chrono::high_resolution_clock::time_point renderStartTime;
        std::chrono::high_resolution_clock::time_point childrenRenderStartTime;
        size_t childCount = 0;
    };

    std::map<std::string, std::map<std::string, PerformanceMetrics>> metrics;
    std::map<std::string, LayerData> layerData;
    
    void printLayerReport(const std::string& layerId) {
        std::cout << "======================================\n";
        std::cout << "Performance Report for " << layerId << "\n";
        std::cout << "======================================\n";
        
        for (auto& entry : metrics[layerId]) {
            entry.second.update();
            std::cout << std::left << std::setw(20) << entry.first << ": ";
            std::cout << "avg " << std::fixed << std::setprecision(2) << (entry.second.average / 1000.0) << " μs, ";
            std::cout << "min " << (entry.second.min / 1000.0) << " μs, ";
            std::cout << "max " << (entry.second.max / 1000.0) << " μs, ";
            std::cout << "med " << (entry.second.median / 1000.0) << " μs";
            std::cout << " (" << entry.second.samples.size() << " samples)\n";
        }
        
        if (layerData.count(layerId) > 0) {
            std::cout << "Children count: " << layerData[layerId].childCount << "\n";
        }
        
        std::cout << "======================================\n";
    }
};
```

## Tips for Performance Testing

1. **Run multiple iterations**: To get stable metrics, run the render method multiple times (at least 100) and gather statistics.
2. **Test different structures**: Test layers with different child counts, depths, and structures to understand performance characteristics.
3. **Compare against baselines**: Keep track of performance metrics over time to detect regressions.
4. **Isolate components**: Test individual rendering phases separately when possible.
5. **Consider real-world scenarios**: Test with realistic layer hierarchies that mimic your actual application.

## When to Use This Approach

This observer-based performance testing approach is particularly valuable when:

- You need to test the actual implementation, not a mock
- You want to avoid modifying core rendering code
- You need detailed metrics about different rendering phases
- You want to run tests both in isolation and in a production-like environment

By leveraging the observer pattern, we get the best of both worlds: we can test the actual Layer render method's performance while still having full visibility into its internal processing stages.