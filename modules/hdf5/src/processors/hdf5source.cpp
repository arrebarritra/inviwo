/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2014-2025 Inviwo Foundation
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

#include <modules/hdf5/processors/hdf5source.h>
#include <modules/hdf5/datastructures/hdf5handle.h>

#include <fmt/std.h>

namespace inviwo::hdf5 {

const ProcessorInfo Source::processorInfo_{
    "org.inviwo.hdf5.Source",  // Class identifier
    "HDF Source",              // Display name
    "Data Input",              // Category
    CodeState::Stable,         // Code state
    Tags::CPU | Tag{"HDF5"},   // Tags
    "Open a handle to a HDF File"_help,
};
const ProcessorInfo& Source::getProcessorInfo() const { return processorInfo_; }

Source::Source()
    : Processor()
    , file_("filename", "HDF File", "File to open"_help)
    , port_("outport", "A HDF5 file handle"_help) {
    addPort(port_);
    addProperty(file_);
}

void Source::process() {
    if (file_.get().empty()) {
        port_.setData(nullptr);
        return;
    }

    try {
        auto data = std::make_shared<Handle>(file_.get());
        port_.setData(data);
    } catch (H5::Exception& e) {
        log::warn("Could not load file: {}: {}", file_.get(), e.getCDetailMsg());
    } catch (...) {
        log::warn("Could not load file: {}", file_.get());
    }
}

}  // namespace inviwo::hdf5
