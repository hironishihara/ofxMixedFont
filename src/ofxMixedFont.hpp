#pragma once

#include <string>
#include <vector>
#include "ofxMixedFontUtil.hpp"

class ofTexture;
class ofPath;

typedef std::shared_ptr<ofxMixedFontUtil::ofxBaseFont> ofxBaseFontPtr;
typedef std::vector<ofxMixedFontUtil::ofxGlyphData> ofxGlyphDataList;
// class ofxMixedFont;
// typedef std::shared_ptr<ofxMixedFont> ofxMixedFontPtr;


class ofxMixedFont : public ofxMixedFontUtil::ofxBaseFont
{
public:
    ofxMixedFont() {};
    virtual ~ofxMixedFont() {};
    
    ofxMixedFont(const ofxMixedFont &) = delete;
    ofxMixedFont(ofxMixedFont &&) = delete;
    ofxMixedFont &operator=(const ofxMixedFont &) = delete;
    ofxMixedFont &operator=(ofxMixedFont &&) = delete;
    
    bool add(const ofxBaseFontPtr &font);
        
    ofxMixedFontUtil::ofxGlyphData makeGlyphData(const std::u32string &utf32_character, const int &index, int &length) override;
    void drawGlyphs(const std::vector<ofxMixedFontUtil::ofxGlyphData> &glyph_list) override;
    void drawGlyphsWithTexture(const std::vector<ofxMixedFontUtil::ofxGlyphData> &glyph_list) override;
    void drawGlyphsWithPath(const std::vector<ofxMixedFontUtil::ofxGlyphData> &glyph_list) override;
    
    void drawString(const std::u32string &utf32_string, const ofPoint &coord, const ofxMixedFontUtil::ofxCompFunc &func = ofxMixedFontUtil::defaultCompFunc) override;
    ofTexture getStringAsTexture(const std::u32string &utf32_string, const ofxMixedFontUtil::ofxCompFunc &func = ofxMixedFontUtil::defaultCompFunc) override;
    std::vector<ofPath> getStringAsPath(const std::u32string &utf32_string, const ofxMixedFontUtil::ofxCompFunc &func = ofxMixedFontUtil::defaultCompFunc) override;
    
    using ofxBaseFont::isReady;
    using ofxBaseFont::textureIsEnabled;
    using ofxBaseFont::pathIsEnabled;
    using ofxBaseFont::getFontProps;
    using ofxBaseFont::getDPI;
    using ofxBaseFont::drawString;
    using ofxBaseFont::drawStringWithTexture;
    using ofxBaseFont::drawStringWithPath;
    using ofxBaseFont::getStringBoundingBox;
    using ofxBaseFont::getGlyphBoundingBoxes;
    using ofxBaseFont::getStringAsTexture;
    using ofxBaseFont::getStringAsPath;
    
protected:
    int setNotDefGlyphProps() override {};
    int setSpaceGlyphProps() override {};
    int setFullWidthSpaceGlyphProps() override {};
    int setLineFeedGlyphProps() override {};
    using ofxBaseFont::typesetString;
    
private:
    std::vector<ofxBaseFontPtr> font_list;
    
};
