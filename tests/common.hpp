#ifndef CARGO_TESTS_COMMON_HPP
#define CARGO_TESTS_COMMON_HPP

#include <vector>
#include <cargo.hpp>
#include <unistd.h>

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

std::vector<cargo::dataset>
prepare_datasets(cargo::dataset::type type, const std::string& pattern,
                 size_t n);

#endif // CARGO_TESTS_COMMON_HPP
