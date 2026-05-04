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

#include <cassert>
#include <string>
#include <variant>

// ut0ut.h isn't self-contained.
#include "ut0mem.h"
#include "ut0test.h"
#include "ut0ut.h"

#include <my_rapidjson_size_t.h>
#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>

#include "my_sys.h"
#include "mysqld_error.h"

using namespace std;
using rapidjson::Document;
using rapidjson::SchemaDocument;
using rapidjson::SchemaValidator;

namespace storage::innobase::vec0vec {

bool validate_options(string_view opts) {
  Document schema_doc;
  schema_doc.Parse(schema_json);
  assert(!schema_doc.HasParseError());

  SchemaDocument schema(schema_doc);
  SchemaValidator validator(schema);

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

VectorIndexParam parse_options(string_view opts) {
  if (opts.empty()) {
    return std::monostate{};
  }

  Document schema_doc;
  schema_doc.Parse(schema_json);
  assert(!schema_doc.HasParseError());

  SchemaDocument schema(schema_doc);
  SchemaValidator validator(schema);

  Document doc;
  auto &gendoc = doc.Parse(opts.data(), opts.length());

  if (validate_options(opts)) {
    ib::warn(ER_IB_MSG_466) << "Vector table index options validation failed, "
                               "using default values. Options: "
                            << opts;
  }

  auto get_or_default = [&gendoc](const char *key, auto &member) {
    if (gendoc.HasMember(key)) {
      member = gendoc[key].GetInt();
    }
  };

  // since there's only HNSW for now.
  HnswParam param;

  get_or_default("M", param.M);
  get_or_default("max_elements", param.max_elements);
  get_or_default("ef_construction", param.ef_construction);

  return param;
}

}  // namespace storage::innobase::vec0vec