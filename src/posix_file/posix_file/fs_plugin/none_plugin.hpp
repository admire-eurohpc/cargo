
#ifndef NONE_PLUGIN_HPP
#define NONE_PLUGIN_HPP

#include "fs_plugin.hpp"
#include <iostream>
namespace cargo {
class none_plugin : public FSPlugin {

public:
    none_plugin();

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
} // namespace cargo
#endif // NONE_PLUGIN_HPP
