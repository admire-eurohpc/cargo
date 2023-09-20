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

#include <system_error>
#include <boost/mpi/error_string.hpp>
#include "cargo/error.hpp"

// clang-format off
#define EXPAND(s) case s: return #s
// clang-format on

constexpr std::string_view
errno_name(int ec) {
    switch(ec) {
        EXPAND(EPERM);
        EXPAND(ENOENT);
        EXPAND(ESRCH);
        EXPAND(EINTR);
        EXPAND(EIO);
        EXPAND(ENXIO);
        EXPAND(E2BIG);
        EXPAND(ENOEXEC);
        EXPAND(EBADF);
        EXPAND(ECHILD);
        EXPAND(EAGAIN);
        EXPAND(ENOMEM);
        EXPAND(EACCES);
        EXPAND(EFAULT);
        EXPAND(ENOTBLK);
        EXPAND(EBUSY);
        EXPAND(EEXIST);
        EXPAND(EXDEV);
        EXPAND(ENODEV);
        EXPAND(ENOTDIR);
        EXPAND(EISDIR);
        EXPAND(EINVAL);
        EXPAND(ENFILE);
        EXPAND(EMFILE);
        EXPAND(ENOTTY);
        EXPAND(ETXTBSY);
        EXPAND(EFBIG);
        EXPAND(ENOSPC);
        EXPAND(ESPIPE);
        EXPAND(EROFS);
        EXPAND(EMLINK);
        EXPAND(EPIPE);
        EXPAND(EDOM);
        EXPAND(ERANGE);
        EXPAND(EDEADLK);
        EXPAND(ENAMETOOLONG);
        EXPAND(ENOLCK);
        EXPAND(ENOSYS);
        EXPAND(ENOTEMPTY);
        EXPAND(ELOOP);
        // EXPAND(EWOULDBLOCK);
        EXPAND(ENOMSG);
        EXPAND(EIDRM);
        EXPAND(ECHRNG);
        EXPAND(EL2NSYNC);
        EXPAND(EL3HLT);
        EXPAND(EL3RST);
        EXPAND(ELNRNG);
        EXPAND(EUNATCH);
        EXPAND(ENOCSI);
        EXPAND(EL2HLT);
        EXPAND(EBADE);
        EXPAND(EBADR);
        EXPAND(EXFULL);
        EXPAND(ENOANO);
        EXPAND(EBADRQC);
        EXPAND(EBADSLT);
        // EXPAND(EDEADLOCK);
        EXPAND(EBFONT);
        EXPAND(ENOSTR);
        EXPAND(ENODATA);
        EXPAND(ETIME);
        EXPAND(ENOSR);
        EXPAND(ENONET);
        EXPAND(ENOPKG);
        EXPAND(EREMOTE);
        EXPAND(ENOLINK);
        EXPAND(EADV);
        EXPAND(ESRMNT);
        EXPAND(ECOMM);
        EXPAND(EPROTO);
        EXPAND(EMULTIHOP);
        EXPAND(EDOTDOT);
        EXPAND(EBADMSG);
        EXPAND(EOVERFLOW);
        EXPAND(ENOTUNIQ);
        EXPAND(EBADFD);
        EXPAND(EREMCHG);
        EXPAND(ELIBACC);
        EXPAND(ELIBBAD);
        EXPAND(ELIBSCN);
        EXPAND(ELIBMAX);
        EXPAND(ELIBEXEC);
        EXPAND(EILSEQ);
        EXPAND(ERESTART);
        EXPAND(ESTRPIPE);
        EXPAND(EUSERS);
        EXPAND(ENOTSOCK);
        EXPAND(EDESTADDRREQ);
        EXPAND(EMSGSIZE);
        EXPAND(EPROTOTYPE);
        EXPAND(ENOPROTOOPT);
        EXPAND(EPROTONOSUPPORT);
        EXPAND(ESOCKTNOSUPPORT);
        EXPAND(EOPNOTSUPP);
        EXPAND(EPFNOSUPPORT);
        EXPAND(EAFNOSUPPORT);
        EXPAND(EADDRINUSE);
        EXPAND(EADDRNOTAVAIL);
        EXPAND(ENETDOWN);
        EXPAND(ENETUNREACH);
        EXPAND(ENETRESET);
        EXPAND(ECONNABORTED);
        EXPAND(ECONNRESET);
        EXPAND(ENOBUFS);
        EXPAND(EISCONN);
        EXPAND(ENOTCONN);
        EXPAND(ESHUTDOWN);
        EXPAND(ETOOMANYREFS);
        EXPAND(ETIMEDOUT);
        EXPAND(ECONNREFUSED);
        EXPAND(EHOSTDOWN);
        EXPAND(EHOSTUNREACH);
        EXPAND(EALREADY);
        EXPAND(EINPROGRESS);
        EXPAND(ESTALE);
        EXPAND(EUCLEAN);
        EXPAND(ENOTNAM);
        EXPAND(ENAVAIL);
        EXPAND(EISNAM);
        EXPAND(EREMOTEIO);
        EXPAND(EDQUOT);
        EXPAND(ENOMEDIUM);
        EXPAND(EMEDIUMTYPE);
        EXPAND(ECANCELED);
        EXPAND(ENOKEY);
        EXPAND(EKEYEXPIRED);
        EXPAND(EKEYREVOKED);
        EXPAND(EKEYREJECTED);
        EXPAND(EOWNERDEAD);
        EXPAND(ENOTRECOVERABLE);
        EXPAND(ERFKILL);
        EXPAND(EHWPOISON);
        default:
            return "EUNKNOWN";
    }
}

constexpr std::string_view
mpi_error_name(int ec) {

    switch(ec) {
        EXPAND(MPI_SUCCESS);
        EXPAND(MPI_ERR_BUFFER);
        EXPAND(MPI_ERR_COUNT);
        EXPAND(MPI_ERR_TYPE);
        EXPAND(MPI_ERR_TAG);
        EXPAND(MPI_ERR_COMM);
        EXPAND(MPI_ERR_RANK);
        EXPAND(MPI_ERR_REQUEST);
        EXPAND(MPI_ERR_ROOT);
        EXPAND(MPI_ERR_GROUP);
        EXPAND(MPI_ERR_OP);
        EXPAND(MPI_ERR_TOPOLOGY);
        EXPAND(MPI_ERR_DIMS);
        EXPAND(MPI_ERR_ARG);
        EXPAND(MPI_ERR_UNKNOWN);
        EXPAND(MPI_ERR_TRUNCATE);
        EXPAND(MPI_ERR_OTHER);
        EXPAND(MPI_ERR_INTERN);
        EXPAND(MPI_ERR_IN_STATUS);
        EXPAND(MPI_ERR_PENDING);
        EXPAND(MPI_ERR_ACCESS);
        EXPAND(MPI_ERR_AMODE);
        EXPAND(MPI_ERR_ASSERT);
        EXPAND(MPI_ERR_BAD_FILE);
        EXPAND(MPI_ERR_BASE);
        EXPAND(MPI_ERR_CONVERSION);
        EXPAND(MPI_ERR_DISP);
        EXPAND(MPI_ERR_DUP_DATAREP);
        EXPAND(MPI_ERR_FILE_EXISTS);
        EXPAND(MPI_ERR_FILE_IN_USE);
        EXPAND(MPI_ERR_FILE);
        EXPAND(MPI_ERR_INFO_KEY);
        EXPAND(MPI_ERR_INFO_NOKEY);
        EXPAND(MPI_ERR_INFO_VALUE);
        EXPAND(MPI_ERR_INFO);
        EXPAND(MPI_ERR_IO);
        EXPAND(MPI_ERR_KEYVAL);
        EXPAND(MPI_ERR_LOCKTYPE);
        EXPAND(MPI_ERR_NAME);
        EXPAND(MPI_ERR_NO_MEM);
        EXPAND(MPI_ERR_NOT_SAME);
        EXPAND(MPI_ERR_NO_SPACE);
        EXPAND(MPI_ERR_NO_SUCH_FILE);
        EXPAND(MPI_ERR_PORT);
        EXPAND(MPI_ERR_QUOTA);
        EXPAND(MPI_ERR_READ_ONLY);
        EXPAND(MPI_ERR_RMA_CONFLICT);
        EXPAND(MPI_ERR_RMA_SYNC);
        EXPAND(MPI_ERR_SERVICE);
        EXPAND(MPI_ERR_SIZE);
        EXPAND(MPI_ERR_SPAWN);
        EXPAND(MPI_ERR_UNSUPPORTED_DATAREP);
        EXPAND(MPI_ERR_UNSUPPORTED_OPERATION);
        EXPAND(MPI_ERR_WIN);
        EXPAND(MPI_T_ERR_MEMORY);
        EXPAND(MPI_T_ERR_NOT_INITIALIZED);
        EXPAND(MPI_T_ERR_CANNOT_INIT);
        EXPAND(MPI_T_ERR_INVALID_INDEX);
        EXPAND(MPI_T_ERR_INVALID_ITEM);
        EXPAND(MPI_T_ERR_INVALID_HANDLE);
        EXPAND(MPI_T_ERR_OUT_OF_HANDLES);
        EXPAND(MPI_T_ERR_OUT_OF_SESSIONS);
        EXPAND(MPI_T_ERR_INVALID_SESSION);
        EXPAND(MPI_T_ERR_CVAR_SET_NOT_NOW);
        EXPAND(MPI_T_ERR_CVAR_SET_NEVER);
        EXPAND(MPI_T_ERR_PVAR_NO_STARTSTOP);
        EXPAND(MPI_T_ERR_PVAR_NO_WRITE);
        EXPAND(MPI_T_ERR_PVAR_NO_ATOMIC);
        EXPAND(MPI_ERR_RMA_RANGE);
        EXPAND(MPI_ERR_RMA_ATTACH);
        EXPAND(MPI_ERR_RMA_FLAVOR);
        EXPAND(MPI_ERR_RMA_SHARED);
        EXPAND(MPI_T_ERR_INVALID);
        EXPAND(MPI_T_ERR_INVALID_NAME);
        default:
            return "MPI_ERR_UNKNOWN";
    }
}

namespace cargo {

[[nodiscard]] std::string_view
error_code::name() const {

    switch(m_category) {
        case error_category::generic_error:
            break;
        case error_category::system_error:
            return errno_name(static_cast<int>(m_value));
        case error_category::mpi_error:
            return mpi_error_name(static_cast<int>(m_value));
        default:
            return "CARGO_UNKNOWN_ERROR";
    }

    switch(m_value) {
        case error_value::success:
            return "CARGO_SUCCESS";
        case error_value::snafu:
            return "CARGO_SNAFU";
        case error_value::not_implemented:
            return "CARGO_NOT_IMPLEMENTED";
        default:
            return "CARGO_UNKNOWN_ERROR";
    }
};

[[nodiscard]] std::string
error_code::message() const {

    switch(m_category) {
        case error_category::generic_error:
            switch(m_value) {
                case error_value::success:
                    return "success";
                case error_value::snafu:
                    return "snafu";
                case error_value::not_implemented:
                    return "not implemented";
                default:
                    return "unknown error";
            }
        case error_category::system_error: {
            std::error_code std_ec =
                    std::make_error_code(static_cast<std::errc>(m_value));
            return std_ec.message();
        }
        case error_category::mpi_error:
            return boost::mpi::error_string(static_cast<int>(m_value));
        default:
            return "unknown error category";
    }
}

} // namespace cargo