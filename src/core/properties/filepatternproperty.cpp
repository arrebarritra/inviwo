/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2013-2025 Inviwo Foundation
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

#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/properties/filepatternproperty.h>
#include <inviwo/core/util/filesystem.h>
#include <inviwo/core/util/filedialogstate.h>

#include <algorithm>
#include <iomanip>
#include <iterator>

namespace inviwo {

std::string_view FilePatternProperty::getClassIdentifier() const { return classIdentifier; }

FilePatternProperty::FilePatternProperty(std::string_view identifier, std::string_view displayName,
                                         Document help, const std::filesystem::path& pattern,
                                         std::string_view contentType,
                                         InvalidationLevel invalidationLevel,
                                         PropertySemantics semantics)
    : CompositeProperty(identifier, displayName, help, invalidationLevel, std::move(semantics))
    , helpText_("helpText", "",
                "A pattern might include '#' as placeholder for digits, where "
                "multiple '###' indicate leading zeros. Wildcards('*', '?') are supported.")
    , pattern_("pattern", "Pattern", {pattern}, contentType)
    , updateBtn_("updateBtn", "Update File List")
    , sort_("sorting", "Sort File Names", true)
    , matchShorterNumbers_("matchShorterNumbers", "Match Numbers with less Digits", true)
    , rangeSelection_("rangeSelection", "Range Selection", false)
    , minIndex_("minIndex", "Minimum Index", 0, -1, std::numeric_limits<int>::max())
    , maxIndex_("maxIndex", "Maximum Index", 100, -1, std::numeric_limits<int>::max()) {

    helpText_.setReadOnly(true);
    helpText_.setSemantics(PropertySemantics::Multiline);

    addProperty(pattern_);
    addProperty(helpText_);
    addProperty(sort_);
    addProperty(matchShorterNumbers_);

    addProperty(rangeSelection_);
    rangeSelection_.addProperty(minIndex_);
    rangeSelection_.addProperty(maxIndex_);

    pattern_.setAcceptMode(AcceptMode::Open);
    pattern_.setFileMode(FileMode::ExistingFiles);

    minIndex_.setSemantics(PropertySemantics::Text);
    maxIndex_.setSemantics(PropertySemantics::Text);
    minIndex_.onChange([this]() {
        if (rangeSelection_.isChecked()) {
            updateFileList();
        }
    });
    maxIndex_.onChange([this]() {
        if (rangeSelection_.isChecked()) {
            updateFileList();
        }
    });

    minIndex_.setCurrentStateAsDefault();
    maxIndex_.setCurrentStateAsDefault();
    rangeSelection_.setCollapsed(true);
    rangeSelection_.setCurrentStateAsDefault();

    auto update = [this]() { updateFileList(); };
    pattern_.onChange(update);
    rangeSelection_.onChange(update);

    sort_.onChange([this]() { sort(); });
    matchShorterNumbers_.onChange([this]() { updateFileList(); });
    if (!pattern_.get().empty()) {
        updateFileList();
    }
}

FilePatternProperty::FilePatternProperty(std::string_view identifier, std::string_view displayName,
                                         const std::filesystem::path& pattern,
                                         std::string_view contentType,
                                         InvalidationLevel invalidationLevel,
                                         PropertySemantics semantics)
    : FilePatternProperty(identifier, displayName, Document{}, pattern, contentType,
                          invalidationLevel, std::move(semantics)) {}

FilePatternProperty::FilePatternProperty(const FilePatternProperty& rhs)
    : CompositeProperty(rhs)
    , helpText_{rhs.helpText_}
    , pattern_{rhs.pattern_}
    , updateBtn_{rhs.updateBtn_}
    , sort_{rhs.sort_}
    , matchShorterNumbers_{rhs.matchShorterNumbers_}
    , rangeSelection_{rhs.rangeSelection_}
    , minIndex_{rhs.minIndex_}
    , maxIndex_{rhs.maxIndex_} {

    addProperty(pattern_);
    addProperty(helpText_);
    addProperty(sort_);
    addProperty(matchShorterNumbers_);

    addProperty(rangeSelection_);
    rangeSelection_.addProperty(minIndex_);
    rangeSelection_.addProperty(maxIndex_);

    minIndex_.onChange([this]() {
        if (rangeSelection_.isChecked()) {
            updateFileList();
        }
    });
    maxIndex_.onChange([this]() {
        if (rangeSelection_.isChecked()) {
            updateFileList();
        }
    });
    auto update = [this]() { updateFileList(); };
    pattern_.onChange(update);
    rangeSelection_.onChange(update);
    sort_.onChange([this]() { sort(); });
    matchShorterNumbers_.onChange([this]() { updateFileList(); });
    if (!pattern_.get().empty()) {
        updateFileList();
    }
}

FilePatternProperty* FilePatternProperty::clone() const { return new FilePatternProperty(*this); }

FilePatternProperty::~FilePatternProperty() = default;

std::string FilePatternProperty::getFilePattern() const {
    if (!pattern_.get().empty()) {
        return pattern_.get().front().filename().string();
    } else {
        return std::string();
    }
}

std::filesystem::path FilePatternProperty::getFilePatternPath() const {
    if (!pattern_.get().empty()) {
        return pattern_.get().front().parent_path();
    } else {
        return {};
    }
}

std::vector<std::filesystem::path> FilePatternProperty::getFileList() const {
    std::vector<std::filesystem::path> fileList;
    std::transform(files_.begin(), files_.end(), std::back_inserter(fileList),
                   [](const auto& elem) { return std::get<1>(elem); });
    return fileList;
}

std::vector<int> FilePatternProperty::getFileIndices() const {
    std::vector<int> indexList;
    std::transform(files_.begin(), files_.end(), std::back_inserter(indexList),
                   [](const auto& elem) { return std::get<0>(elem); });
    return indexList;
}

bool FilePatternProperty::hasOutOfRangeMatches() const { return outOfRangeMatches_; }

bool FilePatternProperty::hasRangeSelection() const { return rangeSelection_.isChecked(); }

int FilePatternProperty::getMinRange() const { return minIndex_.get(); }

int FilePatternProperty::getMaxRange() const { return maxIndex_.get(); }

const FileExtension& FilePatternProperty::getSelectedExtension() const {
    return pattern_.getSelectedExtension();
}

void FilePatternProperty::setSelectedExtension(const FileExtension& ext) {
    pattern_.setSelectedExtension(ext);
}

void FilePatternProperty::updateFileList() {
    files_.clear();
    outOfRangeMatches_ = false;

    if (pattern_.get().empty()) {
        return;
    }

    for (const auto& item : pattern_.get()) {
        try {
            const std::filesystem::path filePath = item.parent_path();
            const std::filesystem::path pattern = item.filename();
            const std::string strPattern = pattern.generic_string();

            auto fileList = filesystem::getDirectoryContents(filePath);

            // apply pattern
            bool hasDigits = (strPattern.find('#') != std::string::npos);
            bool hasWildcard =
                hasDigits || (strPattern.find_first_of("*?", 0) != std::string::npos);

            if (!hasWildcard) {
                // look for exact match
                if (util::contains(fileList, pattern)) {
                    files_.push_back(std::make_tuple(-1, item));
                }
            } else if (hasDigits) {
                ivec2 indexRange{-1, std::numeric_limits<int>::max()};
                if (rangeSelection_.isChecked()) {
                    indexRange = ivec2(minIndex_.get(), maxIndex_.get());
                }

                const bool matchShorterNumbers = matchShorterNumbers_.get();
                const bool matchLongerNumbers = true;
                bool found = false;
                for (auto file : fileList) {
                    int index = -1;
                    if (filesystem::wildcardStringMatchDigits(strPattern, file.generic_string(),
                                                              index, matchShorterNumbers,
                                                              matchLongerNumbers)) {
                        // match found
                        found = true;
                        // check index
                        if ((index >= indexRange.x) && (index <= indexRange.y)) {
                            files_.push_back(std::make_tuple(index, filePath / file));
                        }
                    }
                }
                outOfRangeMatches_ = (found && files_.empty());
            } else {
                // apply range selection, assume file names are sorted
                if (rangeSelection_.isChecked()) {
                    ivec2 indexRange(minIndex_.get(), maxIndex_.get());
                    if (indexRange.y < static_cast<int>(fileList.size())) {
                        // remove all files after the maximum index
                        fileList.erase(
                            fileList.begin() + std::max(static_cast<std::size_t>(indexRange.y + 1),
                                                        fileList.size()),
                            fileList.end());
                        // remove file names at the begin
                        fileList.erase(
                            fileList.begin(),
                            fileList.begin() + static_cast<std::size_t>(indexRange.y + 1));
                    }
                }
                for (const auto& file : fileList) {
                    if (filesystem::wildcardStringMatch(strPattern, file.generic_string())) {
                        // match found
                        files_.push_back(std::make_tuple(-1, filePath / file));
                    }
                }
            }
        } catch (FileException& e) {
            log::error("Error (file exception): {}", e.what());
        }
    }

    // sort file names
    sort();
}

std::string FilePatternProperty::getFormattedFileList() const {
    std::ostringstream oss;
    for (const auto& elem : files_) {
        oss << std::setw(6) << std::get<0>(elem) << ": " << std::get<1>(elem) << "\n";
    }
    return oss.str();
}

void FilePatternProperty::sort() {
    if (!sort_.get()) return;

    std::sort(files_.begin(), files_.end());
}

std::string FilePatternProperty::guessFilePattern() const {
    log::error("not implemented yet");
    return "";
}

void FilePatternProperty::clearNameFilters() { pattern_.clearNameFilters(); }

void FilePatternProperty::addNameFilter(std::string filter) { pattern_.addNameFilter(filter); }

void FilePatternProperty::addNameFilter(FileExtension filter) { pattern_.addNameFilter(filter); }

void FilePatternProperty::addNameFilters(const std::vector<FileExtension>& filters) {
    pattern_.addNameFilters(filters);
}

}  // namespace inviwo
