
#include "fs_plugin.hpp"
#include "hercules_plugin.hpp"


#include <iostream>
namespace cargo {
hercules_plugin::hercules_plugin() {
    int result = hercules_init();
    if (result != 0) {
        std::cerr << "Failed to initialize hercules" << std::endl;
    }
}

hercules_plugin::~hercules_plugin() {
    int result = hercules_end();
    if (result != 0) {
        std::cerr << "Failed to finalize hercules" << std::endl;
    }
}
// Override the open function
int
hercules_plugin::open(const std::string& path, int flags, unsigned int mode) {
    return hercules_open(path, flags, mode);
}

// Override the pread function
ssize_t
hercules_plugin::pread(int fd, void* buf, size_t count, off_t offset) {
    return hercules_pread_ws(fd, buf, count, offset);
}

// Override the pwrite function
ssize_t
hercules_plugin::pwrite(int fd, const void* buf, size_t count, off_t offset) {
    return hercules_pwrite_ws(fd, buf, count, offset);
}


bool
hercules_plugin::mkdir(const std::string& path, mode_t mode) {
    int result = hercules_create(path, mode | S_IFDIR);
    return result;
}

bool
hercules_plugin::close(int fd) {
    return hercules_close(fd);
}

off_t
hercules_plugin::lseek(int fd, off_t offset, int whence) {
    return hercules_lseek(fd, offset, whence);
}

off_t
hercules_plugin::fallocate(int fd, int mode, off_t offset, off_t len) {
    (void) fd;
    (void) mode;
    (void) offset;
    (void) len;
    return len;
}
} // namespace cargo
