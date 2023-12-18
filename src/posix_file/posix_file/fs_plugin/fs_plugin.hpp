#ifndef FS_PLUGIN_HPP
#define FS_PLUGIN_HPP

#include <string>
#include <filesystem>
#include <utility>
#include <fcntl.h>

namespace cargo {
class FSPlugin {
public:
    enum class type {
        posix,
        parallel,
        none,
        gekkofs,
        hercules,
        expand,
        dataclay
    };
    static std::unique_ptr<FSPlugin> make_fs(type);

    virtual ~FSPlugin() = default;

    virtual int
    open(const std::string& path, int flags, unsigned int mode) = 0;
    virtual bool
    close(int fd) = 0;
    virtual ssize_t
    pread(int fd, void* buf, size_t count, off_t offset) = 0;
    virtual ssize_t
    pwrite(int fd, const void* buf, size_t count, off_t offset) = 0;
    virtual bool
    mkdir(const std::string& path, mode_t mode) = 0;
    virtual off_t
    lseek(int fd, off_t offset, int whence) = 0;
    virtual off_t
    fallocate(int fd, int mode, off_t offset, off_t len) = 0;
};
} // namespace cargo
#endif // FS_PLUGIN_HPP