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

#ifndef INC_SH__FUNCTION_REF_HPP
#define INC_SH__FUNCTION_REF_HPP

/**	@file
 *	This file declares a function reference-like facility with type erasure.
 */

#include <cassert>
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
	 *	@tparam NoExcept True if this targeting a nothrow invocable and false otherwise.
	 *	@tparam ResultType The result of calling this.
	 *	@tparam Args The arguments necessary to call this.
	 */
	template <typename Callable, bool NoExcept, typename ResultType, typename... Args>
	static ResultType function_ref_invoke_target(void* const target, Args... args) noexcept(NoExcept)
	{
		// Clang 15 had issues with making this a member of function_ref and inheriting the template parameters.
		assert(target != nullptr);
		return std::invoke(
			*reinterpret_cast<Callable>(target),
			std::forward<Args>(args)...);
	}

	/**	Implements a (mostly) non-nullable, callable reference to an invocable target.
	 *	@note Required as MSVC does not support deduction of function signature noexcept in template specialization.
	 *	@tparam NoExcept True if this targets a nothrow invocable and false otherwise.
	 *	@tparam ResultType The result of calling this.
	 *	@tparam Args The arguments necessary to call this.
	 */
	template <bool NoExcept, typename ResultType, typename... Args>
	class function_ref
	{
	public:
		using result_type = ResultType;

		function_ref() = delete;
		function_ref& operator=(const function_ref&) = delete;
		function_ref& operator=(function_ref&&) = delete;

		/**	Default copy constructor.
		 */
		function_ref(const function_ref&) = default;
		/**	Default move constructor.
		 */
		function_ref(function_ref&&) = default;
		/**	Constructor from a given callable.
		 *	@param callable An invocable target to which this will hold a pointer.
		 *	@tparam Callable The type of the given invocable target.
		 */
		template <typename Callable,
			typename = std::enable_if_t<
				std::is_invocable_r_v<result_type, Callable&&, Args...>
				&& false == std::is_same_v<std::decay_t<Callable>, function_ref>
				&& false == std::is_pointer_v<Callable>
				&& false == std::is_member_function_pointer_v<Callable>
				&& false == std::is_member_object_pointer_v<Callable>
			>
		>
		constexpr function_ref(Callable&& callable) noexcept
			: m_invoke_target{ &detail::function_ref_invoke_target<std::add_pointer_t<Callable>, NoExcept, result_type, Args...> }
			, m_target{ const_cast<void*>(reinterpret_cast<const void*>(std::addressof(callable))) }
		{
			static_assert(NoExcept == false || std::is_nothrow_invocable_r_v<result_type, Callable, Args...>, "function_ref requires nothrow invocable.");
		}

		/**	Invoke the referenced callable.
		 *	@param args The arguments to pass to the pointed-to callable.
		 *	@tparam OperatorArgs The arguments to forward to m_invoke_target.
		 *	@return The result of invoking the pointed-to callable with args.
		 */
		template <typename... OperatorArgs>
		result_type operator()(OperatorArgs&&... args) const noexcept(NoExcept)
		{
			return m_invoke_target(m_target, std::forward<OperatorArgs>(args)...);
		}

	private:
		/**	The function pointer type of m_invoke_target.
		 *	@details Accepts m_target as its first argument followed by "Args..." and returns return_type.
		 */
		using invoke_target_type = result_type(*)(void*, Args...) noexcept(NoExcept);

		/**	A function pointer equal to nullptr or a specialization of &detail::function_ref_invoke_target.
		 *	@details Non-null. Accepts m_target as its first argument followed by "Args..." and returns return_type.
		 */
		invoke_target_type m_invoke_target;
		/**	A pointer value used by m_invoke_target to call a given callable object.
		 *	@details Non-null.
		 */
		void* m_target;
	};

} // namespace detail

/**	Implements a (mostly) non-nullable, callable reference to an invocable target.
 *	@tparam Signature The function signature.
 */
template <typename Signature>
class function_ref;

/**	Implements a (mostly) non-nullable, callable reference to an invocable target.
 *	@tparam ResultType The result of calling this.
 *	@tparam Args The arguments necessary to call this.
 */
template <typename ResultType, typename... Args>
class function_ref <ResultType(Args...)> : public detail::function_ref<false, ResultType, Args...>
{
public:
	using detail::function_ref<false, ResultType, Args...>::function_ref;
};

/**	Implements a (mostly) non-nullable, callable reference to a nothrow invocable target.
 *	@tparam ResultType The result of calling this.
 *	@tparam Args The arguments necessary to call this.
 */
template <typename ResultType, typename... Args>
class function_ref <ResultType(Args...) noexcept> : public detail::function_ref<true, ResultType, Args...>
{
public:
	using detail::function_ref<true, ResultType, Args...>::function_ref;
};

} // namespace sh

#endif
