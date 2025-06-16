/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2018-2025 Inviwo Foundation
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
#include <inviwo/core/io/serialization/serialization.h>
#include <inviwo/core/util/glm.h>
#include <inviwo/core/util/observer.h>

#include <span>

namespace inviwo {

class TFPrimitive;

class IVW_CORE_API TFPrimitiveObserver : public Observer {
public:
    virtual void onTFPrimitiveChange(const TFPrimitive& p);
};

struct IVW_CORE_API TFPrimitiveData {
    double pos;
    vec4 color;

    constexpr auto operator<=>(const TFPrimitiveData& other) const {
        return std::tie(pos, color.a) <=> std::tie(other.pos, other.color.a);
    }
    constexpr bool operator==(const TFPrimitiveData& other) const = default;
};

/**
 * \class TFPrimitive
 * \brief Base class for a variety of primitives as used by a transfer function
 */
class IVW_CORE_API TFPrimitive : public Observable<TFPrimitiveObserver>, public Serializable {
public:
    explicit TFPrimitive(double pos = 0.0, const vec4& color = vec4(0.0f));
    // Cannot use default constructors and assignment operator for TFPrimitive!
    //
    // Default constructors would call the base class constructor of Observable and thereby
    // copy all observers. This must be avoided since TFPrimitives are a part of a property
    // and when setting/assigning a property, no observers must be copied!
    explicit TFPrimitive(const TFPrimitiveData& data);
    TFPrimitive(const TFPrimitive& rhs);
    TFPrimitive(TFPrimitive&& rhs) noexcept;
    TFPrimitive& operator=(const TFPrimitive& that);
    TFPrimitive& operator=(TFPrimitive&& that) noexcept;

    TFPrimitive& operator=(const TFPrimitiveData& that);

    virtual ~TFPrimitive() = default;

    void setData(const TFPrimitiveData& data);
    inline const TFPrimitiveData& getData() const;

    void setPosition(double pos);
    inline double getPosition() const;

    void setAlpha(float alpha);
    inline float getAlpha() const;

    void setPositionAlpha(double pos, float alpha);
    void setPositionAlpha(const dvec2& p);

    void setColor(const vec3& color);
    void setColor(const vec4& color);
    inline const vec4& getColor() const;

    void notifyTFPrimitiveObservers();

    virtual void serialize(Serializer& s) const override;
    virtual void deserialize(Deserializer& d) override;

    constexpr auto operator<=>(const TFPrimitive& other) const { return data_ <=> other.data_; }
    constexpr bool operator==(const TFPrimitive& other) const { return data_ == other.data_; }

private:
    TFPrimitiveData data_;
};

inline const TFPrimitiveData& TFPrimitive::getData() const { return data_; }
inline double TFPrimitive::getPosition() const { return data_.pos; }
inline float TFPrimitive::getAlpha() const { return data_.color.a; }
inline const vec4& TFPrimitive::getColor() const { return data_.color; }

namespace util {
IVW_CORE_API vec4 interpolateColor(const TFPrimitiveData& p1, const TFPrimitiveData& p2, double x);
IVW_CORE_API vec4 interpolateColor(std::span<const TFPrimitiveData> line, double x);

};  // namespace util

}  // namespace inviwo
