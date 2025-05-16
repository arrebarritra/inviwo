/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2017-2025 Inviwo Foundation
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

#pragma once

#include <modules/userinterfacegl/userinterfaceglmoduledefine.h>  // for IVW_MODULE_USERINTERFAC...

#include <inviwo/core/ports/imageport.h>                 // for ImageInport, ImageOutport
#include <inviwo/core/processors/processor.h>            // for Processor
#include <inviwo/core/processors/processorinfo.h>        // for ProcessorInfo
#include <inviwo/core/properties/boolproperty.h>         // for BoolProperty
#include <inviwo/core/properties/buttonproperty.h>       // for ButtonProperty
#include <inviwo/core/properties/compositeproperty.h>    // for CompositeProperty
#include <inviwo/core/properties/eventproperty.h>        // for EventProperty
#include <inviwo/core/properties/filepatternproperty.h>  // for FilePatternProperty
#include <inviwo/core/properties/ordinalproperty.h>      // for IntProperty
#include <inviwo/core/properties/stringproperty.h>       // for StringProperty
#include <inviwo/core/util/fileextension.h>              // for FileExtension

#include <memory>  // for shared_ptr
#include <string>  // for string
#include <vector>  // for vector

namespace inviwo {

class Event;
class Image;

/**
 * @brief processor for switching between slide images and another image inport
 */
class IVW_MODULE_USERINTERFACEGL_API PresentationProcessor : public Processor {
public:
    PresentationProcessor();
    virtual ~PresentationProcessor() = default;

    virtual void process() override;

    virtual const ProcessorInfo& getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

private:
    void updateSlideImage();
    void onFindFiles();
    bool isValidImageFile(const std::filesystem::path& file);
    void updateProperties();
    void updateFileName();

    void nextSlide(Event* e);
    void previousSlide(Event* e);

    ImageInport inport_;
    ImageOutport outport_;

    BoolProperty presentationMode_;

    FilePatternProperty imageFilePattern_;
    ButtonProperty findFilesButton_;
    IntProperty slideIndex_;
    StringProperty imageFileName_;

    CompositeProperty interactions_;
    EventProperty toggleMode_;
    EventProperty quitPresentation_;
    EventProperty nextSlide_;
    EventProperty prevSlide_;
    EventProperty nextSlideAlt_;
    EventProperty prevSlideAlt_;
    EventProperty mouseNextSlide_;
    EventProperty mousePrevSlide_;

    std::vector<FileExtension> validExtensions_;
    std::vector<std::filesystem::path> fileList_;

    std::shared_ptr<Image> currentSlide_;
};

}  // namespace inviwo
