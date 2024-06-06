#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "none_plugin.hpp"

namespace cargo {

none_plugin::none_plugin() {}


int
none_plugin::open(const std::string& path, int flags, unsigned int mode) {
    (void) path;
    (void) flags;
    (void) mode;
    return 0;
}

bool
none_plugin::close(int fd) {
    (void) fd;
    return true;
}

ssize_t
none_plugin::pread(int fd, void* buf, size_t count, off_t offset) {
    (void) fd;
    (void) buf;
    (void) count;
    (void) offset;
    return count;
}

ssize_t
none_plugin::pwrite(int fd, const void* buf, size_t count, off_t offset) {
    (void) fd;
    (void) buf;
    (void) count;
    (void) offset;
    return count;
}

bool
none_plugin::mkdir(const std::string& path, mode_t mode) {
    (void) path;
    (void) mode;
    return true;
}

off_t
none_plugin::lseek(int fd, off_t offset, int whence) {
    (void) fd;
    (void) whence;
    return offset;
}

off_t
none_plugin::fallocate(int fd, int mode, off_t offset, off_t len) {
    (void) fd;
    (void) mode;
    (void) len;
    return offset;
}

std::vector<std::string>
none_plugin::readdir(const std::string& path) {
    (void) path;
    std::vector<std::string> files;
   
    return files;
}


int
none_plugin::unlink(const std::string& path) {
    (void) path;
    return 0;
}
int
none_plugin::stat(const std::string& path, struct stat* buf) {
    (void) path;
    (void) buf;
    return 0;
}

ssize_t
none_plugin::size(const std::string& path) {
    (void) path;
    return 0;
}

}; // namespace cargo