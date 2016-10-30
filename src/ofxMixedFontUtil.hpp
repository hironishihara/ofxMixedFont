#pragma once

#include "ofTypes.h"

#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <memory>

class ofTexture;
class ofPath;
class ofRectangle;

namespace ofxMixedFontUtil {

static const std::string VERSION = "16.10.1";

// Note: px = pt * DPI / PT_PER_INCH
static const float PT_PER_INCH = 72.0;
static const float DPI = 96.0;

static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf_converter;
const std::u32string convertStringToU32string(const std::string &src);

typedef struct {
    float font_size_pt;
    float x_ppem;
    float y_ppem;
    float line_height;
    float ascender_height;
    float descender_height;
} ofxFontProps;

class ofxBaseFont;

typedef struct {
    std::u32string code_point;
    int height;
    int width;
    int bearing_x;
    int bearing_y;
    int advance;
    int vertical_bearing_x;
    int vertical_bearing_y;
    int vertical_advance;
} ofxGlyphProps;

typedef struct {
    std::weak_ptr<ofxBaseFont> font;
    ofxGlyphProps props;
    ofPoint coord;
} ofxGlyphData;

typedef std::function<void (const std::shared_ptr<ofxBaseFont> &font, const ofPoint &coord, std::vector<ofxGlyphData> &glyph_list)> ofxCompFunc;
void defaultCompFunc(const std::shared_ptr<ofxBaseFont> &font, const ofPoint &coord, std::vector<ofxGlyphData> &glyph_list);

class ofxBaseFont : public enable_shared_from_this<ofxBaseFont>
{
public:
    ofxBaseFont();
    virtual ~ofxBaseFont() {};
    
    ofxBaseFont(const ofxBaseFont &) = delete;
    ofxBaseFont(ofxBaseFont &&) = delete;
    ofxBaseFont & operator=(const ofxBaseFont &) = delete;
    ofxBaseFont & operator=(ofxBaseFont &&) = delete;
    
    virtual bool isReady() const final;
    virtual bool textureIsEnabled() const final;
    virtual bool pathIsEnabled() const final;
    virtual ofxFontProps getFontProps() const final;
    virtual float getDPI() const final;
    
    virtual ofxGlyphData makeGlyphData(const std::u32string &utf32_character, const int &index, int &length) = 0;
    virtual void drawGlyphs(const std::vector<ofxGlyphData> &glyph_list) = 0;
    virtual void drawGlyphsWithTexture(const std::vector<ofxGlyphData> &glyph_list) = 0;
    virtual void drawGlyphsWithPath(const std::vector<ofxGlyphData> &glyph_list) = 0;
    
    // utf32
    virtual void drawString(const std::u32string &utf32_string, const ofPoint &coord, const ofxCompFunc &func = defaultCompFunc) = 0;
    virtual void drawStringWithTexture(const std::u32string &utf32_string, const ofPoint &coord, const ofxCompFunc &func = defaultCompFunc) final;
    virtual void drawStringWithPath(const std::u32string &utf32_string, const ofPoint &coord, const ofxCompFunc &func = defaultCompFunc) final;
    virtual ofRectangle getStringBoundingBox(const std::u32string &utf32_string, const ofPoint &coord, const ofxCompFunc &func = defaultCompFunc) final;
    virtual std::vector<ofRectangle> getGlyphBoundingBoxes(const std::u32string &utf32_string, const ofPoint &coord, const ofxCompFunc &func = defaultCompFunc) final;

    virtual ofTexture getStringAsTexture(const std::u32string &utf32_string, const ofxCompFunc &func = defaultCompFunc) = 0;
    virtual std::vector<ofPath> getStringAsPath(const std::u32string &utf32_string, const ofxCompFunc &func = defaultCompFunc) = 0;
    
    virtual void drawString(const std::u32string &utf32_string, const float &x, const float &y, const float &z = 0.f, const ofxCompFunc &func = defaultCompFunc) final;
    virtual void drawStringWithTexture(const std::u32string &utf32_string, const float &x, const float &y, const float &z = 0.f, const ofxCompFunc &func = defaultCompFunc) final;
    virtual void drawStringWithPath(const std::u32string &utf32_string, const float &x, const float &y, const float &z = 0.f, const ofxCompFunc &func = defaultCompFunc) final;
    virtual ofRectangle getStringBoundingBox(const std::u32string &utf32_string, const float &x, const float &y, const float &z = 0.f, const ofxCompFunc &func = defaultCompFunc) final;
    virtual std::vector<ofRectangle> getGlyphBoundingBoxes(const std::u32string &utf32_string, const float &x, const float &y, const float &z = 0.f, const ofxCompFunc &func = defaultCompFunc) final;
    
    virtual void drawString(const std::u32string &utf32_string, const float &x, const float &y, const ofxCompFunc &func) final;
    virtual void drawStringWithTexture(const std::u32string &utf32_string, const float &x, const float &y, const ofxCompFunc &func) final;
    virtual void drawStringWithPath(const std::u32string &utf32_string, const float &x, const float &y, const ofxCompFunc &func) final;
    virtual ofRectangle getStringBoundingBox(const std::u32string &utf32_string, const float &x, const float &y, const ofxCompFunc &func = defaultCompFunc) final;
    virtual std::vector<ofRectangle> getGlyphBoundingBoxes(const std::u32string &utf32_string, const float &x, const float &y, const ofxCompFunc &func = defaultCompFunc) final;
    
    // utf8 (or narrow multibyte)
    virtual void drawString(const std::string &src_string, const ofPoint &coord, const ofxCompFunc &func = defaultCompFunc) final;
    virtual void drawStringWithTexture(const std::string &src_string, const ofPoint &coord, const ofxCompFunc &func = defaultCompFunc) final;
    virtual void drawStringWithPath(const std::string &src_string, const ofPoint &coord, const ofxCompFunc &func = defaultCompFunc) final;
    virtual ofRectangle getStringBoundingBox(const std::string &src_string, const ofPoint &coord, const ofxCompFunc &func = defaultCompFunc) final;
    virtual std::vector<ofRectangle> getGlyphBoundingBoxes(const std::string &src_string, const ofPoint &coord, const ofxCompFunc &func = defaultCompFunc) final;

    virtual ofTexture getStringAsTexture(const std::string &src_string, const ofxCompFunc &func = defaultCompFunc) final;
    virtual std::vector<ofPath> getStringAsPath(const std::string &src_string, const ofxCompFunc &func = defaultCompFunc) final;
    
    virtual void drawString(const std::string &src_string, const float &x, const float &y, const float &z = 0.f, const ofxCompFunc &func = defaultCompFunc) final;
    virtual void drawStringWithTexture(const std::string &src_string, const float &x, const float &y, const float &z = 0.f, const ofxCompFunc &func = defaultCompFunc) final;
    virtual void drawStringWithPath(const std::string &src_string, const float &x, const float &y, const float &z = 0.f, const ofxCompFunc &func = defaultCompFunc) final;
    virtual ofRectangle getStringBoundingBox(const std::string &src_string, const float &x, const float &y, const float &z = 0.f, const ofxCompFunc &func = defaultCompFunc) final;
    virtual std::vector<ofRectangle> getGlyphBoundingBoxes(const std::string &src_string, const float &x, const float &y, const float &z = 0.f, const ofxCompFunc &func = defaultCompFunc) final;
    
    virtual void drawString(const std::string &src_string, const float &x, const float &y, const ofxCompFunc &func = defaultCompFunc) final;
    virtual void drawStringWithTexture(const std::string &src_string, const float &x, const float &y, const ofxCompFunc &func = defaultCompFunc) final;
    virtual void drawStringWithPath(const std::string &src_string, const float &x, const float &y, const ofxCompFunc &func = defaultCompFunc) final;
    virtual ofRectangle getStringBoundingBox(const std::string &src_string, const float &x, const float &y, const ofxCompFunc &func = defaultCompFunc) final;
    virtual std::vector<ofRectangle> getGlyphBoundingBoxes(const std::string &src_string, const float &x, const float &y, const ofxCompFunc &func = defaultCompFunc) final;
    
protected:
    virtual int setNotDefGlyphProps() = 0;
    virtual int setSpaceGlyphProps() = 0;
    virtual int setFullWidthSpaceGlyphProps() = 0;
    virtual int setLineFeedGlyphProps() = 0;
    virtual std::vector<ofxGlyphData> typesetString(const std::u32string &utf32_string, const ofPoint &coord, const ofxCompFunc &func) final;
    
    ofxFontProps font_props_;
    bool is_ready_;
    bool texture_is_enabled_;
    bool path_is_enabled_;

};

}
