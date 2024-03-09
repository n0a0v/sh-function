#include <gtest/gtest.h>

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
