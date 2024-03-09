#include <gtest/gtest.h>

#include <sh/move_only_function.hpp>

#include <memory>

using sh::move_only_function;

namespace
{
	int plus_1(const int input)
	{
		return input + 1;
	}

	struct counter final
	{
		int* m_value;

		counter(int* const value)
			: m_value(value)
		{
			if (m_value != nullptr)
			{
				++(*m_value);
			}
		}
		~counter()
		{
			if (m_value != nullptr)
			{
				--(*m_value);
			}
		}
		counter(const counter& other)
			: m_value(other.m_value)
		{
			if (m_value != nullptr)
			{
				++(*m_value);
			}
		}
		counter(counter&& other) noexcept
			: m_value(other.m_value)
		{
			other.m_value = nullptr;
		}

		counter& operator=(const counter& other)
		{
			if (m_value != nullptr)
			{
				--(*m_value);
			}
			m_value = other.m_value;
			if (m_value != nullptr)
			{
				++(*m_value);
			}
			return *this;
		}
		counter& operator=(counter&& other) noexcept
		{
			if (m_value != nullptr)
			{
				--(*m_value);
			}
			m_value = std::exchange(other.m_value, nullptr);
			return *this;
		}
	};
} // anonymous namespace

TEST(sh_move_only_function, ctor_default)
{
	move_only_function<int(int)> x;
	EXPECT_FALSE(bool(x));
	EXPECT_EQ(x, nullptr);
}
TEST(sh_move_only_function, ctor_nullptr)
{
	move_only_function<int(int)> x(nullptr);
	EXPECT_FALSE(bool(x));
	EXPECT_EQ(x, nullptr);
}
TEST(sh_move_only_function, ctor_move_small)
{
	int value = 0;
	{
		auto lambda = [c = counter(&value)]() { };
		static_assert(sizeof(lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");

		move_only_function<void()> x(std::move(lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		EXPECT_EQ(value, 1);

		move_only_function<void()> y = std::move(x);
		ASSERT_FALSE(bool(x));
		ASSERT_EQ(x, nullptr);
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		EXPECT_EQ(value, 1);
	}
	EXPECT_EQ(value, 0);
}
TEST(sh_move_only_function, ctor_move_large)
{
	int value = 0;
	{
		int values[64] = { 1, 0 };
		auto lambda = [c = counter(&value), values]() { };
		static_assert(sizeof(lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");

		move_only_function<void()> x(std::move(lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		EXPECT_EQ(value, 1);

		move_only_function<void()> y = std::move(x);
		ASSERT_FALSE(bool(x));
		ASSERT_EQ(x, nullptr);
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		EXPECT_EQ(value, 1);
	}
	EXPECT_EQ(value, 0);
}
TEST(sh_move_only_function, assign_small)
{
	move_only_function<char()> x;
	ASSERT_FALSE(bool(x));
	ASSERT_EQ(x, nullptr);

	x = []() { return 'x'; };
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	ASSERT_EQ(x(), 'x');
}
TEST(sh_move_only_function, assign_large)
{
	move_only_function<char()> x;
	ASSERT_FALSE(bool(x));
	ASSERT_EQ(x, nullptr);

	int values[64] = { 1, 0 };
	auto lambda = [values]() { return 'x'; };
	static_assert(sizeof(lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");
	x = std::move(lambda);
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	ASSERT_EQ(x(), 'x');
}
TEST(sh_move_only_function, assign_nullptr_small)
{
	int value = 0;
	auto lambda = [c = counter(&value)]() { return 'x'; };
	static_assert(sizeof(lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");

	move_only_function<char()> x(std::move(lambda));
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	ASSERT_EQ(x(), 'x');
	EXPECT_EQ(value, 1);

	x = nullptr;
	ASSERT_FALSE(bool(x));
	ASSERT_EQ(x, nullptr);
	EXPECT_EQ(value, 0);
}
TEST(sh_move_only_function, assign_nullptr_large)
{
	int value = 0;
	int values[64] = { 1, 0 };
	auto lambda = [c = counter(&value), values]() { return 'x'; };
	static_assert(sizeof(lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");

	move_only_function<char()> x(std::move(lambda));
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	ASSERT_EQ(x(), 'x');
	EXPECT_EQ(value, 1);

	x = nullptr;
	ASSERT_FALSE(bool(x));
	ASSERT_EQ(x, nullptr);
	EXPECT_EQ(value, 0);
}
TEST(sh_move_only_function, assign_move_small_small)
{
	int a_value = 0, b_value = 0;
	{
		auto a_lambda = [c = counter(&a_value)]() { return 'a'; };
		static_assert(sizeof(a_lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");
		auto b_lambda = [c = counter(&b_value)]() { return 'b'; };
		static_assert(sizeof(b_lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");

		move_only_function<char()> x(std::move(a_lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		ASSERT_EQ(x(), 'a');
		EXPECT_EQ(a_value, 1);

		move_only_function<char()> y(std::move(b_lambda));
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		ASSERT_EQ(y(), 'b');
		EXPECT_EQ(b_value, 1);

		y = std::move(x);
		ASSERT_FALSE(bool(x));
		ASSERT_EQ(x, nullptr);
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		EXPECT_EQ(a_value, 1);
		EXPECT_EQ(b_value, 0);
		EXPECT_EQ(y(), 'a');
	}
	EXPECT_EQ(a_value, 0);
	EXPECT_EQ(b_value, 0);
}
TEST(sh_move_only_function, assign_move_small_large)
{
	int a_value = 0, b_value = 0;
	{
		auto a_lambda = [c = counter(&a_value)]() { return 'a'; };
		static_assert(sizeof(a_lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");
		int values[64] = { 1, 0 };
		auto b_lambda = [c = counter(&b_value), values]() { return 'b'; };
		static_assert(sizeof(b_lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");

		move_only_function<char()> x(std::move(a_lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		ASSERT_EQ(x(), 'a');
		EXPECT_EQ(a_value, 1);

		move_only_function<char()> y(std::move(b_lambda));
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		ASSERT_EQ(y(), 'b');
		EXPECT_EQ(b_value, 1);

		y = std::move(x);
		ASSERT_FALSE(bool(x));
		ASSERT_EQ(x, nullptr);
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		EXPECT_EQ(a_value, 1);
		EXPECT_EQ(b_value, 0);
		EXPECT_EQ(y(), 'a');
	}
	EXPECT_EQ(a_value, 0);
	EXPECT_EQ(b_value, 0);
}
TEST(sh_move_only_function, assign_move_large_small)
{
	int a_value = 0, b_value = 0;
	{
		int values[64] = { 1, 0 };
		auto a_lambda = [c = counter(&a_value), values]() { return 'a'; };
		static_assert(sizeof(a_lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");
		auto b_lambda = [c = counter(&b_value)]() { return 'b'; };
		static_assert(sizeof(b_lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");

		move_only_function<char()> x(std::move(a_lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		ASSERT_EQ(x(), 'a');
		EXPECT_EQ(a_value, 1);

		move_only_function<char()> y(std::move(b_lambda));
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		ASSERT_EQ(y(), 'b');
		EXPECT_EQ(b_value, 1);

		y = std::move(x);
		ASSERT_FALSE(bool(x));
		ASSERT_EQ(x, nullptr);
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		EXPECT_EQ(a_value, 1);
		EXPECT_EQ(b_value, 0);
		EXPECT_EQ(y(), 'a');
	}
	EXPECT_EQ(a_value, 0);
	EXPECT_EQ(b_value, 0);
}
TEST(sh_move_only_function, assign_move_large_large)
{
	int a_value = 0, b_value = 0;
	{
		int values[64] = { 1, 0 };
		auto a_lambda = [c = counter(&a_value), values]() { return 'a'; };
		static_assert(sizeof(a_lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");
		auto b_lambda = [c = counter(&b_value), values]() { return 'b'; };
		static_assert(sizeof(b_lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");

		move_only_function<char()> x(std::move(a_lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		ASSERT_EQ(x(), 'a');
		EXPECT_EQ(a_value, 1);

		move_only_function<char()> y(std::move(b_lambda));
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		ASSERT_EQ(y(), 'b');
		EXPECT_EQ(b_value, 1);

		y = std::move(x);
		ASSERT_FALSE(bool(x));
		ASSERT_EQ(x, nullptr);
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		EXPECT_EQ(a_value, 1);
		EXPECT_EQ(b_value, 0);
		EXPECT_EQ(y(), 'a');
	}
	EXPECT_EQ(a_value, 0);
	EXPECT_EQ(b_value, 0);
}
TEST(sh_move_only_function, swap_small)
{
	int value = 0;
	{
		auto lambda = [c = counter(&value)]() { };
		static_assert(sizeof(lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");

		move_only_function<void()> x(std::move(lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		EXPECT_EQ(value, 1);

		move_only_function<void()> y;
		ASSERT_FALSE(bool(y));
		ASSERT_EQ(y, nullptr);
		EXPECT_EQ(value, 1);

		x.swap(y);
		EXPECT_FALSE(bool(x));
		EXPECT_EQ(x, nullptr);
		EXPECT_TRUE(bool(y));
		EXPECT_NE(y, nullptr);
		EXPECT_EQ(value, 1);
	}
	EXPECT_EQ(value, 0);
}
TEST(sh_move_only_function, swap_large)
{
	int value = 0;
	{
		int values[64] = { 1, 0 };
		auto lambda = [c = counter(&value), values]() { };
		static_assert(sizeof(lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");

		move_only_function<void()> x(std::move(lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		EXPECT_EQ(value, 1);

		move_only_function<void()> y;
		ASSERT_FALSE(bool(y));
		ASSERT_EQ(y, nullptr);
		EXPECT_EQ(value, 1);

		x.swap(y);
		EXPECT_FALSE(bool(x));
		EXPECT_EQ(x, nullptr);
		EXPECT_TRUE(bool(y));
		EXPECT_NE(y, nullptr);
		EXPECT_EQ(value, 1);
	}
	EXPECT_EQ(value, 0);
}
TEST(sh_move_only_function, swap_move_small_small)
{
	int a_value = 0, b_value = 0;
	{
		auto a_lambda = [c = counter(&a_value)]() { return 'a'; };
		static_assert(sizeof(a_lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");
		auto b_lambda = [c = counter(&b_value)]() { return 'b'; };
		static_assert(sizeof(b_lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");

		move_only_function<char()> x(std::move(a_lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		ASSERT_EQ(x(), 'a');
		EXPECT_EQ(a_value, 1);

		move_only_function<char()> y(std::move(b_lambda));
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		ASSERT_EQ(y(), 'b');
		EXPECT_EQ(b_value, 1);

		x.swap(y);
		EXPECT_TRUE(bool(x));
		EXPECT_NE(x, nullptr);
		EXPECT_TRUE(bool(y));
		EXPECT_NE(y, nullptr);
		EXPECT_EQ(a_value, 1);
		EXPECT_EQ(b_value, 1);
		EXPECT_EQ(x(), 'b');
		EXPECT_EQ(y(), 'a');
	}
	EXPECT_EQ(a_value, 0);
	EXPECT_EQ(b_value, 0);
}
TEST(sh_move_only_function, swap_move_small_large)
{
	int a_value = 0, b_value = 0;
	{
		auto a_lambda = [c = counter(&a_value)]() { return 'a'; };
		static_assert(sizeof(a_lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");
		int values[64] = { 1, 0 };
		auto b_lambda = [c = counter(&b_value), values]() { return 'b'; };
		static_assert(sizeof(b_lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");

		move_only_function<char()> x(std::move(a_lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		ASSERT_EQ(x(), 'a');
		EXPECT_EQ(a_value, 1);

		move_only_function<char()> y(std::move(b_lambda));
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		ASSERT_EQ(y(), 'b');
		EXPECT_EQ(b_value, 1);

		x.swap(y);
		EXPECT_TRUE(bool(x));
		EXPECT_NE(x, nullptr);
		EXPECT_TRUE(bool(y));
		EXPECT_NE(y, nullptr);
		EXPECT_EQ(a_value, 1);
		EXPECT_EQ(b_value, 1);
		EXPECT_EQ(x(), 'b');
		EXPECT_EQ(y(), 'a');
	}
	EXPECT_EQ(a_value, 0);
	EXPECT_EQ(b_value, 0);
}
TEST(sh_move_only_function, swap_move_large_small)
{
	int a_value = 0, b_value = 0;
	{
		int values[64] = { 1, 0 };
		auto a_lambda = [c = counter(&a_value), values]() { return 'a'; };
		static_assert(sizeof(a_lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");
		auto b_lambda = [c = counter(&b_value)]() { return 'b'; };
		static_assert(sizeof(b_lambda) <= sh::detail::move_only_function_storage::capacity, "small test isn't storing small callable.");

		move_only_function<char()> x(std::move(a_lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		ASSERT_EQ(x(), 'a');
		EXPECT_EQ(a_value, 1);

		move_only_function<char()> y(std::move(b_lambda));
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		ASSERT_EQ(y(), 'b');
		EXPECT_EQ(b_value, 1);

		x.swap(y);
		EXPECT_TRUE(bool(x));
		EXPECT_NE(x, nullptr);
		EXPECT_TRUE(bool(y));
		EXPECT_NE(y, nullptr);
		EXPECT_EQ(a_value, 1);
		EXPECT_EQ(b_value, 1);
		EXPECT_EQ(x(), 'b');
		EXPECT_EQ(y(), 'a');
	}
	EXPECT_EQ(a_value, 0);
	EXPECT_EQ(b_value, 0);
}
TEST(sh_move_only_function, swap_move_large_large)
{
	int a_value = 0, b_value = 0;
	{
		int values[64] = { 1, 0 };
		auto a_lambda = [c = counter(&a_value), values]() { return 'a'; };
		static_assert(sizeof(a_lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");
		auto b_lambda = [c = counter(&b_value), values]() { return 'b'; };
		static_assert(sizeof(b_lambda) > sh::detail::move_only_function_storage::capacity, "large test isn't storing large callable.");

		move_only_function<char()> x(std::move(a_lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		ASSERT_EQ(x(), 'a');
		EXPECT_EQ(a_value, 1);

		move_only_function<char()> y(std::move(b_lambda));
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		ASSERT_EQ(y(), 'b');
		EXPECT_EQ(b_value, 1);

		x.swap(y);
		EXPECT_TRUE(bool(x));
		EXPECT_NE(x, nullptr);
		EXPECT_TRUE(bool(y));
		EXPECT_NE(y, nullptr);
		EXPECT_EQ(a_value, 1);
		EXPECT_EQ(b_value, 1);
		EXPECT_EQ(x(), 'b');
		EXPECT_EQ(y(), 'a');
	}
	EXPECT_EQ(a_value, 0);
	EXPECT_EQ(b_value, 0);
}

TEST(sh_move_only_function, call)
{
	bool called = false;
	move_only_function<void(bool&)> x{ [](bool& called) { called = true; } };
	x(called);
	EXPECT_TRUE(called);
}
TEST(sh_move_only_function, func)
{
	move_only_function<int(int)> x(plus_1);
	static_assert(sizeof(&plus_1) <= sh::detail::move_only_function_storage::capacity, "func test isn't storing small callable.");
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 1);
}
TEST(sh_move_only_function, func_ptr)
{
	move_only_function<int(int)> x(&plus_1);
	static_assert(sizeof(&plus_1) <= sh::detail::move_only_function_storage::capacity, "func_ptr test isn't storing small callable.");
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 1);
}
TEST(sh_move_only_function, functor_small)
{
	struct plus_2 final
	{
		std::unique_ptr<int> m_two = std::make_unique<int>(2);
		int operator()(const int input) const
		{
			return input + *m_two;
		}
	};
	static_assert(sizeof(plus_2) <= sh::detail::move_only_function_storage::capacity, "functor_small test isn't storing small callable.");
	move_only_function<int(int)> x(plus_2{});
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 2);
}
TEST(sh_move_only_function, functor_large)
{
	struct plus_values final
	{
		std::unique_ptr<int> m_zero = std::make_unique<int>(0);
		int values[64] = { 1, 0 };
		int operator()(int input) const
		{
			for (std::size_t index = 0; values[index] != 0; ++index)
			{
				input += values[index];
			}
			return input + *m_zero;
		}
	};
	static_assert(sizeof(plus_values) > sh::detail::move_only_function_storage::capacity, "functor_large test isn't storing large callable.");
	move_only_function<int(int)> x(plus_values{});
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 1);
}
TEST(sh_move_only_function, lambda_small)
{
	auto plus_3 = [
		three = std::make_unique<int>(3)
	](const int input) -> int
	{
		return input + *three;
	};
	static_assert(sizeof(plus_3) <= sh::detail::move_only_function_storage::capacity, "lambda_small test isn't storing small callable.");
	move_only_function<int(int)> x(std::move(plus_3));
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 3);
}
TEST(sh_move_only_function, lambda_large)
{
	int values[64] = { 1, 0 };
	auto plus_values = [
		values,
		zero = std::make_unique<int>(0)
	](int input) -> int
	{
		for (std::size_t index = 0; values[index] != 0; ++index)
		{
			input += values[index];
		}
		return input + *zero;
	};
	static_assert(sizeof(plus_values) > sh::detail::move_only_function_storage::capacity, "lambda_large test isn't storing large callable.");
	move_only_function<int(int)> x(std::move(plus_values));
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 1);
}
