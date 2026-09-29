/*****************************************************************************

Copyright (c) 2026, Percona Inc.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2.0, as published by the
Free Software Foundation.

This program is designed to work with certain software (including
but not limited to OpenSSL) that is licensed under separate terms,
as designated in a particular file or component or in included license
documentation.  The authors of MySQL hereby grant you an additional
permission to link the program and your derivative works with the
separately licensed software that they have either included with
the program or referenced in the documentation.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License, version 2.0,
for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

*****************************************************************************/

#pragma once

#include <charconv>
#include <concepts>
#include <type_traits>

#include "lex_string.h"
#include "mysql/strings/m_ctype.h"
#include "mysqld_cs.h"
#include "vec0vec.h"

namespace storage::innobase::vec::detail {

// Parses a value of type T out of a LEX_CSTRING. Returns false on failure.
template <typename T>
bool parse_value(const LEX_CSTRING &value, T &out) = delete;

template <>
inline bool parse_value<int>(const LEX_CSTRING &value, int &out) {
  const auto *last = value.str + value.length;
  auto result = std::from_chars(value.str, last, out);
  return result.ptr == last && result.ec == std::errc();
}

template <>
inline bool parse_value<vector_constants::Metric>(
    const LEX_CSTRING &value, vector_constants::Metric &out) {
  const auto *m = vector_constants::metric_from_name({value.str, value.length});
  if (m == nullptr) return false;
  out = *m;
  return true;
}

/**
  A single named option, mapping the key string to a member of HnswParam.
  Tracks whether it has already been consumed, to reject duplicates.
*/
template <typename T>
struct Option {
  const char *name;
  T HnswParam::*member;
  // Called with the parsed value; returning false rejects it as kBadValue.
  // A plain function pointer rather than std::function so that Option stays
  // a literal type and an option table can be constexpr. type_identity_t makes
  // this a non-deduced context, so T comes from the member pointer alone and
  // a bare captureless lambda converts to the pointer instead of failing
  // deduction.
  std::type_identity_t<bool (*)(const T &)> validate = [](const T &) {
    return true;
  };
  bool is_used = false;

  bool matches(const LEX_CSTRING &key) const {
    return my_strcasecmp(system_charset_info, key.str, name) == 0;
  }
};

}  // namespace storage::innobase::vec::detail
