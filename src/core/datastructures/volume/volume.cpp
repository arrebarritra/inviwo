/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2012-2025 Inviwo Foundation
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

#include <inviwo/core/datastructures/volume/volume.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <inviwo/core/algorithm/histogram1d.h>
#include <inviwo/core/util/document.h>

#include <fmt/format.h>

namespace inviwo {

Volume::Volume(size3_t defaultDimensions, const DataFormatBase* defaultFormat,
               const SwizzleMask& defaultSwizzleMask, InterpolationType interpolation,
               const Wrapping3D& wrapping)
    : Data<Volume, VolumeRepresentation>{}
    , StructuredGridEntity<3>{}
    , MetaDataOwner{}
    , dataMap{defaultFormat}
    , axes{util::defaultAxes<3>()}
    , defaultDimensions_{defaultDimensions}
    , defaultDataFormat_{defaultFormat}
    , defaultSwizzleMask_{defaultSwizzleMask}
    , defaultInterpolation_{interpolation}
    , defaultWrapping_{wrapping}
    , histograms_{} {}

Volume::Volume(const VolumeConfig& config)
    : Data<Volume, VolumeRepresentation>{}
    , StructuredGridEntity<3>{config.model.value_or(VolumeConfig::defaultModel),
                              config.world.value_or(VolumeConfig::defaultWorld)}
    , MetaDataOwner{}
    , dataMap{config.dataMap()}
    , axes{config.xAxis.value_or(VolumeConfig::defaultXAxis),
           config.yAxis.value_or(VolumeConfig::defaultYAxis),
           config.zAxis.value_or(VolumeConfig::defaultZAxis)}
    , defaultDimensions_{config.dimensions.value_or(VolumeConfig::defaultDimensions)}
    , defaultDataFormat_{config.format ? config.format : VolumeConfig::defaultFormat}
    , defaultSwizzleMask_{config.swizzleMask.value_or(VolumeConfig::defaultSwizzleMask)}
    , defaultInterpolation_{config.interpolation.value_or(VolumeConfig::defaultInterpolation)}
    , defaultWrapping_{config.wrapping.value_or(VolumeConfig::defaultWrapping)}
    , histograms_{} {}

Volume::Volume(std::shared_ptr<VolumeRepresentation> in)
    : Data<Volume, VolumeRepresentation>{}
    , StructuredGridEntity<3>{}
    , MetaDataOwner{}
    , dataMap{in->getDataFormat()}
    , axes{util::defaultAxes<3>()}
    , defaultDimensions_{in->getDimensions()}
    , defaultDataFormat_{in->getDataFormat()}
    , defaultSwizzleMask_{in->getSwizzleMask()}
    , defaultInterpolation_{in->getInterpolation()}
    , defaultWrapping_{in->getWrapping()}
    , histograms_{} {

    addRepresentation(std::move(in));
}

Volume::Volume(const Volume& rhs, NoData, const VolumeConfig& config)
    : Data<Volume, VolumeRepresentation>{}
    , StructuredGridEntity<3>{config.model.value_or(rhs.getModelMatrix()),
                              config.world.value_or(rhs.getWorldMatrix())}
    , MetaDataOwner{rhs}
    , dataMap{config.dataMap(rhs.dataMap)}
    , axes{config.xAxis.value_or(rhs.axes[0]), config.yAxis.value_or(rhs.axes[1]),
           config.zAxis.value_or(rhs.axes[2])}
    , defaultDimensions_{config.dimensions.value_or(rhs.getDimensions())}
    , defaultDataFormat_{config.format ? config.format : rhs.getDataFormat()}
    , defaultSwizzleMask_{config.swizzleMask.value_or(rhs.getSwizzleMask())}
    , defaultInterpolation_{config.interpolation.value_or(rhs.getInterpolation())}
    , defaultWrapping_{config.wrapping.value_or(rhs.getWrapping())}
    , histograms_{} {}

Volume* Volume::clone() const { return new Volume(*this); }
Volume::~Volume() = default;

void Volume::setDimensions(const size3_t& dim) {
    defaultDimensions_ = dim;
    setLastAndInvalidateOther(&VolumeRepresentation::setDimensions, dim);
}

size3_t Volume::getDimensions() const {
    return getLastOr(&VolumeRepresentation::getDimensions, defaultDimensions_);
}

void Volume::setDataFormat(const DataFormatBase* format) { defaultDataFormat_ = format; }

const DataFormatBase* Volume::getDataFormat() const {
    return getLastOr(&VolumeRepresentation::getDataFormat, defaultDataFormat_);
}

void Volume::setSwizzleMask(const SwizzleMask& mask) {
    defaultSwizzleMask_ = mask;
    setLastAndInvalidateOther(&VolumeRepresentation::setSwizzleMask, mask);
}

SwizzleMask Volume::getSwizzleMask() const {
    return getLastOr(&VolumeRepresentation::getSwizzleMask, defaultSwizzleMask_);
}

void Volume::setInterpolation(InterpolationType interpolation) {
    defaultInterpolation_ = interpolation;
    setLastAndInvalidateOther(&VolumeRepresentation::setInterpolation, interpolation);
}

InterpolationType Volume::getInterpolation() const {
    return getLastOr(&VolumeRepresentation::getInterpolation, defaultInterpolation_);
}

void Volume::setWrapping(const Wrapping3D& wrapping) {
    defaultWrapping_ = wrapping;
    setLastAndInvalidateOther(&VolumeRepresentation::setWrapping, wrapping);
}

Wrapping3D Volume::getWrapping() const {
    return getLastOr(&VolumeRepresentation::getWrapping, defaultWrapping_);
}

Document Volume::getInfo() const {
    using P = Document::PathComponent;
    using H = utildoc::TableBuilder::Header;
    Document doc;
    doc.append("b", "Volume", {{"style", "color:white;"}});
    utildoc::TableBuilder tb(doc.handle(), P::end());

    tb(H("Format"), getDataFormat()->getString());
    tb(H("Dimension"), getDimensions());
    tb(H("SwizzleMask"), getSwizzleMask());
    tb(H("Interpolation"), getInterpolation());
    tb(H("Wrapping"), getWrapping());
    tb(H("Data Range"), dataMap.dataRange);
    tb(H("Value Range"), dataMap.valueRange);
    tb(H("Value"), fmt::format("{}{: [}", dataMap.valueAxis.name, dataMap.valueAxis.unit));
    tb(H("Axis 1"), fmt::format("{}{: [}", axes[0].name, axes[0].unit));
    tb(H("Axis 2"), fmt::format("{}{: [}", axes[1].name, axes[1].unit));
    tb(H("Axis 3"), fmt::format("{}{: [}", axes[2].name, axes[2].unit));

    tb(H("Basis"), getBasis());
    tb(H("Offset"), getOffset());

    histograms_.forEach([&](const Histogram1D& histogram, size_t channel) {
        tb(H("Stats"), fmt::format("Channel {} Min: {}, Mean: {}, Max: {}, Std: {}", channel,
                                   histogram.dataStats.min, histogram.dataStats.mean,
                                   histogram.dataStats.max, histogram.dataStats.standardDeviation));
        tb(H("Percentiles"),
           fmt::format("(1: {}, 25: {}, 50: {}, 75: {}, 99: {})",
                       histogram.dataStats.percentiles[1], histogram.dataStats.percentiles[25],
                       histogram.dataStats.percentiles[50], histogram.dataStats.percentiles[75],
                       histogram.dataStats.percentiles[99]));
    });
    return doc;
}

vec3 Volume::getWorldSpaceGradientSpacing() const {
    const auto textureToWorld = mat3(getCoordinateTransformer().getTextureToWorldMatrix());
    // Basis vectors with a length of one voxel.
    // Basis vectors may be non-orthogonal
    const auto dimensions = getDimensions();
    const vec3 a = textureToWorld[0] / static_cast<float>(dimensions[0]);
    const vec3 b = textureToWorld[1] / static_cast<float>(dimensions[1]);
    const vec3 c = textureToWorld[2] / static_cast<float>(dimensions[2]);
    // Project the voxel basis vectors
    // onto the world space x/y/z axes,
    // and choose the longest projected vector
    // for each axis.
    // Using the fact that
    // vec3 x{ 1.f, 0, 0 };
    // vec3 y{ 0, 1.f, 0 };
    // vec3 z{ 0, 0, 1.f };
    // such that
    // ax' = dot(x, a) = a.x
    // bx' = dot(x, b) = b.x
    // cx' = dot(x, c) = c.x
    // and so on.
    auto signedMax = [](const float& x1, const float& x2) {
        return (std::abs(x1) >= std::abs(x2)) ? x1 : x2;
    };

    const vec3 ds{signedMax(a.x, signedMax(b.x, c.x)), signedMax(a.y, signedMax(b.y, c.y)),
                  signedMax(a.z, signedMax(b.z, c.z))};

    // Return the spacing in world space,
    // actually given by:
    // { gradientSpacing.x         0                     0
    //         0             gradientSpacing.y           0
    //         0                   0               gradientSpacing.z }
    return ds;
}

const Axis* Volume::getAxis(size_t index) const {
    if (index >= 3) {
        return nullptr;
    }
    return &axes[index];
}

VolumeConfig Volume::config() const {
    return {.dimensions = getDimensions(),
            .format = getDataFormat(),
            .swizzleMask = getSwizzleMask(),
            .interpolation = getInterpolation(),
            .wrapping = getWrapping(),
            .xAxis = axes[0],
            .yAxis = axes[1],
            .zAxis = axes[2],
            .valueAxis = dataMap.valueAxis,
            .dataRange = dataMap.dataRange,
            .valueRange = dataMap.valueRange,
            .model = getModelMatrix(),
            .world = getWorldMatrix()};
}

namespace {

auto histCalc(const Volume& v) {
    return [dataMap = v.dataMap, repr = v.getRepresentationShared<VolumeRAM>()]() {
        return repr->dispatch<std::vector<Histogram1D>>(
            [dataMap]<typename T>(const VolumeRAMPrecision<T>* rp) {
                return util::calculateHistograms(rp->getView(), dataMap, 2048);
            });
    };
}

}  // namespace

void Volume::discardHistograms() { histograms_.discard(histCalc(*this)); }

HistogramCache::Result Volume::calculateHistograms(
    const std::function<void(const std::vector<Histogram1D>&)>& whenDone) const {
    return histograms_.calculateHistograms(histCalc(*this), whenDone);
}

template class IVW_CORE_TMPL_INST DataReaderType<Volume>;
template class IVW_CORE_TMPL_INST DataWriterType<Volume>;
template class IVW_CORE_TMPL_INST DataReaderType<VolumeSequence>;

template class IVW_CORE_TMPL_INST DataInport<Volume>;
template class IVW_CORE_TMPL_INST DataInport<Volume, 0, false>;
template class IVW_CORE_TMPL_INST DataInport<Volume, 0, true>;
template class IVW_CORE_TMPL_INST DataInport<DataSequence<Volume>>;
template class IVW_CORE_TMPL_INST DataInport<DataSequence<Volume>, 0, false>;
template class IVW_CORE_TMPL_INST DataInport<DataSequence<Volume>, 0, true>;
template class IVW_CORE_TMPL_INST DataOutport<Volume>;
template class IVW_CORE_TMPL_INST DataOutport<DataSequence<Volume>>;
}  // namespace inviwo
