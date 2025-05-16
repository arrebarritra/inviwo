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

#include <modules/base/basemoduledefine.h>  // for IVW_MODULE_BASE_API

#include <inviwo/core/datastructures/light/baselightsource.h>  // for LightSource
#include <inviwo/core/interaction/interactionhandler.h>        // for InteractionHandler
#include <inviwo/core/interaction/trackball.h>                 // for Trackball
#include <inviwo/core/interaction/trackballobject.h>           // for TrackballObject, Trackball...
#include <inviwo/core/ports/dataoutport.h>                     // for DataOutport
#include <inviwo/core/processors/processor.h>                  // for Processor
#include <inviwo/core/processors/processorinfo.h>              // for ProcessorInfo
#include <inviwo/core/properties/boolproperty.h>               // for BoolProperty
#include <inviwo/core/properties/cameraproperty.h>             // for CameraProperty
#include <inviwo/core/properties/compositeproperty.h>          // for CompositeProperty
#include <inviwo/core/properties/optionproperty.h>             // for OptionPropertyInt
#include <inviwo/core/properties/ordinalproperty.h>            // for FloatVec2Property, FloatPr...
#include <inviwo/core/properties/positionproperty.h>           // for PositionProperty
#include <inviwo/core/util/glmvec.h>                           // for vec3, vec2

#include <memory>  // for shared_ptr
#include <string>  // for string, operator+

#include <fmt/core.h>  // for format

namespace inviwo {

class Camera;
class Deserializer;
class Event;
class PointLight;
class PointLightInteractionHandler;
class Serializer;

class IVW_MODULE_BASE_API PointLightTrackball : public Trackball {
public:
    virtual std::string_view getClassIdentifier() const override;
    static constexpr std::string_view classIdentifier{"org.inviwo.PointLightTrackball"};

    explicit PointLightTrackball(PointLightInteractionHandler* p);
    virtual ~PointLightTrackball() = default;
};

/*
 * Enables light source to be placed relative to camera using middle mouse button or pan gesture
 * with two fingers. Uses trackball interaction for all other types of interaction.
 */
class IVW_MODULE_BASE_API PointLightInteractionHandler : public InteractionHandler,
                                                         public TrackballObject {
public:
    PointLightInteractionHandler(PositionProperty*, CameraProperty*, BoolProperty*,
                                 FloatVec2Property*);
    virtual ~PointLightInteractionHandler();

    const Camera& getCamera();

    virtual void invokeEvent(Event* event) override;
    void setHandleEventsOptions(int);

    /**
     * \brief Changes the direction of the light source, relative to the camera,
     * such that it acts as if it comes from the direction where the user clicked on the screen.
     *
     * Intersects a sphere covering the scene and places the light source
     * in the direction of the intersection point but at the same distance from the origin as
     * before. If the intersection is outside the sphere the light source will be placed
     * perpendicular to the camera at the same distance as before.
     *
     *
     * @param normalizedScreenCoord Coordinates in [0 1], where y coordinate is 0 at top of
     * screen.
     */
    void setLightPosFromScreenCoords(const vec2& normalizedScreenCoord);

    // Update up vector when camera changes
    void onCameraChanged();

    // Necessary for trackball
    [[nodiscard]] virtual vec3 getLookTo() const override;
    [[nodiscard]] virtual vec3 getLookFrom() const override;
    [[nodiscard]] virtual vec3 getLookUp() const override;

    virtual TrackballObject& setLookTo(vec3 lookTo) override;
    virtual TrackballObject& setLookFrom(vec3 lookFrom) override;
    virtual TrackballObject& setLookUp(vec3 lookUp) override;

    [[nodiscard]] virtual vec3 getLookFromMinValue() const override;
    [[nodiscard]] virtual vec3 getLookFromMaxValue() const override;

    [[nodiscard]] virtual vec3 getLookToMinValue() const override;
    [[nodiscard]] virtual vec3 getLookToMaxValue() const override;

    virtual TrackballObject& setLook(vec3 lookFrom, vec3 lookTo, vec3 lookUp) override;

    [[nodiscard]] virtual float getNearPlaneDist() const override;
    [[nodiscard]] virtual float getFarPlaneDist() const override;

    virtual void zoom(const ZoomOptions& opts) override;

    [[nodiscard]] virtual vec3 getWorldPosFromNormalizedDeviceCoords(
        const vec3& ndcCoords) const override;
    [[nodiscard]] virtual vec3 getNormalizedDeviceFromNormalizedScreenAtFocusPointDepth(
        const vec2& normalizedScreenCoord) const override;

private:
    PositionProperty* lightPosition_;
    CameraProperty* camera_;
    BoolProperty* screenPosEnabled_;
    FloatVec2Property* screenPos_;
    vec3 lookUp_;  ///< Necessary for trackball
    vec3 lookTo_;  ///< Necessary for trackball
    PointLightTrackball trackball_;
    int interactionEventOption_;
};

class IVW_MODULE_BASE_API PointLightSourceProcessor : public Processor {
public:
    PointLightSourceProcessor();
    virtual ~PointLightSourceProcessor();

    virtual const ProcessorInfo& getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

protected:
    virtual void process() override;

    /**
     * Update light source parameters. Transformation will be given in texture space.
     * @param lightSource
     */
    void updatePointLightSource(PointLight* lightSource);

private:
    DataOutport<LightSource> outport_;

    CameraProperty camera_;
    PositionProperty lightPosition_;
    CompositeProperty lighting_;
    FloatProperty lightPowerProp_;
    FloatProperty lightSize_;
    FloatVec3Property lightDiffuse_;
    BoolProperty lightEnabled_;
    BoolProperty lightScreenPosEnabled_;
    FloatVec2Property lightScreenPos_;

    OptionPropertyInt interactionEvents_;
    PointLightInteractionHandler lightInteractionHandler_;
    std::shared_ptr<PointLight> lightSource_;
};

}  // namespace inviwo
