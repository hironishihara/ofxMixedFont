#pragma once

#include "ofxMixedFontUtil.hpp"

struct FT_FaceRec_;
struct FT_LibraryRec_;
class ofTexture;
class ofPath;
template<typename T> class ofPixels_;
typedef ofPixels_<unsigned char> ofPixels;
template<typename T> class ofColor_;
typedef ofColor_<unsigned char> ofColor;
class ofMesh;

class ofxFT2Font : public ofxMixedFontUtil::ofxBaseFont
{
public:
    ofxFT2Font();
    ofxFT2Font(const std::string &file_name, float font_size_pt);
    virtual ~ofxFT2Font() {};
    
    ofxFT2Font(const ofxFT2Font &) = delete;
    ofxFT2Font(ofxFT2Font &&) = delete;
    ofxFT2Font &operator=(const ofxFT2Font &) = delete;
    ofxFT2Font &operator=(ofxFT2Font &&) = delete;
    
    int initialize(const std::string &file_name, float font_size_pt);
    int reset();
    enum DrawingMode { TEXTURE_MODE, PATH_MODE };
    bool selectDrawingMode(const DrawingMode &drawing_mode);
    
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
    int setNotDefGlyphProps() override;
    int setSpaceGlyphProps() override;
    int setFullWidthSpaceGlyphProps() override;
    int setLineFeedGlyphProps() override;
    using ofxBaseFont::typesetString;
    
private:
    static std::shared_ptr<FT_LibraryRec_> ft_library_;
    
    std::string file_path_;
    std::shared_ptr<FT_FaceRec_> ft_face_;
    bool is_mono_font_;
    float internal_scale_factor_;
    DrawingMode drawing_mode_;

    int getGlyphIndex(const std::u32string &code_point);
    int loadGlyph(const std::u32string &code_point, bool try_load_sub = false);
    int makeSpaceGlyphProps(const char32_t &code_point, const float &scale);

    std::vector<ofxMixedFontUtil::ofxGlyphData> loaded_glyphs_;
    std::vector<ofPath> loaded_glyph_outlines_;
    
    static const int ATLAS_TEXTURE_SIZE;
    std::shared_ptr<ofPixels> atlas_pixels_;
    std::shared_ptr<ofTexture> atlas_texture_;
    int atlas_offset_x_;
    int atlas_offset_y_;
    int next_atlas_offset_y_;
    bool atlas_pixels_have_been_updated_;
    std::shared_ptr<ofMesh> string_quads_;
    
    bool blend_is_enabled_;
    int blend_src_, blend_dst_;
    std::unique_ptr<ofColor> color_;
    
    void bind();
    void addCharQuad(const int &glyph_index, const ofPoint &coord);
    void unbind();
    
};




