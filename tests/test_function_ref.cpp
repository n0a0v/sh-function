#include <gtest/gtest.h>

#include <cstring>
#include <sh/function_ref.hpp>

using sh::function_ref;

namespace
{
	void invert_bool(bool& arg)
	{
		arg = !arg;
	}
	void invert_bool_noexcept(bool& arg) noexcept
	{
		arg = !arg;
	}

	struct InvertBool final
	{
		void operator()(bool& arg) const
		{
			arg = !arg;
		}
	};
	struct InvertBoolNoExcept final
	{
		void operator()(bool& arg) const noexcept
		{
			arg = !arg;
		}
	};
} // anonymous namespace

TEST(sh_function_ref, ctor_call_func)
{
	function_ref<void(bool&)> x(invert_bool);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ref, ctor_call_func_noexcept)
{
	function_ref<void(bool&) noexcept> x(invert_bool_noexcept);
	static_assert(std::is_nothrow_invocable_v<decltype(x), bool&>);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ref, ctor_call_functor)
{
	InvertBool functor;
	function_ref<void(bool&)> x(functor);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ref, ctor_call_functor_noexcept)
{
	InvertBoolNoExcept functor;
	function_ref<void(bool&) noexcept> x(functor);
	static_assert(std::is_nothrow_invocable_v<decltype(x), bool&>);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ref, ctor_call_lambda)
{
	const auto lambda = [](bool& arg) { arg = !arg; };
	function_ref<void(bool&)> x(lambda);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ref, ctor_call_lambda_noexcept)
{
	const auto lambda = [](bool& arg) noexcept { arg = !arg; };
	function_ref<void(bool&) noexcept> x(lambda);
	static_assert(std::is_nothrow_invocable_v<decltype(x), bool&>);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ref, ctor_call_function_ref)
{
	std::unique_ptr<function_ref<void(bool&)>> p = std::make_unique<function_ref<void(bool&)>>(invert_bool);
	function_ref<void(bool&)> x{ *p };

	// Ensure bad things happen if x is referencing p.
	static_assert(std::is_trivially_destructible_v<function_ref<void(bool&)>>);
	std::memset(p.get(), 0, sizeof(function_ref<void(bool&)>));
	p.reset();

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ref, ctor_call_function_ref_noexcept)
{
	std::unique_ptr<function_ref<void(bool&) noexcept>> p = std::make_unique<function_ref<void(bool&) noexcept>>(invert_bool_noexcept);
	function_ref<void(bool&)> x{ *p };

	static_assert(false == std::is_constructible_v<function_ref<void(bool&) noexcept>, function_ref<void(bool&)>>);

	// Ensure bad things happen if x is referencing p.
	static_assert(std::is_trivially_destructible_v<function_ref<void(bool&)>>);
	std::memset(p.get(), 0, sizeof(function_ref<void(bool&)>));
	p.reset();

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
