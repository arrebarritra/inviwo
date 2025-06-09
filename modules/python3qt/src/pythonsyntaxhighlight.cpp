/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2020-2025 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include <modules/python3qt/pythonsyntaxhighlight.h>

#include <inviwo/core/properties/optionproperty.h>   // for OptionPropertyString
#include <inviwo/core/properties/ordinalproperty.h>  // for FloatVec4Property, ordinalColor, Int...
#include <inviwo/core/properties/property.h>         // for Property
#include <inviwo/core/util/settings/settings.h>      // for Settings
#include <modules/qtwidgets/inviwoqtutils.h>         // for toQColor, getDefaultMonoSpaceFontIndex
#include <modules/qtwidgets/syntaxhighlighter.h>     // for SyntaxHighlighter, background, comment

#include <array>        // for array
#include <chrono>       // for literals
#include <string>       // for string
#include <string_view>  // for operator""sv, string_view, basic_str...

#include <QColor>           // for QColor
#include <QTextCharFormat>  // for QTextCharFormat

using namespace std::literals;

namespace inviwo {

PythonSyntaxHighlight::PythonSyntaxHighlight()
    : Settings("Python Syntax Highlighting")
    , font("font", "Font", utilqt::getMonoSpaceFonts(), utilqt::getDefaultMonoSpaceFontIndex())
    , fontSize("fontSize", "Size", syntax::fontSize, 1, 72)
    , textColor("text", "Text", util::ordinalColor(syntax::text))
    , backgroundColor("background", "Background", util::ordinalColor(syntax::background))
    , highLightColor("highLight", "HighLight", util::ordinalColor(syntax::highLight))
    , keywordColor("type", "Type", util::ordinalColor(syntax::keyword))
    , literalColor("literal", "String Literal", util::ordinalColor(syntax::literal))
    , constantColor("constant", "Constant", util::ordinalColor(syntax::constant))
    , commentColor("comment", "Comment", util::ordinalColor(syntax::comment)) {
    addProperties(font, fontSize, textColor, backgroundColor, highLightColor, keywordColor,
                  literalColor, constantColor, commentColor);

    load();
}

namespace {

constexpr const std::array pythonKeywords = {
    "and"sv,   "as"sv,     "assert"sv, "break"sv, "class"sv,   "continue"sv, "def"sv,  "del"sv,
    "elif"sv,  "else"sv,   "except"sv, "exec"sv,  "finally"sv, "for"sv,      "from"sv, "global"sv,
    "if"sv,    "import"sv, "in"sv,     "is"sv,    "lambda"sv,  "not"sv,      "or"sv,   "pass"sv,
    "print"sv, "raise"sv,  "return"sv, "try"sv,   "while"sv,   "with"sv,     "yield"sv};

}  // namespace

std::vector<std::shared_ptr<std::function<void()>>> utilqt::setPythonSyntaxHighlight(
    SyntaxHighlighter& sh, PythonSyntaxHighlight& settings) {

    QColor bgColor = utilqt::toQColor(settings.backgroundColor);

    QTextCharFormat defaultFormat;
    defaultFormat.setBackground(bgColor);
    defaultFormat.setForeground(utilqt::toQColor(settings.textColor));

    QTextCharFormat keywordformat;
    keywordformat.setBackground(bgColor);
    keywordformat.setForeground(utilqt::toQColor(settings.keywordColor));

    QTextCharFormat constantsformat;
    constantsformat.setBackground(bgColor);
    constantsformat.setForeground(utilqt::toQColor(settings.constantColor));

    QTextCharFormat commentformat;
    commentformat.setBackground(bgColor);
    commentformat.setForeground(utilqt::toQColor(settings.commentColor));

    QTextCharFormat literalFormat;
    literalFormat.setBackground(bgColor);
    literalFormat.setForeground(utilqt::toQColor(settings.literalColor));

    sh.clear();

    sh.setFont(settings.font);
    sh.setFontSize(settings.fontSize);
    sh.setHighlight(settings.highLightColor);
    sh.setDefaultFormat(defaultFormat);

    sh.addWordBoundaryPattern(keywordformat, pythonKeywords);
    sh.addPattern(constantsformat, "\\b([0-9]+\\.)?[0-9]+([eE][+-]?[0-9]+)?");

    sh.addPattern(literalFormat, R"("([^"\\]|\\.)*")");
    sh.addPattern(literalFormat, R"('([^'\\]|\\.)*')");

    sh.addPattern(commentformat, "#.*$");
    sh.addMultBlockPattern(literalFormat, R"(""")"sv, R"(""")"sv);
    sh.addMultBlockPattern(literalFormat, R"(''')"sv, R"(''')"sv);

    sh.update();

    std::vector<std::shared_ptr<std::function<void()>>> callbacks;
    for (auto p : settings) {
        callbacks.emplace_back(p->onChangeScoped(
            [psh = &sh, psettings = &settings]() { setPythonSyntaxHighlight(*psh, *psettings); }));
    }
    return callbacks;
}

std::vector<std::shared_ptr<std::function<void()>>> utilqt::setPythonOutputSyntaxHighlight(
    SyntaxHighlighter& sh, PythonSyntaxHighlight& settings) {

    QColor bgColor = utilqt::toQColor(settings.backgroundColor);

    QTextCharFormat defaultFormat;
    defaultFormat.setBackground(bgColor);
    defaultFormat.setForeground(utilqt::toQColor(settings.textColor));

    QTextCharFormat constantsformat;
    constantsformat.setBackground(bgColor);
    constantsformat.setForeground(utilqt::toQColor(settings.constantColor));

    sh.clear();

    sh.setFont(settings.font);
    sh.setFontSize(settings.fontSize);
    sh.setHighlight(settings.highLightColor);
    sh.setDefaultFormat(defaultFormat);

    sh.addPattern(constantsformat, "\\b([0-9]+\\.)?[0-9]+([eE][+-]?[0-9]+)?");

    sh.update();

    std::vector<std::shared_ptr<std::function<void()>>> callbacks;
    for (auto p : settings) {
        callbacks.emplace_back(p->onChangeScoped([psh = &sh, psettings = &settings]() {
            setPythonOutputSyntaxHighlight(*psh, *psettings);
        }));
    }
    return callbacks;
}

}  // namespace inviwo
