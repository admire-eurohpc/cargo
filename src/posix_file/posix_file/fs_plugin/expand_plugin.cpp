
#include "fs_plugin.hpp"
#include "expand_plugin.hpp"


#ifdef __cplusplus
extern "C" {
#endif

#include <xpn_client/xpn.h>

#ifdef __cplusplus
}
#endif


#include <iostream>
namespace cargo {
expand_plugin::expand_plugin() {
    int result = xpn_init();
    if (result != 0) {
        std::cerr << "Failed to initialize expand" << std::endl;
    }
}

expand_plugin::~expand_plugin() {
    int result = xpn_destroy();
    if (result != 0) {
        std::cerr << "Failed to finalize expand" << std::endl;
    }
}
// Override the open function
int
expand_plugin::open(const std::string& path, int flags, unsigned int mode) {
    return xpn_open(path.c_str(), flags, mode);
}

// Override the pread function
ssize_t
expand_plugin::pread(int fd, void* buf, size_t count, off_t offset) {
    xpn_lseek(fd, offset, SEEK_SET);
    return xpn_read(fd, buf, count);
}

// Override the pwrite function
ssize_t
expand_plugin::pwrite(int fd, const void* buf, size_t count, off_t offset) {
    xpn_lseek(fd, offset, SEEK_SET);
    return xpn_write(fd, buf, count);
}


bool
expand_plugin::mkdir(const std::string& path, mode_t mode) {
    int result = xpn_mkdir(path.c_str(), mode);
    return result;
}

bool
expand_plugin::close(int fd) {
    return xpn_close(fd);
}

off_t
expand_plugin::lseek(int fd, off_t offset, int whence) {
    return xpn_lseek(fd, offset, whence);
}

off_t
expand_plugin::fallocate(int fd, int mode, off_t offset, off_t len) {
    (void) fd;
    (void) mode;
    (void) offset;
    (void) len;
    return len;
}

int
expand_plugin::unlink(const std::string& path) {

    (void) path;
    std::cerr << "expand_plugin unlink not supported" << std::endl;
    return 0;
}


std::vector<std::string>
expand_plugin::readdir(const std::string& path) {
    (void) path;
    std::cerr << "expand_plugin readdir not supported" << std::endl;
    return {};
}

// stat
int
expand_plugin::stat(const std::string& path, struct stat* buf) {
    (void) path;
    (void) buf;
    std::cerr << "expand_plugin stat not supported" << std::endl;
    return 0;
}

ssize_t
expand_plugin::size(const std::string& path) {
    (void) path;
    std::cerr << "expand_plugin size not supported" << std::endl;
    return 0;
}
} // namespace cargo