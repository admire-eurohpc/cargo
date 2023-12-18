
#include "fs_plugin.hpp"
#include "expand_plugin.hpp"


#include <iostream>
namespace cargo {
expand_plugin::expand_plugin() {
    int result = expand_init();
    if (result != 0) {
        std::cerr << "Failed to initialize expand" << std::endl;
    }
}

expand_plugin::~expand_plugin() {
    int result = expand_end();
    if (result != 0) {
        std::cerr << "Failed to finalize expand" << std::endl;
    }
}
// Override the open function
int
expand_plugin::open(const std::string& path, int flags, unsigned int mode) {
    return expand_open(path, flags, mode);
}

// Override the pread function
ssize_t
expand_plugin::pread(int fd, void* buf, size_t count, off_t offset) {
    return expand_pread_ws(fd, buf, count, offset);
}

// Override the pwrite function
ssize_t
expand_plugin::pwrite(int fd, const void* buf, size_t count, off_t offset) {
    return expand_pwrite_ws(fd, buf, count, offset);
}


bool
expand_plugin::mkdir(const std::string& path, mode_t mode) {
    int result = expand_create(path, mode | S_IFDIR);
    return result;
}

bool
expand_plugin::close(int fd) {
    return expand_close(fd);
}

off_t
expand_plugin::lseek(int fd, off_t offset, int whence) {
    return expand_lseek(fd, offset, whence);
}

off_t
expand_plugin::fallocate(int fd, int mode, off_t offset, off_t len) {
    (void) fd;
    (void) mode;
    (void) offset;
    (void) len;
    return len;
}
} // namespace cargo
