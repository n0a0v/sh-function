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

#ifndef INC_SH__FUNCTION_PTR_HPP
#define INC_SH__FUNCTION_PTR_HPP

/**	@file
 *	This file declares a function pointer-like facility with type erasure.
 */

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace sh
{

namespace detail
{
	/**	A function at which to point m_invoke_target to invoke the given target argument as a Callable type object.
	 *	@param target A pointer value from m_target to cast to Callable.
	 *	@param args The arguments to pass to m_target.
	 *	@return The result of invoking Callable.
	 *	@tparam Callable The type to which to cast target in order to invoke it.
	 *	@tparam NoExcept True if targeting a nothrow invocable and false otherwise.
	 *	@tparam ResultType The result of calling this.
	 *	@tparam Args The arguments necessary to call this.
	 */
	template <typename Callable, bool NoExcept, typename ResultType, typename... Args>
	ResultType function_ptr_invoke_target(void* const target, Args... args) noexcept(NoExcept)
	{
		// Clang 15 had issues with making this a member of function_ptr and inheriting the template parameters.
		assert(target != nullptr);
		return std::invoke(
			*reinterpret_cast<Callable>(target),
			std::forward<Args>(args)...);
	}

	/**	Implements a nullable, callable pointer to an invocable target.
	 *	@note Required as MSVC does not support deduction of function signature noexcept in template specialization.
	 *	@tparam NoExcept True if this targets a nothrow invocable and false otherwise.
	 *	@tparam ResultType The result of calling this.
	 *	@tparam Args The arguments necessary to call this.
	 */
	template <bool NoExcept, typename ResultType, typename... Args>
	class function_ptr
	{
	public:
		using result_type = ResultType;

		/**	Default constructor.
		 *	@detail No target is assigned and calling results in undefined behavior.
		 */
		constexpr function_ptr() noexcept
			: m_target{ nullptr }
		{ }
		/**	Null constructor.
		 *	@detail No target is assigned and calling results in undefined behavior.
		 */
		constexpr function_ptr(const std::nullptr_t) noexcept
			: m_target{ nullptr }
		{ }
		/**	Default copy constructor.
		 */
		function_ptr(const function_ptr&) = default;
		/**	Default move constructor.
		 */
		function_ptr(function_ptr&&) = default;
		/**	Constructor from a given callable.
		 *	@param callable An invocable target to which this will hold a pointer.
		 *	@tparam Callable The type of the given invocable target.
		 */
		template <typename Callable,
			typename = std::enable_if_t<
				std::is_invocable_r_v<result_type, Callable&&, Args...>
				&& false == std::is_same_v<std::decay_t<Callable>, function_ptr>
				&& false == std::is_member_function_pointer_v<Callable>
				&& false == std::is_member_object_pointer_v<Callable>
			>
		>
		constexpr function_ptr(Callable&& callable) noexcept
		{
			assign(std::forward<Callable>(callable));
		}

		/**	Default copy assigment.
		 */
		function_ptr& operator=(const function_ptr&) = default;
		/**	Default move assigment.
		 */
		function_ptr& operator=(function_ptr&&) = default;
		/**	Assigment a given callable as the pointed-to invocation target.
		 *	@param callable An invocable target to which this will hold a pointer and invoke upon operator().
		 *	@return A reference to this.
		 *	@tparam Callable The type of the given invocable target.
		 */
		template <typename Callable,
			typename = std::enable_if_t<
				std::is_invocable_r_v<result_type, Callable&&, Args...>
				&& false == std::is_same_v<std::decay_t<Callable>, function_ptr>
				&& false == std::is_member_function_pointer_v<Callable>
				&& false == std::is_member_object_pointer_v<Callable>
			>
		>
		constexpr function_ptr& operator=(Callable&& callable) noexcept
		{
			assign(std::forward<Callable>(callable));
			return *this;
		}
		/**	Null assignment.
		 *	@detail Afterwards, no target is assigned and calling results in undefined behavior.
		 */
		function_ptr& operator=(const std::nullptr_t) noexcept
		{
			m_target = nullptr;
			return *this;
		}
		/**	Invoke the pointed-to callable.
		 *	@detail If this function_ptr is null, undefined behavior will result.
		 *	@param args The arguments to pass to the pointed-to callable.
		 *	@tparam OperatorArgs The arguments to forward to m_invoke_target.
		 *	@return The result of invoking the pointed-to callable with args.
		 */
		template <typename... OperatorArgs>
		result_type operator()(OperatorArgs&&... args) const noexcept(NoExcept)
		{
			assert(m_target != nullptr);
			return m_invoke_target(m_target, std::forward<OperatorArgs>(args)...);
		}
		/**	Test if this is callable.
		 *	@return True if this is non-null and callable via operator().
		 */
		constexpr explicit operator bool() const noexcept
		{
			return m_target != nullptr;
		}
		/**	Test if this is null.
		 *	@detail True if this is null and calling operator() will result in undefined behavior.
		 */
		constexpr bool operator==(std::nullptr_t) const noexcept
		{
			return m_target == nullptr;
		}
		/**	Test if this is non-null.
		 *	@return True if this is non-null and callable via operator().
		 */
		constexpr bool operator!=(std::nullptr_t) const noexcept
		{
			return m_target != nullptr;
		}
		/**	Swap this with another function_ptr.
		 *	@param other The function_ptr with which to swap contents.
		 */
		void swap(function_ptr& other) noexcept
		{
			std::swap(m_target, other.m_target);
			std::swap(m_invoke_target, other.m_invoke_target);
		}
		/**	Swap the two given function_ptr objects.
		 *	@param lhs The function_ptr with which to swap contents with rhs.
		 *	@param rhs The function_ptr with which to swap contents with lhs.
		 */
		friend void swap(function_ptr& lhs, function_ptr& rhs) noexcept
		{
			lhs.swap(rhs);
		}

	private:
		/**	The function pointer type of m_invoke_target.
		 *	@details Accepts m_target as its first argument followed by "Args..." and returns return_type.
		 */
		using invoke_target_type = result_type(*)(void*, Args...) noexcept(NoExcept);

		/**	Used by the construction and assignment operators to accept a callable as the target.
		 *	@param callable The target to assign to m_target or point at with m_target.
		 *	@tparam Callable The type of the given callable.
		 */
		template <typename Callable>
		void assign(Callable&& callable) noexcept
		{
			static_assert(NoExcept == false || std::is_nothrow_invocable_r_v<result_type, Callable, Args...>, "function_ptr requires nothrow invocable.");
			if constexpr (std::is_pointer_v<Callable>)
			{
				// Do not take the address of callable, as it's already a (function) pointer.
				m_target = reinterpret_cast<void*>(callable);
				m_invoke_target = &detail::function_ptr_invoke_target<Callable, NoExcept, result_type, Args...>;
			}
			else
			{
				m_target = const_cast<void*>(reinterpret_cast<const void*>(std::addressof(callable)));
				m_invoke_target = &detail::function_ptr_invoke_target<std::add_pointer_t<Callable>, NoExcept, result_type, Args...>;
			}
		}

		/**	A pointer value used by m_invoke_target to call a given callable object or pointer.
		 *	@details Nullable.
		 */
		void* m_target;
		/**	A function pointer equal to a specialization of &detail::function_ptr_invoke_target.
		 *	@details Only valid if m_target is non-null. Accepts m_target as its first argument followed by "Args..." and returns return_type.
		 */
		invoke_target_type m_invoke_target;
	};

} // namespace detail

/**	Implements a nullable, callable pointer to an invocable target.
 *	@tparam Signature The function signature.
 */
template <typename Signature>
class function_ptr;

/**	Implements a nullable, callable pointer to an invocable target.
 *	@tparam ResultType The result of calling this.
 *	@tparam Args The arguments necessary to call this.
 */
template <typename ResultType, typename... Args>
class function_ptr <ResultType(Args...)> : public detail::function_ptr<false, ResultType, Args...>
{
public:
	using detail::function_ptr<false, ResultType, Args...>::function_ptr;
};

/**	Implements a nullable, callable pointer to a nothrow invocable target.
 *	@tparam ResultType The result of calling this.
 *	@tparam Args The arguments necessary to call this.
 */
template <typename ResultType, typename... Args>
class function_ptr <ResultType(Args...) noexcept> : public detail::function_ptr<true, ResultType, Args...>
{
public:
	using detail::function_ptr<true, ResultType, Args...>::function_ptr;
};

} // namespace sh

#endif
