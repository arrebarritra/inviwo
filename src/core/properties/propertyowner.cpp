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

#include <inviwo/core/interaction/events/event.h>
#include <inviwo/core/properties/property.h>
#include <inviwo/core/properties/eventproperty.h>
#include <inviwo/core/properties/compositeproperty.h>
#include <inviwo/core/properties/propertyowner.h>
#include <inviwo/core/io/serialization/serializable.h>
#include <inviwo/core/io/serialization/versionconverter.h>
#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/util/exception.h>
#include <inviwo/core/network/networkvisitor.h>
#include <inviwo/core/network/lambdanetworkvisitor.h>

#include <iterator>

namespace inviwo {

PropertyOwner::PropertyOwner()
    : PropertyOwnerObservable()
    , properties_{}
    , eventProperties_{}
    , compositeProperties_{}
    , ownedProperties_{}
    , invalidationLevel_{InvalidationLevel::Valid} {}

PropertyOwner::PropertyOwner(const PropertyOwner& rhs)
    : PropertyOwnerObservable{rhs}
    , properties_{}
    , eventProperties_{}
    , compositeProperties_{}
    , ownedProperties_{}
    , invalidationLevel_{rhs.invalidationLevel_} {

    for (const auto& p : rhs.ownedProperties_) {
        addProperty(p->clone());
    }
}

PropertyOwner::PropertyOwner(PropertyOwner&& rhs)
    : PropertyOwnerObservable{}
    , properties_{}
    , eventProperties_{}
    , compositeProperties_{}
    , ownedProperties_{std::move(rhs.ownedProperties_)}
    , invalidationLevel_(std::move(rhs.invalidationLevel_)) {

    for (auto& p : ownedProperties_) {
        std::erase(rhs.properties_, p.get());
        std::erase(rhs.eventProperties_, p.get());
        std::erase(rhs.compositeProperties_, p.get());
        insertPropertyImpl(properties_.end(), p.get(),false);
    }
    rhs.clear();

    PropertyOwnerObservable::operator=(std::move(rhs));
}

PropertyOwner& PropertyOwner::operator=(PropertyOwner&& that) {
    if (this != &that) {
        clear();

        ownedProperties_ = std::move(that.ownedProperties_);
        for (auto& p : ownedProperties_) {
            std::erase(that.properties_, p.get());
            std::erase(that.eventProperties_, p.get());
            std::erase(that.compositeProperties_, p.get());
            insertPropertyImpl(properties_.end(), p.get(),false);
        }
        that.clear();

        PropertyOwnerObservable::operator=(std::move(that));
    }
    return *this;
}

PropertyOwner::~PropertyOwner() { clear(); }

void PropertyOwner::addProperty(Property* property, bool owner) {
    insertProperty(properties_.size(), property, owner);
}

void PropertyOwner::addProperty(Property& property) {
    insertProperty(properties_.size(), &property, false);
}

Property* PropertyOwner::addProperty(std::unique_ptr<Property> property) {
    return insertProperty(properties_.size(), std::move(property));
}

void PropertyOwner::insertProperty(size_t index, Property& property) {
    insertProperty(index, &property, false);
}

Property* PropertyOwner::insertProperty(size_t index, std::unique_ptr<Property> property) {
    insertProperty(index, property.get(), false);
    // Need to serialize everything for owned properties
    property->setSerializationMode(PropertySerializationMode::All);

    ownedProperties_.push_back(std::move(property));
    return ownedProperties_.back().get();
}

void PropertyOwner::insertProperty(size_t index, Property* property, bool owner) {
    if (index > properties_.size()) {
        index = properties_.size();
    }

    if (auto* existing = getPropertyByIdentifier(property->getIdentifier()); existing != nullptr) {
        throw Exception(SourceContext{},
                        "Cannot add Property: [id: '{}', class id: '{}'] to PropertyOwner '{}'"
                        ", the identifier is already used by [id: '{}', class id: '{}']",
                        property->getIdentifier(), property->getClassIdentifier(), getIdentifier(),
                        existing->getIdentifier(), existing->getClassIdentifier());
    }
    if (auto parent = dynamic_cast<Property*>(this)) {
        if (parent == property) {
            throw Exception(SourceContext{},
                            "Cannot add Property: [id: '{}', class id: '{}'] to itself.",
                            property->getIdentifier(), property->getClassIdentifier());
        }
    }

    notifyObserversWillAddProperty(this, property, index);
    insertPropertyImpl(properties_.begin() + index, property, owner);
    notifyObserversDidAddProperty(property, index);
}

void PropertyOwner::insertPropertyImpl(iterator it, Property* property, bool owner) {
    properties_.insert(it, property);
    property->setOwner(this);

    if (dynamic_cast<EventProperty*>(property)) {
        eventProperties_.push_back(static_cast<EventProperty*>(property));
    }
    if (dynamic_cast<CompositeProperty*>(property)) {
        compositeProperties_.push_back(static_cast<CompositeProperty*>(property));
    }

    if (owner) {  // Assume ownership of property;
        ownedProperties_.emplace_back(property);
        // Need to serialize everything for owned properties
        property->setSerializationMode(PropertySerializationMode::All);
    }
}

Property* PropertyOwner::removeProperty(std::string_view identifier) {
    return removeProperty(
        std::find_if(properties_.begin(), properties_.end(),
                     [&identifier](Property* p) { return p->getIdentifier() == identifier; }));
}

Property* PropertyOwner::removeProperty(Property* property) {
    return removeProperty(std::find(properties_.begin(), properties_.end(), property));
}

Property* PropertyOwner::removeProperty(Property& property) { return removeProperty(&property); }

Property* PropertyOwner::removeProperty(size_t index) {
    if (index >= size()) {
        throw RangeException(SourceContext{},
                             "Index '{}' out of range while removing property, ({} elements)",
                             index, size());
    }
    return removeProperty(begin() + index);
}

Property* PropertyOwner::removeProperty(iterator it) { return removePropertyImpl(it); }

Property* PropertyOwner::removePropertyImpl(iterator it) {
    Property* prop = nullptr;
    if (it != properties_.end()) {
        prop = *it;
        size_t index = std::distance(properties_.begin(), it);
        notifyObserversWillRemoveProperty(prop, index);

        std::erase(eventProperties_, *it);
        std::erase(compositeProperties_, *it);

        prop->setOwner(nullptr);
        properties_.erase(it);
        notifyObserversDidRemoveProperty(this, prop, index);

        // This will delete the property if owned; in that case set prop to nullptr.
        std::erase_if(ownedProperties_, [&prop](const std::unique_ptr<Property>& p) {
            if (p.get() == prop) {
                prop = nullptr;
                return true;
            } else {
                return false;
            }
        });
    }
    return prop;
}

void PropertyOwner::clear() {
    while (!properties_.empty()) {
        removePropertyImpl(--properties_.end());
    }
}

void PropertyOwner::forEachProperty(std::function<void(Property&)> callback,
                                    bool recursiveSearch) const {
    LambdaNetworkVisitor visitor{[&](Property& property) {
        callback(property);
        return recursiveSearch;
    }};
    for (auto* elem : properties_) {
        elem->accept(visitor);
    }
}

const std::vector<Property*>& PropertyOwner::getProperties() const { return properties_; }

const std::vector<CompositeProperty*>& PropertyOwner::getCompositeProperties() const {
    return compositeProperties_;
}

std::vector<Property*>& PropertyOwner::getPropertiesRecursive(
    std::vector<Property*>& destination) const {
    destination.reserve(destination.size() + properties_.size());
    destination.insert(destination.end(), properties_.begin(), properties_.end());

    for (auto comp : compositeProperties_) {
        comp->getPropertiesRecursive(destination);
    }
    return destination;
}

std::vector<Property*> PropertyOwner::getPropertiesRecursive() const {
    std::vector<Property*> result;
    getPropertiesRecursive(result);
    return result;
}

Property* PropertyOwner::getPropertyByIdentifier(std::string_view identifier,
                                                 bool recursiveSearch) const {
    for (auto* property : properties_) {
        if (property->getIdentifier() == identifier) return property;
    }
    if (recursiveSearch) {
        for (auto* compositeProperty : compositeProperties_) {
            if (auto* p = compositeProperty->getPropertyByIdentifier(identifier, true)) return p;
        }
    }
    return nullptr;
}

Property* PropertyOwner::getPropertyByPath(std::string_view path) const {
    if (path.empty()) return nullptr;

    const auto [first, rest] = util::splitByFirst(path, '.');
    if (rest.empty()) {
        return getPropertyByIdentifier(first);
    } else {
        auto it = std::find_if(compositeProperties_.begin(), compositeProperties_.end(),
                               [f = first](auto* comp) { return comp->getIdentifier() == f; });
        if (it != compositeProperties_.end()) {
            return (*it)->getPropertyByPath(rest);
        } else {
            return nullptr;
        }
    }
}

bool PropertyOwner::empty() const { return properties_.empty(); }

size_t PropertyOwner::size() const { return properties_.size(); }

Property* PropertyOwner::operator[](size_t i) { return properties_[i]; }

const Property* PropertyOwner::operator[](size_t i) const { return properties_[i]; }

auto PropertyOwner::find(Property* property) const -> const_iterator {
    return std::find(begin(), end(), property);
}

bool PropertyOwner::move(Property* property, size_t newIndex) {
    if (auto it = find(property); it != cend()) {
        auto index = std::distance(cbegin(), it);
        notifyObserversWillRemoveProperty(property, index);
        properties_.erase(it);
        notifyObserversDidRemoveProperty(this, property, index);

        notifyObserversWillAddProperty(this, property, newIndex);
        properties_.insert(properties_.begin() + newIndex, property);
        notifyObserversDidAddProperty(property, newIndex);
        return true;
    } else {
        return false;
    }
}

PropertyOwner::iterator PropertyOwner::begin() { return properties_.begin(); }

PropertyOwner::iterator PropertyOwner::end() { return properties_.end(); }

PropertyOwner::const_iterator PropertyOwner::begin() const { return properties_.begin(); }

PropertyOwner::const_iterator PropertyOwner::end() const { return properties_.end(); }

PropertyOwner::const_iterator PropertyOwner::cbegin() const { return properties_.cbegin(); }

PropertyOwner::const_iterator PropertyOwner::cend() const { return properties_.cend(); }

bool PropertyOwner::isValid() const { return invalidationLevel_ == InvalidationLevel::Valid; }

void PropertyOwner::setValid() {
    for (auto& elem : properties_) elem->setValid();
    invalidationLevel_ = InvalidationLevel::Valid;
}

InvalidationLevel PropertyOwner::getInvalidationLevel() const { return invalidationLevel_; }

void PropertyOwner::invalidate(InvalidationLevel invalidationLevel, Property*) {
    invalidationLevel_ = std::max(invalidationLevel_, invalidationLevel);
}

Processor* PropertyOwner::getProcessor() { return nullptr; }

const Processor* PropertyOwner::getProcessor() const { return nullptr; }

const PropertyOwner* PropertyOwner::getOwner() const { return nullptr; }

PropertyOwner* PropertyOwner::getOwner() { return nullptr; }

void PropertyOwner::serialize(Serializer& s) const {
    s.serialize(
        "OwnedPropertyIdentifiers", ownedProperties_, "PropertyIdentifier", util::alwaysTrue{},
        [](const std::unique_ptr<Property>& p) -> decltype(auto) { return p->getIdentifier(); });

    s.serialize("Properties", properties_, "Property",
                [](const Property* p) { return p->needsSerialization(); });
}

void PropertyOwner::deserialize(Deserializer& d) {
    if (d.getVersion() < 3) {
        // This is for finding renamed composites, and moving old properties to new composites.
        NodeVersionConverter tvc([this](TxElement* node) {
            std::vector<const CompositeProperty*> props;
            std::copy(compositeProperties_.begin(), compositeProperties_.end(),
                      std::back_inserter(props));
            return xml::findMatchingSubPropertiesForComposites(node, props);
        });
        d.convertVersion(&tvc);
    }

    std::vector<std::string> ownedIdentifiers;
    d.deserialize("OwnedPropertyIdentifiers", ownedIdentifiers, "PropertyIdentifier");

    d.deserialize(
        "Properties", properties_, "Property",
        deserializer::IdentifierFunctions{
            .getID = [](Property* const& p) -> std::string_view { return p->getIdentifier(); },
            .makeNew = []() -> Property* { return nullptr; },
            .filter = [&](std::string_view id, size_t) -> bool {
                return util::contains(ownedIdentifiers, id);
            },
            .onNew = [&](Property*& p, size_t i) { insertProperty(i, p, true); },
            .onRemove =
                [&](std::string_view id) {
                    if (util::contains_if(ownedProperties_, [&](std::unique_ptr<Property>& op) {
                            return op->getIdentifier() == id;
                        })) {
                        removeProperty(id);
                    } else {
                        // The property was not serialized since it was in its
                        // default state. Make sure we reset it to that state again.
                        if (auto* p = getPropertyByIdentifier(id)) {
                            if (p->getSerializationMode() == PropertySerializationMode::Default) {
                                p->resetToDefaultState();
                            }
                        }
                    }
                },
            .onMove = [&](Property*& p, size_t i) { move(p, i); }});
}

void PropertyOwner::setAllPropertiesCurrentStateAsDefault() {
    for (auto& elem : properties_) (elem)->setCurrentStateAsDefault();
}

void PropertyOwner::resetAllProperties() {
    for (auto& elem : properties_) elem->resetToDefaultState();
}

const std::string& PropertyOwner::getIdentifier() const {
    static std::string id;
    return id;
}

void PropertyOwner::invokeEvent(Event* event) {
    for (auto elem : eventProperties_) {
        elem->invokeEvent(event);
        if (event->hasBeenUsed()) return;
    }
    for (auto elem : compositeProperties_) {
        elem->invokeEvent(event);
        if (event->hasBeenUsed()) return;
    }
}

}  // namespace inviwo
