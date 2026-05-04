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

#include "vec0vec.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <string>
#include <variant>

// ut0ut.h isn't self-contained.
#include "handler.h"
#include "lex_string.h"
#include "my_base.h"
#include "mysql/strings/m_ctype.h"
#include "mysqld_cs.h"
#include "ut0mem.h"
#include "ut0test.h"
#include "ut0ut.h"

#include <my_rapidjson_size_t.h>
#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>

#include "key_spec.h"
#include "my_sys.h"
#include "mysqld_error.h"

using namespace std;

namespace {
const char *alg_to_string(ha_key_alg alg) {
  switch (alg) {
    case HA_KEY_ALG_SE_SPECIFIC:
      assert(false);
      return nullptr;
    case HA_KEY_ALG_BTREE:
      return "BTREE";
    case HA_KEY_ALG_RTREE:
      return "RTREE";
    case HA_KEY_ALG_HASH:
      return "HASH";
    case HA_KEY_ALG_FULLTEXT:
      return "FULLTEXT";
    case HA_KEY_ALG_VECTOR:
      return "VECTOR";
  }

  assert(false);
  return nullptr;
}
}  // namespace

namespace storage::innobase::vec {

bool validate_options(const Key_spec &index_def) {
  VectorIndexParam vip;
  return parse_options(index_def, vip);
}

bool parse_options(const Key_spec &index_def, VectorIndexParam &vip) {
  assert(index_def.type == KEYTYPE_VECTOR);

  // prepare_key() will make sure there's only one column.
  // Possibly this check belongs there, too.
  if (index_def.columns[0]->get_prefix_length() != 0) {
    my_error(ER_WRONG_SUB_KEY, MYF(0));
    return true;
  }

  if (index_def.key_create_info.algorithm != HA_KEY_ALG_SE_SPECIFIC) {
    my_error(ER_INDEX_TYPE_NOT_SUPPORTED, MYF(0),
             alg_to_string(index_def.key_create_info.algorithm), "vector");
    return true;
  }

  if (index_def.key_create_info.vector_index_type.str == nullptr) {
    my_error(ER_NO_INDEX_TYPE, MYF(0), "");
    return true;
  }
  if (my_strcasecmp(system_charset_info,
                    index_def.key_create_info.vector_index_type.str,
                    "HNSW") == 0) {
    vip = HnswParam();
    auto &hnsw_param = std::get<HnswParam>(vip);
    for (const IndexConstructionParam &p : index_def.construction_params) {
      if (my_strcasecmp(system_charset_info, p.key.str, "M") == 0) {
        if (!std::all_of(p.value.str, p.value.str + p.value.length,
                         [](auto c) { return std::isdigit(c); })) {
          my_error(ER_ILLEGAL_INDEX_CONSTRUCTION_PARAMETER_VALUE, MYF(0),
                   p.value.str);
          return true;
        }
        hnsw_param.M = std::atoi(p.value.str);
      } else if (my_strcasecmp(system_charset_info, p.key.str, "metric") == 0) {
        bool is_valid = false;
        for (const char *metric : {"euclidean"}) {
          if (my_strcasecmp(system_charset_info, p.value.str, metric) == 0) {
            hnsw_param.metric = metric;
            is_valid = true;
            break;
          }
        }
        if (!is_valid) {
          my_error(ER_ILLEGAL_INDEX_CONSTRUCTION_PARAMETER_VALUE, MYF(0),
                   p.value.str);
          return true;
        }
      } else {
        my_error(ER_ILLEGAL_INDEX_CONSTRUCTION_PARAMETER, MYF(0), p.key.str);
        return true;
      }
    }
    return false;
  }
  my_error(ER_INDEX_TYPE_NOT_SUPPORTED, MYF(0),
           index_def.key_create_info.vector_index_type.str, "vector");
  return true;
}

}  // namespace storage::innobase::vec
