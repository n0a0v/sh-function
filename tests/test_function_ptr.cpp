#include <gtest/gtest.h>

#include <cstring>
#include <sh/function_ptr.hpp>

using sh::function_ptr;
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

TEST(sh_function_ptr, ctor_default)
{
	function_ptr<void(bool&)> x;

	EXPECT_FALSE(bool(x));
	EXPECT_TRUE(x == nullptr);
	EXPECT_FALSE(x != nullptr);
}
TEST(sh_function_ptr, ctor_default_noexcept)
{
	function_ptr<void(bool&) noexcept> x;

	EXPECT_FALSE(bool(x));
	EXPECT_TRUE(x == nullptr);
	EXPECT_FALSE(x != nullptr);
}
TEST(sh_function_ptr, ctor_nullptr)
{
	function_ptr<void(bool&)> x{ nullptr };

	EXPECT_FALSE(bool(x));
	EXPECT_TRUE(x == nullptr);
	EXPECT_FALSE(x != nullptr);
}
TEST(sh_function_ptr, ctor_nullptr_noexcept)
{
	function_ptr<void(bool&) noexcept> x{ nullptr };

	EXPECT_FALSE(bool(x));
	EXPECT_TRUE(x == nullptr);
	EXPECT_FALSE(x != nullptr);
}
TEST(sh_function_ptr, ctor_call_func)
{
	function_ptr<void(bool&)> x(invert_bool);

	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, ctor_call_func_noexcept)
{
	function_ptr<void(bool&) noexcept> x(invert_bool_noexcept);
	static_assert(std::is_nothrow_invocable_v<decltype(x), bool&>);

	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, ctor_call_func_ptr)
{
	function_ptr<void(bool&)> x(&invert_bool);

	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, ctor_call_func_ptr_noexcept)
{
	function_ptr<void(bool&) noexcept> x(&invert_bool_noexcept);
	static_assert(std::is_nothrow_invocable_v<decltype(x), bool&>);

	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, ctor_call_functor)
{
	InvertBool functor;
	function_ptr<void(bool&)> x(functor);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, ctor_call_functor_noexcept)
{
	InvertBoolNoExcept functor;
	function_ptr<void(bool&) noexcept> x(functor);
	static_assert(std::is_nothrow_invocable_v<decltype(x), bool&>);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, ctor_call_lambda)
{
	const auto lambda = [](bool& arg) { arg = !arg; };
	function_ptr<void(bool&)> x(lambda);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, ctor_call_lambda_noexcept)
{
	const auto lambda = [](bool& arg) noexcept { arg = !arg; };
	function_ptr<void(bool&) noexcept> x(lambda);
	static_assert(std::is_nothrow_invocable_v<decltype(x), bool&>);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);

}
TEST(sh_function_ptr, ctor_call_function_ptr_nullptr)
{
	std::unique_ptr<function_ptr<void(bool&)>> p = std::make_unique<function_ptr<void(bool&)>>(nullptr);
	function_ptr<void(bool&)> x{ *p };

	// Ensure bad things happen if x is referencing p.
	static_assert(std::is_trivially_destructible_v<function_ptr<void(bool&)>>);
	std::memset(p.get(), 0, sizeof(function_ptr<void(bool&)>));
	p.reset();

	EXPECT_FALSE(bool(x));
	EXPECT_TRUE(x == nullptr);
	EXPECT_FALSE(x != nullptr);
}
TEST(sh_function_ptr, ctor_call_function_ptr)
{
	std::unique_ptr<function_ptr<void(bool&)>> p = std::make_unique<function_ptr<void(bool&)>>(invert_bool);
	function_ptr<void(bool&)> x{ *p };

	// Ensure bad things happen if x is referencing p.
	static_assert(std::is_trivially_destructible_v<function_ptr<void(bool&)>>);
	std::memset(p.get(), 0, sizeof(function_ptr<void(bool&)>));
	p.reset();

	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, ctor_call_function_ptr_noexcept)
{
	std::unique_ptr<function_ptr<void(bool&) noexcept>> p = std::make_unique<function_ptr<void(bool&) noexcept>>(invert_bool_noexcept);
	function_ptr<void(bool&)> x{ *p };

	static_assert(false == std::is_constructible_v<function_ptr<void(bool&) noexcept>, function_ptr<void(bool&)>>);
	static_assert(false == std::is_assignable_v<function_ptr<void(bool&) noexcept>&, function_ptr<void(bool&)>>);

	// Ensure bad things happen if x is referencing p.
	static_assert(std::is_trivially_destructible_v<function_ptr<void(bool&)>>);
	std::memset(p.get(), 0, sizeof(function_ptr<void(bool&)>));
	p.reset();

	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, ctor_call_function_ref)
{
	std::unique_ptr<function_ref<void(bool&)>> p = std::make_unique<function_ref<void(bool&)>>(invert_bool);
	function_ptr<void(bool&)> x{ *p };

	// Ensure bad things happen if x is referencing p.
	static_assert(std::is_trivially_destructible_v<function_ref<void(bool&)>>);
	std::memset(p.get(), 0, sizeof(function_ref<void(bool&)>));
	p.reset();

	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, ctor_call_function_ref_noexcept)
{
	std::unique_ptr<function_ref<void(bool&) noexcept>> p = std::make_unique<function_ref<void(bool&) noexcept>>(invert_bool_noexcept);
	function_ptr<void(bool&)> x{ *p };

	static_assert(false == std::is_constructible_v<function_ptr<void(bool&) noexcept>, function_ref<void(bool&)>>);
	static_assert(false == std::is_assignable_v<function_ptr<void(bool&) noexcept>&, function_ref<void(bool&)>>);

	// Ensure bad things happen if x is referencing p.
	static_assert(std::is_trivially_destructible_v<function_ref<void(bool&)>>);
	std::memset(p.get(), 0, sizeof(function_ref<void(bool&)>));
	p.reset();

	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, assign)
{
	const auto inc = [](int& arg) { ++arg; };
	const auto dec = [](int& arg) { --arg; };

	int param = 0;
	function_ptr<void(int&)> x(inc);

	x(param);
	ASSERT_EQ(param, 1);

	x = dec;
	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	x(param);
	ASSERT_EQ(param, 0);

	x = function_ptr<void(int&)>(inc);

	x(param);
	ASSERT_EQ(param, 1);

	x = nullptr;
	EXPECT_FALSE(bool(x));
	EXPECT_TRUE(x == nullptr);
	EXPECT_FALSE(x != nullptr);
}
TEST(sh_function_ptr, assign_noexcept)
{
	const auto inc = [](int& arg) noexcept { ++arg; };
	const auto dec = [](int& arg) noexcept { --arg; };

	int param = 0;
	function_ptr<void(int&) noexcept> x(inc);

	x(param);
	ASSERT_EQ(param, 1);

	x = dec;
	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	x(param);
	ASSERT_EQ(param, 0);

	x = function_ptr<void(int&) noexcept>(inc);

	x(param);
	ASSERT_EQ(param, 1);

	x = nullptr;
	EXPECT_FALSE(bool(x));
	EXPECT_TRUE(x == nullptr);
	EXPECT_FALSE(x != nullptr);
}
TEST(sh_function_ptr, assign_call_function_ptr_nullptr)
{
	function_ptr<void(bool&)> x{ invert_bool };

	{
		std::unique_ptr<function_ptr<void(bool&)>> p = std::make_unique<function_ptr<void(bool&)>>(nullptr);

		x = *p;

		// Ensure bad things happen if x is referencing p.
		static_assert(std::is_trivially_destructible_v<function_ptr<void(bool&)>>);
		std::memset(p.get(), 0, sizeof(function_ptr<void(bool&)>));
	}

	EXPECT_FALSE(bool(x));
	EXPECT_TRUE(x == nullptr);
	EXPECT_FALSE(x != nullptr);
}
TEST(sh_function_ptr, assign_call_function_ptr)
{
	function_ptr<void(bool&)> x;

	{
		std::unique_ptr<function_ptr<void(bool&)>> p = std::make_unique<function_ptr<void(bool&)>>(invert_bool);

		x = *p;

		// Ensure bad things happen if x is referencing p.
		static_assert(std::is_trivially_destructible_v<function_ptr<void(bool&)>>);
		std::memset(p.get(), 0, sizeof(function_ptr<void(bool&)>));
	}

	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, assign_call_function_ref)
{
	function_ptr<void(bool&)> x;

	{
		std::unique_ptr<function_ref<void(bool&)>> p = std::make_unique<function_ref<void(bool&)>>(invert_bool);

		x = *p;

		// Ensure bad things happen if x is referencing p.
		static_assert(std::is_trivially_destructible_v<function_ref<void(bool&)>>);
		std::memset(p.get(), 0, sizeof(function_ref<void(bool&)>));
	}

	EXPECT_TRUE(bool(x));
	EXPECT_FALSE(x == nullptr);
	EXPECT_TRUE(x != nullptr);

	bool param = false;
	x(param);
	EXPECT_TRUE(param);
}
TEST(sh_function_ptr, swap)
{
	const auto inc = [](int& arg) { ++arg; };
	const auto dec = [](int& arg) { --arg; };

	int param = 0;
	function_ptr<void(int&)> x(inc);
	function_ptr<void(int&)> y(dec);

	x(param);
	ASSERT_EQ(param, 1);
	y(param);
	ASSERT_EQ(param, 0);

	std::swap(x, y);

	x(param);
	ASSERT_EQ(param, -1);
	y(param);
	ASSERT_EQ(param, 0);

	x.swap(y);

	x(param);
	ASSERT_EQ(param, 1);
	y(param);
	ASSERT_EQ(param, 0);
}
TEST(sh_function_ptr, swap_noexcept)
{
	const auto inc = [](int& arg) noexcept { ++arg; };
	const auto dec = [](int& arg) noexcept { --arg; };

	int param = 0;
	function_ptr<void(int&) noexcept> x(inc);
	function_ptr<void(int&) noexcept> y(dec);

	x(param);
	ASSERT_EQ(param, 1);
	y(param);
	ASSERT_EQ(param, 0);

	std::swap(x, y);

	x(param);
	ASSERT_EQ(param, -1);
	y(param);
	ASSERT_EQ(param, 0);

	x.swap(y);

	x(param);
	ASSERT_EQ(param, 1);
	y(param);
	ASSERT_EQ(param, 0);
}
