#include "fs_plugin.hpp"
#include "posix_plugin.hpp"
#ifdef GEKKO_PLUGIN
#include "gekko_plugin.hpp"
#endif
namespace cargo {

std::unique_ptr<FSPlugin>
FSPlugin::make_fs(type t) {

    switch(t) {
        case type::posix:
            return std::make_unique<cargo::posix_plugin>();
#ifdef GEKKO_PLUGIN
        case type::gekkofs:
            return std::make_unique<cargo::gekko_plugin>();
#endif
        default:
            return {};
    }
}
} // namespace cargo