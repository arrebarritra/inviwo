/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2017-2025 Inviwo Foundation
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

#include <inviwo/core/common/modulemanager.h>
#include <inviwo/core/common/inviwomodule.h>
#include <inviwo/core/common/version.h>
#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/util/filesystem.h>
#include <inviwo/core/util/settings/systemsettings.h>
#include <inviwo/core/util/sharedlibrary.h>
#include <inviwo/core/util/vectoroperations.h>
#include <inviwo/core/util/utilities.h>
#include <inviwo/core/util/capabilities.h>
#include <inviwo/core/network/processornetwork.h>
#include <inviwo/core/common/inviwocommondefines.h>
#include <inviwo/core/network/workspacemanager.h>

#include <string>
#include <functional>
#include <ranges>

#include <fmt/std.h>

namespace inviwo {

namespace {

void topologicalSort(std::vector<ModuleContainer>& containers) {

    auto helper = [](auto& self, std::vector<ModuleContainer>& containers, const std::string& lname,
                     std::unordered_set<std::string>& visited,
                     std::unordered_set<std::string>& tmpVisited,
                     std::vector<std::string>& sorted) -> void {
        auto it = std::ranges::find(containers, lname, &ModuleContainer::identifier);
        if (it == containers.end()) {
            throw Exception(SourceContext{}, "Missing module dependency {}", lname);
        }

        if (visited.contains(lname)) return;  // Already visited;
        if (tmpVisited.contains(lname)) {
            throw Exception(SourceContext{}, "Module Dependency graph is not a DAG");
        }

        tmpVisited.insert(lname);
        for (const auto& [dependency, version] : it->dependencies()) {
            self(self, containers, dependency, visited, tmpVisited, sorted);
        }
        visited.insert(lname);
        sorted.push_back(lname);
    };

    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> tmpVisited;
    std::vector<std::string> sorted;
    for (const auto& item : containers) {
        helper(helper, containers, item.identifier(), visited, tmpVisited, sorted);
    }
    // Sort modules according to dependency graph
    std::sort(containers.begin(), containers.end(), [&](const auto& a, const auto& b) {
        return std::ranges::find(sorted, a.identifier()) <
               std::ranges::find(sorted, b.identifier());
    });
}

}  // namespace

ModuleManager::ModuleManager(InviwoApplication* app)
    : app_{app}, onModulesDidRegister_{}, onModulesWillUnregister_{}, inviwoModules_{} {}

ModuleManager::~ModuleManager() {
    // Need to clear the modules in reverse order since they might depend on each other.
    // The destruction order of vector is undefined.
    for (auto& cont : inviwoModules_ | std::views::reverse) {
        cont.resetModule();
    }
}
bool ModuleManager::isRuntimeModuleReloadingEnabled() {
    return app_->getSystemSettings().runtimeModuleReloading_;
}

void ModuleManager::registerModules(std::vector<std::unique_ptr<InviwoModuleFactoryObject>> mfo) {
    std::vector<ModuleContainer> inviwoModules;
    for (auto& obj : mfo) {
        inviwoModules.emplace_back(std::move(obj));
    }
    registerModules(std::move(inviwoModules));
}

void ModuleManager::registerModules(std::vector<ModuleContainer> inviwoModules) {
    // Topological sort to make sure that we load modules in correct order
    topologicalSort(inviwoModules);

    for (auto& cont : inviwoModules) {
        app_->postProgress("Loading module: " + cont.name());
        if (getModuleByIdentifier(cont.identifier())) continue;  // already loaded
        if (!checkDependencies(cont.factoryObject())) continue;

        try {
            cont.createModule(app_);
            cont.setReloadCallback(app_, [this](ModuleContainer&) { reloadModules(); });
            inviwoModules_.push_back(std::move(cont));

        } catch (const ModuleInitException& e) {
            const auto dereg = deregisterDependentModules(e.getModulesToDeregister());

            const auto err = dereg.empty() ? ""
                                           : fmt::format("\nUnregistered dependent modules: {}",
                                                         fmt::join(dereg, ", "));
            log::exception(e, "Failed to register module: {}. Reason:\n {}{}", cont.name(),
                           e.getMessage(), err);
        } catch (const Exception& e) {
            log::exception(e, "Failed to register module: {}. Reason:\n{}", cont.name(),
                           e.getMessage());
        } catch (const std::exception& e) {
            log::error("Failed to register module: {}. Reason:\n{}", cont.name(), e.what());
        }
    }

    ModuleContainer::updateGraph(inviwoModules_);

    app_->postProgress("Loading Capabilities");
    for (auto& cont : inviwoModules_) {
        if (auto* inviwoModule = cont.getModule()) {
            for (auto& capability : inviwoModule->getCapabilities()) {
                capability->retrieveStaticInfo();
                capability->printInfo();
            }
        }
    }

    onModulesDidRegister_.invoke();
}

std::function<bool(std::string_view)> ModuleManager::getEnabledFilter() {
    // Load enabled modules if file "application_name-enabled-modules.txt" exists,
    // otherwise load all modules

    const auto exeName = filesystem::getExecutablePath().stem();
    const auto exepath = filesystem::getExecutablePath().parent_path();

    const auto enabledModuleFileName = exeName.string() + "-enabled-modules.txt";

#ifdef __APPLE__
    // Executable path is inviwo.app/Content/MacOs
    std::filesystem::path enabledModulesFilePath(exepath / "../../.." / enabledModuleFileName);
#else
    std::filesystem::path enabledModulesFilePath(exepath / enabledModuleFileName);
#endif
    if (!std::filesystem::is_regular_file(enabledModulesFilePath)) {
        return [](std::string_view) { return true; };
    }

    std::ifstream enabledModulesFile{enabledModulesFilePath};
    std::vector<std::string> enabledModules;
    std::copy(std::istream_iterator<std::string>(enabledModulesFile),
              std::istream_iterator<std::string>(), std::back_inserter(enabledModules));
    std::for_each(std::begin(enabledModules), std::end(enabledModules), toLower);

    return [enabledModules](std::string_view name) { return util::contains(enabledModules, name); };
}

void ModuleManager::reloadModules() {
    if (!isRuntimeModuleReloadingEnabled()) return;

    // 1. Serialize network
    // 2. Clear modules/unload module libraries
    // 3. Non-protected modules will be removed.
    // 4. Loaded dynamic module libraries will be unloaded (unless marked as protected).
    // 5. Load module libraries and register them
    // 6. De-serialize network

    log::info("Reloading modules");

    // Serialize network
    std::stringstream stream;
    try {
        app_->getWorkspaceManager()->save(stream, filesystem::findBasePath());
    } catch (SerializationException& e) {
        log::exception(e, "Unable to save network due to {}", e.getMessage());
        return;
    }

    app_->getProcessorNetwork()->clear();

    onModulesWillUnregister_.invoke();

    // Need to clear the modules in reverse order since they might depend on each other.
    // The destruction order of vector is undefined.
    for (auto& cont : inviwoModules_ | std::views::reverse) {
        if (!cont.isProtectedModule()) {
            cont.resetModule();
        }
    }

    for (auto& cont : inviwoModules_ | std::views::reverse) {
        if (!cont.isProtectedLibrary()) {
            cont.unload();
        }
    }
    for (auto& cont : inviwoModules_) {
        if (!cont.isProtectedLibrary()) {
            cont.load(true);
        }
    }

    for (auto& cont : inviwoModules_) {
        if (!cont.isProtectedModule()) {
            try {
                cont.createModule(app_);
            } catch (const ModuleInitException& e) {
                const auto dereg = deregisterDependentModules(e.getModulesToDeregister());

                const auto err = dereg.empty() ? ""
                                               : fmt::format("\nUnregistered dependent modules: {}",
                                                             fmt::join(dereg, ", "));
                log::exception(e, "Failed to register module: {}. Reason:\n {}{}", cont.name(),
                               e.getMessage(), err);
            } catch (const Exception& e) {
                log::exception(e, "Failed to register module: {}. Reason:\n{}", cont.name(),
                               e.getMessage());
            } catch (const std::exception& e) {
                log::error("Failed to register module: {}. Reason:\n{}", cont.name(), e.what());
            }
        }
    }

    ModuleContainer::updateGraph(inviwoModules_);

    for (auto& cont : inviwoModules_) {
        if (!cont.isProtectedModule()) {
            if (auto* inviwoModule = cont.getModule()) {
                for (auto& capability : inviwoModule->getCapabilities()) {
                    capability->retrieveStaticInfo();
                    capability->printInfo();
                }
            }
        }
    }

    onModulesDidRegister_.invoke();

    // De-serialize network
    try {
        // Lock the network that so no evaluations are triggered during the de-serialization
        app_->getWorkspaceManager()->load(stream, filesystem::findBasePath());
    } catch (SerializationException& e) {
        log::exception(e, "Unable to load network due to {}", e.getMessage());
        return;
    }
}

std::vector<ModuleContainer> ModuleManager::findRuntimeModules(
    std::span<const std::filesystem::path> searchPaths) {
    return findRuntimeModules(searchPaths, getEnabledFilter(), isRuntimeModuleReloadingEnabled());
}

std::vector<ModuleContainer> ModuleManager::findRuntimeModules(
    std::span<const std::filesystem::path> searchPaths,
    std::function<bool(std::string_view)> isEnabled) {

    return findRuntimeModules(searchPaths, isEnabled, isRuntimeModuleReloadingEnabled());
}

std::vector<ModuleContainer> ModuleManager::findRuntimeModules(
    std::span<const std::filesystem::path> searchPaths,
    std::function<bool(std::string_view)> isEnabled, bool runtimeReloading) {

    const auto libraryTypes = SharedLibrary::libraryFileExtensions();
    // Remove unsupported files and files belonging to already loaded modules.
    auto valid = [&](const std::filesystem::path& file) {
        const auto name = file.filename().generic_string();
        return libraryTypes.contains(file.extension()) &&
               (name.find("inviwo-module") != std::string::npos ||
                name.find("inviwo-core") != std::string::npos);
    };

    std::vector<ModuleContainer> modules;

    for (auto path : searchPaths) {
        // Make sure that we have an absolute path to avoid duplicates
        path = std::filesystem::weakly_canonical(path);
        using enum std::filesystem::directory_options;
        std::error_code ec;
        for (auto&& file : std::filesystem::recursive_directory_iterator{
                 path, follow_directory_symlink | skip_permission_denied, ec}) {

            if (!valid(file)) continue;

            if (!isEnabled(util::stripModuleFileNameDecoration(file))) continue;

            try {
                modules.emplace_back(file, runtimeReloading);
            } catch (const Exception& e) {
                log::warn("Could not load library: {}", file.path());
                log::exception(e);
            }
        }
    }

    return modules;
}

void ModuleManager::registerModules(RuntimeModuleLoading,
                                    std::function<bool(std::string_view)> isEnabled) {
    // Perform the following steps
    // 1. Recursively get all library files and the folders they are in
    // 2. Filter out files with correct extension, named inviwo-module
    //    and listed in application_name-enabled-modules.txt (if it exist).
    // 3. Load libraries and see if createModule function exist.
    // 4. Start observing file if reloadLibrariesWhenChanged
    // 5. Pass module factories to registerModules

    // Find unique files and directories in specified search paths
    auto librarySearchPaths = util::getLibrarySearchPaths();

    auto modules =
        findRuntimeModules(librarySearchPaths, isEnabled, isRuntimeModuleReloadingEnabled());

    registerModules(std::move(modules));
}

InviwoModule* ModuleManager::getModuleByIdentifier(std::string_view identifier) const {
    const auto it = std::ranges::find_if(inviwoModules_, [&](const ModuleContainer& m) {
        return iCaseCmp(m.identifier(), identifier);
    });
    if (it != inviwoModules_.end()) {
        return it->getModule();
    } else {
        return nullptr;
    }
}

std::vector<InviwoModule*> ModuleManager::getModulesByAlias(std::string_view alias) const {
    std::vector<InviwoModule*> res;
    for (const auto& cont : inviwoModules_) {
        if (std::ranges::find(cont.factoryObject().aliases, alias) !=
            cont.factoryObject().aliases.end()) {
            if (auto* m = cont.getModule()) {
                res.push_back(m);
            }
        }
    }
    return res;
}

InviwoModuleFactoryObject* ModuleManager::getFactoryObject(std::string_view identifier) const {
    auto it = std::ranges::find_if(
        inviwoModules_, [&](const auto& cont) { return iCaseCmp(cont.identifier(), identifier); });
    // Check if dependent module is of correct version
    if (it != inviwoModules_.end()) {
        return &it->factoryObject();
    } else {
        return nullptr;
    }
}

std::shared_ptr<std::function<void()>> ModuleManager::onModulesDidRegister(
    std::function<void()> callback) {
    return onModulesDidRegister_.add(callback);
}

std::shared_ptr<std::function<void()>> ModuleManager::onModulesWillUnregister(
    std::function<void()> callback) {
    return onModulesWillUnregister_.add(callback);
}

bool ModuleManager::checkDependencies(const InviwoModuleFactoryObject& obj) const {
    std::string err;

    // Make sure that the module supports the current inviwo core version
    if (!build::version.semanticVersionEqual(obj.inviwoCoreVersion)) {
        fmt::format_to(std::back_inserter(err),
                       "\nModule was built for Inviwo version {}, current version is {}",
                       obj.inviwoCoreVersion, build::version);
    }

    // Check if dependency modules have correct versions.
    // Note that the module version only need to be increased
    // when changing and the inviwo core version has not changed
    // since we are ensuring the they must be built for the
    // same core version.
    for (const auto& [name, version] : obj.dependencies) {

        if (auto depObj = getFactoryObject(name)) {
            if (!getModuleByIdentifier(depObj->name)) {
                fmt::format_to(std::back_inserter(err),
                               "\nModule dependency: {} failed to register", depObj->name);
            } else if (!depObj->version.semanticVersionEqual(obj.version)) {
                fmt::format_to(std::back_inserter(err),
                               "\nModule depends on {} version {} but version {} was loaded",
                               depObj->name, version, depObj->version);
            }
        } else {
            fmt::format_to(std::back_inserter(err),
                           "\nModule depends on {} version {} but no such module was found", name,
                           version);
        }
    }
    if (!err.empty()) {
        log::error("Failed to register module: {}. Reason: {}", obj.name, err);
        return false;
    } else {
        return true;
    }
}

std::vector<std::string> ModuleManager::findDependentModules(std::string_view moduleId) const {
    std::vector<std::string> dependencies;
    for (const auto& item : inviwoModules_) {
        if (item.dependsOn(moduleId)) {
            auto deps = findDependentModules(item.identifier());
            util::append(dependencies, deps);
            dependencies.push_back(item.identifier());
        }
    }
    std::vector<std::string> unique;
    for (const auto& item : dependencies) {
        util::push_back_unique(unique, item);
    }
    return unique;
}

std::vector<std::string> ModuleManager::deregisterDependentModules(
    const std::vector<std::string>& toDeregister) {

    std::set<std::string> deregister;
    for (const auto& m : toDeregister) {
        deregister.insert(m);
        auto dependents = findDependentModules(m);
        deregister.insert(dependents.begin(), dependents.end());
    }

    std::vector<std::string> deregistered;
    for (auto& cont : inviwoModules_ | std::views::reverse) {
        if (deregister.contains(cont.identifier())) {
            deregistered.push_back(cont.identifier());
            cont.resetModule();
        }
    }

    return deregistered;
}

void ModuleManager::setModuleLocator(
    std::function<std::filesystem::path(const InviwoModule&)> moduleLocator) {
    moduleLocator_ = moduleLocator;
}
std::filesystem::path ModuleManager::locateModule(const InviwoModule& m) const {
    if (moduleLocator_) {
        return moduleLocator_(m);
    } else {
        auto path = filesystem::findBasePath() / "modules" / toLower(m.getIdentifier());
        return path.lexically_normal();
    }
}

}  // namespace inviwo
