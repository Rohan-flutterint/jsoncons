// Copyright 2013-2025 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#include <jsoncons_ext/jsonschema/jsonschema.hpp>

#include <jsoncons/json.hpp>
#include <jsoncons/utility/byte_string.hpp>

#include <fstream>
#include <iostream>
#include <regex>
#include <catch/catch.hpp>

using jsoncons::json;
using jsoncons::ojson;
namespace jsonschema = jsoncons::jsonschema;

TEST_CASE("jsonschema validation report tests")
{
    json schema = json::parse(R"(
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "$id": "https://test.com/schema",
    "$defs": {
        "integer": {
            "type": "integer"
        },      
        "minimum": {
            "minimum": 5
        }      
    },
    "type" : "object",
    "properties" : {
        "passes" : true,
        "fails" : false,
        "refs" : {"$ref" : "#/$defs/integer"},
        "multi" : {
            "allOf" : [{"$ref" : "#/$defs/integer"},{"$ref" : "#/$defs/minimum"}]
        }
    }
}
    )");

    SECTION("Test 1")
    {
        ojson expected = ojson::parse(R"(
[
    {
        "valid": false,
        "evaluationPath": "/properties/fails",
        "schemaLocation": "https://test.com/schema#/properties/fails",
        "instanceLocation": "/fails",
        "error": "False schema always fails"
    }
]
        )");

        auto aset = jsoncons::make_alloc_set(std::allocator<char>(),std::allocator<char>());
        jsonschema::json_schema<json> compiled = jsonschema::make_json_schema(aset, schema);
    
        json data = json::parse(R"({"fails":"value"})");
    
        jsoncons::json_decoder<ojson> decoder;    
        compiled.validate(data, decoder);
        
        ojson output = decoder.get_result();
        CHECK(expected == output);
        //std::cout << pretty_print(output) << "\n";
    }
}

