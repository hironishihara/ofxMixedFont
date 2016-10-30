#include "ofxMixedFont.hpp"

#include "ofTexture.h"
#include "ofPath.h"

typedef struct {
    std::weak_ptr<ofxMixedFontUtil::ofxBaseFont> font;
    std::vector<bool> assigned_char_flags;
} charAssignation;

bool ofxMixedFont::add(const ofxBaseFontPtr &font)
{
    bool is_successful = false;
    
    if (font->isReady()) {
        font_list.push_back(font);
        if (font_list.size() == 1) {
            is_ready_ = true;
            texture_is_enabled_ = font->textureIsEnabled();
            path_is_enabled_ = font->pathIsEnabled();
            font_props_ = font->getFontProps();
         }
        else {
            texture_is_enabled_ &= font->textureIsEnabled();
            path_is_enabled_ &= font->pathIsEnabled();
        }
        is_successful = true;
    }
    else {
        
    }
    
    return is_successful;
}

ofxMixedFontUtil::ofxGlyphData ofxMixedFont::makeGlyphData(const std::u32string &utf32_character, const int &index, int &length)
{
    length = -1; // Note: This font is not ready.
    if (!isReady()) return ofxMixedFontUtil::ofxGlyphData();
    
    length = 0;
    ofxMixedFontUtil::ofxGlyphData glyph;
    bool glyph_is_found = false;
    for (auto &font : font_list) {
        int tmp_length = 0;
        ofxMixedFontUtil::ofxGlyphData tmp_glyph = font->makeGlyphData(utf32_character, index, tmp_length);
        if (tmp_length > 0) {
            glyph = tmp_glyph;
            length = tmp_length;
            glyph_is_found = true;
            break;
        }
    }
    if (!glyph_is_found && font_list.size() > 0) {
        glyph = font_list[0]->makeGlyphData(utf32_character, index, length);
    }
    
    return glyph;
}

void ofxMixedFont::drawGlyphs(const std::vector<ofxMixedFontUtil::ofxGlyphData> &glyph_list)
{
    if (!isReady()) return;
    
    for (auto &font : font_list) {
        font->drawGlyphs(glyph_list);
    }
}

void ofxMixedFont::drawGlyphsWithTexture(const std::vector<ofxMixedFontUtil::ofxGlyphData> &glyph_list)
{
    if (!isReady()) return;
    // if (!textureIsEnabled()) return;
    
    for (auto &font : font_list) {
        font->drawGlyphsWithTexture(glyph_list);
    }
}

void ofxMixedFont::drawGlyphsWithPath(const std::vector<ofxMixedFontUtil::ofxGlyphData> &glyph_list)
{
    if (!isReady()) return;
    // if (!pathIsEnabled()) return;
    
    for (auto &font : font_list) {
        font->drawGlyphsWithPath(glyph_list);
    }
}

void ofxMixedFont::drawString(const std::u32string &utf32_string, const ofPoint &coord, const ofxMixedFontUtil::ofxCompFunc &func)
{
    if (!isReady()) return;
    
    std::vector<ofxMixedFontUtil::ofxGlyphData> glyph_list = typesetString(utf32_string, coord, func);
    drawGlyphs(glyph_list);
}

ofTexture ofxMixedFont::getStringAsTexture(const std::u32string &utf32_string, const ofxMixedFontUtil::ofxCompFunc &func)
{
    // if (!isReady()) return ofTexture();
    return ofTexture();
}

std::vector<ofPath> ofxMixedFont::getStringAsPath(const std::u32string &utf32_string, const ofxMixedFontUtil::ofxCompFunc &func)
{
    // if (!isReady()) return std::vector<ofPath>();
    return std::vector<ofPath>();
}




