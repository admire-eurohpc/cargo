/******************************************************************************
 * Copyright 2022-2023, Barcelona Supercomputing Center (BSC), Spain
 *
 * This software was partially supported by the EuroHPC-funded project ADMIRE
 *   (Project ID: 956748, https://www.admire-eurohpc.eu).
 *
 * This file is part of Cargo.
 *
 * Cargo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cargo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cargo.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *****************************************************************************/

#ifndef POSIX_FILE_FILE_HPP
#define POSIX_FILE_FILE_HPP

#include "types.hpp"

#include <filesystem>
#include <utility>
#include <fcntl.h>
#include <tl/expected.hpp>
#include "fs_plugin/fs_plugin.hpp"
#include "cargo.hpp"
#include <iostream>
extern "C" {
#include <unistd.h>
};

namespace posix_file {

class file_handle {

private:
    constexpr static const int init_value{-1}; ///< initial file descriptor

    int m_fd{init_value}; ///< file descriptor

public:
    file_handle() = default;

    explicit file_handle(int fd) noexcept : m_fd(fd) {}

    file_handle(file_handle&& rhs) noexcept {
        this->m_fd = rhs.m_fd;
        rhs.m_fd = init_value;
    }

    file_handle(const file_handle& other) = delete;

    file_handle&
    operator=(file_handle&& rhs) noexcept {
        this->m_fd = rhs.m_fd;
        rhs.m_fd = init_value;
        return *this;
    }

    file_handle&
    operator=(const file_handle& other) = delete;

    explicit operator bool() const noexcept {
        return valid();
    }

    bool
    operator!() const noexcept {
        return !valid();
    }

    /**
     * @brief Checks for valid file descriptor value.
     * @return boolean if valid file descriptor
     */
    [[nodiscard]] bool
    valid() const noexcept {
        return m_fd != init_value;
    }

    /**
     * @brief Retusn the file descriptor value used in this file handle
     * operation.
     * @return file descriptor value
     */
    [[nodiscard]] int
    native() const noexcept {
        return m_fd;
    }

    /**
     * @brief Closes file descriptor and resets it to initial value
     * @return boolean if file descriptor was successfully closed
     */
    bool
    close() noexcept {
        if(m_fd != init_value) {
            if(::close(m_fd) < 0) {
                return false;
            }
        }
        m_fd = init_value;
        return true;
    }

    /**
     * @brief Destructor implicitly closes the internal file descriptor.
     */
    ~file_handle() {
        if(m_fd != init_value) {
            close();
        }
    }
};

class io_error : public std::exception {

public:
    io_error(std::string_view fun, int ec) : m_fun(fun), m_error_code(ec) {}

    [[nodiscard]] std::uint32_t
    error_code() const noexcept {
        return m_error_code;
    }

    [[nodiscard]] const char*
    what() const noexcept override {
        m_message.assign(
                std::make_error_code(static_cast<std::errc>(m_error_code))
                        .message());
        return m_message.c_str();
    }

    [[nodiscard]] std::string_view
    where() const noexcept {
        return m_fun;
    }

private:
    mutable std::string m_message;
    std::string_view m_fun;
    int m_error_code;
};

class file {


public:
    file(cargo::FSPlugin::type t) {
        m_fs_plugin = cargo::FSPlugin::make_fs(t);
    };

    explicit file(std::filesystem::path filepath) noexcept
        : m_path(std::move(filepath)) {}

    file(std::filesystem::path filepath, int fd,
         std::unique_ptr<cargo::FSPlugin> fs_plugin) noexcept
        : m_path(std::move(filepath)), m_handle(fd),
          m_fs_plugin(std::move(fs_plugin)) {}


    std::filesystem::path
    path() const noexcept {
        return m_path;
    }

    posix_file::offset
    eof() const noexcept {
        return static_cast<posix_file::offset>(size());
    }

    std::size_t
    size() const noexcept {
        return std::filesystem::file_size(m_path);
    }

    auto
    remove() noexcept {
        return std::filesystem::remove(m_path);
    }

    void
    fallocate(int mode, offset offset, std::size_t len) const {

        if(!m_handle) {
            throw io_error("posix_file::file::fallocate (handle)", EBADF);
        }

        int ret = m_fs_plugin->fallocate(m_handle.native(), mode, offset,
                              static_cast<off_t>(len));

        if(ret == -1) {
            throw io_error("posix_file::file::fallocate", errno);
        }
    }

    template <typename MemoryBuffer>
    std::size_t
    pread(MemoryBuffer&& buf, offset offset, std::size_t size) const {

        assert(buf.size() >= size);

        if(!m_handle) {
            throw io_error("posix_file::file::pread", EBADF);
        }

        std::size_t bytes_read = 0;
        std::size_t bytes_left = size;

        while(bytes_read < size) {

            ssize_t n = m_fs_plugin->pread(m_handle.native(),
                                           buf.data() + bytes_read, bytes_left,
                                           offset + bytes_read);

            if(n == 0) {
                // EOF
                return 0;
            }

            if(n == -1) {
                // Interrupted by a signal, retry
                if(errno == EINTR) {
                    continue;
                }

                // Some other error condition, report
                throw io_error("posix_file::file::pread", errno);
            }

            bytes_read += n;
            bytes_left -= n;
        }

        return bytes_read;
    }

    template <typename MemoryBuffer>
    std::size_t
    pwrite(MemoryBuffer&& buf, offset offset, std::size_t size) const {

        assert(buf.size() >= size);

        if(!m_handle) {
            throw io_error("posix_file::file::pwrite", EBADF);
        }

        std::size_t bytes_written = 0;
        std::size_t bytes_left = size;

        while(bytes_written < size) {

            ssize_t n = m_fs_plugin->pwrite(m_handle.native(),
                                            buf.data() + bytes_written,
                                            bytes_left, offset + bytes_written);

            if(n == -1) {
                // Interrupted by a signal, retry
                if(errno == EINTR) {
                    continue;
                }

                // Some other error condition, report
                throw io_error("posix_file::file::pwrite", errno);
            }

            bytes_written += n;
            bytes_left -= n;
        }

        return bytes_written;
    }


protected:
    const std::filesystem::path m_path;
    file_handle m_handle;
    std::unique_ptr<cargo::FSPlugin> m_fs_plugin;
};


static inline file
open(const std::filesystem::path& filepath, int flags, ::mode_t mode,
     cargo::FSPlugin::type t) {

    std::unique_ptr<cargo::FSPlugin> fs_plugin;

    fs_plugin = cargo::FSPlugin::make_fs(t);
    // We don't check if it exists, we just create it if flags is set to O_CREAT

    if(flags & O_CREAT) {
        fs_plugin->mkdir(filepath.parent_path().c_str(), 0755);
    }
    std::cout << "Opening file " << filepath << std::endl;
    int fd = fs_plugin->open(filepath.c_str(), flags, mode);
    std::cout << "File opened? " << fd << " -- " << flags << " mode: " << mode << std::endl;
    if(fd == -1) {
        throw io_error("posix_file::open_gekko", errno);
    }

    return file{filepath, fd, std::move(fs_plugin)};
}

static inline file
create(const std::filesystem::path& filepath, int flags, ::mode_t mode,
       cargo::FSPlugin::type t) {
    return open(filepath, O_CREAT | flags, mode, t);
}

} // namespace posix_file

#endif // POSIX_FILE_FILE_HPP
