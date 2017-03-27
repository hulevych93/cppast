// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_variable.hpp>

#include <cppast/cpp_decltype_type.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_variable")
{
    auto code = R"(
// basic
int a;
unsigned long long b = 42;
float c = 3.f + 0.14f;

// with storage class specifiers
extern int d; // actually declaration
static int e;
thread_local int f;
thread_local static int g;

// constexpr
constexpr int h = 12;

// inline definition
struct foo {} i;
const struct bar {} j = bar();
struct baz {} k{};
static struct {} l;

// auto
auto m = 128;
const auto& n = m;

// decltype
decltype(0) o;
const decltype(o)& p = o;
)";

    cpp_entity_index idx;
    auto check_variable = [&](const cpp_variable& var, const cpp_type& type,
                              type_safe::optional_ref<const cpp_expression> default_value,
                              int storage_class, bool is_constexpr, bool is_declaration) {
        if (is_declaration)
        {
            REQUIRE(var.is_declaration());
            REQUIRE(!var.is_definition());
        }
        else
        {
            REQUIRE(var.is_definition());
            REQUIRE(!var.is_declaration());
        }

        REQUIRE(equal_types(idx, var.type(), type));
        if (var.default_value())
        {
            REQUIRE(default_value);
            REQUIRE(equal_expressions(var.default_value().value(), default_value.value()));
        }
        else
            REQUIRE(!default_value);

        REQUIRE(var.storage_class() == storage_class);
        REQUIRE(var.is_constexpr() == is_constexpr);
    };

    auto file = parse(idx, "cpp_variable.cpp", code);

    auto int_type = cpp_builtin_type::build("int");
    auto count    = test_visit<cpp_variable>(*file, [&](const cpp_variable& var) {
        if (var.name() == "a")
            check_variable(var, *int_type, nullptr, cpp_storage_class_none, false, false);
        else if (var.name() == "b")
            check_variable(var, *cpp_builtin_type::build("unsigned long long"),
                           // unexposed due to implicit cast, I think
                           type_safe::ref(
                               *cpp_unexposed_expression::build(cpp_builtin_type::build("int"),
                                                                "42")),
                           cpp_storage_class_none, false, false);
        else if (var.name() == "c")
            check_variable(var, *cpp_builtin_type::build("float"),
                           type_safe::ref(
                               *cpp_unexposed_expression::build(cpp_builtin_type::build("float"),
                                                                "3.f+0.14f")),
                           cpp_storage_class_none, false, false);
        else if (var.name() == "d")
            check_variable(var, *int_type, nullptr, cpp_storage_class_extern, false, true);
        else if (var.name() == "e")
            check_variable(var, *int_type, nullptr, cpp_storage_class_static, false, false);
        else if (var.name() == "f")
            check_variable(var, *int_type, nullptr, cpp_storage_class_thread_local, false, false);
        else if (var.name() == "g")
            check_variable(var, *int_type, nullptr,
                           cpp_storage_class_static | cpp_storage_class_thread_local, false, false);
        else if (var.name() == "h")
            check_variable(var, *cpp_cv_qualified_type::build(cpp_builtin_type::build("int"),
                                                              cpp_cv_const),
                           type_safe::ref(
                               *cpp_literal_expression::build(cpp_builtin_type::build("int"),
                                                              "12")),
                           cpp_storage_class_none, true, false);
        else if (var.name() == "i")
            check_variable(var,
                           *cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "foo")),
                           nullptr, cpp_storage_class_none, false, false);
        else if (var.name() == "j")
            check_variable(var, *cpp_cv_qualified_type::build(cpp_user_defined_type::build(
                                                                  cpp_type_ref(cpp_entity_id(""),
                                                                               "bar")),
                                                              cpp_cv_const),
                           type_safe::ref(
                               *cpp_unexposed_expression::build(cpp_user_defined_type::build(
                                                                    cpp_type_ref(cpp_entity_id(""),
                                                                                 "bar")),
                                                                "bar()")),
                           cpp_storage_class_none, false, false);
        else if (var.name() == "k")
            check_variable(var,
                           *cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "baz")),
                           type_safe::ref(
                               *cpp_unexposed_expression::build(cpp_user_defined_type::build(
                                                                    cpp_type_ref(cpp_entity_id(""),
                                                                                 "baz")),
                                                                "{}")),
                           cpp_storage_class_none, false, false);
        else if (var.name() == "l")
            check_variable(var, *cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "")),
                           nullptr, cpp_storage_class_static, false, false);
        else if (var.name() == "m")
            check_variable(var, *cpp_auto_type::build(),
                           type_safe::ref(
                               *cpp_literal_expression::build(cpp_builtin_type::build("int"),
                                                              "128")),
                           cpp_storage_class_none, false, false);
        else if (var.name() == "n")
            check_variable(var, *cpp_reference_type::
                                    build(cpp_cv_qualified_type::build(cpp_auto_type::build(),
                                                                       cpp_cv_const),
                                          cpp_ref_lvalue),
                           type_safe::ref(
                               *cpp_unexposed_expression::build(cpp_builtin_type::build("int"),
                                                                "m")),
                           cpp_storage_class_none, false, false);
        else if (var.name() == "o")
            check_variable(var,
                           *cpp_decltype_type::build(
                               cpp_literal_expression::build(cpp_builtin_type::build("int"), "0")),
                           nullptr, cpp_storage_class_none, false, false);
        else if (var.name() == "p")
            check_variable(var,
                           *cpp_reference_type::
                               build(cpp_cv_qualified_type::
                                         build(cpp_decltype_type::build(
                                                   cpp_unexposed_expression::
                                                       build(cpp_builtin_type::build("int"), "o")),
                                               cpp_cv_const),
                                     cpp_ref_lvalue),
                           type_safe::ref(
                               *cpp_unexposed_expression::build(cpp_builtin_type::build("int"),
                                                                "o")),
                           cpp_storage_class_none, false, false);
        else
            REQUIRE(false);
    });
    REQUIRE(count == 16u);
}

TEST_CASE("static cpp_variable")
{
    auto code = R"(
struct test
{
    static int a;
    static const int b;
    static thread_local int c;
};
)";

    auto file  = parse({}, "static_cpp_variable.cpp", code);
    auto count = test_visit<cpp_variable>(*file, [&](const cpp_variable& var) {
        REQUIRE(var.is_declaration());
        REQUIRE(!var.is_definition());
        REQUIRE(!var.default_value());
        REQUIRE(!var.is_constexpr());
        REQUIRE(is_static(var.storage_class()));

        if (var.name() == "a")
            REQUIRE(equal_types({}, var.type(), *cpp_builtin_type::build("int")));
        else if (var.name() == "b")
            REQUIRE(equal_types({}, var.type(),
                                *cpp_cv_qualified_type::build(cpp_builtin_type::build("int"),
                                                              cpp_cv_const)));
        else if (var.name() == "c")
        {
            REQUIRE(equal_types({}, var.type(), *cpp_builtin_type::build("int")));
            REQUIRE(is_thread_local(var.storage_class()));
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 3u);
}
