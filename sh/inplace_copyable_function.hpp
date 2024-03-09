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

#ifndef INC_SH__INPLACE_COPYABLE_FUNCTION_HPP
#define INC_SH__INPLACE_COPYABLE_FUNCTION_HPP

/**	@file
 *	This file declares a std::function-like facility without defined behaviour
 *	if called while null that is stored in-place.
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
	/**	Type wrapper for inplace_copyable_function_vtable constructor.
	 *	@tparam Callable The callable type.
	 */
	template <typename Callable>
	struct inplace_copyable_function_callable final
	{
		/**	The callable type.
		 */
		using type = Callable;
	};

	/**	Table of functions to operate on inplace_copyable_function.
	 *	@tparam NoExcept True if the callable is nothrow.
	 *	@tparam ResultType The result of calling.
	 *	@tparam Args The arguments passed on call.
	 */
	template <bool NoExcept, typename ResultType, typename... Args>
	struct inplace_copyable_function_vtable final
	{
		using call_type = ResultType(*)(void* const, Args&&...) noexcept(NoExcept);
		using dtor_type = void(*)(void* const) noexcept;
		using copy_type = void(*)(void*, const void*);
		using move_type = void(*)(void* const, void* const) noexcept;

		/**	Calls the given storage.
		 */
		const call_type m_call;
		/**	Destructs the given storage.
		 */
		const dtor_type m_dtor;
		/**	Copies source storage into destination storage.
		 */
		const copy_type m_copy;
		/**	Moves source storage into destination storage and then destructs the source.
		 */
		const move_type m_move;

		/**	Construct a vtable for an empty inplace_copyable_function.
		 */
		explicit inplace_copyable_function_vtable(const std::nullptr_t) noexcept
			: m_call{ [](void* const storage, Args&&... args) noexcept(NoExcept) -> ResultType
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
			, m_dtor{ [](void* const storage) noexcept -> void
			{ } }
			, m_copy{ [](void* const dst_storage, const void* const src_storage) -> void
			{ } }
			, m_move{ [](void* const dst_storage, void* const src_storage) noexcept -> void
			{ } }
		{ }

		/**	Construct a vtable for a inplace_copyable_function with the given callable.
		 *	@tparam Callable The callable type.
		 */
		template <typename Callable>
		explicit inplace_copyable_function_vtable(const inplace_copyable_function_callable<Callable>&) noexcept
			: m_call{ [](void* const storage, Args&&... args) noexcept(NoExcept) -> ResultType
			{ 
				return (*static_cast<Callable*>(storage))(std::forward<Args>(args)...);
			} }
			, m_dtor{ [](void* const storage) noexcept -> void
			{
				reinterpret_cast<Callable*>(storage)->~Callable();
			} }
			, m_copy{ [](void* const dst_storage, const void* const src_storage) -> void
			{ 
				new(dst_storage) Callable(*static_cast<const Callable*>(src_storage));
			} }
			, m_move{ [](void* const dst_storage, void* const src_storage) noexcept -> void
			{
				static_assert(std::is_nothrow_move_constructible_v<Callable>, "Callable must be nothrow move constructible.");
				new(dst_storage) Callable{ std::move(*static_cast<Callable*>(src_storage)) };
				reinterpret_cast<Callable*>(src_storage)->~Callable();
			} }
		{ }

		inplace_copyable_function_vtable(const inplace_copyable_function_vtable&) = delete;
		inplace_copyable_function_vtable(inplace_copyable_function_vtable&&) = delete;
		inplace_copyable_function_vtable& operator=(const inplace_copyable_function_vtable&) = delete;
		inplace_copyable_function_vtable& operator=(inplace_copyable_function_vtable&&) = delete;
	};

} // namespace detail

/**	Implements a nullable, callable wrapper of an invocable that is stored in-place.
 *	@tparam Signature The function signature.
 *	@tparam Capacity The number of in-place storage bytes.
 *	@tparam Alignment The alighment of the in-place storage in bytes.
 */
template <typename Signature, std::size_t Capacity, std::size_t Alignment = alignof(void*)>
class inplace_copyable_function;

/**	Implements a nullable, callable wrapper of an invocable that is stored in-place.
 *	@tparam NoExcept True if this wraps a nothrow invocable and false otherwise.
 *	@tparam Capacity The number of in-place storage bytes.
 *	@tparam Alignment The alighment of the in-place storage in bytes.
 *	@tparam ResultType The result of invoking this.
 *	@tparam Args The arguments necessary to invoking this.
 */
template <bool NoExcept, std::size_t Capacity, std::size_t Alignment, typename ResultType, typename... Args>
class inplace_copyable_function <ResultType(Args...) noexcept(NoExcept), Capacity, Alignment>
{
public:
	using result_type = ResultType;
	static constexpr std::size_t capacity = Capacity;

	/**	Default constructor.
	 *	@detail calling results in undefined behavior.
	 */
	inplace_copyable_function() noexcept
		: m_vtable{ &null_vtable() }
	{ }
	/**	Null constructor.
	 *	@detail calling results in undefined behavior.
	 */
	inplace_copyable_function(const std::nullptr_t) noexcept
		: m_vtable{ &null_vtable() }
	{ }
	/**	Copy constructor.
	 *	@param other The inplace_copyable_function to copy into this.
	 */
	inplace_copyable_function(const inplace_copyable_function& other)
		: m_vtable{ other.m_vtable }
	{
		m_vtable->m_copy(&m_storage, &other.m_storage);
	}
	/**	Move constructor.
	 *	@param other The inplace_copyable_function to move into this.
	 */
	inplace_copyable_function(inplace_copyable_function&& other) noexcept
		: m_vtable{ std::exchange(other.m_vtable, &null_vtable()) }
	{
		m_vtable->m_move(&m_storage, &other.m_storage);
	}
	/**	Constructor from a given callable.
 	 *	@param callable An invocable to wrap and call from operator().
 	 *	@tparam Callable The type of the given invocable target.
	 */
	template <typename Callable,
		typename = std::enable_if_t<std::is_invocable_r_v<result_type, Callable, Args...>>>
	inplace_copyable_function(Callable&& callable)
	{
		static_assert(NoExcept == false || std::is_nothrow_invocable_r_v<result_type, Callable, Args...>, "inplace_copyable_function requires nothrow invocable.");
		using callable_type = std::decay_t<Callable>;
		static_assert(sizeof(callable_type) <= Capacity, "Callable too large for Capacity");
		m_vtable = &callable_vtable<callable_type>();
		new(&m_storage) callable_type{ std::forward<Callable>(callable) };
	}
	/**	Destructor.
	 */
	~inplace_copyable_function()
	{
		m_vtable->m_dtor(&m_storage);
	}

	/**	Copy assigment.
	 *	@param other The inplace_copyable_function to copy into this.
	 *	@return A reference to this.
	 */
	inplace_copyable_function& operator=(const inplace_copyable_function& other)
	{
		m_vtable->m_dtor(&m_storage);
		m_vtable->m_copy(&m_storage, &other.m_storage);
		return *this;
	}
	/**	Move assigment.
	 *	@param other The inplace_copyable_function to move into this.
	 *	@return A reference to this.
	 */
	inplace_copyable_function& operator=(inplace_copyable_function&& other) noexcept
	{
		assert(this != &other);
		m_vtable->m_dtor(&m_storage);
		m_vtable = std::exchange(other.m_vtable, &null_vtable());
		m_vtable->m_move(&m_storage, &other.m_storage);
		return *this;
	}
	/**	Null assignment.
	 *	@detail Afterwards, calling results in undefined behavior.
	 */
	inplace_copyable_function& operator=(const std::nullptr_t) noexcept
	{
		m_vtable->m_dtor(&m_storage);
		m_vtable = &null_vtable();
		return *this;
	}
	/**	Assign a given callable as the wrapped invocable.
 	 *	@param callable An invocable target to which this will hold a pointer and invoke upon operator().
	 *	@return A reference to this.
 	 *	@tparam Callable The type of the given invocable target.
	 */
	template <typename Callable,
		typename = std::enable_if_t<std::is_invocable_r_v<result_type, Callable, Args...>>>
	inplace_copyable_function& operator=(Callable&& callable) noexcept
	{
		static_assert(NoExcept == false || std::is_nothrow_invocable_r_v<result_type, Callable, Args...>, "inplace_copyable_function requires nothrow invocable.");
		using callable_type = std::decay_t<Callable>;
		static_assert(sizeof(callable_type) <= Capacity, "Callable too large for Capacity");
		m_vtable->m_dtor(&m_storage);
		m_vtable = &callable_vtable<callable_type>();
		new(&m_storage) callable_type{ std::forward<Callable>(callable) };
		return *this;
	}
	/**	Invoke the wrapped callable.
	 *	@detail If this inplace_copyable_function is null, undefined behavior will result.
	 *	@param args The arguments to pass to the pointed-to callable.
	 *	@tparam OperatorArgs The arguments to forward to m_vtable->m_call.
	 *	@return The result of invoking the pointed-to callable with args.
	 */
	template <typename... OperatorArgs>
	ResultType operator()(OperatorArgs&&... args) const noexcept(NoExcept)
	{
		assert(m_vtable != &null_vtable());
		return m_vtable->m_call(&m_storage, std::forward<OperatorArgs>(args)...);
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

	/**	Swap this with another inplace_copyable_function.
	 *	@param other The inplace_copyable_function with which to swap contents.
	 */
	void swap(inplace_copyable_function& other) noexcept
	{
		storage_type temp;
		m_vtable->m_move(&temp, &m_storage);
		other.m_vtable->m_move(&m_storage, &other.m_storage);
		m_vtable->m_move(&other.m_storage, &temp);
		std::swap(m_vtable, other.m_vtable);
	}
	/**	Swap the two given inplace_copyable_function objects.
	 *	@param lhs The inplace_copyable_function with which to swap contents with rhs.
	 *	@param rhs The inplace_copyable_function with which to swap contents with lhs.
	 */
	friend void swap(inplace_copyable_function& lhs, inplace_copyable_function& rhs) noexcept
	{
		lhs.swap(rhs);
	}

private:
	using vtable_type = detail::inplace_copyable_function_vtable<NoExcept, ResultType, Args...>;

	/**	Internal storage space object for inplace_copyable_function.
	 */
	struct alignas(Alignment) storage_type final
	{
		std::byte m_inplace[Capacity];
	};

	/**	A vtable that does operates upon storage containing the given callable type.
	 *	@return A reference to a static vtable for the given callable type.
	 *	@tparam Callable The callable type.
	 */
	template <typename Callable>
	static const vtable_type& callable_vtable() noexcept
	{
		static const vtable_type instance{ detail::inplace_copyable_function_callable<Callable>{} };
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
	mutable storage_type m_storage;
};

} // namespace sh

#endif
