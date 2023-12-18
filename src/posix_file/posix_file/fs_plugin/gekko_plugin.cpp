
#include "fs_plugin.hpp"
#include "gekko_plugin.hpp"
#include <gkfs/user_functions.hpp>

#include <iostream>
namespace cargo {
gekko_plugin::gekko_plugin() {
    int result = gkfs_init();
    if (result != 0) {
        std::cerr << "Failed to initialize gekkofs" << std::endl;
    }
}

gekko_plugin::~gekko_plugin() {
    int result = gkfs_end();
    if (result != 0) {
        std::cerr << "Failed to finalize gekkofs" << std::endl;
    }
}
// Override the open function
int
gekko_plugin::open(const std::string& path, int flags, unsigned int mode) {
    // Call to gekkofs has the signature inverted
    return gkfs::syscall::gkfs_open(path, mode, flags);
}

// Override the pread function
ssize_t
gekko_plugin::pread(int fd, void* buf, size_t count, off_t offset) {
    return gkfs::syscall::gkfs_pread_ws(fd, buf, count, offset);
}

// Override the pwrite function
ssize_t
gekko_plugin::pwrite(int fd, const void* buf, size_t count, off_t offset) {
    return gkfs::syscall::gkfs_pwrite_ws(fd, buf, count, offset);
}


bool
gekko_plugin::mkdir(const std::string& path, mode_t mode) {
    int result = gkfs::syscall::gkfs_create(path, mode | S_IFDIR);
    return result;
}

bool
gekko_plugin::close(int fd) {
    return gkfs::syscall::gkfs_close(fd);
}

off_t
gekko_plugin::lseek(int fd, off_t offset, int whence) {
    return gkfs::syscall::gkfs_lseek(fd, offset, whence);
}

off_t
gekko_plugin::fallocate(int fd, int mode, off_t offset, off_t len) {
    (void) fd;
    (void) mode;
    (void) offset;
    (void) len;
    return len;
}
} // namespace cargo
