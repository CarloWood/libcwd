// SPDX-FileCopyrightText: 2000-2004, 2018, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef EXEC_PROG_H
#define EXEC_PROG_H

namespace libcwd {

extern int ST_exec_prog(char const* prog_name, char const* const argv[], char const* const envp[], int (*decode_stdout)(char const*, size_t));

} // namespace libcwd

#endif // EXEC_PROG_H
