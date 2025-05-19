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

#pragma once

#include <modules/hdf5/hdf5moduledefine.h>
#include <inviwo/core/processors/processor.h>
#include <modules/hdf5/ports/hdf5port.h>
#include <modules/hdf5/datastructures/hdf5metadata.h>
#include <modules/hdf5/hdf5utils.h>
#include <inviwo/core/datastructures/volume/volume.h>
#include <inviwo/core/ports/volumeport.h>
#include <inviwo/core/properties/minmaxproperty.h>
#include <inviwo/core/properties/optionproperty.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <inviwo/core/properties/boolproperty.h>
#include <inviwo/core/properties/buttonproperty.h>
#include <inviwo/core/properties/compositeproperty.h>
#include <inviwo/core/properties/stringproperty.h>

namespace inviwo {

namespace hdf5 {

class IVW_MODULE_HDF5_API HDF5ToVolume : public Processor {
public:
    HDF5ToVolume();
    virtual ~HDF5ToVolume();

    virtual const ProcessorInfo& getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

protected:
    virtual void process() override;

private:
    class DimSelection : public CompositeProperty {
    public:
        DimSelection(const std::string& identifier, const std::string& displayName,
                     InvalidationLevel = InvalidationLevel::InvalidOutput);

        DimSelection(const DimSelection& rhs) = default;
        virtual ~DimSelection() = default;

        IntMinMaxProperty range;
        IntProperty stride;

        void update(int newMax);
    };

    class DimSelections : public CompositeProperty {
    public:
        DimSelections(const std::string& identifier, const std::string& displayName, size_t maxRank,
                      InvalidationLevel = InvalidationLevel::InvalidOutput);

        DimSelections(const DimSelections& rhs) = default;
        virtual ~DimSelections() = default;

        std::vector<Handle::Selection> getSelection() const;
        std::vector<Handle::Selection> getMaxSelection() const;

        void update(const MetaData& meta);

        BoolProperty adjustBasis_;

    private:
        size_t maxRank_;
        size_t rank_;
        std::vector<std::unique_ptr<DimSelection>> selection_;
    };

    void makeVolume();
    void onDataChange();

    void onSelectionChange();
    void onBasisSelecionChange();
    mat4 getBasisFromMeta(MetaData);
    std::string getDescription(const MetaData& meta);

    std::vector<MetaData> volumeMatches_;
    std::vector<MetaData> basisMatches_;

    Inport inport_;
    VolumeOutport outport_;
    std::shared_ptr<Volume> volume_;

    OptionPropertyString volumeSelection_;

    BoolProperty automaticEvaluation_;
    ButtonProperty evaluate_;

    CompositeProperty basisGroup_;
    OptionPropertyString basisSelection_;
    FloatMat4Property basis_;
    FloatVec3Property spacing_;

    CompositeProperty information_;
    DoubleMinMaxProperty dataRange_;
    StringProperty dataDimensions_;

    CompositeProperty outputGroup_;
    OptionPropertyInt overrideRange_;
    DoubleMinMaxProperty outDataRange_;
    DoubleMinMaxProperty valueRange_;
    StringProperty valueUnit_;

    OptionPropertyInt datatype_;

    DimSelections selection_;

    bool dirty_;
};

}  // namespace hdf5

}  // namespace inviwo
