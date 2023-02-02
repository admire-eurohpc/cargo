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

#ifndef UTILS_C_PTR_HPP
#define UTILS_C_PTR_HPP

#include <memory>
#include <vector>

namespace utils {

// A manager for a C raw pointer. It allows to automatically delete dynamically
// allocated C structs in a RAII manner (provided that there is a deleter
// function for the struct available).
template <typename T, auto fn>
struct deleter {
    void
    operator()(T* ptr) {
        fn(ptr);
    }
};

template <typename T, auto fn>
using ctype_ptr =
        std::unique_ptr<typename std::remove_pointer<T>::type,
                        deleter<typename std::remove_pointer<T>::type, fn>>;

// A manager for a vector of C raw pointers. It allows to automatically
// delete the dynamically allocated C structs pointed by each vector elements
// in a RAII manner (provided that there is a deleter function for the struct
// available). Can also be used to directly pass an array of C pointers to C
// APIs by means of the data() function.
template <typename T, auto fn>
struct ctype_ptr_vector {

    ctype_ptr_vector() = default;

    ctype_ptr_vector(T* const data, size_t size) {
        reserve(size);

        for(size_t i = 0; i < size; ++i) {
            emplace_back(data[i]);
        }
    }

    ctype_ptr_vector(ctype_ptr_vector&& rhs) noexcept
        : m_data(std::move(rhs.m_data)), m_addrs(std::move(rhs.m_addrs)) {}

    ctype_ptr_vector&
    operator=(ctype_ptr_vector&&) noexcept = default;

    ~ctype_ptr_vector() = default;

    constexpr void
    reserve(size_t n) {
        m_data.reserve(n);
        m_addrs.reserve(n);
    }

    template <typename... Args>
    constexpr void
    emplace_back(Args&&... args) {
        const auto& tmp = m_data.emplace_back(args...);
        m_addrs.push_back(tmp.get());
    }

    constexpr const T*
    data() const noexcept {
        return m_addrs.data();
    }

    constexpr T*
    data() noexcept {
        return m_addrs.data();
    }

    constexpr std::size_t
    size() const noexcept {
        return m_data.size();
    }

    constexpr T*
    release() noexcept {

        auto* data = (T*) calloc(m_data.size(), sizeof(T));

        for(size_t i = 0; i < m_data.size(); ++i) {
            data[i] = m_data[i].release();
            m_addrs[i] = nullptr;
        }

        return data;
    }


    std::vector<utils::ctype_ptr<T, fn>> m_data{};
    std::vector<T> m_addrs{};
};

} // namespace utils

#endif // UTILS_C_PTR_HPP
