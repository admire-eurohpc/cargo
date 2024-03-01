#include "fs_plugin.hpp"
#include "posix_plugin.hpp"
#ifdef GEKKOFS_PLUGIN
#include "gekko_plugin.hpp"
#endif
#ifdef HERCULES_PLUGIN
#include "hercules_plugin.hpp"
#endif
#ifdef EXPAND_PLUGIN
#include "expand_plugin.hpp"
#endif

namespace cargo {

std::unique_ptr<FSPlugin>
FSPlugin::make_fs(type t) {

    switch(t) {
        case type::posix:
        case type::parallel:
            return std::make_unique<cargo::posix_plugin>();
#ifdef GEKKOFS_PLUGIN
        case type::gekkofs:
            return std::make_unique<cargo::gekko_plugin>();
#endif
#ifdef HERCULES_PLUGIN
        case type::hercules:
            return std::make_unique<cargo::hercules_plugin>();
#endif
#ifdef EXPAND_PLUGIN
        case type::expand:
            return std::make_unique<cargo::expand_plugin>();
#endif
        default:
            return {};
    }
}
} // namespace cargo