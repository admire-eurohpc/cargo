
#ifndef DATACLAY_PLUGIN_HPP
#define DATACLAY_PLUGIN_HPP

#include "fs_plugin.hpp"

namespace cargo {
class dataclay_plugin : public FSPlugin {

public:
    dataclay_plugin();
    ~dataclay_plugin();
    int
    open(const std::string& path, int flags, unsigned int mode) final;
    bool
    close(int fd) final;
    ssize_t
    pread(int fd, void* buf, size_t count, off_t offset) final;
    ssize_t
    pwrite(int fd, const void* buf, size_t count, off_t offset) final;
    bool
    mkdir(const std::string& path, mode_t mode) final;
    off_t
    lseek(int fd, off_t offset, int whence) final;
    // Fallocate is not needed in dataclay as pwrite takes care of it.
    off_t
    fallocate(int fd, int mode, off_t offset, off_t len) final;
    std::vector<std::string>
    readdir(const std::string& path) final;
    int
    unlink(const std::string& path) final;
    int
    stat(const std::string& path, struct stat* buf) final;
    ssize_t
    size(const std::string& path) final;
};
}; // namespace cargo

#endif // dataclay_PLUGIN_HPP
