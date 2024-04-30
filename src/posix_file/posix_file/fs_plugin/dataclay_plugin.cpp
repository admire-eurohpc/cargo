
#include "fs_plugin.hpp"
#include "dataclay_plugin.hpp"
extern "C" {
#include <dataclay-plugin/dataclayplugin.h>
}
#include <iostream>
namespace cargo {
dataclay_plugin::dataclay_plugin() {
    ::dataclay_plugin("cargo", DataClay_PATH);
    std::cout << "dataclay_plugin loaded" << std::endl;
}

dataclay_plugin::~dataclay_plugin() {}
// Override the open function
int
dataclay_plugin::open(const std::string& path, int flags, unsigned int mode) {
    return dataclay_open((char *)path.c_str(), flags, mode);
}

// Override the pread function
ssize_t
dataclay_plugin::pread(int fd, void* buf, size_t count, off_t offset) {
    return dataclay_pread(fd, (char*) buf, count, offset);
}

// Override the pwrite function
ssize_t
dataclay_plugin::pwrite(int fd, const void* buf, size_t count, off_t offset) {
    return dataclay_pwrite(fd, (char*) buf, count, offset);
}


bool
dataclay_plugin::mkdir(const std::string& path, mode_t mode) {
    (void) path;
    (void) mode;
    return true; // We don't have directories
}

bool
dataclay_plugin::close(int fd) {
    dataclay_close(fd);
    return true;
}

off_t
dataclay_plugin::lseek(int fd, off_t offset, int whence) {
    (void) fd;
    (void) offset;
    (void) whence;
    std::cerr << "dataclay_plugin lseek not supported" << std::endl;
    return 0;
}

off_t
dataclay_plugin::fallocate(int fd, int mode, off_t offset, off_t len) {
    (void) fd;
    (void) mode;
    (void) offset;
    (void) len;
    return len;
}

int
dataclay_plugin::unlink(const std::string& path) {

    (void) path;
    std::cerr << "dataclay_plugin unlink not supported" << std::endl;
    return 0;
}


std::vector<std::string>
dataclay_plugin::readdir(const std::string& path) {
    (void) path;
    std::cerr << "dataclay_plugin readdir not supported" << std::endl;
    return {};
}

// stat
int
dataclay_plugin::stat(const std::string& path, struct stat* buf) {
    (void) path;
    (void) buf;
    std::cerr << "dataclay_plugin stat not supported" << std::endl;
    return 0;
}

ssize_t
dataclay_plugin::size(const std::string& path) {
    (void) path;
    std::cerr << "dataclay_plugin size not supported" << std::endl;
    return 0;
}


} // namespace cargo
