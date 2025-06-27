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

#include <modules/base/io/datvolumewriter.h>

#include <inviwo/core/datastructures/datamapper.h>                      // for DataMapper
#include <inviwo/core/datastructures/image/imagetypes.h>                // for enumToStr, Swizzl...
#include <inviwo/core/datastructures/representationconverter.h>         // for RepresentationCon...
#include <inviwo/core/datastructures/representationconverterfactory.h>  // for RepresentationCon...
#include <inviwo/core/datastructures/unitsystem.h>                      // for Axis, Unit
#include <inviwo/core/datastructures/volume/volume.h>                   // for Volume, DataWrite...
#include <inviwo/core/datastructures/volume/volumeram.h>                // for VolumeRAM
#include <inviwo/core/io/datawriter.h>                                  // for DataWriterType
#include <inviwo/core/io/datawriterexception.h>                         // for DataWriterException
#include <inviwo/core/io/inviwofileformattypes.h>                       // for ByteOrder...
#include <inviwo/core/metadata/metadata.h>                              // for StringMetaData
#include <inviwo/core/metadata/metadatamap.h>                           // for MetaDataMap
#include <inviwo/core/util/fileextension.h>                             // for FileExtension
#include <inviwo/core/util/filesystem.h>                                // for ofstream, getFile...
#include <inviwo/core/util/stdextensions.h>                             // for overloaded

#include <array>          // for array
#include <fstream>        // for stringstream, bas...
#include <memory>         // for unique_ptr
#include <string>         // for char_traits, string
#include <unordered_set>  // for unordered_set
#include <vector>         // for vector

#include <fmt/core.h>     // for basic_string_view
#include <fmt/format.h>   // for join
#include <fmt/ostream.h>  // for print
#include <fmt/std.h>

#include <glm/fwd.hpp>           // for mat4, mat3, vec3
#include <glm/gtc/type_ptr.hpp>  // for value_ptr
#include <glm/gtx/range.hpp>     // for begin, end
#include <glm/mat3x3.hpp>        // for mat<>::col_type
#include <glm/mat4x4.hpp>        // for mat<>::col_type
#include <glm/matrix.hpp>        // for transpose

namespace inviwo {

DatVolumeWriter::DatVolumeWriter() : DataWriterType<Volume>() {
    addExtension(FileExtension("dat", "Inviwo dat Volume file format"));
}

DatVolumeWriter* DatVolumeWriter::clone() const { return new DatVolumeWriter(*this); }

void DatVolumeWriter::writeData(const Volume* data, const std::filesystem::path& filePath) const {
    util::writeDatVolume(*data, filePath, getOverwrite());
}

namespace util {

void writeDatVolume(const Volume& data, const std::filesystem::path& filePath,
                    Overwrite overwrite) {
    auto rawPath = filePath;
    rawPath.replace_extension("raw");

    DataWriter::checkOverwrite(filePath, overwrite);
    DataWriter::checkOverwrite(rawPath, overwrite);

    auto fileName = filePath.stem().string();
    // Write the .dat file content
    std::stringstream ss;
    const VolumeRAM* vr = data.getRepresentation<VolumeRAM>();
    glm::mat3 basis = glm::transpose(data.getBasis());
    glm::vec3 offset = data.getOffset();
    glm::mat4 wtm = glm::transpose(data.getWorldMatrix());

    auto print = util::overloaded{
        [&ss](std::string_view key, const std::string& val) {
            fmt::print(ss, "{}: {}\n", key, val);
        },
        [&ss](std::string_view key, std::string_view val) { fmt::print(ss, "{}: {}\n", key, val); },
        [&ss](std::string_view key, InterpolationType val) {
            fmt::print(ss, "{}: {}\n", key, val);
        },
        [&ss](std::string_view key, const SwizzleMask& mask) {
            fmt::print(ss, "{}: {}{}{}{}\n", key, mask[0], mask[1], mask[2], mask[3]);
        },
        [&ss](std::string_view key, const Wrapping3D& wrapping) {
            fmt::print(ss, "{}: {} {} {}\n", key, wrapping[0], wrapping[1], wrapping[2]);
        },
        [&ss](std::string_view key, const Unit& unit) { fmt::print(ss, "{}: {}\n", key, unit); },
        [&ss](std::string_view key, ByteOrder byteOrder) {
            fmt::print(ss, "{}: {}\n", key, byteOrder);
        },
        [&ss](std::string_view key, Compression compression) {
            fmt::print(ss, "{}: {}\n", key, compression);
        },
        [&ss](std::string_view key, const auto& vec) {
            fmt::print(ss, "{}: {}\n", key, fmt::join(begin(vec), end(vec), " "));
        }};

    print("RawFile", fmt::format("{}.raw", fileName));
    print("Resolution", vr->getDimensions());
    print("Format", vr->getDataFormatString());
    print("ByteOrder", ByteOrder::LittleEndian);
    print("ByteOffset", std::string("0"));
    print("Compression", Compression::Disabled);
    print("BasisVector1", basis[0]);
    print("BasisVector2", basis[1]);
    print("BasisVector3", basis[2]);
    print("Offset", offset);
    print("WorldVector1", wtm[0]);
    print("WorldVector2", wtm[1]);
    print("WorldVector3", wtm[2]);
    print("WorldVector4", wtm[3]);
    print("DataRange", data.dataMap.dataRange);
    print("ValueRange", data.dataMap.valueRange);
    print("ValueUnit", data.dataMap.valueAxis.unit);
    print("ValueName", data.dataMap.valueAxis.name);

    print("Axis1Name", data.axes[0].name);
    print("Axis2Name", data.axes[1].name);
    print("Axis3Name", data.axes[2].name);

    print("Axis1Unit", data.axes[0].unit);
    print("Axis2Unit", data.axes[1].unit);
    print("Axis3Unit", data.axes[2].unit);

    print("SwizzleMask", vr->getSwizzleMask());
    print("Interpolation", vr->getInterpolation());
    print("Wrapping", vr->getWrapping());

    for (const auto& key : data.getMetaDataMap()->getKeys()) {
        const auto* m = data.getMetaDataMap()->get(key);
        if (const auto* sm = dynamic_cast<const StringMetaData*>(m)) print(key, sm->get());
    }

    if (auto f = std::ofstream(filePath)) {
        f << ss.str();
    } else {
        throw DataWriterException(SourceContext{}, "Could not write to dat file: {}", filePath);
    }

    if (auto f = std::ofstream(rawPath, std::ios::out | std::ios::binary)) {
        f.write(static_cast<const char*>(vr->getData()), vr->getNumberOfBytes());
    } else {
        throw DataWriterException(SourceContext{}, "Could not write to raw file: {}", rawPath);
    }
}

}  // namespace util

}  // namespace inviwo
