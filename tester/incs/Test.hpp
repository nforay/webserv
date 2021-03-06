#ifndef TEST_HPP
#define TEST_HPP

#include <exception>
#include <iostream>

constexpr const char* msg_ko = "\033[31mKO";
constexpr const char* msg_ok =  "\033[32mOK";
constexpr const char* msg_reset = "\033[39m";

constexpr auto NoThrowTest = [](auto snippet) { try { snippet(); } catch (std::exception& e) { return false; } return true; };
constexpr auto ThrowTest = [](auto snippet) { try { snippet(); } catch (std::exception& e) { return true; } return false; };

constexpr auto EqualTest = [](auto a, auto b) { return a() == b(); };
constexpr auto NotEqualTest = [](auto a, auto b) { return a() != b(); };
constexpr auto GreaterTest = [](auto a, auto b) { return a() > b(); };
constexpr auto GreaterEqualTest = [](auto a, auto b) { return a() >= b(); };
constexpr auto LessTest = [](auto a, auto b) { return a() < b(); };
constexpr auto LessEqualTest = [](auto a, auto b) { return a() <= b(); };

constexpr auto StringStartsWithTest = [](auto str, auto startswith) { auto s = str(); auto s2 = startswith(); return s.size() >= s2.size() && s.substr(0, s2.size()) == s2; };
constexpr auto StringContainsTest = [](auto str, auto contains) { auto s = str(); auto s2 = contains(); return s.find(s2) != std::string::npos; };
constexpr auto StringStartsWithTestAndContains = [](auto str, auto startswith, auto contains) { auto s = str(); auto s2 = startswith(); auto s3 = contains(); return s.size() >= s2.size() && s.substr(0, s2.size()) == s2 && s.find(s3) != std::string::npos; };
constexpr auto StringStartsWithTestAndContainsNot = [](auto str, auto startswith, auto contains) { auto s = str(); auto s2 = startswith(); auto s3 = contains(); return s.size() >= s2.size() && s.substr(0, s2.size()) == s2 && s.find(s3) == std::string::npos; };


class Test
{
	protected:
		std::string m_label;
		uint32_t m_id;
		bool m_is_valid = false;

	public:
		Test() = default;
		virtual ~Test() = default;
		Test(const Test&) = default;
		Test(const std::string& label, uint32_t id) : m_label(label), m_id(id), m_is_valid(false) {}

		template <auto ValidityCheck, class... Args>
		bool check(Args... args)
		{
			return m_is_valid = ValidityCheck(args...);
		}

		friend std::ostream& operator<<(std::ostream& out, const Test& test);
};



std::ostream& operator<<(std::ostream& out, const Test& test)
{
	out << test.m_label << " " << (test.m_is_valid ? msg_ok : msg_ko) << msg_reset;
	return out;
}

#endif
