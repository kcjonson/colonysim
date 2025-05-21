#include "FontRenderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> // Required for std::max in measureText if you use my previous measureText

FontRenderer::FontRenderer() : VAO(0), VBO(0), library(nullptr), maxGlyphHeightUnscaled(0.0f) {
    // std::cout << "FontRenderer constructor called" << std::endl;
}

FontRenderer::~FontRenderer() {
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
    }
    if (library) {
        FT_Done_FreeType(library);
    }
}

bool FontRenderer::initialize() {
    std::cout << "Initializing FontRenderer..." << std::endl;
    
    // Initialize FreeType
    if (FT_Init_FreeType(&library)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return false;
    }
    std::cout << "FreeType initialized successfully" << std::endl;

    // Load font
    if (!loadFont("fonts/Roboto-Regular.ttf")) {
        std::cerr << "Failed to load font" << std::endl;
        return false;
    }
    std::cout << "Font loaded successfully" << std::endl;

    // Initialize the shader
    if (!shader.loadFromFile("text.vert", "text.frag")) {
        std::cerr << "Failed to load text shaders" << std::endl;
        return false;
    }
    std::cout << "Shaders compiled successfully" << std::endl;

    // Setup buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "FontRenderer initialization complete" << std::endl;
    return true;
}

bool FontRenderer::loadFont(const std::string& fontPath) {
    std::cout << "Loading font from: " << fontPath << std::endl;
    
    if (FT_New_Face(library, fontPath.c_str(), 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 16); // Set base font size to 16px
    std::cout << "Font face loaded successfully" << std::endl;

    // Store the ascender for the base font size.
    // The 'scale' parameter in renderText will be applied to this.
    // face->size->metrics.ascender is in 26.6 fixed point format.
    this->scaledAscender = (float)(face->size->metrics.ascender >> 6);
    // Store the maximum glyph line height (ascender-descender) for the base font size.
    // face->size->metrics.height is in 26.6 fixed point format.
    this->maxGlyphHeightUnscaled = (float)(face->size->metrics.height >> 6);

    // Load first 128 characters of ASCII set
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph " << (int)c << std::endl;
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        characters[c] = character;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face); // After this, 'face' member is no longer valid for metrics.

    std::cout << "Loaded " << characters.size() << " characters" << std::endl;
    return true;
}

void FontRenderer::renderText(const std::string& text, const glm::vec2& position, float scale, const glm::vec3& color) {
    // Enable blending for text transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    shader.use(); // Ensure shader is active
    glUniform3f(glGetUniformLocation(shader.getProgram(), "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Use the stored scaledAscender and apply the current rendering scale.
    float ascender_at_current_scale = this->scaledAscender * scale;
    glm::vec2 pen_position = position;
    // position.y is the top-left y. Baseline is top-left y + ascender.
    pen_position.y += ascender_at_current_scale; 

    // Iterate through each character in the text
    std::string::const_iterator iter;
    for (iter = text.begin(); iter != text.end(); ++iter) {
        char current_char = *iter;
        
        auto char_it = characters.find(current_char);
        const Character* ch_to_render_ptr;

        if (char_it != characters.end()) {
            ch_to_render_ptr = &char_it->second;
        } else {
            // Fallback to '?' if the character is not found
            auto fallback_it = characters.find('?');
            if (fallback_it != characters.end()) {
                ch_to_render_ptr = &fallback_it->second;
                 // Advance pen for the original missing char using '?' advance, then continue to next char in text
                if (characters.count(current_char) == 0) { // Only advance if truly missing, not if '?' is the char
                    pen_position.x += (fallback_it->second.advance >> 6) * scale;
                }
            }
            // If '?' is also not found, or if we don't want to render '?' for missing chars, just advance and skip.
            // For now, if '?' is not found, we just skip rendering this char.
            // A more robust fallback might use a default advance.
            if (!ch_to_render_ptr && fallback_it == characters.end()){
                 // Advance by a small amount or average char width if char and '?' are missing
                 // For simplicity, just skip if '?' is also missing.
                continue;
            }
             if (!ch_to_render_ptr) continue; // Skip if no valid char or fallback
        }
        const Character& ch = *ch_to_render_ptr;

        // Calculate position of character quad
        float xpos = pen_position.x + ch.bearing.x * scale;
        float ypos = pen_position.y - ch.bearing.y * scale; // ypos is the top of the glyph bitmap

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos,       0.0f, 0.0f },            
            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 1.0f },

            { xpos,     ypos,       0.0f, 0.0f },
            { xpos + w, ypos + h,   1.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 0.0f }   
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        pen_position.x += (ch.advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}

glm::vec2 FontRenderer::measureText(const std::string& text, float scale) const {
    if (text.empty()) {
        return glm::vec2(0.0f, 0.0f);
    }

    float total_width = 0.0f;
    float max_glyph_top_from_baseline_unscaled = 0.0f;
    float min_glyph_bottom_from_baseline_unscaled = 0.0f;
    bool first_char = true;

    for (char c_char : text) {
        auto it = characters.find(c_char);
        const Character* pCh;

        if (it != characters.end()) {
            pCh = &it->second;
        } else {
            auto it_q = characters.find('?');
            if (it_q != characters.end()) {
                pCh = &it_q->second;
            } else {
                continue; 
            }
        }
        const Character& ch = *pCh;
        
        float glyph_top_unscaled = (float)ch.bearing.y;
        float glyph_bottom_unscaled = (float)ch.bearing.y - (float)ch.size.y;

        if (first_char) {
            max_glyph_top_from_baseline_unscaled = glyph_top_unscaled;
            min_glyph_bottom_from_baseline_unscaled = glyph_bottom_unscaled;
            first_char = false;
        } else {
            max_glyph_top_from_baseline_unscaled = std::max(max_glyph_top_from_baseline_unscaled, glyph_top_unscaled);
            min_glyph_bottom_from_baseline_unscaled = std::min(min_glyph_bottom_from_baseline_unscaled, glyph_bottom_unscaled);
        }

        // For total width, sum advances. The last character's actual extent might be different.
        // This simple sum of advances is common.
        total_width += (ch.advance >> 6);
    }
    
    // If the last character has a visual extent beyond its advance point,
    // the width calculation might need adjustment. For now, sum of advances.
    // A more precise width might be:
    // total_width_precise = 0;
    // if (!text.empty()) {
    //    for (size_t i = 0; i < text.length() -1; ++i) total_width_precise += (characters.at(text[i]).advance >> 6);
    //    const Character& last_ch = characters.at(text.back()); // Assuming text not empty and char exists
    //    total_width_precise += std::max((float)(last_ch.advance >> 6), (float)last_ch.bearing.x + last_ch.size.x);
    // }
    // total_width = total_width_precise;


    float scaled_total_width = total_width * scale;
    float actual_height_scaled = 0.0f;
    if (!first_char) { // if at least one character was processed
        actual_height_scaled = (max_glyph_top_from_baseline_unscaled - min_glyph_bottom_from_baseline_unscaled) * scale;
    }
    actual_height_scaled = std::max(0.0f, actual_height_scaled);

    return glm::vec2(scaled_total_width, actual_height_scaled);
}

// Get the maximum height of any glyph (ascender + |descender|) at the specified scale
float FontRenderer::getMaxGlyphHeight(float scale) const {
    // maxGlyphHeightUnscaled holds the line height (distance between baselines) in unscaled pixels
    return maxGlyphHeightUnscaled * scale;
}