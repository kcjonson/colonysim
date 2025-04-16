#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../TestUtils.h"

// Mock OpenGL class for tracking rendering operations during tests
class MockOpenGL {
public:
    // Reset all counters to zero
    static void resetCounters();
    
    // Simulate OpenGL functions
    static void drawArrays(int mode, int first, int count);
    static void drawElements(int mode, int count, int type, const void* indices);
    static void enable(int cap);
    static void disable(int cap);
    static void bindTexture(int target, int texture);
    static void useProgram(int program);
    
    // Additional methods used in tests
    static void incrementDrawCalls();
    static void addVertices(int count);
    static void updateUniform(const std::string& name);
    
    // Get counter values
    static int getDrawCallCount();
    static int getVertexCount();
    static int getStateChangeCount();
    
private:
    static int s_drawCallCount;
    static int s_vertexCount;
    static int s_stateChangeCount;
    static std::unordered_map<std::string, bool> s_enabledState;
    static std::vector<std::pair<std::string, int>> s_boundTextures;
    static int s_currentShaderProgram;
};

// Simple camera mock for testing
class MockCamera {
public:
    MockCamera() : m_position(0.0f, 0.0f, 100.0f), m_matricesDirty(true) {}
    
    void setPosition(const glm::vec3& pos);
    glm::vec3 getPosition() const;
    const glm::mat4& getViewMatrix();
    const glm::mat4& getProjectionMatrix();
    
private:
    void updateMatrices();
    
    glm::vec3 m_position;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    bool m_matricesDirty;
};

// Mock world for testing rendering performance
class TestWorld {
public:
    TestWorld(int width, int height);
    void render();
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getTileCount() const { return m_width * m_height; }
    
    // Helper method to check if a tile is visible
    static bool isTileVisible(const glm::vec2& pos, float size, const glm::vec4& bounds);
    
private:
    int m_width;
    int m_height;
};