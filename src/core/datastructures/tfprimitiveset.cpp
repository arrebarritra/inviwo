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

#include <inviwo/core/datastructures/tfprimitiveset.h>

#include <inviwo/core/util/zip.h>
#include <inviwo/core/util/exception.h>
#include <inviwo/core/util/vectoroperations.h>

#include <algorithm>
#include <set>

namespace inviwo {

void TFPrimitiveSetObserver::onTFPrimitiveAdded(const TFPrimitiveSet&, TFPrimitive&) {}

void TFPrimitiveSetObserver::onTFPrimitiveRemoved(const TFPrimitiveSet&, TFPrimitive&) {}

void TFPrimitiveSetObserver::onTFPrimitiveChanged(const TFPrimitiveSet&, const TFPrimitive&) {}

void TFPrimitiveSetObserver::onTFTypeChanged(const TFPrimitiveSet&, TFPrimitiveSetType) {}

void TFPrimitiveSetObserver::onTFMaskChanged(const TFPrimitiveSet&, dvec2) {}

void TFPrimitiveSetObservable::notifyTFPrimitiveAdded(const TFPrimitiveSet& set, TFPrimitive& p) {
    forEachObserver([&](TFPrimitiveSetObserver* o) { o->onTFPrimitiveAdded(set, p); });
}

void TFPrimitiveSetObservable::notifyTFPrimitiveRemoved(const TFPrimitiveSet& set, TFPrimitive& p) {
    forEachObserver([&](TFPrimitiveSetObserver* o) { o->onTFPrimitiveRemoved(set, p); });
}

void TFPrimitiveSetObservable::notifyTFPrimitiveChanged(const TFPrimitiveSet& set,
                                                        const TFPrimitive& p) {
    forEachObserver([&](TFPrimitiveSetObserver* o) { o->onTFPrimitiveChanged(set, p); });
}

void TFPrimitiveSetObservable::notifyTFTypeChanged(const TFPrimitiveSet& set,
                                                   TFPrimitiveSetType type) {
    forEachObserver([&](TFPrimitiveSetObserver* o) { o->onTFTypeChanged(set, type); });
}

void TFPrimitiveSetObservable::notifyTFMaskChanged(const TFPrimitiveSet& set, dvec2 mask) {
    forEachObserver([&](TFPrimitiveSetObserver* o) { o->onTFMaskChanged(set, mask); });
}

TFPrimitiveSet::TFPrimitiveSet(const std::vector<TFPrimitiveData>& values, TFPrimitiveSetType type)
    : type_(type) {
    add(values);
}

TFPrimitiveSet::TFPrimitiveSet(const TFPrimitiveSet& rhs) : type_(rhs.type_) {
    for (const auto& v : rhs.values_) {
        add(*v);
    }
}

TFPrimitiveSet& TFPrimitiveSet::operator=(const TFPrimitiveSet& rhs) {
    if (this != &rhs) {
        type_ = rhs.type_;
        for (size_t i = 0; i < std::min(values_.size(), rhs.values_.size()); i++) {
            *values_[i] = *rhs.values_[i];
        }
        for (size_t i = std::min(values_.size(), rhs.values_.size()); i < rhs.values_.size(); i++) {
            add(*rhs.values_[i]);
        }
        while (values_.size() > rhs.values_.size()) {
            remove(--values_.end());
        }
    }
    return *this;
}

TFPrimitiveSet& TFPrimitiveSet::operator=(TFPrimitiveSet&& rhs) noexcept {
    if (this != &rhs) {
        type_ = rhs.type_;
        for (size_t i = 0; i < std::min(values_.size(), rhs.values_.size()); i++) {
            *values_[i] = *rhs.values_[i];
        }
        for (size_t i = std::min(values_.size(), rhs.values_.size()); i < rhs.values_.size(); i++) {
            add(*rhs.values_[i]);
        }
        while (values_.size() > rhs.values_.size()) {
            remove(--values_.end());
        }
    }
    return *this;
}
void TFPrimitiveSet::set(std::span<const TFPrimitiveData> points) {
    auto sbegin = points.begin();
    const auto send = points.end();

    auto dbegin = values_.begin();
    const auto dend = values_.end();

    while (dbegin != dend && sbegin != send) {
        verifyPoint(*sbegin);
        **dbegin++ = *sbegin++;
    }
    while (sbegin != send) {
        add(*sbegin++);
    }
    const size_t targetSize = points.size();
    while (values_.size() > targetSize) {
        remove(--values_.end());
    }
}

void TFPrimitiveSet::set(const_iterator sbegin, const_iterator send) {
    const size_t targetSize = std::distance(sbegin, send);

    auto dbegin = values_.begin();
    const auto dend = values_.end();

    while (dbegin != dend && sbegin != send) {
        verifyPoint(*sbegin);
        **dbegin++ = *sbegin++;
    }
    while (sbegin != send) {
        add(*sbegin++);
    }
    while (values_.size() > targetSize) {
        remove(--values_.end());
    }
}

void TFPrimitiveSet::setType(TFPrimitiveSetType type) {
    if (type_ != type) {
        type_ = type;
        notifyTFTypeChanged(*this, type_);
    }
}

dvec2 TFPrimitiveSet::getRange() const {
    switch (type_) {
        case TFPrimitiveSetType::Absolute: {
            if (sorted_.empty()) {
                return {0.0, 1.0};
            }
            return {sorted_.front()->getPosition(), sorted_.back()->getPosition()};
        }
        case TFPrimitiveSetType::Relative:
        default:
            return {0.0, 1.0};
    }
}

size_t TFPrimitiveSet::size() const { return values_.size(); }

bool TFPrimitiveSet::empty() const { return values_.empty(); }

TFPrimitive& TFPrimitiveSet::operator[](size_t i) { return *sorted_[i]; }

const TFPrimitive& TFPrimitiveSet::operator[](size_t i) const { return *sorted_[i]; }

auto TFPrimitiveSet::begin() -> iterator {
    return iterator{[](TFPrimitive* p) -> TFPrimitive& { return *p; }, sorted_.begin()};
}

auto TFPrimitiveSet::end() -> iterator {
    return iterator{[](TFPrimitive* p) -> TFPrimitive& { return *p; }, sorted_.end()};
}

auto TFPrimitiveSet::begin() const -> const_iterator {
    return const_iterator{[](const TFPrimitive* const p) -> const TFPrimitive& { return *p; },
                          sorted_.cbegin()};
}

auto TFPrimitiveSet::end() const -> const_iterator {
    return const_iterator{[](const TFPrimitive* const p) -> const TFPrimitive& { return *p; },
                          sorted_.cend()};
}

auto TFPrimitiveSet::cbegin() const -> const_iterator {
    return const_iterator{[](const TFPrimitive* const p) -> const TFPrimitive& { return *p; },
                          sorted_.cbegin()};
}

auto TFPrimitiveSet::cend() const -> const_iterator {
    return const_iterator{[](const TFPrimitive* const p) -> const TFPrimitive& { return *p; },
                          sorted_.cend()};
}

TFPrimitive& TFPrimitiveSet::get(size_t i) { return *sorted_[i]; }

const TFPrimitive& TFPrimitiveSet::get(size_t i) const { return *sorted_[i]; }

TFPrimitive& TFPrimitiveSet::front() { return *sorted_.front(); }

const TFPrimitive& TFPrimitiveSet::front() const { return *sorted_.front(); }

TFPrimitive& TFPrimitiveSet::back() { return *sorted_.back(); }

const TFPrimitive& TFPrimitiveSet::back() const { return *sorted_.back(); }

std::vector<TFPrimitiveData> TFPrimitiveSet::get() const {
    std::vector<TFPrimitiveData> result;
    result.reserve(sorted_.size());
    std::transform(sorted_.begin(), sorted_.end(), std::back_inserter(result),
                   [](auto& v) { return v->getData(); });
    return result;
}

std::vector<TFPrimitiveData> TFPrimitiveSet::getUnsorted() const {
    std::vector<TFPrimitiveData> result;
    result.reserve(values_.size());
    std::transform(values_.begin(), values_.end(), std::back_inserter(result),
                   [](auto& v) { return v->getData(); });
    return result;
}

std::pair<std::vector<double>, std::vector<vec4>> TFPrimitiveSet::getVectors() const {
    return {getPositions(), getColors()};
}

std::pair<std::vector<float>, std::vector<vec4>> TFPrimitiveSet::getVectorsf() const {
    return {getPositionsf(), getColors()};
}

std::vector<double> TFPrimitiveSet::getPositions() const {
    std::vector<double> result;
    result.reserve(sorted_.size());
    std::transform(sorted_.begin(), sorted_.end(), std::back_inserter(result),
                   [](const TFPrimitive* p) { return p->getPosition(); });
    return result;
}

std::vector<float> TFPrimitiveSet::getPositionsf() const {
    std::vector<float> result;
    result.reserve(sorted_.size());
    std::transform(sorted_.begin(), sorted_.end(), std::back_inserter(result),
                   [](const TFPrimitive* p) { return static_cast<float>(p->getPosition()); });
    return result;
}

std::vector<vec4> TFPrimitiveSet::getColors() const {
    std::vector<vec4> result;
    result.reserve(sorted_.size());
    std::transform(sorted_.begin(), sorted_.end(), std::back_inserter(result),
                   [](const TFPrimitive* p) { return p->getColor(); });
    return result;
}

TFPrimitive& TFPrimitiveSet::add(const TFPrimitive& primitive) {
    return add(std::make_unique<TFPrimitive>(primitive));
}

TFPrimitive& TFPrimitiveSet::add(double pos, const vec4& color) {
    return add(std::make_unique<TFPrimitive>(pos, color));
}

TFPrimitive& TFPrimitiveSet::add(double pos, double alpha) {
    const vec4 color(vec3(interpolateColor(pos)), static_cast<float>(alpha));
    return add(std::make_unique<TFPrimitive>(pos, color));
}

TFPrimitive& TFPrimitiveSet::add(const dvec2& pos) {
    const vec4 color(vec3(interpolateColor(pos.x)), static_cast<float>(pos.y));
    return add(std::make_unique<TFPrimitive>(pos.x, color));
}

TFPrimitive& TFPrimitiveSet::add(const TFPrimitiveData& data) {
    return add(std::make_unique<TFPrimitive>(data));
}

void TFPrimitiveSet::add(const std::vector<TFPrimitiveData>& primitives) {
    for (const auto& v : primitives) {
        add(v);
    }
}

bool TFPrimitiveSet::remove(const TFPrimitive& primitive) {
    auto it = std::find_if(values_.begin(), values_.end(),
                           [&](const auto& v) { return &primitive == v.get(); });

    return remove(it);
}

void TFPrimitiveSet::verifyPoint(double pos) const {
    if ((type_ == TFPrimitiveSetType::Relative) && (pos < 0.0f || pos > 1.0f)) {
        throw RangeException(
            SourceContext{},
            "TFPrimitive at {} outside of valid range [0,1] for a relative TFPrimitiveSet", pos);
    }
}

void TFPrimitiveSet::verifyPoint(const TFPrimitiveData& primitive) const {
    verifyPoint(primitive.pos);
}

void TFPrimitiveSet::verifyPoint(const TFPrimitive& primitive) const {
    verifyPoint(primitive.getPosition());
}

TFPrimitive& TFPrimitiveSet::add(std::unique_ptr<TFPrimitive> primitive) {
    verifyPoint(*primitive);

    primitive->addObserver(this);
    auto it = std::upper_bound(sorted_.begin(), sorted_.end(), primitive.get(), comparePtr{});
    sorted_.insert(it, primitive.get());
    values_.push_back(std::move(primitive));

    notifyTFPrimitiveAdded(*this, *values_.back());

    return *values_.back();
}

bool TFPrimitiveSet::remove(std::vector<std::unique_ptr<TFPrimitive>>::iterator it) {
    if (it != values_.end()) {
        // make sure we call the destructor after we have removed the point from points_
        auto dp = std::move(*it);
        values_.erase(it);
        std::erase(sorted_, dp.get());
        notifyTFPrimitiveRemoved(*this, *dp);
        return true;
    } else {
        return false;
    }
}

void TFPrimitiveSet::clear() {
    while (!values_.empty()) {
        remove(--values_.end());
    }
}

void TFPrimitiveSet::setPosition(std::span<TFPrimitive*> primitives, double pos) {
    verifyPoint(pos);

    // selected primitives need to be moved in correct order to maintain overall order of TF
    // That is, TF primitives closest to pos must be moved first
    std::set<TFPrimitive*> primitiveSet(primitives.begin(), primitives.end());

    std::vector<TFPrimitive*> sortedSelection;
    std::copy_if(sorted_.begin(), sorted_.end(), std::back_inserter(sortedSelection),
                 [&primitiveSet](auto item) { return primitiveSet.count(item) > 0; });

    // partition set of primitives at position pos
    auto partition =
        std::lower_bound(sortedSelection.begin(), sortedSelection.end(), pos,
                         [](const auto& p, double val) { return p->getPosition() < val; });

    // update upper half, i.e. all elements to the right of pos in ascending order
    for (auto it = partition; it != sortedSelection.end(); ++it) {
        (*it)->setPosition(pos);
    }

    // update lower half, i.e. all elements to the left of pos in descending order
    // to do this reverse sorted primitives from begin to the partition point
    std::reverse(sortedSelection.begin(), partition);
    for (auto it = sortedSelection.begin(); it != partition; ++it) {
        (*it)->setPosition(pos);
    }
}

void TFPrimitiveSet::onTFPrimitiveChange(const TFPrimitive& p) {
    sort();
    notifyTFPrimitiveChanged(*this, p);
}

void TFPrimitiveSet::serialize(Serializer& s) const {
    s.serialize("type", type_);
    s.serialize(serializationKey(), values_, serializationItemKey());
}

void TFPrimitiveSet::deserialize(Deserializer& d) {
    TFPrimitiveSetType type = type_;
    d.deserialize("type", type);
    if (type_ != type) {
        type_ = type;
        notifyTFTypeChanged(*this, type_);
    }

    d.deserialize(serializationKey(), values_, serializationItemKey(),
                  deserializer::IndexFunctions{
                      .makeNew = []() { return std::unique_ptr<TFPrimitive>{}; },
                      .onNew =
                          [&](std::unique_ptr<TFPrimitive>& p, size_t) {
                              p->addObserver(this);
                              auto it = std::upper_bound(sorted_.begin(), sorted_.end(), p.get(),
                                                         comparePtr{});
                              sorted_.insert(it, p.get());
                              notifyTFPrimitiveAdded(*this, *p);
                          },
                      .onRemove =
                          [&](std::unique_ptr<TFPrimitive>& p) {
                              std::erase(sorted_, p.get());
                              notifyTFPrimitiveRemoved(*this, *p);
                          }});
}

void TFPrimitiveSet::sort() { std::stable_sort(sorted_.begin(), sorted_.end(), comparePtr{}); }

vec4 TFPrimitiveSet::interpolateColor(double t) const {
    if (empty()) return vec4(0.0f);

    auto it = std::upper_bound(begin(), end(), t,
                               [](double val, const auto& p) { return val < p.getPosition(); });

    if (it == begin()) {
        return front().getColor();
    } else if (it == end()) {
        return back().getColor();
    }

    const auto next = it--;
    return util::interpolateColor(it->getData(), next->getData(), t);
}

void TFPrimitiveSet::interpolateAndStoreColors(std::span<vec4> data) const {
    if (empty()) {  // in case of 0 points
        std::fill(data.begin(), data.end(), vec4(0.0f));
    } else if (size() == 1) {  // in case of 1 point
        std::fill(data.begin(), data.end(), front().getColor());
    } else {  // in case of more than 1 points
        const auto sizeM1 = static_cast<double>(data.size() - 1);
        const auto toInd = [&](const TFPrimitive& p) {
            return static_cast<std::ptrdiff_t>(ceil(p.getPosition() * sizeM1));
        };

        const auto leftX = toInd(front());
        const auto rightX = toInd(back());

        std::fill(data.begin(), data.begin() + leftX + 1, front().getColor());
        std::fill(data.begin() + rightX, data.end(), back().getColor());

        auto pLeft = begin();
        auto pRight = ++begin();

        while (pRight != end()) {
            const auto lrgba = pLeft->getColor();
            const auto rrgba = pRight->getColor();
            const auto lx = pLeft->getPosition() * sizeM1;
            const auto rx = pRight->getPosition() * sizeM1;

            for (std::ptrdiff_t n = toInd(*pLeft); n < toInd(*pRight); ++n) {
                const auto x = static_cast<float>((static_cast<double>(n) - lx) / (rx - lx));
                data[n] = glm::mix(lrgba, rrgba, x);
            }
            ++pLeft;
            ++pRight;
        }
    }
}

std::string_view TFPrimitiveSet::serializationKey() const { return "TFPrimitives"; }

std::string_view TFPrimitiveSet::serializationItemKey() const { return "TFPrimitive"; }

bool operator==(const TFPrimitiveSet& lhs, const TFPrimitiveSet& rhs) {
    return std::equal(rhs.sorted_.begin(), rhs.sorted_.end(), lhs.sorted_.begin(),
                      lhs.sorted_.end(), [](TFPrimitive* a, TFPrimitive* b) { return *a == *b; });
}

bool operator!=(const TFPrimitiveSet& lhs, const TFPrimitiveSet& rhs) {
    return !operator==(lhs, rhs);
}

bool TFPrimitiveSet::contains(const TFPrimitive* primitive) const {
    if (!primitive) return false;
    return util::contains(sorted_, primitive);
}

void util::distributeAlphaEvenly(std::vector<TFPrimitive*> selection) {
    if (selection.size() < 2) {
        return;
    }
    const auto [min, max] =
        std::minmax_element(selection.begin(), selection.end(),
                            [](auto a, auto b) { return a->getAlpha() < b->getAlpha(); });
    const auto minAlpha = (*min)->getAlpha();
    const auto maxAlpha = (*max)->getAlpha();

    std::stable_sort(selection.begin(), selection.end(), comparePtr{});

    for (auto&& [index, elem] : util::enumerate(selection)) {
        elem->setAlpha(
            glm::mix(minAlpha, maxAlpha,
                     static_cast<float>(index) / static_cast<float>(selection.size() - 1)));
    }
}

void util::distributePositionEvenly(std::vector<TFPrimitive*> selection) {
    if (selection.size() < 2) {
        return;
    }
    const auto [min, max] =
        std::minmax_element(selection.begin(), selection.end(),
                            [](auto a, auto b) { return a->getPosition() < b->getPosition(); });
    const auto minPosition = (*min)->getPosition();
    const auto maxPosition = (*max)->getPosition();

    std::stable_sort(selection.begin(), selection.end(), comparePtr{});

    for (auto&& [index, elem] : util::enumerate(selection)) {
        elem->setPosition(
            glm::mix(minPosition, maxPosition,
                     static_cast<double>(index) / static_cast<double>(selection.size() - 1)));
    }
}

void util::alignAlphaToMean(const std::vector<TFPrimitive*>& selection) {
    if (selection.size() < 2) {
        return;
    }
    const auto alpha =
        std::transform_reduce(selection.begin(), selection.end(), 0.0f, std::plus<>{},
                              [](TFPrimitive* p) { return p->getAlpha(); }) /
        static_cast<float>(selection.size());
    std::ranges::for_each(selection, [&](TFPrimitive* p) { p->setAlpha(alpha); });
}

void util::alignAlphaToTop(const std::vector<TFPrimitive*>& selection) {
    if (selection.size() < 2) {
        return;
    }

    const float alpha = (*std::max_element(selection.begin(), selection.end(), [](auto a, auto b) {
                            return a->getAlpha() < b->getAlpha();
                        }))->getAlpha();
    std::ranges::for_each(selection, [&](TFPrimitive* p) { p->setAlpha(alpha); });
}

void util::alignAlphaToBottom(const std::vector<TFPrimitive*>& selection) {
    if (selection.size() < 2) {
        return;
    }
    const float alpha = (*std::min_element(selection.begin(), selection.end(), [](auto a, auto b) {
                            return a->getAlpha() < b->getAlpha();
                        }))->getAlpha();
    std::ranges::for_each(selection, [&](TFPrimitive* p) { p->setAlpha(alpha); });
}

void util::alignPositionToMean(std::vector<TFPrimitive*> selection) {
    if (selection.size() < 2) {
        return;
    }

    const auto pos = std::transform_reduce(selection.begin(), selection.end(), 0.0, std::plus<>{},
                                           [](TFPrimitive* p) { return p->getPosition(); }) /
                     static_cast<float>(selection.size());

    std::stable_sort(selection.begin(), selection.end(),
                     [&](const TFPrimitive* a, const TFPrimitive* b) {
                         return std::abs(a->getPosition() - pos) < std::abs(b->getPosition() - pos);
                     });
    std::ranges::for_each(selection, [&](TFPrimitive* p) { p->setPosition(pos); });
}

void util::alignPositionToLeft(std::vector<TFPrimitive*> selection) {
    if (selection.size() < 2) {
        return;
    }

    const auto pos = (*std::min_element(selection.begin(), selection.end(), [](auto a, auto b) {
                         return a->getPosition() < b->getPosition();
                     }))->getPosition();

    std::stable_sort(selection.begin(), selection.end(),
                     [&](const TFPrimitive* a, const TFPrimitive* b) {
                         return std::abs(a->getPosition() - pos) < std::abs(b->getPosition() - pos);
                     });
    std::ranges::for_each(selection, [&](TFPrimitive* p) { p->setPosition(pos); });
}

void util::alignPositionToRight(std::vector<TFPrimitive*> selection) {
    if (selection.size() < 2) {
        return;
    }

    const auto pos = (*std::max_element(selection.begin(), selection.end(), [](auto a, auto b) {
                         return a->getPosition() < b->getPosition();
                     }))->getPosition();

    std::sort(selection.begin(), selection.end(), [&](const TFPrimitive* a, const TFPrimitive* b) {
        return std::abs(a->getPosition() - pos) < std::abs(b->getPosition() - pos);
    });
    std::ranges::for_each(selection, [&](TFPrimitive* p) { p->setPosition(pos); });
}

void util::interpolateAlpha(const std::vector<TFPrimitive*>& selection) {
    if (selection.size() < 2) {
        return;
    }
    const auto [min, max] =
        std::minmax_element(selection.begin(), selection.end(),
                            [](auto a, auto b) { return a->getAlpha() < b->getAlpha(); });
    const auto minAlpha = (*min)->getAlpha();
    const auto maxAlpha = (*max)->getAlpha();
    const auto minPosition = (*min)->getPosition();
    const auto maxPosition = (*max)->getPosition();

    std::ranges::for_each(selection, [&](TFPrimitive* p) {
        const auto t = static_cast<std::remove_const_t<decltype(minAlpha)>>(
            (p->getPosition() - minPosition) / (maxPosition - minPosition));
        p->setAlpha(glm::mix(minAlpha, maxAlpha, t));
    });
}

void util::flipPositions(const std::vector<TFPrimitive*>& selection) {
    if (selection.size() < 2) {
        return;
    }

    const auto [min, max] =
        std::minmax_element(selection.begin(), selection.end(),
                            [](auto a, auto b) { return a->getPosition() < b->getPosition(); });
    const auto minPosition = (*min)->getPosition();
    const auto maxPosition = (*max)->getPosition();

    std::ranges::for_each(selection, [&](TFPrimitive* p) {
        p->setPosition(maxPosition - (p->getPosition() - minPosition));
    });
}

}  // namespace inviwo
