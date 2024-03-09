#include <gtest/gtest.h>

#include <sh/inplace_copyable_function.hpp>

using sh::inplace_copyable_function;

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

TEST(sh_inplace_copyable_function, ctor_default)
{
	inplace_copyable_function<int(int), sizeof(void*)> x;
	EXPECT_FALSE(bool(x));
	EXPECT_EQ(x, nullptr);
}
TEST(sh_inplace_copyable_function, ctor_nullptr)
{
	inplace_copyable_function<int(int), sizeof(void*)> x(nullptr);
	EXPECT_FALSE(bool(x));
	EXPECT_EQ(x, nullptr);
}
TEST(sh_inplace_copyable_function, ctor_move)
{
	int value = 0;
	{
		auto lambda = [c = counter(&value)]() { };

		inplace_copyable_function<void(), sizeof(lambda)> x(std::move(lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		EXPECT_EQ(value, 1);

		inplace_copyable_function<void(), sizeof(lambda)> y = std::move(x);
		ASSERT_FALSE(bool(x));
		ASSERT_EQ(x, nullptr);
		ASSERT_TRUE(bool(y));
		ASSERT_NE(y, nullptr);
		EXPECT_EQ(value, 1);
	}
	EXPECT_EQ(value, 0);
}
TEST(sh_inplace_copyable_function, assign)
{
	auto lambda = []() { return 'x'; };
	inplace_copyable_function<char(), sizeof(lambda)> x;
	ASSERT_FALSE(bool(x));
	ASSERT_EQ(x, nullptr);

	x = std::move(lambda);
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	ASSERT_EQ(x(), 'x');
}
TEST(sh_inplace_copyable_function, assign_nullptr)
{
	int value = 0;
	auto lambda = [c = counter(&value)]() { return 'x'; };
	inplace_copyable_function<char(), sizeof(lambda)> x(std::move(lambda));
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	ASSERT_EQ(x(), 'x');
	EXPECT_EQ(value, 1);

	x = nullptr;
	ASSERT_FALSE(bool(x));
	ASSERT_EQ(x, nullptr);
	EXPECT_EQ(value, 0);
}
TEST(sh_inplace_copyable_function, assign_move)
{
	int a_value = 0, b_value = 0;
	{
		auto a_lambda = [c = counter(&a_value)]() { return 'a'; };
		auto b_lambda = [c = counter(&b_value)]() { return 'b'; };

		inplace_copyable_function<char(), std::max(sizeof(a_lambda), sizeof(b_lambda))> x(std::move(a_lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		ASSERT_EQ(x(), 'a');
		EXPECT_EQ(a_value, 1);

		inplace_copyable_function<char(), std::max(sizeof(a_lambda), sizeof(b_lambda))> y(std::move(b_lambda));
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
TEST(sh_inplace_copyable_function, swap_empty)
{
	int value = 0;
	{
		auto lambda = [c = counter(&value)]() { };

		inplace_copyable_function<void(), sizeof(lambda)> x(std::move(lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		EXPECT_EQ(value, 1);

		inplace_copyable_function<void(), sizeof(lambda)> y;
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
TEST(sh_inplace_copyable_function, swap)
{
	int a_value = 0, b_value = 0;
	{
		auto a_lambda = [c = counter(&a_value)]() { return 'a'; };
		auto b_lambda = [c = counter(&b_value)]() { return 'b'; };

		inplace_copyable_function<char(), std::max(sizeof(a_lambda), sizeof(b_lambda))> x(std::move(a_lambda));
		ASSERT_TRUE(bool(x));
		ASSERT_NE(x, nullptr);
		ASSERT_EQ(x(), 'a');
		EXPECT_EQ(a_value, 1);

		inplace_copyable_function<char(), std::max(sizeof(a_lambda), sizeof(b_lambda))> y(std::move(b_lambda));
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
TEST(sh_inplace_copyable_function, call)
{
	const auto lambda = [](bool& called) { called = true; };
	bool called = false;
	inplace_copyable_function<void(bool&), sizeof(lambda)> x{ lambda };
	x(called);
	EXPECT_TRUE(called);
}
TEST(sh_inplace_copyable_function, func)
{
	inplace_copyable_function<int(int), sizeof(&plus_1)> x(plus_1);
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 1);
}
TEST(sh_inplace_copyable_function, func_ptr)
{
	inplace_copyable_function<int(int), sizeof(&plus_1)> x(&plus_1);
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 1);
}
TEST(sh_inplace_copyable_function, functor)
{
	struct plus_2 final
	{
		int value{ 0 };

		int operator()(const int input)
		{
			value = input;
			return input + 2;
		}
	};
	inplace_copyable_function<int(int), sizeof(plus_2)> x(plus_2{});
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 2);
}
TEST(sh_inplace_copyable_function, functor_const)
{
	struct plus_2 final
	{
		int operator()(const int input) const
		{
			return input + 2;
		}
	};
	inplace_copyable_function<int(int), sizeof(plus_2)> x(plus_2{});
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 2);
}
TEST(sh_inplace_copyable_function, lambda)
{
	auto plus_3 = [
		three = 3
	](const int input) -> int
	{
		return input + three;
	};
	inplace_copyable_function<int(int), sizeof(plus_3)> x(std::move(plus_3));
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 3);
}
TEST(sh_inplace_copyable_function, lambda_mutable)
{
	auto plus_3 = [
		three = 3,
		value = 0
	](const int input) mutable -> int
	{
		value = input;
		return input + three;
	};
	inplace_copyable_function<int(int), sizeof(plus_3)> x(std::move(plus_3));
	ASSERT_TRUE(bool(x));
	ASSERT_NE(x, nullptr);
	EXPECT_EQ(x(0), 3);
}
