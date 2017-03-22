// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENTITY_INDEX_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_INDEX_HPP_INCLUDED

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <type_safe/optional_ref.hpp>
#include <type_safe/reference.hpp>
#include <type_safe/strong_typedef.hpp>

namespace cppast
{
    class cpp_entity;
    class cpp_namespace;

    /// \exclude
    namespace detail
    {
        constexpr std::size_t fnv_basis = 14695981039346656037ull;
        constexpr std::size_t fnv_prime = 1099511628211ull;

        // FNV-1a 64 bit hash
        constexpr std::size_t id_hash(const char* str, std::size_t hash = fnv_basis)
        {
            return *str ? id_hash(str + 1, (hash ^ *str) * fnv_prime) : hash;
        }
    } // namespace detail

    /// A [ts::strong_typedef]() representing the unique id of a [cppast::cpp_entity]().
    ///
    /// It is comparable for equality.
    struct cpp_entity_id : type_safe::strong_typedef<cpp_entity_id, std::size_t>,
                           type_safe::strong_typedef_op::equality_comparison<cpp_entity_id>
    {
        explicit cpp_entity_id(const std::string& str) : cpp_entity_id(str.c_str())
        {
        }

        explicit cpp_entity_id(const char* str) : strong_typedef(detail::id_hash(str))
        {
        }
    };

    inline namespace literals
    {
        /// \returns A new [cppast::cpp_entity_id]() created from the given string.
        inline cpp_entity_id operator""_id(const char* str, std::size_t)
        {
            return cpp_entity_id(str);
        }
    }

    /// An index of all [cppast::cpp_entity]() objects created.
    ///
    /// It maps [cppast::cpp_entity_id]() to references to the [cppast::cpp_entity]() objects.
    class cpp_entity_index
    {
    public:
        /// \effects Registers a new [cppast::cpp_entity]() which is a definition.
        /// It will override any previously registered declarations of the same entity.
        /// \requires If the entity has been registered before, it must be as declaration,
        /// and the entity must live as long as the index lives.
        /// \requires The entity must not be a namespace.
        /// \notes This operation is thread safe.
        void register_definition(cpp_entity_id                           id,
                                 type_safe::object_ref<const cpp_entity> entity) const;

        /// \effects Registers a new [cppast::cpp_entity]() which is a declaration.
        /// Only the first declaration will be registered.
        /// \requires The entity must live as long as the index lives.
        /// \requires The entity must not be a namespace.
        /// \notes This operation is thread safe.
        void register_forward_declaration(cpp_entity_id                           id,
                                          type_safe::object_ref<const cpp_entity> entity) const;

        /// \effects Registers a new [cppast::cpp_namespace]().
        /// \notes The namespace object must live as long as the index lives.
        /// \notes This operation is thread safe.
        void register_namespace(cpp_entity_id                              id,
                                type_safe::object_ref<const cpp_namespace> ns) const;

        /// \returns A [ts::optional_ref]() corresponding to the entity(/ies) of the given [cppast::cpp_entity_id]().
        /// If no definition has been registered, it return the first declaration that was registered.
        /// If the id resolves to a namespaces, returns an empty optional.
        /// \notes This operation is thread safe.
        type_safe::optional_ref<const cpp_entity> lookup(const cpp_entity_id& id) const noexcept;

        /// \returns A [ts::optional_ref]() corresponding to the entity of the given [cppast::cpp_entity_id]().
        /// If no definition has been registered, it returns an empty optional.
        /// \notes This operation is thread safe.
        type_safe::optional_ref<const cpp_entity> lookup_definition(const cpp_entity_id& id) const
            noexcept;

        /// \returns A [ts::array_ref]() of references to all namespaces matching the given [cppast::cpp_entity_id]().
        /// If no namespace is found, it returns an empty array reference.
        /// \notes This operation is thread safe.
        auto lookup_namespace(const cpp_entity_id& id) const noexcept
            -> type_safe::array_ref<type_safe::object_ref<const cpp_namespace>>;

    private:
        struct hash
        {
            std::size_t operator()(const cpp_entity_id& id) const noexcept
            {
                return static_cast<std::size_t>(id);
            }
        };

        struct value
        {
            type_safe::object_ref<const cpp_entity> entity;
            bool                                    is_definition;

            value(type_safe::object_ref<const cpp_entity> e, bool def)
            : entity(std::move(e)), is_definition(def)
            {
            }
        };

        mutable std::mutex mutex_;
        mutable std::unordered_map<cpp_entity_id, value, hash> map_;
        mutable std::unordered_map<cpp_entity_id,
                                   std::vector<type_safe::object_ref<const cpp_namespace>>, hash>
            ns_;
    };
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_INDEX_HPP_INCLUDED
