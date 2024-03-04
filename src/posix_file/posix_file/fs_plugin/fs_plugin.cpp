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

static std::shared_ptr<FSPlugin> m_fs_posix;
static std::shared_ptr<FSPlugin> m_fs_gekkofs;

std::shared_ptr<FSPlugin>
FSPlugin::make_fs(type t) {

    switch(t) {
        case type::posix:
        case type::parallel:
            if(m_fs_posix == nullptr)
                m_fs_posix = std::make_shared<cargo::posix_plugin>();
            return m_fs_posix;
#ifdef GEKKOFS_PLUGIN
        case type::gekkofs:
            if(m_fs_gekkofs == nullptr)
                m_fs_gekkofs = std::make_shared<cargo::gekko_plugin>();
            return m_fs_gekkofs;

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