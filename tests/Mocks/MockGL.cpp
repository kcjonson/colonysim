#include "MockGL.h"
#include <random>
#include <algorithm>

// Initialize static members for MockOpenGL
int MockOpenGL::s_drawCallCount = 0;
int MockOpenGL::s_vertexCount = 0;
int MockOpenGL::s_stateChangeCount = 0;
std::unordered_map<std::string, bool> MockOpenGL::s_enabledState;
std::vector<std::pair<std::string, int>> MockOpenGL::s_boundTextures;
int MockOpenGL::s_currentShaderProgram = 0;

// MockOpenGL implementation
void MockOpenGL::resetCounters() {
    s_drawCallCount = 0;
    s_vertexCount = 0;
    s_stateChangeCount = 0;
    s_enabledState.clear();
    s_boundTextures.clear();
    s_currentShaderProgram = 0;
}

void MockOpenGL::drawArrays(int mode, int first, int count) {
    s_drawCallCount++;
    s_vertexCount += count;
}

void MockOpenGL::drawElements(int mode, int count, int type, const void* indices) {
    s_drawCallCount++;
    s_vertexCount += count;
}

// Additional methods implementations
void MockOpenGL::incrementDrawCalls() {
    s_drawCallCount++;
}

void MockOpenGL::addVertices(int count) {
    s_vertexCount += count;
}

void MockOpenGL::updateUniform(const std::string& name) {
    s_stateChangeCount++;
}

void MockOpenGL::enable(int cap) {
    std::string capStr = std::to_string(cap);
    if (!s_enabledState[capStr]) {
        s_enabledState[capStr] = true;
        s_stateChangeCount++;
    }
}

void MockOpenGL::disable(int cap) {
    std::string capStr = std::to_string(cap);
    if (s_enabledState[capStr]) {
        s_enabledState[capStr] = false;
        s_stateChangeCount++;
    }
}

void MockOpenGL::bindTexture(int target, int texture) {
    std::string targetStr = std::to_string(target);
    bool found = false;
    
    for (auto& pair : s_boundTextures) {
        if (pair.first == targetStr) {
            if (pair.second != texture) {
                pair.second = texture;
                s_stateChangeCount++;
            }
            found = true;
            break;
        }
    }
    
    if (!found) {
        s_boundTextures.push_back(std::make_pair(targetStr, texture));
        s_stateChangeCount++;
    }
}

void MockOpenGL::useProgram(int program) {
    if (s_currentShaderProgram != program) {
        s_currentShaderProgram = program;
        s_stateChangeCount++;
    }
}

int MockOpenGL::getDrawCallCount() {
    return s_drawCallCount;
}

int MockOpenGL::getVertexCount() {
    return s_vertexCount;
}

int MockOpenGL::getStateChangeCount() {
    return s_stateChangeCount;
}

// MockCamera implementation
void MockCamera::setPosition(const glm::vec3& pos) {
    m_position = pos;
    m_matricesDirty = true;
}

glm::vec3 MockCamera::getPosition() const {
    return m_position;
}

const glm::mat4& MockCamera::getViewMatrix() {
    if (m_matricesDirty) {
        updateMatrices();
    }
    return m_viewMatrix;
}

const glm::mat4& MockCamera::getProjectionMatrix() {
    if (m_matricesDirty) {
        updateMatrices();
    }
    return m_projectionMatrix;
}

void MockCamera::updateMatrices() {
    // Create a simple view matrix (looking down at the world)
    m_viewMatrix = glm::lookAt(
        m_position,                 // Camera position
        glm::vec3(m_position.x, m_position.y, 0.0f), // Looking at origin
        glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
    );
    
    // Create an orthographic projection matrix
    float aspectRatio = 16.0f / 9.0f; // Assuming 16:9 aspect ratio
    float viewWidth = 1000.0f;
    float viewHeight = viewWidth / aspectRatio;
    
    m_projectionMatrix = glm::ortho(
        -viewWidth / 2.0f, viewWidth / 2.0f,
        -viewHeight / 2.0f, viewHeight / 2.0f,
        0.1f, 1000.0f
    );
    
    m_matricesDirty = false;
}

// TestWorld implementation
TestWorld::TestWorld(int width, int height) : m_width(width), m_height(height) {}

void TestWorld::render() {
    // This would simulate rendering the world
    // For now, we just update the mock counters
    // Assuming on average we'd make one draw call per 100 tiles
    int visibleTiles = getTileCount();
    int drawCalls = std::max(1, visibleTiles / 100);
    
    // Reset counters before our test render
    MockOpenGL::resetCounters();
    
    // Simulate rendering by incrementing counters
    MockOpenGL::useProgram(1); // Set shader program
    MockOpenGL::bindTexture(0, 1); // Bind a texture
    
    for (int i = 0; i < drawCalls; i++) {
        MockOpenGL::drawElements(0, 6 * 100, 0, nullptr); // 6 vertices per quad, 100 quads per batch
    }
}

bool TestWorld::isTileVisible(const glm::vec2& pos, float size, const glm::vec4& bounds) {
    // Check if the tile is within view bounds
    // bounds = (left, top, right, bottom)
    return (pos.x + size >= bounds.x && pos.x - size <= bounds.z &&
            pos.y + size >= bounds.w && pos.y - size <= bounds.y);
}