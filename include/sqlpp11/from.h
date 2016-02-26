/*
 * Copyright (c) 2013-2015, Roland Bock
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SQLPP_FROM_H
#define SQLPP_FROM_H

#include <sqlpp11/table_ref.h>
#include <sqlpp11/type_traits.h>
#include <sqlpp11/no_data.h>
#include <sqlpp11/interpretable_list.h>
#include <sqlpp11/interpret_tuple.h>
#include <sqlpp11/logic.h>
#include <sqlpp11/detail/sum.h>
#include <sqlpp11/policy_update.h>
#include <sqlpp11/dynamic_join.h>

namespace sqlpp
{
  // FROM DATA
  template <typename Database, typename Table>
  struct from_data_t
  {
    from_data_t(Table table) : _table(table)
    {
    }

    from_data_t(const from_data_t&) = default;
    from_data_t(from_data_t&&) = default;
    from_data_t& operator=(const from_data_t&) = default;
    from_data_t& operator=(from_data_t&&) = default;
    ~from_data_t() = default;

    Table _table;
    interpretable_list_t<Database> _dynamic_tables;
  };

  // FROM
  template <typename Database, typename Table>
  struct from_t
  {
    using _traits = make_traits<no_value_t, tag::is_from>;
    using _nodes = detail::type_vector<Table>;
    using _is_dynamic = is_database<Database>;

    // Data
    using _data_t = from_data_t<Database, Table>;

    // Member implementation with data and methods
    template <typename Policies>
    struct _impl_t
    {
      // workaround for msvc bug https://connect.microsoft.com/VisualStudio/Feedback/Details/2091069
      _impl_t() = default;
      _impl_t(const _data_t& data) : _data(data)
      {
      }

      template <typename DynamicJoin>
      void add(DynamicJoin dynamicJoin)
      {
        static_assert(_is_dynamic::value, "from::add() must not be called for static from()");
        static_assert(is_dynamic_join_t<DynamicJoin>::value, "invalid argument in from::add(), expected dynamic_join");
        using _known_tables = provided_tables_of<Table>;  // Hint: Joins contain more than one table
        // workaround for msvc bug https://connect.microsoft.com/VisualStudio/feedback/details/2173198
        //		using _known_table_names = detail::transform_set_t<name_of, _known_tables>;
        using _known_table_names = detail::make_name_of_set_t<_known_tables>;
        using _joined_tables = provided_tables_of<DynamicJoin>;
        using _joined_table_names = detail::make_name_of_set_t<_joined_tables>;
        static_assert(detail::is_disjunct_from<_joined_table_names, _known_table_names>::value,
                      "Must not use the same table name twice in from()");
        using _serialize_check = sqlpp::serialize_check_t<typename Database::_serializer_context_t, DynamicJoin>;
        _serialize_check::_();

        using ok = logic::all_t<_is_dynamic::value, is_table_t<DynamicJoin>::value, _serialize_check::type::value>;

        _add_impl(dynamicJoin, ok());  // dispatch to prevent compile messages after the static_assert
      }

    private:
      template <typename DynamicJoin>
      void _add_impl(DynamicJoin dynamicJoin, const std::true_type&)
      {
        return _data._dynamic_tables.emplace_back(from_table(dynamicJoin));
      }

      template <typename DynamicJoin>
      void _add_impl(DynamicJoin dynamicJoin, const std::false_type&);

    public:
      _data_t _data;
    };

    // Base template to be inherited by the statement
    template <typename Policies>
    struct _base_t
    {
      using _data_t = from_data_t<Database, Table>;

      // workaround for msvc bug https://connect.microsoft.com/VisualStudio/Feedback/Details/2091069
      template <typename... Args>
      _base_t(Args&&... args)
          : from{std::forward<Args>(args)...}
      {
      }

      _impl_t<Policies> from;
      _impl_t<Policies>& operator()()
      {
        return from;
      }
      const _impl_t<Policies>& operator()() const
      {
        return from;
      }

      template <typename T>
      static auto _get_member(T t) -> decltype(t.from)
      {
        return t.from;
      }

      // FIXME: We might want to check if we have too many tables define in the FROM
      using _consistency_check = consistent_t;
    };
  };

  struct no_from_t
  {
    using _traits = make_traits<no_value_t, tag::is_noop>;
    using _nodes = detail::type_vector<>;

    // Data
    using _data_t = no_data_t;

    // Member implementation with data and methods
    template <typename Policies>
    struct _impl_t
    {
      // workaround for msvc bug https://connect.microsoft.com/VisualStudio/Feedback/Details/2091069
      _impl_t() = default;
      _impl_t(const _data_t& data) : _data(data)
      {
      }

      _data_t _data;
    };

    // Base template to be inherited by the statement
    template <typename Policies>
    struct _base_t
    {
      using _data_t = no_data_t;

      // workaround for msvc bug https://connect.microsoft.com/VisualStudio/Feedback/Details/2091069
      template <typename... Args>
      _base_t(Args&&... args)
          : no_from{std::forward<Args>(args)...}
      {
      }

      _impl_t<Policies> no_from;
      _impl_t<Policies>& operator()()
      {
        return no_from;
      }
      const _impl_t<Policies>& operator()() const
      {
        return no_from;
      }

      template <typename T>
      static auto _get_member(T t) -> decltype(t.no_from)
      {
        return t.no_from;
      }

      using _database_t = typename Policies::_database_t;

      // workaround for msvc bug https://connect.microsoft.com/VisualStudio/Feedback/Details/2173269
      //	  template <typename... T>
      //	  using _check = logic::all_t<is_table_t<T>::value...>;
      template <typename... T>
      struct _check : logic::all_t<is_table_t<T>::value...>
      {
      };

      template <typename Check, typename T>
      using _new_statement_t = new_statement_t<Check::value, Policies, no_from_t, T>;

      using _consistency_check = consistent_t;

      template <typename Table>
      auto from(Table table) const -> _new_statement_t<_check<Table>, from_t<void, from_table_t<Table>>>
      {
        static_assert(_check<Table>::value, "argument is not a table or join in from()");
        return _from_impl<void>(_check<Table>{}, table);
      }

      template <typename Table>
      auto dynamic_from(Table table) const -> _new_statement_t<_check<Table>, from_t<_database_t, from_table_t<Table>>>
      {
        static_assert(not std::is_same<_database_t, void>::value,
                      "dynamic_from must not be called in a static statement");
        static_assert(_check<Table>::value, "argument is not a table or join in from()");
        return _from_impl<_database_t>(_check<Table>{}, table);
      }

    private:
      template <typename Database, typename Table>
      auto _from_impl(const std::false_type&, Table table) const -> bad_statement;

      template <typename Database, typename Table>
      auto _from_impl(const std::true_type&, Table table) const
          -> _new_statement_t<std::true_type, from_t<Database, from_table_t<Table>>>
      {
        static_assert(required_tables_of<from_t<Database, Table>>::size::value == 0,
                      "at least one table depends on another table in from()");

        static constexpr std::size_t _number_of_tables = detail::sum(provided_tables_of<Table>::size::value);
        using _unique_tables = provided_tables_of<Table>;
        using _unique_table_names = detail::make_name_of_set_t<_unique_tables>;
        static_assert(_number_of_tables == _unique_tables::size::value,
                      "at least one duplicate table detected in from()");
        static_assert(_number_of_tables == _unique_table_names::size::value,
                      "at least one duplicate table name detected in from()");

        return {static_cast<const derived_statement_t<Policies>&>(*this),
                from_data_t<Database, from_table_t<Table>>{from_table(table)}};
      }
    };
  };

  // Interpreters
  template <typename Context, typename Database, typename Table>
  struct serializer_t<Context, from_data_t<Database, Table>>
  {
    using _serialize_check = serialize_check_of<Context, Table>;
    using T = from_data_t<Database, Table>;

    static Context& _(const T& t, Context& context)
    {
      context << " FROM ";
      serialize(t._table, context);
      if (not t._dynamic_tables.empty())
      {
        context << ' ';
        interpret_list(t._dynamic_tables, ' ', context);
      }
      return context;
    }
  };

  template <typename T>
  auto from(T&& t) -> decltype(statement_t<void, no_from_t>().from(std::forward<T>(t)))
  {
    return statement_t<void, no_from_t>().from(std::forward<T>(t));
  }
}

#endif
