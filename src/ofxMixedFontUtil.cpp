#include "ofxMixedFontUtil.hpp"

#include "ofTexture.h"
#include "ofPath.h"

#include <locale>
#include <codecvt>

namespace ofxMixedFontUtil {

const float convertPtToPx(const float &pt)
{
    return pt * DPI / PT_PER_INCH;
}

const std::u32string convertStringToU32string(const std::string &src_string)
{
#ifdef TARGET_WIN32
    if (src_string.empty()) {
        return std::u32string();
    }
    
    // multibyte -> utf16
    const int utf16_length = ::MultiByteToWideChar(CP_ACP, 0, src_string.c_str(), -1, NULL, 0);
    vector<wchar_t> utf16_buffer(utf16_length);
    ::MultiByteToWideChar(CP_ACP, 0, src_string.c_str(), -1, &utf16_buffer[0], utf16_length);
    
    // utf16 -> utf8
    const int utf8_length = ::WideCharToMultiByte(CP_UTF8, 0, &utf16_buffer[0], -1, NULL, 0, NULL, 0);
    vector<char> utf8_buffer(utf8_length);
    ::WideCharToMultiByte(CP_UTF8, 0, &utf16_buffer[0], -1, &utf8_buffer[0], utf8_length, NULL, 0);
    
    // utf8 -> utf32
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    return converter.from_bytes(&utf8_buffer[0]);
#else
    // utf8 -> utf32
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    return converter.from_bytes(src_string.c_str());
#endif
}

void defaultCompFunc(const std::shared_ptr<ofxBaseFont> &font, const ofPoint &coord, std::vector<ofxGlyphData> &glyph_list)
{
    ofPoint pos = coord;
    for (auto &glyph : glyph_list) {
        glyph.coord = pos;
        
        if (glyph.props.code_point == U"\n") {
            pos.x = coord.x;
            pos.y += font->getFontProps().line_height;
        }
        else if (glyph.props.code_point == U" ") {
            pos.x += font->getFontProps().x_ppem / 2.f;
        }
        else if (glyph.props.code_point == U"　") {
            pos.x += font->getFontProps().x_ppem;
        }
        else {
            pos.x += glyph.props.advance;
        }
    }
}

ofxBaseFont::ofxBaseFont()
: font_props_(), is_ready_(false), texture_is_enabled_(false), path_is_enabled_(false)
{
    
}

bool ofxBaseFont::isReady() const
{
    return is_ready_;
}

bool ofxBaseFont::textureIsEnabled() const
{
    return texture_is_enabled_;
}

bool ofxBaseFont::pathIsEnabled() const
{
    return path_is_enabled_;
}

ofxFontProps ofxBaseFont::getFontProps() const
{
    return font_props_;
}

float ofxBaseFont::getDPI() const
{
    return ofxMixedFontUtil::DPI;
}

// utf32
void ofxBaseFont::drawStringWithTexture(const std::u32string &utf32_string, const ofPoint &coord, const ofxCompFunc &func)
{
    if (!isReady()) return;
    
    std::vector<ofxGlyphData> glyph_list = typesetString(utf32_string, coord, func);
    drawGlyphsWithTexture(glyph_list);
}

void ofxBaseFont::drawStringWithPath(const std::u32string &utf32_string, const ofPoint &coord, const ofxCompFunc &func)
{
    if (!isReady()) return;
    
    std::vector<ofxGlyphData> glyph_list = typesetString(utf32_string, coord, func);
    drawGlyphsWithPath(glyph_list);
}

ofRectangle ofxBaseFont::getStringBoundingBox(const std::u32string &utf32_string, const ofPoint &coord, const ofxCompFunc &func)
{
    if (!isReady()) return ofRectangle();
    
    std::vector<ofxGlyphData> glyph_list = typesetString(utf32_string, coord, func);
    ofPoint min(coord.x, coord.y);
    ofPoint max(coord.x, coord.y);
    
    for (auto &glyph : glyph_list) {
        if (glyph.coord.x + glyph.props.bearing_x < min.x) { min.x = glyph.coord.x + glyph.props.bearing_x; }
        if (glyph.coord.y - glyph.props.bearing_y < min.y) { min.y = glyph.coord.y - glyph.props.bearing_y; }
        if (glyph.coord.x + glyph.props.bearing_x + glyph.props.width > max.x) { max.x = glyph.coord.x + glyph.props.bearing_x + glyph.props.width; }
        if (glyph.coord.y - glyph.props.bearing_y + glyph.props.height > max.y) { max.y = glyph.coord.y - glyph.props.bearing_y + glyph.props.height; }
    }
    
    return ofRectangle(min.x, min.y, max.x - min.x, max.y - min.y);
}

std::vector<ofRectangle> ofxBaseFont::getGlyphBoundingBoxes(const std::u32string &utf32_string, const ofPoint &coord, const ofxCompFunc &func)
{
    std::vector<ofRectangle> bboxes;
    if (!isReady()) return bboxes;
    
    std::vector<ofxGlyphData> glyph_list = typesetString(utf32_string, coord, func);
    
    for (auto &glyph : glyph_list) {
        bboxes.push_back(ofRectangle(glyph.coord.x + glyph.props.bearing_x, glyph.coord.y - glyph.props.bearing_y, glyph.props.width, glyph.props.height));
    }
    
    return bboxes;
}

void ofxBaseFont::drawString(const std::u32string &utf32_character, const float &x, const float &y, const float &z, const ofxCompFunc &func)
{
    return drawString(utf32_character, ofPoint(x, y, z), func);
}

void ofxBaseFont::drawStringWithTexture(const std::u32string &utf32_character, const float &x, const float &y, const float &z, const ofxCompFunc &func)
{
    return drawStringWithTexture(utf32_character, ofPoint(x, y, z), func);
}

void ofxBaseFont::drawStringWithPath(const std::u32string &utf32_character, const float &x, const float &y, const float &z, const ofxCompFunc &func)
{
    return drawStringWithPath(utf32_character, ofPoint(x, y, z), func);
}

ofRectangle ofxBaseFont::getStringBoundingBox(const std::u32string &utf32_string, const float &x, const float &y, const float &z, const ofxCompFunc &func)
{
    return getStringBoundingBox(utf32_string, ofPoint(x, y, z), func);
}

std::vector<ofRectangle> ofxBaseFont::getGlyphBoundingBoxes(const std::u32string &utf32_string, const float &x, const float &y, const float &z, const ofxCompFunc &func)
{
    return getGlyphBoundingBoxes(utf32_string, ofPoint(x, y, z), func);
}

void ofxBaseFont::drawString(const std::u32string &utf32_character, const float &x, const float &y, const ofxCompFunc &func)
{
    return drawString(utf32_character, ofPoint(x, y, 0), func);
}

void ofxBaseFont::drawStringWithTexture(const std::u32string &utf32_character, const float &x, const float &y, const ofxCompFunc &func)
{
    return drawStringWithTexture(utf32_character, ofPoint(x, y, 0), func);
}

void ofxBaseFont::drawStringWithPath(const std::u32string &utf32_character, const float &x, const float &y, const ofxCompFunc &func)
{
    return drawStringWithPath(utf32_character, ofPoint(x, y, 0), func);
}

ofRectangle ofxBaseFont::getStringBoundingBox(const std::u32string &utf32_string, const float &x, const float &y, const ofxCompFunc &func)
{
    return getStringBoundingBox(utf32_string, ofPoint(x, y, 0), func);
}

std::vector<ofRectangle> ofxBaseFont::getGlyphBoundingBoxes(const std::u32string &utf32_string, const float &x, const float &y, const ofxCompFunc &func)
{
    return getGlyphBoundingBoxes(utf32_string, ofPoint(x, y, 0), func);
}

// utf8 (or narrow multibyte) 
void ofxBaseFont::drawString(const std::string &src_string, const ofPoint &coord, const ofxCompFunc &func)
{
    return drawString(utf_converter.from_bytes(src_string), coord, func);
}

void ofxBaseFont::drawStringWithTexture(const std::string &src_string, const ofPoint &coord, const ofxCompFunc &func)
{
    return drawStringWithTexture(utf_converter.from_bytes(src_string), coord, func);
}

void ofxBaseFont::drawStringWithPath(const std::string &src_string, const ofPoint &coord, const ofxCompFunc &func)
{
    return drawStringWithPath(utf_converter.from_bytes(src_string), coord, func);
}

ofRectangle ofxBaseFont::getStringBoundingBox(const std::string &src_string, const ofPoint &coord, const ofxCompFunc &func)
{
    return getStringBoundingBox(utf_converter.from_bytes(src_string), coord, func);
}

std::vector<ofRectangle> ofxBaseFont::getGlyphBoundingBoxes(const std::string &src_string, const ofPoint &coord, const ofxCompFunc &func)
{
    return getGlyphBoundingBoxes(utf_converter.from_bytes(src_string), coord, func);
}

ofTexture ofxBaseFont::getStringAsTexture(const std::string &src_string, const ofxCompFunc &func)
{
    return getStringAsTexture(utf_converter.from_bytes(src_string), func);
}

std::vector<ofPath> ofxBaseFont::getStringAsPath(const std::string &src_string, const ofxCompFunc &func)
{
    return getStringAsPath(utf_converter.from_bytes(src_string), func);
}

void ofxBaseFont::drawString(const std::string &src_string, const float &x, const float &y, const float &z, const ofxCompFunc &func)
{
    return drawString(src_string, ofPoint(x, y, z), func);
}

void ofxBaseFont::drawStringWithTexture(const std::string &src_string, const float &x, const float &y, const float &z, const ofxCompFunc &func)
{
    return drawStringWithTexture(src_string, ofPoint(x, y, z), func);
}

void ofxBaseFont::drawStringWithPath(const std::string &src_string, const float &x, const float &y, const float &z, const ofxCompFunc &func)
{
    return drawStringWithPath(src_string, ofPoint(x, y, z), func);
}

ofRectangle ofxBaseFont::getStringBoundingBox(const std::string &src_string, const float &x, const float &y, const float &z, const ofxCompFunc &func)
{
    return getStringBoundingBox(utf_converter.from_bytes(src_string), ofPoint(x, y, z), func);
}

std::vector<ofRectangle> ofxBaseFont::getGlyphBoundingBoxes(const std::string &src_string, const float &x, const float &y, const float &z, const ofxCompFunc &func)
{
    return getGlyphBoundingBoxes(utf_converter.from_bytes(src_string), ofPoint(x, y, z), func);
}

void ofxBaseFont::drawString(const std::string &src_string, const float &x, const float &y, const ofxCompFunc &func)
{
    return drawString(src_string, ofPoint(x, y, 0), func);
}

void ofxBaseFont::drawStringWithTexture(const std::string &src_string, const float &x, const float &y, const ofxCompFunc &func)
{
    return drawStringWithTexture(src_string, ofPoint(x, y, 0), func);
}

void ofxBaseFont::drawStringWithPath(const std::string &src_string, const float &x, const float &y, const ofxCompFunc &func)
{
    return drawStringWithPath(src_string, ofPoint(x, y, 0), func);
}

ofRectangle ofxBaseFont::getStringBoundingBox(const std::string &src_string, const float &x, const float &y, const ofxCompFunc &func)
{
    return getStringBoundingBox(utf_converter.from_bytes(src_string), ofPoint(x, y, 0), func);
}

std::vector<ofRectangle> ofxBaseFont::getGlyphBoundingBoxes(const std::string &src_string, const float &x, const float &y, const ofxCompFunc &func)
{
    return getGlyphBoundingBoxes(utf_converter.from_bytes(src_string), ofPoint(x, y, 0), func);
}

std::vector<ofxMixedFontUtil::ofxGlyphData> ofxBaseFont::typesetString(const std::u32string &utf32_string, const ofPoint &coord, const ofxMixedFontUtil::ofxCompFunc &func)
{
    std::vector<ofxMixedFontUtil::ofxGlyphData> glyph_list;
    for (int index = 0; index < utf32_string.length();) {
        int length = 0;
        glyph_list.push_back(makeGlyphData(utf32_string, index, length));
        index += (length > 0) ? length : 1;
    }
    
    func(shared_from_this(), coord, glyph_list);
    
    return glyph_list;
}

}
