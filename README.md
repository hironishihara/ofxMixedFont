# ofxMixedFont

![ofxMixedFont Screenshot](https://raw.githubusercontent.com/hironishihara/ofxMixedFont/images/screenshot_0.png)

## Overview

ofxMixedFont is an openFrameworks addon to handle multiple fonts. It can render UNICODE characters including color emoji. Tested on macOS.

## Features

- ofxMixedFont enables the user to handle multiple fonts as if they were single font
- ofxMixedFont can render UNICODE characters including color emoji (Google's CBLC+CBDT and Apple's SBIX)
- The user may create own font class that derived from ofxBaseFont class, and ofxMixedFont can be used with it
- The user may overwrite typesetting function of ofxMixedFont by the use of std::function

## Installation

1. Download it ( https://github.com/hironishihara/ofxMixedFont/archive/master.zip )
1. Extract the zip file, then rename the extracted folder to ```ofxMixedFont```
1. Put the ```ofxMixedFont``` folder in ```OF_ROOT/addons```

## Usage

1. Launch ```projectGenerator```, then create new sketch with ofxMixedFont addon
1. Close ```projectGenerator```, then open the new sketch project
1. __Copy ```libfreetype.a``` in ```ofxMixedFont/libs/freetype2/lib``` and rename it to ```freetype.a```, then substitute ```freetype.a``` in ```OF_ROOT/libs/freetype/lib/freetype.a``` with it__
1. Put font files (ex. ```yourFont.ttf``` and ```emojiFont.ttf```) in ```YOUR_PROJECT/bin/data```
1. Include ```ofxMixedFont.hpp``` and ```ofxFT2Font.hpp```

   ```cpp
   // ofApp.h
   #include "ofxMixedFont.hpp"
   #include "ofxFT2Font.hpp"
   ```

1. Declare an instance of ```std::shared_ptr<ofxMixedFont>``` and ```std::shared_ptr<ofxFT2Font>```

   ```cpp
   // ofApp.h
   class ofApp : public ofBaseApp{
     ...
     std::shared_ptr<ofxMixedFont> mixedFont;
     std::shared_ptr<ofxFT2Font> yourFont, emojiFont;
   };
   ```

1. Load fonts

   ```cpp
   // ofApp.cpp
   void ofApp::setup(){
     ...
     mixedFont = std::make_shared<ofxMixedFont>();

     yourFont = std::make_shared<ofxFT2Font>("your_font.ttf", 64);
     emojiFont = std::make_shared<ofxFT2Font>("emoji_font.otf", 64);

     mixedFont->add(yourFont);
     mixedFont->add(emojiFont);
     mixedFont->add(std::make_shared<ofxFT2Font>("barfoo_font.otf", 64));
   }
   ```

1. Draw string

   ```cpp
   // ofApp.cpp
   void ofApp::draw(){
     ...
     mixedFont->drawString("こんにちは🙂", ofPoint(100, 100));
   }
   ```

1. Overwrite typesetting function

  ```cpp
  // ofApp.cpp
  void ofApp::draw(){
    ...
    auto func = [&](const ofxBaseFontPtr &font, const ofPoint &point, ofxGlyphDataList &glyph_list) {
    ofPoint pos = point;
    for (auto &glyph : glyph_list) {
        glyph.coord = pos;

        if (glyph.props.code_point == U"\n") {
            pos.x = point.x;
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
    };

    mixedFont->drawString("こんにちは🙂", ofPoint(100, 100), func);
  }
  ```

  Refer ```ofxMixedFont.hpp``` and ```ofxMixedFontUtil.hpp``` with regard to other functions.

## Contribution

1. Fork it ( http://github.com/hironishihara/ofxMixedFont/fork )
1. Create your feature branch (git checkout -b my-new-feature)
1. Commit your changes (git commit -am 'Add some feature')
1. Rebase your local changes against the master branch
1. Make sure that your feature performs as expected
1. Make sure that your feature properly works together with functions of ofxMixedFont
1. Push to the branch (git push origin my-new-feature)
1. Create new Pull Request

## System Tested

- macOS (10.12) + Xcode (8.0) + openFrameworks 0.9.4 (osx)

## ToDo

- kerning function
- ligature function
- range of unicode characters
- other directions (Right to Left, Top to Bottom, etc.)

## License

ofxMixedFont is distributed under the MIT License.
This gives everyone the freedoms to use ofxMixedFont in any context: commercial or non-commercial, public or private, open or closed source.
Please see License.txt for details.
