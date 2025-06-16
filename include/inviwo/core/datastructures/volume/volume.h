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

#pragma once

#include <inviwo/core/common/inviwocoredefine.h>
#include <inviwo/core/datastructures/data.h>
#include <inviwo/core/datastructures/spatialdata.h>
#include <inviwo/core/datastructures/histogramtools.h>
#include <inviwo/core/datastructures/image/imagetypes.h>
#include <inviwo/core/datastructures/volume/volumeconfig.h>
#include <inviwo/core/datastructures/datamapper.h>
#include <inviwo/core/datastructures/representationtraits.h>
#include <inviwo/core/datastructures/datasequence.h>
#include <inviwo/core/datastructures/volume/volumerepresentation.h>
#include <inviwo/core/datastructures/unitsystem.h>
#include <inviwo/core/metadata/metadataowner.h>
#include <inviwo/core/util/glmvec.h>
#include <inviwo/core/util/document.h>
#include <inviwo/core/io/datareader.h>
#include <inviwo/core/io/datawriter.h>
#include <inviwo/core/ports/datainport.h>
#include <inviwo/core/ports/dataoutport.h>

namespace inviwo {

class Camera;

/**
 * \ingroup datastructures
 *
 * \class Volume
 * Data structure for volumetric data in form of a structured three-dimensional grid. Basis and
 * offset determine the position and extent of the volume in model space. Skewed volumes are
 * represented by providing a non-orthogonal basis.
 *
 * In case a volume was loaded via VolumeSource or Volume, the filename of the source data is
 * available via MetaData.
 */
class IVW_CORE_API Volume : public Data<Volume, VolumeRepresentation>,
                            public StructuredGridEntity<3>,
                            public MetaDataOwner {
public:
    using Config = VolumeConfig;
    explicit Volume(size3_t defaultDimensions = VolumeConfig::defaultDimensions,
                    const DataFormatBase* defaultFormat = VolumeConfig::defaultFormat,
                    const SwizzleMask& defaultSwizzleMask = VolumeConfig::defaultSwizzleMask,
                    InterpolationType interpolation = VolumeConfig::defaultInterpolation,
                    const Wrapping3D& wrapping = VolumeConfig::defaultWrapping);
    explicit Volume(const VolumeConfig& config);
    explicit Volume(std::shared_ptr<VolumeRepresentation>);

    /**
     * Create a volume based on @p rhs without copying any data. State from @p rhs can be
     * overridden by the @p config
     * @param rhs             source volume providing the necessary information like dimensions,
     *                        swizzle masks, interpolation, spatial transformations, etc.
     * @param noData          Tag type to indicate that representations should not be copied from
     *                        rhs
     * @param config          custom parameters overriding values from @p rhs
     */
    Volume(const Volume& rhs, NoData noData, const VolumeConfig& config = {});

    Volume(const Volume&) = default;
    Volume(Volume&&) = default;
    Volume& operator=(const Volume& that) = default;
    Volume& operator=(Volume&& that) = default;
    virtual ~Volume();
    virtual Volume* clone() const override;

    Document getInfo() const;

    /**
     * Resize to dimension. This is destructive, the data will not be
     * preserved.
     * @note Resizes the last valid representation and erases all representations.
     * Last valid representation will remain valid after changing the dimension.
     */
    virtual void setDimensions(const size3_t& dim);
    virtual size3_t getDimensions() const override;

    /**
     * Set the default data format. Existing representations will not be affected.
     * @note Only useful before any representations have been created.
     * @see DataFormatBase
     * @param format The format of the data.
     */
    void setDataFormat(const DataFormatBase* format);
    const DataFormatBase* getDataFormat() const;

    /**
     * \brief update the swizzle mask of the color channels when sampling the volume
     *
     * @param mask new swizzle mask
     */
    void setSwizzleMask(const SwizzleMask& mask);
    SwizzleMask getSwizzleMask() const;

    void setInterpolation(InterpolationType interpolation);
    InterpolationType getInterpolation() const;

    void setWrapping(const Wrapping3D& wrapping);
    Wrapping3D getWrapping() const;

    /**
     * \brief Computes the spacing to be used for gradient computation. Also works for volume with
     * non-orthogonal basis.
     *
     * For orthogonal lattices this will be equal to the world space voxel spacing.
     * For non-orthogonal lattices it will be the longest of the axes projected
     * onto the world space axes.
     *
     *        World space
     *
     *         b ^           ^
     *          /            |
     * y ^     /             dy
     *   |    /  Voxel       |
     *   |   /__________>a   \/
     *   |   <----dx--->
     *   |____________> x
     *
     *
     * The actual gradient spacing vectors are given by
     * mat3{ gradientSpacing.x,        0,                    0,
     *             0,            gradientSpacing.y,          0,
     *             0,                  0,              gradientSpacing.z }
     * However, we do not return the zeroes.
     *
     * To get the spacing in texture space use:
     * mat3(glm::scale(worldToTextureMatrix, getWorldSpaceGradientSpacing()));
     * @return Step size for gradient computation in world space.
     */
    vec3 getWorldSpaceGradientSpacing() const;

    /**
     * @copydoc SpatialEntity::getAxis
     */
    virtual const Axis* getAxis(size_t index) const override;

    DataMapper dataMap;
    std::array<Axis, 3> axes;

    static constexpr uvec3 colorCode{188, 101, 101};
    static constexpr std::string_view classIdentifier{"org.inviwo.Volume"};
    static constexpr std::string_view dataName{"Volume"};

    template <typename Kind>
    const typename representation_traits<Volume, Kind>::type* getRep() const;

    [[nodiscard]] HistogramCache::Result calculateHistograms(
        const std::function<void(const std::vector<Histogram1D>&)>& whenDone) const;
    void discardHistograms();

    VolumeConfig config() const;

protected:
    size3_t defaultDimensions_;
    const DataFormatBase* defaultDataFormat_;
    SwizzleMask defaultSwizzleMask_;
    InterpolationType defaultInterpolation_;
    Wrapping3D defaultWrapping_;
    HistogramCache histograms_;
};

template <typename Kind>
const typename representation_traits<Volume, Kind>::type* Volume::getRep() const {
    static_assert(
        !std::is_same<typename representation_traits<Volume, Kind>::type, std::nullptr_t>::value,
        "No representation of specified kind found");
    return getRepresentation<typename representation_traits<Volume, Kind>::type>();
}

using VolumeSequence = DataSequence<Volume>;

extern template class IVW_CORE_TMPL_EXP DataReaderType<Volume>;
extern template class IVW_CORE_TMPL_EXP DataWriterType<Volume>;
extern template class IVW_CORE_TMPL_EXP DataReaderType<VolumeSequence>;

extern template class IVW_CORE_TMPL_EXP DataInport<Volume>;
extern template class IVW_CORE_TMPL_EXP DataInport<Volume, 0, false>;
extern template class IVW_CORE_TMPL_EXP DataInport<Volume, 0, true>;
extern template class IVW_CORE_TMPL_EXP DataInport<DataSequence<Volume>>;
extern template class IVW_CORE_TMPL_EXP DataInport<DataSequence<Volume>, 0, false>;
extern template class IVW_CORE_TMPL_EXP DataInport<DataSequence<Volume>, 0, true>;
extern template class IVW_CORE_TMPL_EXP DataOutport<Volume>;
extern template class IVW_CORE_TMPL_EXP DataOutport<DataSequence<Volume>>;

}  // namespace inviwo
