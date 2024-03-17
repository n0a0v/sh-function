/*	BSD 3-Clause License

	Copyright (c) 2024, Paul Varga
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	   contributors may be used to endorse or promote products derived from
	   this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef INC_SH__MOVE_ONLY_FUNCTION_HPP
#define INC_SH__MOVE_ONLY_FUNCTION_HPP

/**	@file
 *	This file declares a std::function-like facility, without defined behavior
 *	if called while null, that may only be moved.
 */

#include <cassert>
#include <cstddef>
#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

namespace sh
{

namespace detail
{
	/**	Type wrapper for move_only_function_vtable constructor.
	 *	@tparam Callable The callable type.
	 */
	template <typename Callable>
	struct move_only_function_callable final
	{
		/**	The callable type.
		 */
		using type = Callable;
	};

	/**	Internal storage space object for move_only_function.
	 */
	union move_only_function_storage final
	{
		constexpr static std::size_t capacity = sizeof(void*) * 2;
		constexpr static std::size_t alignment = alignof(void*);

		/**	Return true if the provided type can be stored in-place.
		 *	@detail If the type is too large, it must be stored in externally
		 *	allocated memory. If the type is not nothrow move constructible,
		 *	move_only_function cannot assume that it's safe to move and remain
		 *	itself nothrow movable, hence it will likewise require storaging in
		 *	externally allocate memory.
		 */
		template <typename Callable>
		constexpr static bool store_inplace()
		{
			return sizeof(Callable) <= capacity && std::is_nothrow_move_constructible_v<Callable>;
		}

		alignas(alignment) std::byte m_inplace[capacity];
		void* m_allocated;
	};

	/**	Table of functions to operate on move_only_function.
	 *	@tparam NoExcept True if the callable is nothrow.
	 *	@tparam ResultType The result of calling.
	 *	@tparam Args The arguments passed on call.
	 */
	template <bool NoExcept, typename ResultType, typename... Args>
	struct move_only_function_vtable final
	{
		using call_type = ResultType(*)(move_only_function_storage&, Args&&...) noexcept(NoExcept);
		using dtor_type = void(*)(move_only_function_storage&) noexcept;
		using move_type = void(*)(move_only_function_storage&, move_only_function_storage&) noexcept;

		/**	Calls the given storage.
		 */
		const call_type m_call;
		/**	Destructs the given storage.
		 */
		const dtor_type m_dtor;
		/**	Moves source storage into destination storage and then destructs the source.
		 */
		const move_type m_move;

		/**	Construct a vtable for an empty move_only_function.
		 */
		explicit move_only_function_vtable(const std::nullptr_t) noexcept
			: m_call{ [](move_only_function_storage& storage, Args&&... args) noexcept(NoExcept) -> ResultType
			{
				// Undefined behavior "defined" here:
				if constexpr (NoExcept)
				{
					std::terminate();
				}
				else
				{
					throw std::bad_function_call();
				}
			} }
			, m_dtor{ [](move_only_function_storage& storage) noexcept -> void
			{ } }
			, m_move{ [](move_only_function_storage& dst_storage, move_only_function_storage& src_storage) noexcept -> void
			{ } }
		{ }

		/**	Construct a vtable for a move_only_function with the given callable.
		 *	@tparam Callable The callable type.
		 */
		template <typename Callable>
		explicit move_only_function_vtable(const move_only_function_callable<Callable>&) noexcept
			: m_call{ [](move_only_function_storage& storage, Args&&... args) noexcept(NoExcept) -> ResultType
			{ 
				if constexpr (move_only_function_storage::store_inplace<Callable>())
				{
					return (*reinterpret_cast<Callable*>(&storage.m_inplace))(std::forward<Args>(args)...);
				}
				else
				{
					return (*static_cast<Callable*>(storage.m_allocated))(std::forward<Args>(args)...);
				}
			} }
			, m_dtor{ [](move_only_function_storage& storage) noexcept -> void
			{
				if constexpr (move_only_function_storage::store_inplace<Callable>())
				{
					reinterpret_cast<Callable*>(&storage.m_inplace)->~Callable();
				}
				else
				{
					delete static_cast<Callable*>(storage.m_allocated);
				}
			} }
			, m_move{ [](move_only_function_storage& dst_storage, move_only_function_storage& src_storage) noexcept -> void
			{
				if constexpr (move_only_function_storage::store_inplace<Callable>())
				{
					static_assert(std::is_nothrow_move_constructible_v<Callable>, "Callable must be nothrow move constructible.");
					new(&dst_storage.m_inplace) Callable{ std::move(reinterpret_cast<Callable&>(src_storage.m_inplace)) };
					reinterpret_cast<Callable&>(src_storage.m_inplace).~Callable();
				}
				else
				{
					dst_storage.m_allocated = src_storage.m_allocated;
				}
			} }
		{ }

		move_only_function_vtable(const move_only_function_vtable&) = delete;
		move_only_function_vtable(move_only_function_vtable&&) = delete;
		move_only_function_vtable& operator=(const move_only_function_vtable&) = delete;
		move_only_function_vtable& operator=(move_only_function_vtable&&) = delete;
	};

	/**	Implements a nullable, callable wrapper of an invocable that may only be moved.
	 *	@note Required as MSVC does not support deduction of function signature noexcept in template specialization.
	 *	@tparam NoExcept True if this wraps a nothrow invocable and false otherwise.
	 *	@tparam ResultType The result of invoking this.
	 *	@tparam Args The arguments necessary to invoking this.
	 */
	template <bool NoExcept, typename ResultType, typename... Args>
	class move_only_function
	{
	public:
		using result_type = ResultType;

		move_only_function(const move_only_function&) = delete;
		move_only_function& operator=(const move_only_function&) = delete;

		/**	Default constructor.
		 *	@detail calling results in undefined behavior.
		 */
		move_only_function() noexcept
			: m_vtable{ &null_vtable() }
		{ }
		/**	Null constructor.
		 *	@detail calling results in undefined behavior.
		 */
		move_only_function(const std::nullptr_t) noexcept
			: m_vtable{ &null_vtable() }
		{ }
		/**	Move constructor.
		 *	@param other The move_only_function to move into this.
		 */
		move_only_function(move_only_function&& other) noexcept
			: m_vtable{ std::exchange(other.m_vtable, &null_vtable()) }
		{
			m_vtable->m_move(m_storage, other.m_storage);
		}
		/**	Constructor from a given callable.
		 *	@param callable An invocable to wrap and call from operator().
		 *	@tparam Callable The type of the given invocable target.
		 */
		template <typename Callable,
			typename = std::enable_if_t<std::is_invocable_r_v<result_type, Callable, Args...>>>
		move_only_function(Callable&& callable)
		{
			static_assert(NoExcept == false || std::is_nothrow_invocable_r_v<result_type, Callable, Args...>, "move_only_function requires nothrow invocable.");
			using callable_type = std::decay_t<Callable>;
			m_vtable = &callable_vtable<callable_type>();
			if constexpr (detail::move_only_function_storage::store_inplace<callable_type>())
			{
				new(&m_storage.m_inplace) callable_type{ std::forward<Callable>(callable) };
			}
			else
			{
				m_storage.m_allocated = new callable_type{ std::forward<Callable>(callable) };
			}
		}
		/**	Destructor.
		 */
		~move_only_function()
		{
			m_vtable->m_dtor(m_storage);
		}

		/**	Move assigment.
		 *	@param other The move_only_function to move into this.
		 *	@return A reference to this.
		 */
		move_only_function& operator=(move_only_function&& other) noexcept
		{
			assert(this != &other);
			m_vtable->m_dtor(m_storage);
			m_vtable = std::exchange(other.m_vtable, &null_vtable());
			m_vtable->m_move(m_storage, other.m_storage);
			return *this;
		}
		/**	Assign a given callable as the wrapped invocable.
		 *	@param callable An invocable target to which this will hold a pointer and invoke upon operator().
		 *	@return A reference to this.
		 *	@tparam Callable The type of the given invocable target.
		 */
		template <typename Callable,
			typename = std::enable_if_t<std::is_invocable_r_v<result_type, Callable, Args...>>>
		move_only_function& operator=(Callable&& callable) noexcept
		{
			static_assert(NoExcept == false || std::is_nothrow_invocable_r_v<result_type, Callable, Args...>, "move_only_function requires nothrow invocable.");
			using callable_type = std::decay_t<Callable>;
			m_vtable->m_dtor(m_storage);
			m_vtable = &callable_vtable<callable_type>();
			if constexpr (detail::move_only_function_storage::store_inplace<callable_type>())
			{
				new(&m_storage.m_inplace) callable_type{ std::forward<Callable>(callable) };
			}
			else
			{
				m_storage.m_allocated = new callable_type{ std::forward<Callable>(callable) };
			}
			return *this;
		}
		/**	Null assignment.
		 *	@detail Afterwards, calling results in undefined behavior.
		 */
		move_only_function& operator=(const std::nullptr_t) noexcept
		{
			m_vtable->m_dtor(m_storage);
			m_vtable = &null_vtable();
			return *this;
		}
		/**	Invoke the wrapped callable.
		 *	@detail If this move_only_function is null, undefined behavior will result.
		 *	@param args The arguments to pass to the pointed-to callable.
		 *	@tparam OperatorArgs The arguments to forward to m_vtable->m_call.
		 *	@return The result of invoking the pointed-to callable with args.
		 */
		template <typename... OperatorArgs>
		ResultType operator()(OperatorArgs&&... args) const noexcept(NoExcept)
		{
			assert(m_vtable != &null_vtable());
			return m_vtable->m_call(m_storage, std::forward<OperatorArgs>(args)...);
		}
		/**	Test if this is callable.
		 *	@return True if this is non-null and callable via operator().
		 */
		constexpr explicit operator bool() const noexcept
		{
			return m_vtable != &null_vtable();
		}
		/**	Test if this is null.
		 *	@detail True if this is null and calling operator() will result in undefined behavior.
		 */
		constexpr bool operator==(std::nullptr_t) const noexcept
		{
			return m_vtable == &null_vtable();
		}
		/**	Test if this is non-null.
		 *	@return True if this is non-null and callable via operator().
		 */
		constexpr bool operator!=(std::nullptr_t) const noexcept
		{
			return m_vtable != &null_vtable();
		}

		/**	Swap this with another move_only_function.
		 *	@param other The move_only_function with which to swap contents.
		 */
		void swap(move_only_function& other) noexcept
		{
			detail::move_only_function_storage temp;
			m_vtable->m_move(temp, m_storage);
			other.m_vtable->m_move(m_storage, other.m_storage);
			m_vtable->m_move(other.m_storage, temp);
			std::swap(m_vtable, other.m_vtable);
		}
		/**	Swap the two given move_only_function objects.
		 *	@param lhs The move_only_function with which to swap contents with rhs.
		 *	@param rhs The move_only_function with which to swap contents with lhs.
		 */
		friend void swap(move_only_function& lhs, move_only_function& rhs) noexcept
		{
			lhs.swap(rhs);
		}

	private:
		using vtable_type = detail::move_only_function_vtable<NoExcept, ResultType, Args...>;

		/**	A vtable that does operates upon storage containing the given callable type.
		 *	@return A reference to a static vtable for the given callable type.
		 *	@tparam Callable The callable type.
		 */
		template <typename Callable>
		static const vtable_type& callable_vtable() noexcept
		{
			static const vtable_type instance{ detail::move_only_function_callable<Callable>{} };
			return instance;
		}

		/**	A "null" vtable that does not operate upon storage. If called, will result in undefined behavior.
		 *	@return A reference to a static "null" vtable.
		 */
		static const vtable_type& null_vtable() noexcept
		{
			const static vtable_type instance{ nullptr };
			return instance;
		}

		/**	A table of function to call, destroy, or move the invocable stored in m_storage.
		 */
		const vtable_type* m_vtable;

		/**	A stored callable that can be operated upon by passing to m_vtable's functions.
		 */
		mutable detail::move_only_function_storage m_storage;
	};

} // namespace detail

/**	Implements a nullable, callable wrapper of an invocable that may only be moved.
 *	@tparam Signature The function signature.
 */
template <typename Signature>
class move_only_function;

/**	Implements a nullable, callable wrapper of an invocable that may only be moved.
 *	@tparam ResultType The result of calling this.
 *	@tparam Args The arguments necessary to call this.
 */
template <typename ResultType, typename... Args>
class move_only_function <ResultType(Args...)> : public detail::move_only_function<false, ResultType, Args...>
{
public:
	using detail::move_only_function<false, ResultType, Args...>::move_only_function;
};

/**	Implements a nullable, callable wrapper of a nothrow invocable that may only be moved.
 *	@tparam ResultType The result of calling this.
 *	@tparam Args The arguments necessary to call this.
 */
template <typename ResultType, typename... Args>
class move_only_function <ResultType(Args...) noexcept> : public detail::move_only_function<true, ResultType, Args...>
{
public:
	using detail::move_only_function<true, ResultType, Args...>::move_only_function;
};

} // namespace sh

#endif
