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

#pragma once

#include <modules/basegl/baseglmoduledefine.h>  // for IVW_MODULE_BASEGL...

#include <inviwo/core/datastructures/camera/camera.h>                   // for mat4
#include <inviwo/core/datastructures/geometry/typedmesh.h>              // for TypedMesh, Positi...
#include <inviwo/core/datastructures/representationconverter.h>         // for RepresentationCon...
#include <inviwo/core/datastructures/representationconverterfactory.h>  // for RepresentationCon...
#include <inviwo/core/ports/imageport.h>                                // for ImageOutport
#include <inviwo/core/ports/volumeport.h>                               // for VolumeInport
#include <inviwo/core/processors/processor.h>                           // for Processor
#include <inviwo/core/processors/processorinfo.h>                       // for ProcessorInfo
#include <inviwo/core/properties/boolcompositeproperty.h>               // for BoolCompositeProp...
#include <inviwo/core/properties/boolproperty.h>                        // for BoolProperty
#include <inviwo/core/properties/compositeproperty.h>                   // for CompositeProperty
#include <inviwo/core/properties/eventproperty.h>                       // for EventProperty
#include <inviwo/core/properties/optionproperty.h>                      // for OptionPropertyInt
#include <inviwo/core/properties/ordinalproperty.h>                     // for FloatProperty
#include <inviwo/core/properties/transferfunctionproperty.h>            // for TransferFunctionP...
#include <inviwo/core/util/glmvec.h>                                    // for vec2, size3_t, vec3
#include <modules/opengl/shader/shader.h>                               // for Shader

#include <memory>         // for unique_ptr
#include <unordered_map>  // for unordered_map
#include <unordered_set>  // for unordered_set

namespace inviwo {
class Deserializer;
class Event;

class IVW_MODULE_BASEGL_API VolumeSliceGL : public Processor {
public:
    VolumeSliceGL();
    virtual ~VolumeSliceGL();

    virtual const ProcessorInfo& getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

    virtual void initializeResources() override;

    // Overridden to be able to turn off interaction events and detect resize events.
    virtual void invokeEvent(Event*) override;

    bool positionModeEnabled() const { return posPicking_.get(); }

    // override to do member renaming.
    virtual void deserialize(Deserializer& d) override;

protected:
    using ColoredMesh2D = TypedMesh<buffertraits::PositionsBuffer2D, buffertraits::ColorsBuffer>;

    virtual void process() override;

    void shiftSlice(int);

    void modeChange();
    void planeSettingsChanged();
    void updateMaxSliceNumber();

    void renderPositionIndicator();
    // Create a lines and crosshair -  use together with updateIndicatorMesh
    static ColoredMesh2D createIndicatorMesh();
    void updateIndicatorMesh();

    // updates the selected position, pos is given in normalized viewport coordinates, i.e. [0,1]
    void setVolPosFromScreenPos(vec2 pos);
    vec2 getScreenPosFromVolPos();

    vec3 convertScreenPosToVolume(const vec2& screenPos, bool clamp = true) const;

    void invalidateMesh();

    void sliceChange();
    void positionChange();
    void rotationModeChange();

private:
    void eventShiftSlice(Event*);
    void eventSetMarker(Event*);
    void eventStepSliceUp(Event*);
    void eventStepSliceDown(Event*);
    void eventGestureShiftSlice(Event*);
    void eventUpdateMousePos(Event*);

    void updateFromWorldPosition();

    VolumeInport inport_;
    ImageOutport outport_;
    Shader shader_;
    Shader indicatorShader_;

    CompositeProperty trafoGroup_;
    CompositeProperty pickGroup_;
    BoolCompositeProperty tfGroup_;

    OptionPropertyInt sliceAlongAxis_;
    IntProperty sliceX_;
    IntProperty sliceY_;
    IntProperty sliceZ_;

    FloatVec3Property worldPosition_;

    FloatVec3Property planeNormal_;
    FloatVec3Property planePosition_;
    FloatProperty imageScale_;
    OptionPropertyInt rotationAroundAxis_;  // Clockwise rotation around slice axis
    FloatProperty imageRotation_;
    BoolProperty flipHorizontal_;
    BoolProperty flipVertical_;
    OptionPropertyInt volumeWrapping_;
    FloatVec4Property fillColor_;

    BoolProperty posPicking_;
    BoolProperty showIndicator_;
    FloatVec4Property indicatorColor_;
    FloatProperty indicatorSize_;

    OptionPropertyInt channel_;
    TransferFunctionProperty transferFunction_;
    FloatProperty tfAlphaOffset_;

    BoolCompositeProperty sampleQuery_;
    FloatVec4Property normalizedSample_;
    FloatVec4Property volumeSample_;

    BoolProperty handleInteractionEvents_;

    EventProperty mouseShiftSlice_;
    EventProperty mouseSetMarker_;
    EventProperty mousePositionTracker_;

    EventProperty stepSliceUp_;
    EventProperty stepSliceDown_;

    EventProperty gestureShiftSlice_;

    ColoredMesh2D meshCrossHair_ = createIndicatorMesh();

    bool meshDirty_;
    bool updating_;

    mat4 sliceRotation_;
    mat4 inverseSliceRotation_;  // Used to calculate the slice "z position" from the plain point.
    size3_t volumeDimensions_;
    mat4 texToWorld_;
};
}  // namespace inviwo
