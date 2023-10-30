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


#ifndef MPIOXX_HPP
#define MPIOXX_HPP

#include <boost/mpi.hpp>
#include <boost/mpi/error_string.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <logger/logger.hpp>

// very simple RAII wrappers for some MPI types + utility functions

namespace mpioxx {

class io_error : public std::exception {

public:
    io_error(std::string_view fun, int ec) : m_fun(fun), m_error_code(ec) {}

    [[nodiscard]] std::uint32_t
    error_code() const noexcept {
        return m_error_code;
    }

    [[nodiscard]] const char*
    what() const noexcept override {
        m_message.assign(boost::mpi::error_string(m_error_code));
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

using offset = MPI_Offset;

enum file_open_mode : int {
    create = MPI_MODE_CREATE,
    rdonly = MPI_MODE_RDONLY,
    wronly = MPI_MODE_WRONLY,
    rdwr = MPI_MODE_RDWR,
    delete_on_close = MPI_MODE_DELETE_ON_CLOSE,
    unique_open = MPI_MODE_UNIQUE_OPEN,
    excl = MPI_MODE_EXCL,
    append = MPI_MODE_APPEND,
    sequential = MPI_MODE_SEQUENTIAL,
};


inline constexpr file_open_mode
operator&(file_open_mode a, file_open_mode b) {
    return file_open_mode(static_cast<int>(a) & static_cast<int>(b));
}

inline constexpr file_open_mode
operator|(file_open_mode a, file_open_mode b) {
    return file_open_mode(static_cast<int>(a) | static_cast<int>(b));
}

inline constexpr file_open_mode
operator^(file_open_mode a, file_open_mode b) {
    return file_open_mode(static_cast<int>(a) ^ static_cast<int>(b));
}

inline constexpr file_open_mode
operator~(file_open_mode a) {
    return file_open_mode(~static_cast<int>(a));
}

inline const file_open_mode&
operator|=(file_open_mode& a, file_open_mode b) {
    return a = a | b;
}

inline const file_open_mode&
operator&=(file_open_mode& a, file_open_mode b) {
    return a = a & b;
}

inline const file_open_mode&
operator^=(file_open_mode& a, file_open_mode b) {
    return a = a ^ b;
}

class file {

public:
    explicit file(const MPI_File& file) : m_file(file) {}

    ~file() {
        if(m_file != MPI_FILE_NULL) {
            close();
        }
    }

    operator MPI_File() const { // NOLINT
        return m_file;
    }

    static file
    open(const boost::mpi::communicator& comm,
         const std::filesystem::path& filepath, file_open_mode mode) {

        MPI_File result;

        // At this point we may face the possibility of an unexistent directory
        // The File open semantics will not create the directory and fail.
        // As the operation are done in the prolog, we may not been able to create
        // such directory in the parallel filesystem nor the adhoc fs.

        // We will create the needed directories if we are writing.

        if (mode == file_open_mode::wronly) {
            // Decompose the filepath and create the needed directories.
            const std::filesystem::path dir = filepath.parent_path();
            if (!std::filesystem::exists(dir)) {
                std::filesystem::create_directories(dir);
            }
        }

        if(const auto ec =
                   MPI_File_open(comm, filepath.c_str(), static_cast<int>(mode),
                                 MPI_INFO_NULL, &result);
           ec != MPI_SUCCESS) {
            throw io_error("MPI_File_open", ec);
        }

        return file{result};
    }

    void
    close() {
        if(const auto ec = MPI_File_close(&m_file); ec != MPI_SUCCESS) {
            throw io_error("MPI_File_close", ec);
        }
    }

    [[nodiscard]] offset
    size() const {
        MPI_Offset result;
        if(const auto ec = MPI_File_get_size(m_file, &result);
           ec != MPI_SUCCESS) {
            throw io_error("MPI_File_get_size", ec);
        }
        return result;
    }

private:
    MPI_File m_file = MPI_FILE_NULL;
};


} // namespace mpioxx

#endif // MPIOXX_HPP
