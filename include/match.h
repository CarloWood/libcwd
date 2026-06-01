// SPDX-FileCopyrightText: 2000-2004, 2026 Carlo Wood
// SPDX-License-Identifier: MIT

#pragma once

#ifndef MATCH_H
#define MATCH_H

namespace libcwd::_private_ {

extern bool match(char const* mask, size_t masklen, char const* name);

} // namespace libcwd::_private_

#endif // MATCH_H
