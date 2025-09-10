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

#include <inviwo/core/util/dialogfactory.h>
#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/datastructures/unitsystem.h>
#include <inviwo/core/io/rawvolumereader.h>
#include <inviwo/core/util/filesystem.h>
#include <inviwo/core/util/formatconversion.h>
#include <inviwo/core/io/datareaderexception.h>
#include <inviwo/core/io/rawvolumeramloader.h>
#include <inviwo/core/metadata/metadataowner.h>

#include <fmt/format.h>

namespace inviwo {

RawVolumeReader::RawVolumeReader()
    : DataReaderType<Volume>()
    , rawFile_("")
    , byteOrder_(ByteOrder::LittleEndian)
    , dimensions_(0)
    , spacing_(0.01f)
    , format_(nullptr)
    , byteOffset_(0u)
    , parametersSet_(false)
    , compression_{Compression::Disabled} {
    addExtension(FileExtension("raw", "Raw binary file"));
}

RawVolumeReader::RawVolumeReader(const RawVolumeReader& rhs)
    : DataReaderType<Volume>(rhs)
    , rawFile_(rhs.rawFile_)
    , byteOrder_(rhs.byteOrder_)
    , dimensions_(rhs.dimensions_)
    , spacing_(rhs.spacing_)
    , format_(rhs.format_)
    , byteOffset_(rhs.byteOffset_)
    , parametersSet_(false)
    , compression_{rhs.compression_} {}

RawVolumeReader& RawVolumeReader::operator=(const RawVolumeReader& that) {
    if (this != &that) {
        rawFile_ = that.rawFile_;
        byteOrder_ = that.byteOrder_;
        dimensions_ = that.dimensions_;
        spacing_ = that.spacing_;
        format_ = that.format_;
        dataMapper_ = that.dataMapper_;
        byteOffset_ = that.byteOffset_;
        compression_ = that.compression_;
        DataReaderType<Volume>::operator=(that);
    }

    return *this;
}

RawVolumeReader* RawVolumeReader::clone() const { return new RawVolumeReader(*this); }

std::shared_ptr<Volume> RawVolumeReader::readData(const std::filesystem::path& filePath) {
    return readData(filePath, nullptr);
}

std::shared_ptr<Volume> RawVolumeReader::readData(const std::filesystem::path& path,
                                                  MetaDataOwner* metadata) {
    const auto filePath = downloadAndCacheIfUrl(path);

    checkExists(filePath);

    rawFile_ = filePath;

    if (!parametersSet_) {
        auto readerDialog = util::dynamic_unique_ptr_cast<VolumeDataReaderDialog>(
            InviwoApplication::getPtr()->getDialogFactory()->create("RawVolumeReader"));
        if (!readerDialog) {
            throw DataReaderException("No data reader dialog found.");
        }
        readerDialog->setFile(rawFile_);

        if (metadata) {
            readerDialog->setFormat(
                DataFormatBase::get(static_cast<DataFormatId>(metadata->getMetaData<IntMetaData>(
                    "rawReaderData.formatid",
                    static_cast<int>(readerDialog->getFormat()->getId())))));
            readerDialog->setDimensions(metadata->getMetaData<Size3MetaData>(
                "rawReaderData.dimensions", readerDialog->getDimensions()));

            readerDialog->setByteOrder(
                static_cast<ByteOrder>(metadata->getMetaData<IntMetaData>(
                    "rawReaderData.byteOrder", static_cast<int>(readerDialog->getByteOrder()))));
            readerDialog->setCompression(
                static_cast<Compression>(metadata->getMetaData<IntMetaData>(
                    "rawReaderData.compression",
                    static_cast<int>(readerDialog->getCompression()))));

            auto datamap = readerDialog->getDataMapper();
            datamap.dataRange = metadata->getMetaData<DoubleVec2MetaData>(
                "rawReaderData.dataMapper.dataRange", datamap.dataRange);
            datamap.valueRange = metadata->getMetaData<DoubleVec2MetaData>(
                "rawReaderData.dataMapper.valueRange", datamap.valueRange);

            auto unit = units::to_string(datamap.valueAxis.unit);
            unit = metadata->getMetaData<StringMetaData>("rawReaderData.dataMapper.valueAxis.unit",
                                                         unit);
            datamap.valueAxis.unit = units::unit_from_string(unit);

            readerDialog->setDataMapper(datamap);

            readerDialog->setByteOffset(metadata->getMetaData<SizeMetaData>(
                "rawReaderData.byteOffset", readerDialog->getByteOffset()));
        }

        if (readerDialog->show()) {
            format_ = readerDialog->getFormat();
            dimensions_ = readerDialog->getDimensions();
            byteOrder_ = readerDialog->getByteOrder();
            spacing_ = static_cast<glm::vec3>(readerDialog->getSpacing());
            dataMapper_ = readerDialog->getDataMapper();
            byteOffset_ = readerDialog->getByteOffset();
            compression_ = readerDialog->getCompression();

            if (metadata) {
                metadata->setMetaData<IntMetaData>("rawReaderData.formatid",
                                                   static_cast<int>(format_->getId()));
                metadata->setMetaData<Size3MetaData>("rawReaderData.dimensions", dimensions_);
                metadata->setMetaData<IntMetaData>("rawReaderData.byteOrder",
                                                   static_cast<int>(byteOrder_));
                metadata->setMetaData<DoubleVec2MetaData>("rawReaderData.dataMapper.dataRange",
                                                          dataMapper_.dataRange);
                metadata->setMetaData<DoubleVec2MetaData>("rawReaderData.dataMapper.valueRange",
                                                          dataMapper_.valueRange);
                metadata->setMetaData<StringMetaData>("rawReaderData.dataMapper.valueAxis.unit",
                                                      units::to_string(dataMapper_.valueAxis.unit));

                metadata->setMetaData<SizeMetaData>("rawReaderData.byteOffset", byteOffset_);
                metadata->setMetaData<IntMetaData>("rawReaderData.compression",
                                                   static_cast<int>(compression_));
            }

        } else {
            throw DataReaderException("Raw data import terminated by user");
        }
    }

    if (format_) {
        glm::mat3 basis(1.0f);
        glm::mat4 wtm(1.0f);

        basis[0][0] = dimensions_.x * spacing_.x;
        basis[1][1] = dimensions_.y * spacing_.y;
        basis[2][2] = dimensions_.z * spacing_.z;

        // Center the data around origo.
        glm::vec3 offset(-0.5f * (basis[0] + basis[1] + basis[2]));

        auto volume = std::make_shared<Volume>(dimensions_, format_);
        volume->setBasis(basis);
        volume->setOffset(offset);
        volume->setWorldMatrix(wtm);
        auto vd = std::make_shared<VolumeDisk>(filePath, dimensions_, format_);
        auto loader =
            std::make_unique<RawVolumeRAMLoader>(rawFile_, byteOffset_, byteOrder_, compression_);
        vd->setLoader(loader.release());
        volume->addRepresentation(vd);

        volume->dataMap = dataMapper_;
        const auto size = util::formatBytesToString(dimensions_.x * dimensions_.y * dimensions_.z *
                                                    format_->getSizeInBytes());
        log::info("Loaded volume: {:?g}  size: {}", filePath, size);
        return volume;
    } else {
        throw DataReaderException("Raw data import could not determine format");
    }
}

}  // namespace inviwo
