#include "ofxFT2Font.hpp"

#include <ft2build.h>

#ifdef TARGET_LINUX
#include <fontconfig/fontconfig.h>
#endif

#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_TRIGONOMETRY_H
#include FT_TRUETYPE_TABLES_H

#include "ofUtils.h"
#include "ofPixels.h"
#include "ofTexture.h"
#include "ofGraphics.h"
#include "ofPath.h"

std::shared_ptr<FT_LibraryRec_> ofxFT2Font::ft_library_;
const int ofxFT2Font::ATLAS_TEXTURE_SIZE = GL_MAX_TEXTURE_SIZE;

static std::shared_ptr<FT_LibraryRec_> initFTLibrary()
{
    FT_LibraryRec_ *ft_library;
    FT_Error err = FT_Init_FreeType(&ft_library);
    if (err){
        ofLogError("ofxFT2Font") << "initFTLibrary(): couldn't initialize Freetype lib: FT_Error " << err;
        return std::shared_ptr<FT_LibraryRec_>();
    }
    
    return std::shared_ptr<FT_LibraryRec_>(ft_library, FT_Done_FreeType);
}

static std::string resolveFontFilePath(const std::string &file_name)
{
    std::string file_path = ofToDataPath(file_name, true);
    ofFile fontFile(file_path, ofFile::Reference);
    if (!fontFile.exists()) {
        ofLogError("ofxFT2Font") << "resolveFontFilePath(): couldn't find font \"" << file_name << "\"";
        return std::string();
    }
    
    return file_path;
}

static std::shared_ptr<FT_FaceRec_> initFTFace(std::shared_ptr<FT_LibraryRec_> ft_library, const std::string &file_name)
{
    std::string file_path = resolveFontFilePath(file_name);
    if (file_path.empty()) {
        return std::shared_ptr<FT_FaceRec_>();
    }
    
    int font_id = 0;
    FT_FaceRec_ *ft_face;
    FT_Error err = FT_New_Face(ft_library.get(), file_path.c_str(), font_id, &ft_face);
    if (err) {
        string errorString = "unknown font type";
        if(err == 1) errorString = "invalid filename";
        ofLogError("ofxFT2Font") << "initFTFace(): couldn't create new face for \"" << file_name << "\": FT_Error " << err << " " << errorString;
        return std::shared_ptr<FT_FaceRec_>();
    }
    
    return std::shared_ptr<FT_FaceRec_>(ft_face, FT_Done_Face);
}

static bool IsColorBitmapFont(std::shared_ptr<FT_FaceRec_> ft_face) {
    FT_ULong tags_list[] = {
        FT_MAKE_TAG('C', 'B', 'D', 'T'), // Google's CBLC+CBDT
        FT_MAKE_TAG('s', 'b', 'i', 'x'), // Apple's SBIX
    };
    
    bool is_color_font = false;
    for (FT_ULong tag : tags_list) {
        FT_ULong length = 0;
        FT_Error err = FT_Load_Sfnt_Table(ft_face.get(), tag, 0, nullptr, &length);
        if (length > 0) {
            is_color_font |= true;
        }
    }
    
    return is_color_font;
}

static bool setFTCharSize(std::shared_ptr<FT_FaceRec_> ft_face, int font_size_pt, int dpi)
{
    if (ft_face) {
        FT_Error err = FT_Set_Char_Size(ft_face.get(), font_size_pt << 6, font_size_pt << 6, dpi, dpi);
        if (err) {
            ofLogError("ofxFT2Font") << "setFTCharSize(): couldn't set font size: FT_Error " << err;
            return false;
        }
        else {
            return true;
        }
    }
    
    return false;
}

static bool selectFTCharSize(std::shared_ptr<FT_FaceRec_> ft_face, int font_size_pt, int dpi)
{
    if (ft_face->num_fixed_sizes == 0) {
        return true;
    }
    
    float font_size_px = font_size_pt * dpi / ofxMixedFontUtil::PT_PER_INCH;
    int best_match = 0;
    int diff = std::abs(font_size_px - ft_face->available_sizes[0].width);
    for (int i = 1; i < ft_face->num_fixed_sizes; ++i) {
        int ndiff = std::abs(font_size_px - ft_face->available_sizes[i].width);
        if (ndiff < diff) {
            best_match = i;
            diff = ndiff;
        }
    }
    FT_Error err = FT_Select_Size(ft_face.get(), best_match);
    if (err) {
        ofLogError("ofxFT2Font") << "selectFTCharSize(): couldn't select font size: FT_Error " << err;
        return false;
    }
    
    return true;
}

static ofPath makeContoursForCharacter(const FT_Outline &outline)
{
    char *tags = outline.tags;
    FT_Vector *vec = outline.points;
    
    ofPath charOutlines;
    charOutlines.setUseShapeColor(false);
    
    int startPos = 0;
    for (int k = 0; k < outline.n_contours; ++k) {
        if (k > 0) {
            startPos = outline.contours[k-1]+1;
        }
        int endPos = outline.contours[k]+1;
        
        ofPoint lastPoint;
        for (int j = startPos; j < endPos; ++j) {
            if (FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_ON) {
                lastPoint.set((float)vec[j].x, (float)-vec[j].y, 0);
                if (j==startPos) {
                    charOutlines.moveTo(lastPoint/64);
                }
                else {
                    charOutlines.lineTo(lastPoint/64);
                }
            }
            else if (FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_CUBIC) {
                int prevPoint = j - 1;
                if (j == 0) {
                    prevPoint = endPos - 1;
                }
                
                int nextIndex = j + 1;
                if (nextIndex >= endPos) {
                    nextIndex = startPos;
                }
                ofPoint nextPoint((float)vec[nextIndex].x, -(float)vec[nextIndex].y);
                
                bool lastPointCubic = (FT_CURVE_TAG(tags[prevPoint]) != FT_CURVE_TAG_ON) && (FT_CURVE_TAG(tags[prevPoint]) == FT_CURVE_TAG_CUBIC);
                if (lastPointCubic) {
                    ofPoint controlPoint1((float)vec[prevPoint].x, (float)-vec[prevPoint].y);
                    ofPoint controlPoint2((float)vec[j].x, (float)-vec[j].y);
                    ofPoint nextPoint((float) vec[nextIndex].x,	-(float) vec[nextIndex].y);
                    charOutlines.bezierTo(controlPoint1.x/64, controlPoint1.y/64, controlPoint2.x/64, controlPoint2.y/64, nextPoint.x/64, nextPoint.y/64);
                }
            }
            else { // FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_CONIC
                ofPoint conicPoint( (float)vec[j].x,  -(float)vec[j].y );
                if (j == startPos) {
                    bool prevIsConnic = (FT_CURVE_TAG(tags[endPos-1]) != FT_CURVE_TAG_ON) && (FT_CURVE_TAG(tags[endPos-1]) != FT_CURVE_TAG_CUBIC);
                    if (prevIsConnic) {
                        ofPoint lastConnic((float)vec[endPos - 1].x, (float)-vec[endPos - 1].y);
                        lastPoint = (conicPoint + lastConnic) / 2;
                    }
                    else {
                        lastPoint.set((float)vec[endPos-1].x, (float)-vec[endPos-1].y, 0);
                    }
                }
                
                int nextIndex = j+1;
                if(nextIndex >= endPos) {
                    nextIndex = startPos;
                }
                ofPoint nextPoint((float)vec[nextIndex].x, -(float)vec[nextIndex].y);
                
                bool nextIsConnic = (FT_CURVE_TAG(tags[nextIndex]) != FT_CURVE_TAG_ON) && (FT_CURVE_TAG(tags[nextIndex]) != FT_CURVE_TAG_CUBIC);
                if (nextIsConnic) {
                    nextPoint = (conicPoint + nextPoint) / 2;
                }
                charOutlines.quadBezierTo(lastPoint.x/64, lastPoint.y/64, conicPoint.x/64, conicPoint.y/64, nextPoint.x/64, nextPoint.y/64);
                
                if (nextIsConnic) {
                    lastPoint = nextPoint;
                }
            }
        }
        charOutlines.close();
    }
    
    return charOutlines;
}

int ofxFT2Font::initialize(const std::string &file_name, float font_size_pt)
{
    if (!ft_library_) {
        ft_library_ = initFTLibrary();
        if (!ft_library_) {
            return -1;
        }
    }
    
    /*
     #ifdef TARGET_LINUX
     FcBool result = FcInit();
     if(!result){
     return -10;
     }
     #endif
     #ifdef TARGET_WIN32
     initWindows();
     #endif
     */
    
    ft_face_ = initFTFace(ft_library_, file_name);
    if (!ft_face_) {
        ft_face_.reset();
        return -2;
    }
    
    is_mono_font_ = IsColorBitmapFont(ft_face_) ? false : true;
    bool is_ok = false;
    if (is_mono_font_) {
        is_ok = setFTCharSize(ft_face_, font_size_pt, ofxMixedFontUtil::DPI);
    }
    else {
        
        is_ok = selectFTCharSize(ft_face_, font_size_pt, ofxMixedFontUtil::DPI);
    }
    
    if (!is_ok) {
        ft_face_.reset();
        return -3;
    }
    
    if (!is_mono_font_) {
        float font_size = font_size_pt * ofxMixedFontUtil::DPI / ofxMixedFontUtil::PT_PER_INCH;
        internal_scale_factor_ = font_size / ft_face_->size->metrics.x_ppem;
    }
    
    atlas_pixels_ = std::shared_ptr<ofPixels>(new ofPixels());
    if (is_mono_font_) {
        atlas_pixels_->allocate(ATLAS_TEXTURE_SIZE, ATLAS_TEXTURE_SIZE, 2);
        atlas_pixels_->set(0, 255);
    }
    else {
        atlas_pixels_->allocate(ATLAS_TEXTURE_SIZE, ATLAS_TEXTURE_SIZE, 4);
    }
    
    atlas_texture_ = std::shared_ptr<ofTexture>(new ofTexture());
    atlas_texture_->allocate(*atlas_pixels_);
    string_quads_ = std::shared_ptr<ofMesh>(new ofMesh());
    
    std::vector<ofxMixedFontUtil::ofxGlyphData>().swap(loaded_glyphs_);
    std::vector<ofPath>().swap(loaded_glyph_outlines_);
    
    file_path_ = file_name;
    
    is_ready_ = true;
    texture_is_enabled_ = true;
    path_is_enabled_ = is_mono_font_;
    
    font_props_.font_size_pt = font_size_pt;
    font_props_.x_ppem = ft_face_->size->metrics.x_ppem * internal_scale_factor_;
    font_props_.y_ppem = ft_face_->size->metrics.y_ppem * internal_scale_factor_;
    font_props_.line_height = (ft_face_->size->metrics.height >> 6) * internal_scale_factor_;
    font_props_.ascender_height = (ft_face_->size->metrics.ascender >> 6) * internal_scale_factor_;
    font_props_.descender_height = (ft_face_->size->metrics.descender >> 6) * internal_scale_factor_;
    
    if (setNotDefGlyphProps() != 0) {
        ft_face_.reset();
        return -4;
    }
    if (setSpaceGlyphProps() != 0) {
        ft_face_.reset();
        return -5;
    }
    if (setFullWidthSpaceGlyphProps() != 0) {
        ft_face_.reset();
        return -6;
    }
    if (setLineFeedGlyphProps() != 0) {
        ft_face_.reset();
        return -7;
    }
    
    return 0;
}

int ofxFT2Font::reset()
{
    return initialize(file_path_, font_props_.font_size_pt);
}

bool ofxFT2Font::selectDrawingMode(const DrawingMode &drawing_mode) {
    bool is_successful = false;
    
    switch (drawing_mode) {
        case PATH_MODE:
            if (pathIsEnabled()) {
                drawing_mode_ = PATH_MODE;
                is_successful = true;
            }
            break;
        default:
            if (textureIsEnabled()) {
                drawing_mode_ = TEXTURE_MODE;
                is_successful = true;
            }
            break;
    }
    
    return is_successful;
}

ofxFT2Font::ofxFT2Font()
: file_path_(""), is_mono_font_(true), drawing_mode_(TEXTURE_MODE), internal_scale_factor_(1.0), atlas_pixels_have_been_updated_(false), atlas_offset_x_(0), atlas_offset_y_(0), next_atlas_offset_y_(0)
{
    
}

ofxFT2Font::ofxFT2Font(const std::string &file_name, float font_size_pt)
: ofxFT2Font()
{
    initialize(file_name, font_size_pt);
}

ofxMixedFontUtil::ofxGlyphData ofxFT2Font::makeGlyphData(const std::u32string &utf32_character, const int &index, int &length)
{
    length = -1; // Note: This font is not ready.
    if (!isReady()) return ofxMixedFontUtil::ofxGlyphData();
    
    length = 0;
    ofxMixedFontUtil::ofxGlyphData glyph = { shared_from_this(), loaded_glyphs_[0].props, ofPoint(0, 0) }; // NotDef Glyph

    std::u32string code_point = { utf32_character[index] };
    int glyph_index = getGlyphIndex(code_point);
    if (glyph_index > 0) {
        glyph.props = loaded_glyphs_[glyph_index].props;
        ++length;
    }
    
    return glyph;
}

void ofxFT2Font::drawGlyphs(const std::vector<ofxMixedFontUtil::ofxGlyphData> &glyph_list)
{
    if (!isReady()) return;
    
    switch (drawing_mode_) {
        case PATH_MODE:
            drawGlyphsWithPath(glyph_list);
            break;
        default:
            drawGlyphsWithTexture(glyph_list);
            break;
    }
}

void ofxFT2Font::drawGlyphsWithTexture(const std::vector<ofxMixedFontUtil::ofxGlyphData> &glyph_list)
{
    if (!isReady()) return;
    if (!textureIsEnabled()) return;
    
    bind();
    for (auto &glyph : glyph_list) {
        if (shared_from_this() == glyph.font.lock()) {
            int glyph_index = getGlyphIndex(glyph.props.code_point);
            if (glyph_index == -1) {
                continue;
            }
            addCharQuad(glyph_index, glyph.coord);
        }
    }
    unbind();
}

void ofxFT2Font::drawGlyphsWithPath(const std::vector<ofxMixedFontUtil::ofxGlyphData> &glyph_list)
{
    if (!isReady()) return;
    if (!pathIsEnabled()) return;
    
    for (auto &glyph : glyph_list) {
        if (shared_from_this() == glyph.font.lock()) {
            int glyph_index = getGlyphIndex(glyph.props.code_point);
            if (glyph_index == -1) {
                continue;
            }
            loaded_glyph_outlines_[glyph_index].setFilled(false);
            loaded_glyph_outlines_[glyph_index].setStrokeWidth(0.5);
            loaded_glyph_outlines_[glyph_index].draw(glyph.coord.x, glyph.coord.y);
        }
    }
}

static int pasteIntoAtlasPixels(const FT_Bitmap &bitmap, const bool is_mono_font, const std::shared_ptr<ofPixels> &atlas_pixels, int &offset_x, int &offset_y, int &next_offset_y)
{
    ofPixels texturePixels;
    if (is_mono_font) {
        ofPixels bitmapPixels;
        bitmapPixels.setFromExternalPixels(bitmap.buffer, bitmap.width, bitmap.rows, 1);
        texturePixels.allocate(bitmap.width, bitmap.rows, 2);
        texturePixels.set(0, 255);
        texturePixels.setChannel(1, bitmapPixels);
    }
    else {
        texturePixels.setFromExternalPixels(bitmap.buffer, bitmap.width, bitmap.rows, 4);
    }
    
    if (offset_x + bitmap.width > atlas_pixels->getWidth()) {
        offset_x = 0;
        offset_y = next_offset_y + 1;
    }
    if (offset_y + bitmap.rows > atlas_pixels->getHeight()) {
        return -1;
    }
    
    texturePixels.pasteInto(*atlas_pixels, offset_x, offset_y);
    if (offset_y + bitmap.rows > next_offset_y) {
        next_offset_y = offset_y + bitmap.rows;
    }

    return 0;
}

static ofxMixedFontUtil::ofxGlyphData makeInternalGlyphData(const ofxMixedFontUtil::ofxGlyphProps &glyph_props, const ofPoint &coord)
{
    ofxMixedFontUtil::ofxGlyphData glyph = { std::weak_ptr<ofxMixedFontUtil::ofxBaseFont>(), glyph_props, coord };
    return glyph;
}

static ofxMixedFontUtil::ofxGlyphData makeInternalGlyphData(const float &inner_scale, const std::u32string &code_point, const FT_Glyph_Metrics &metrics, const ofPoint &coord)
{
    ofxMixedFontUtil::ofxGlyphProps glyph_props;
    glyph_props.code_point = code_point;
    glyph_props.height = (metrics.height >> 6) * inner_scale;
    glyph_props.width = (metrics.width >> 6) * inner_scale;
    glyph_props.bearing_x = (metrics.horiBearingX >> 6) * inner_scale;
    glyph_props.bearing_y = (metrics.horiBearingY >> 6) * inner_scale;
    glyph_props.advance = (metrics.horiAdvance >> 6) * inner_scale;
    glyph_props.vertical_bearing_x = (metrics.vertBearingX >> 6) * inner_scale;
    glyph_props.vertical_bearing_y = (metrics.vertBearingY >> 6) * inner_scale;
    glyph_props.vertical_advance = (metrics.vertAdvance >> 6) * inner_scale;
    
    return makeInternalGlyphData(glyph_props, coord);
}

void ofxFT2Font::drawString(const std::u32string &utf32_string, const ofPoint &coord, const ofxMixedFontUtil::ofxCompFunc &func)
{
    if (!isReady()) return;
    
    switch (drawing_mode_) {
        case PATH_MODE:
            drawStringWithPath(utf32_string, coord, func);
            break;
        default:
            drawStringWithTexture(utf32_string, coord, func);
            break;
    }
}

ofTexture ofxFT2Font::getStringAsTexture(const std::u32string &utf32_string, const ofxMixedFontUtil::ofxCompFunc &func)
{
    // if (!isReady()) return;
    return ofTexture();
}

std::vector<ofPath> ofxFT2Font::getStringAsPath(const std::u32string &utf32_string, const ofxMixedFontUtil::ofxCompFunc &func)
{
    std::vector<ofPath> outlines;
    
    if (isReady()) {
        if (pathIsEnabled()) {
            std::vector<ofxMixedFontUtil::ofxGlyphData> glyph_list = typesetString(utf32_string, ofPoint(0, 0), func);
            for (auto &glyph : glyph_list) {
                int glyph_index = getGlyphIndex(glyph.props.code_point);
                if (glyph_index == -1) {
                    continue;
                }
                outlines.push_back(loaded_glyph_outlines_[glyph_index]);
            }
        }
    }
    
    return outlines;
}

int ofxFT2Font::getGlyphIndex(const std::u32string &code_point)
{
    int index = 0;
    for (; index != loaded_glyphs_.size(); ++index) {
        if (loaded_glyphs_[index].props.code_point == code_point) {
            return index;
        }
    }
    
    if (index == loaded_glyphs_.size()) {
        index = loadGlyph(code_point);
        if (index == -1) { // Glyph is not found
            index = 0; // glyph index of sub character
        }
    }
    
    return index;
}

int ofxFT2Font::loadGlyph(const u32string &code_point, bool try_load_sub)
{
    std::u32string tmp_code_point = { code_point[0] };
    
    FT_UInt gid = (try_load_sub) ? 0 : FT_Get_Char_Index(ft_face_.get(), tmp_code_point[0]);
    if (!try_load_sub && gid ==0) {
        return 0;
    }
    
    FT_Int32 load_flags = is_mono_font_ ? FT_LOAD_DEFAULT : FT_LOAD_COLOR;
    FT_Error err = FT_Load_Glyph(ft_face_.get(), gid, load_flags);
    if (err) {
        // ofLogError("ofxFT2Font") << "loadGlyph(): error with FT_Load_Glyph \"" << code_point << "\": FT_Error " << err;
        return -2; // This font doesn't have .notdef glyph
    }
    
    FT_Render_Glyph(ft_face_->glyph, FT_RENDER_MODE_NORMAL);
    FT_Bitmap &bitmap = ft_face_->glyph->bitmap;
    if (pasteIntoAtlasPixels(bitmap, is_mono_font_, atlas_pixels_, atlas_offset_x_, atlas_offset_y_, next_atlas_offset_y_) != 0) {
        ofLogError("ofxFT2Font") << "loadGlyph(): atlas texture has been full";
        return -1;
    }
    loaded_glyphs_.push_back(makeInternalGlyphData(internal_scale_factor_, tmp_code_point, ft_face_->glyph->metrics, ofPoint(atlas_offset_x_, atlas_offset_y_)));
    atlas_pixels_have_been_updated_ = true;
    atlas_offset_x_ += bitmap.width + 1;
    
    if (pathIsEnabled()) {
        FT_Outline &outline = ft_face_->glyph->outline;
        loaded_glyph_outlines_.push_back(makeContoursForCharacter(outline));
    }
    
    return loaded_glyphs_.size() - 1;
}

int ofxFT2Font::makeSpaceGlyphProps(const char32_t &code_point, const float &scale)
{
    ofxMixedFontUtil::ofxGlyphProps glyph_props;
    glyph_props.code_point = code_point;
    glyph_props.width = 0;
    glyph_props.height = 0;
    glyph_props.bearing_x = 0;
    glyph_props.bearing_y = 0;
    glyph_props.advance = font_props_.x_ppem * scale;
    glyph_props.vertical_bearing_x = 0;
    glyph_props.vertical_bearing_y = 0;
    glyph_props.vertical_advance = font_props_.y_ppem * scale;

    loaded_glyphs_.push_back(makeInternalGlyphData(glyph_props, ofPoint(0, 0)));
    if (pathIsEnabled()) {
        loaded_glyph_outlines_.push_back(ofPath());
    }

    return 0;
}

int ofxFT2Font::setNotDefGlyphProps()
{
    return (loadGlyph({0}, true) == 0) ? 0 : makeSpaceGlyphProps(0, 1.f);
}

int ofxFT2Font::setSpaceGlyphProps()
{
    return makeSpaceGlyphProps(U' ', 0.5f);
}

int ofxFT2Font::setFullWidthSpaceGlyphProps()
{
    return makeSpaceGlyphProps(U'　', 1.f);
}

int ofxFT2Font::setLineFeedGlyphProps()
{
    return makeSpaceGlyphProps(U'\n', 0.f);
}

void ofxFT2Font::bind()
{
    if (!is_mono_font_) {
        color_.reset(new ofColor(ofGetStyle().color));
        ofSetColor(255, 255, 255);
    }
    
    GLenum format = is_mono_font_ ? GL_LUMINANCE_ALPHA : GL_BGRA;
    GLenum sfactor = is_mono_font_ ? GL_BLEND_SRC : GL_ONE;
    
    if (atlas_pixels_have_been_updated_) {
        atlas_texture_->loadData(*atlas_pixels_, format);
        atlas_texture_->setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
        atlas_pixels_have_been_updated_ = false;
    }
    
    blend_is_enabled_ = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC, &blend_src_);
    glGetIntegerv(GL_BLEND_DST, &blend_dst_);
    
    glEnable(GL_BLEND);
    glBlendFunc(sfactor, GL_ONE_MINUS_SRC_ALPHA);
    
    atlas_texture_->bind();
    string_quads_->clear();
}

void ofxFT2Font::addCharQuad(const int &glyph_index, const ofPoint &coord)
{
    if (glyph_index < 0 || loaded_glyphs_.size() <= glyph_index) {
        return;
    }
    
    GLfloat	t2 = loaded_glyphs_[glyph_index].coord.x;
    GLfloat	v2 = loaded_glyphs_[glyph_index].coord.y;
    GLfloat	t1 = t2 + loaded_glyphs_[glyph_index].props.width / internal_scale_factor_;
    GLfloat	v1 = v2 + loaded_glyphs_[glyph_index].props.height / internal_scale_factor_;
    
    GLfloat	x2 = coord.x + loaded_glyphs_[glyph_index].props.bearing_x;
    GLfloat	y2 = coord.y - loaded_glyphs_[glyph_index].props.bearing_y;
    GLfloat	x1 = x2 + loaded_glyphs_[glyph_index].props.width;
    GLfloat	y1 = y2 + loaded_glyphs_[glyph_index].props.height;
    
    int firstIndex = string_quads_->getVertices().size();
    
    string_quads_->addVertex(ofVec3f(x1,y1));
    string_quads_->addVertex(ofVec3f(x2,y1));
    string_quads_->addVertex(ofVec3f(x2,y2));
    string_quads_->addVertex(ofVec3f(x1,y2));
    
    string_quads_->addTexCoord(ofVec2f(t1,v1));
    string_quads_->addTexCoord(ofVec2f(t2,v1));
    string_quads_->addTexCoord(ofVec2f(t2,v2));
    string_quads_->addTexCoord(ofVec2f(t1,v2));
    
    string_quads_->addIndex(firstIndex);
    string_quads_->addIndex(firstIndex+1);
    string_quads_->addIndex(firstIndex+2);
    string_quads_->addIndex(firstIndex+2);
    string_quads_->addIndex(firstIndex+3);
    string_quads_->addIndex(firstIndex);
}

void ofxFT2Font::unbind()
{
    string_quads_->drawFaces();
    atlas_texture_->unbind();
    
    if(!blend_is_enabled_){
        glDisable(GL_BLEND);
    }
    glBlendFunc(blend_src_, blend_dst_);
    if (!is_mono_font_) {
        ofSetColor(*color_);
    }
}




