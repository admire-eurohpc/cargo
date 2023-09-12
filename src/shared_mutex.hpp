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


#include <cassert>
#include <vector>
#include <abt.h>
#include <fmt/format.h>

#ifndef CARGO_ABT_SHARED_MUTEX_HPP
#define CARGO_ABT_SHARED_MUTEX_HPP

namespace cargo::abt {

#define ABT_RWLOCK_ASSERT(__expr)                                              \
    {                                                                          \
        if(const auto ret = (__expr); ret != ABT_SUCCESS) {                    \
            size_t n;                                                          \
            ABT_error_get_str(ret, NULL, &n);                                  \
            std::vector<char> tmp;                                             \
            tmp.reserve(n + 1);                                                \
            ABT_error_get_str(ret, tmp.data(), &n);                            \
                                                                               \
            throw std::runtime_error(fmt::format("{} failed: {} in {}:{}",     \
                                                 __FUNCTION__, tmp.data(),     \
                                                 ret, __FILE__, __LINE__));    \
        }                                                                      \
    }

class shared_mutex {
public:
    explicit shared_mutex() {
        ABT_RWLOCK_ASSERT(ABT_rwlock_create(&m_lock));
    }

    ~shared_mutex() noexcept {
        ABT_rwlock_free(&m_lock);
    }

    // copy constructor and copy assignment operator are disabled
    shared_mutex(const shared_mutex&) = delete;

    shared_mutex(shared_mutex&& rhs) noexcept {
        m_lock = rhs.m_lock;
        rhs.m_lock = ABT_RWLOCK_NULL;
    }

    shared_mutex&
    operator=(const shared_mutex&) = delete;

    shared_mutex&
    operator=(shared_mutex&& other) noexcept {

        if(this == &other) {
            return *this;
        }

        [[maybe_unused]] const auto ret = ABT_rwlock_free(&m_lock);
        assert(ret == ABT_SUCCESS);
        m_lock = other.m_lock;
        other.m_lock = ABT_RWLOCK_NULL;

        return *this;
    }


    // Exclusive ownership

    void
    lock() {
        ABT_RWLOCK_ASSERT(ABT_rwlock_wrlock(m_lock));
    }

    void
    unlock() {
        ABT_RWLOCK_ASSERT(ABT_rwlock_unlock(m_lock));
    }

    // Shared ownership

    void
    lock_shared() {
        ABT_RWLOCK_ASSERT(ABT_rwlock_rdlock(m_lock));
    }

    void
    unlock_shared() {
        ABT_RWLOCK_ASSERT(ABT_rwlock_unlock(m_lock));
    }

private:
    ABT_rwlock m_lock = ABT_RWLOCK_NULL;
};

#undef ABT_RWLOCK_ASSERT


/// unique_lock
template <typename Mutex>
class unique_lock {
public:
    typedef Mutex mutex_type;

    unique_lock() noexcept : m_device(0), m_owns(false) {}

    explicit unique_lock(mutex_type& m)
        : m_device(std::addressof(m)), m_owns(false) {
        lock();
        m_owns = true;
    }

    ~unique_lock() {
        if(m_owns)
            unlock();
    }

    unique_lock(const unique_lock&) = delete;
    unique_lock&
    operator=(const unique_lock&) = delete;

    unique_lock(unique_lock&& u) noexcept
        : m_device(u.m_device), m_owns(u.m_owns) {
        u.m_device = 0;
        u.m_owns = false;
    }

    unique_lock&
    operator=(unique_lock&& u) noexcept {
        if(m_owns)
            unlock();

        unique_lock(std::move(u)).swap(*this);

        u.m_device = 0;
        u.m_owns = false;

        return *this;
    }

    void
    lock() {
        if(!m_device) {
            throw std::system_error(int(std::errc::operation_not_permitted),
                                    std::system_category());
        } else if(m_owns) {
            throw std::system_error(
                    int(std::errc::resource_deadlock_would_occur),
                    std::system_category());
        } else {
            m_device->lock();
            m_owns = true;
        }
    }

    void
    unlock() {
        if(!m_owns) {
            throw std::system_error(int(std::errc::operation_not_permitted),
                                    std::system_category());
        } else if(m_device) {
            m_device->unlock();
            m_owns = false;
        }
    }

    void
    swap(unique_lock& u) noexcept {
        std::swap(m_device, u.m_device);
        std::swap(m_owns, u.m_owns);
    }

    mutex_type*
    release() noexcept {
        mutex_type* ret = m_device;
        m_device = 0;
        m_owns = false;
        return ret;
    }

    bool
    owns_lock() const noexcept {
        return m_owns;
    }

    explicit operator bool() const noexcept {
        return owns_lock();
    }

    mutex_type*
    mutex() const noexcept {
        return m_device;
    }

private:
    mutex_type* m_device;
    bool m_owns;
};

/// Swap overload for unique_lock objects.
/// @relates unique_lock
template <typename Mutex>
inline void
swap(unique_lock<Mutex>& x, unique_lock<Mutex>& y) noexcept {
    x.swap(y);
}

/// shared_lock
template <typename Mutex>
class shared_lock {
public:
    typedef Mutex mutex_type;

    // Shared locking

    shared_lock() noexcept : m_device(nullptr), m_owns(false) {}

    explicit shared_lock(mutex_type& m)
        : m_device(std::addressof(m)), m_owns(true) {
        m.lock_shared();
    }

    ~shared_lock() {
        if(m_owns) {
            m_device->unlock_shared();
        }
    }

    shared_lock(shared_lock const&) = delete;
    shared_lock&
    operator=(shared_lock const&) = delete;

    shared_lock(shared_lock&& sl) noexcept : shared_lock() {
        swap(sl);
    }

    shared_lock&
    operator=(shared_lock&& sl) noexcept {
        shared_lock(std::move(sl)).swap(*this);
        return *this;
    }

    void
    lock() {
        lockable();
        m_device->lock_shared();
        m_owns = true;
    }

    void
    unlock() {
        if(!m_owns) {
            throw std::system_error(
                    int(std::errc::resource_deadlock_would_occur),
                    std::system_category());
        }
        m_device->unlock_shared();
        m_owns = false;
    }

    // Setters

    void
    swap(shared_lock& u) noexcept {
        std::swap(m_device, u.m_device);
        std::swap(m_owns, u.m_owns);
    }

    mutex_type*
    release() noexcept {
        m_owns = false;
        return std::exchange(m_device, nullptr);
    }

    // Getters

    bool
    owns_lock() const noexcept {
        return m_owns;
    }

    explicit operator bool() const noexcept {
        return m_owns;
    }

    mutex_type*
    mutex() const noexcept {
        return m_device;
    }

private:
    void
    lockable() const {
        if(m_device == nullptr) {
            throw std::system_error(int(std::errc::operation_not_permitted),
                                    std::system_category());
        }
        if(m_owns) {
            throw std::system_error(
                    int(std::errc::resource_deadlock_would_occur),
                    std::system_category());
        }
    }

    mutex_type* m_device;
    bool m_owns;
};

/// Swap specialization for shared_lock
/// @relates shared_mutex
template <typename Mutex>
void
swap(shared_lock<Mutex>& x, shared_lock<Mutex>& y) noexcept {
    x.swap(y);
}

} // namespace cargo::abt

#endif // CARGO_ABT_SHARED_MUTEX_HPP
