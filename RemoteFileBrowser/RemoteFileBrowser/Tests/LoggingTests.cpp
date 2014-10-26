#include "PrecompiledHeader.h"

#if _TESTBUILD

#include "CppUnitTest.h"
#include "Utilities\Utilities.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace Utilities;
#include <codecvt>

TEST_CLASS(LoggingTests)
{
private:
	wstring m_LogFileName;

public:
	LoggingTests() :
		m_LogFileName(Logging::GetLogFileName())
	{
	}
	
	template <typename LogAction, typename AssertAction>
	void DoTest(LogAction logAction, AssertAction assertAction)
	{
		Logging::Initialize(true);
		logAction();
		Logging::Shutdown();

		assertAction();
	}

	vector<wstring> GetLogFileContents()
	{
		wifstream in(m_LogFileName);
		vector<wstring> input;
		wstring line;

		locale oldLocale;
		locale utf8locale(oldLocale, new codecvt_utf8_utf16<wchar_t>);
		in.imbue(utf8locale);

		if (in.peek() == 65279)	// Skip UTF8 BOM if it's the first character
			in.get();

		while (!in.eof())
		{
			getline(in, line);

			if (!in.fail())
			{
				input.push_back(move(line));
			}
		}

		return input;
	}

	void AssertEndsWith(const wstring& expected, const wstring& actual)
	{
		Assert::AreEqual(expected.c_str(), actual.c_str() + actual.length() - expected.length());
	}

	void AssertEndsWith(const vector<wstring>& expectedLines, const vector<wstring>& actualLines)
	{
		Assert::AreEqual(expectedLines.size(), actualLines.size());

		for (auto i = 0u; i < actualLines.size(); i++)
		{
			AssertEndsWith(expectedLines[i], actualLines[i]);
		}
	}

	TEST_METHOD(CanOutputMessage)
	{
		DoTest(
			[]()
			{
				Logging::OutputMessage("Hello, world!");
			},

			[this]()
			{
				auto expected = L"Hello, world!";
				auto actualStr = GetLogFileContents()[0];
				auto actual = actualStr.c_str();

				Assert::AreEqual(expected, actual);
			}
		);
	}

	TEST_METHOD(CanLogOnce)
	{
		DoTest(
			[]()
			{
				Logging::Log("Hello, world!");
			},
		
			[this]()
			{
				AssertEndsWith(L"Hello, world!", GetLogFileContents()[0]);
			}
		);
	}

	TEST_METHOD(CanLogMultipleMessages)
	{
		DoTest(
			[]()
			{
				Logging::Log("Hell", "o, w", "orld!");
			},

			[this]()
			{
				AssertEndsWith(L"Hello, world!", GetLogFileContents()[0].c_str());
			}
		);
	}

	TEST_METHOD(CanLogMultipleTimes)
	{
		DoTest(
			[]()
			{
				Logging::Log("Hello, world!");
				Logging::Log("Hello, world!");
				Logging::Log("Hello, world!");
			},

			[this]()
			{
				vector<wstring> expected;
				expected.push_back(L"Hello, world!");
				expected.push_back(L"Hello, world!");
				expected.push_back(L"Hello, world!");
				AssertEndsWith(expected, GetLogFileContents());
			}
		);
	}

	static const int kThreadCountForThreadSafeTest = 10;
	static const int kMessagesPerThreadForThreadSafeTest = 20;

	TEST_METHOD(LogIsThreadSafe)
	{
		DoTest(
			[]()
			{
				thread threads[kThreadCountForThreadSafeTest];

				for (int i = 0; i < kThreadCountForThreadSafeTest; i++)
				{
					threads[i] = thread([]()
					{
						for (int j = 0; j < kMessagesPerThreadForThreadSafeTest; j++)
						{
							Logging::Log("Hello, world!");							
						}
					});
				}

				for (int i = 0; i < kThreadCountForThreadSafeTest; i++)
				{
					threads[i].join();
				}
			},
				
			[this]()
			{
				vector<wstring> expected;

				for (int i = 0; i < kThreadCountForThreadSafeTest * kMessagesPerThreadForThreadSafeTest; i++)
				{
					expected.push_back(L"Hello, world!");
				}

				AssertEndsWith(expected, GetLogFileContents());
			}
		);
	}

	TEST_METHOD(LogFileIsUtf8)
	{
		DoTest(
			[]()
			{
			},

			[this]()
			{
				auto logContents = Utilities::FileSystem::ReadFileToVector(m_LogFileName);
				Assert::AreEqual(static_cast<size_t>(3), logContents.size());
				Assert::AreEqual(static_cast<uint8_t>(0xEF), logContents[0]);
				Assert::AreEqual(static_cast<uint8_t>(0xBB), logContents[1]);
				Assert::AreEqual(static_cast<uint8_t>(0xBF), logContents[2]);
			}
		);
	}
};

#endif // _TESTBUILD