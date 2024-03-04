#ifndef FS_PLUGIN_HPP
#define FS_PLUGIN_HPP

#include <string>
#include <vector>
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

    static std::shared_ptr<FSPlugin> make_fs(type);
    // One instance per fs type

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
    virtual std::vector<std::string>
    readdir(const std::string& path) = 0;
    virtual int
    unlink(const std::string& path) = 0;
    virtual int
    stat(const std::string& path, struct stat* buf) = 0;
};
} // namespace cargo
#endif // FS_PLUGIN_HPP