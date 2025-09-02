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

#include <inviwo/core/util/dispatcher.h>

#include <functional>
#include <algorithm>

namespace inviwo {

using BaseCallBack = std::function<void()>;
/**
 * Example usage
 * CallBackList list;
 * list.addMemberFunction(&myClassObject, &MYClassObject::myFunction);
 * or
 * list.addLambdaCallback([](){});
 *
 * Copy or assign will clear any callback
 * Move or move assign will move the callback
 */
class CallBackList {
public:
    CallBackList() = default;
    CallBackList(const CallBackList& rhs) : callBackList_{}, dispatcher_{rhs.dispatcher_} {};
    CallBackList(CallBackList&&) = default;
    CallBackList& operator=(const CallBackList& that) {
        if (this != &that) {
            callBackList_.clear();
            dispatcher_ = that.dispatcher_;
        }
        return *this;
    };
    CallBackList& operator=(CallBackList&) = default;
    ~CallBackList() = default;

    void startBlockingCallbacks() { ++callbacksBlocked_; }
    void stopBlockingCallbacks() { --callbacksBlocked_; }

    void invokeAll() const {
        if (callbacksBlocked_ == 0) dispatcher_.invoke();
    }

    const BaseCallBack* addLambdaCallback(std::function<void()> lambda) {
        auto cb = dispatcher_.add(std::move(lambda));
        callBackList_.push_back(cb);
        return callBackList_.back().get();
    }
    std::shared_ptr<std::function<void()>> addLambdaCallbackRaii(std::function<void()> lambda) {
        return dispatcher_.add(std::move(lambda));
    }

    /**
     * \brief Removes callback if the callback was added before.
     * @param callback Callback to be removed.
     * @return bool True if removed, false otherwise.
     */
    bool remove(const BaseCallBack* callback) {
        auto it = std::remove_if(callBackList_.begin(), callBackList_.end(),
                                 [&](const std::shared_ptr<std::function<void()>>& ptr) {
                                     return ptr.get() == callback;
                                 });
        auto nelem = std::distance(it, callBackList_.end());
        callBackList_.erase(it, callBackList_.end());
        return nelem > 0;
    }

    /**
     * \brief Removes all added callbacks.
     */
    void clear() { callBackList_.clear(); }

private:
    int callbacksBlocked_{0};
    std::vector<std::shared_ptr<std::function<void()>>> callBackList_;
    mutable Dispatcher<void()> dispatcher_;
};

}  // namespace inviwo
