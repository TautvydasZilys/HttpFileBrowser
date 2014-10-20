#include "PrecompiledHeader.h"

#if _TESTBUILD

#include "CppUnitTest.h"
#include "Utilities\Utilities.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace Utilities;

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
				Assert::AreEqual(L"Hello, world!", GetLogFileContents()[0].c_str());
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
};

#endif // _TESTBUILD