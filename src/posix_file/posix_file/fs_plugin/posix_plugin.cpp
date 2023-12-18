#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "posix_plugin.hpp"

namespace cargo {

posix_plugin::posix_plugin() {
}


int
posix_plugin::open(const std::string& path, int flags, unsigned int mode) {
    return ::open(path.c_str(), flags, mode);
}

bool
posix_plugin::close(int fd) {
    return ::close(fd) == 0;
}

ssize_t
posix_plugin::pread(int fd, void* buf, size_t count, off_t offset) {
    return ::pread(fd, buf, count, offset);
}

ssize_t
posix_plugin::pwrite(int fd, const void* buf, size_t count, off_t offset) {
    return ::pwrite(fd, buf, count, offset);
}

bool
posix_plugin::mkdir(const std::string& path, mode_t mode) {
    return ::mkdir(path.c_str(), mode) == 0;
}

off_t
posix_plugin::lseek(int fd, off_t offset, int whence) {
    return ::lseek(fd, offset, whence);
}

off_t
posix_plugin::fallocate(int fd, int mode, off_t offset, off_t len) {
    (void) mode;
    return ::posix_fallocate(fd, offset, static_cast<off_t>(len));
}
}; // namespace cargo