/*****************************************************************************

Copyright (c) 2025, Percona Inc.

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
using rapidjson::Document;
using rapidjson::SchemaDocument;
using rapidjson::SchemaValidator;

namespace storage::innobase::vec0vec {

bool validate_options(const Key_spec &index_def) {
  if (index_def.key_create_info.algorithm == HA_KEY_ALG_SE_SPECIFIC) {
    if (my_strcasecmp(system_charset_info,
                      index_def.key_create_info.comment.str, "HNSW") == 0) {
      for (const IndexConstructionParam &p : index_def.construction_params) {
        if (my_strcasecmp(system_charset_info, p.key.str, "M") == 0) {
          if (!std::all_of(p.value.str, p.value.str + p.value.length,
                           [](auto c) { return std::isdigit(c); })) {
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
    my_error(ER_INDEX_TYPE_NOT_SUPPORTED_FOR_VECTOR_INDEX, MYF(0),
             index_def.key_create_info.comment.str);
    return true;
  }

  Document schema_doc;
  schema_doc.Parse(schema_json);
  assert(!schema_doc.HasParseError());

  SchemaDocument schema(schema_doc);
  SchemaValidator validator(schema);

  auto opts =
      to_string_view(index_def.key_create_info.m_secondary_engine_attribute);
  Document doc;
  doc.Parse(opts.data(), opts.length());

  if (!doc.Accept(validator)) {
    rapidjson::StringBuffer sb;
    validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
    my_error(ER_INVALID_JSON_ATTRIBUTE, MYF(0),
             ("Validation failed for vector index options: " +
              string(validator.GetInvalidSchemaKeyword()))
                 .c_str(),
             sb.GetSize(), string(sb.GetString(), sb.GetSize()).c_str());
    return true;
  }

  return false;
}

}  // namespace storage::innobase::vec0vec
