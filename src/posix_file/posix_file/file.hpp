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

class file {

public:
    explicit file(std::filesystem::path filepath) noexcept
        : m_path(std::move(filepath)) {}

    file(std::filesystem::path filepath, int fd) noexcept
        : m_path(std::move(filepath)), m_handle(fd) {}

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

    tl::expected<void, std::error_code>
    fallocate(int mode, offset offset, std::size_t len) const noexcept {

        if(!m_handle) {
            return tl::make_unexpected(
                    std::error_code{EBADF, std::generic_category()});
        }

        int ret = ::fallocate(m_handle.native(), mode, offset,
                              static_cast<off_t>(len));

        if(ret == -1) {
            return tl::make_unexpected(
                    std::error_code{errno, std::generic_category()});
        }

        return {};
    }

    template <typename MemoryBuffer>
    tl::expected<std::size_t, std::error_code>
    pread(MemoryBuffer&& buf, offset offset, std::size_t size) const noexcept {

        assert(buf.size() >= size);

        if(!m_handle) {
            return tl::make_unexpected(
                    std::error_code{EBADF, std::generic_category()});
        }

        std::size_t bytes_read = 0;
        std::size_t bytes_left = size;

        while(bytes_read < size) {

            ssize_t n = ::pread(m_handle.native(), buf.data() + bytes_read,
                                bytes_left, offset + bytes_read);

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
                return tl::make_unexpected(
                        std::error_code{errno, std::generic_category()});
            }

            bytes_read += n;
            bytes_left -= n;
        }

        return bytes_read;
    }

    template <typename MemoryBuffer>
    tl::expected<std::size_t, std::error_code>
    pwrite(MemoryBuffer&& buf, offset offset, std::size_t size) const noexcept {

        assert(buf.size() >= size);

        if(!m_handle) {
            return tl::make_unexpected(
                    std::error_code{EBADF, std::generic_category()});
        }

        std::size_t bytes_written = 0;
        std::size_t bytes_left = size;

        while(bytes_written < size) {

            ssize_t n = ::pwrite(m_handle.native(), buf.data() + bytes_written,
                                 bytes_left, offset + bytes_written);

            if(n == -1) {
                // Interrupted by a signal, retry
                if(errno == EINTR) {
                    continue;
                }

                // Some other error condition, report
                return tl::make_unexpected(
                        std::error_code{errno, std::generic_category()});
            }

            bytes_written += n;
            bytes_left -= n;
        }

        return bytes_written;
    }

protected:
    const std::filesystem::path m_path;
    file_handle m_handle;
};

static inline tl::expected<file, std::error_code>
open(const std::filesystem::path& filepath, int flags, ::mode_t mode = 0) {

    int fd = ::open(filepath.c_str(), flags, mode);

    if(fd == -1) {
        return tl::make_unexpected(
                std::error_code{errno, std::generic_category()});
    }

    return file{filepath, fd};
}

static inline tl::expected<file, std::error_code>
create(const std::filesystem::path& filepath, int flags, ::mode_t mode) {
    return open(filepath, O_CREAT | flags, mode);
}

} // namespace posix_file

#endif // POSIX_FILE_FILE_HPP
