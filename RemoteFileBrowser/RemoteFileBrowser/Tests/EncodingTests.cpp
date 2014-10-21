#include "PrecompiledHeader.h"

#if _TESTBUILD

#include "CppUnitTest.h"
#include "Utilities\Utilities.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace Utilities;

TEST_CLASS(EncodingTests)
{
public:
	TEST_METHOD(CanConvertUtf8ToUtf16AsciiCharacters)
	{
		const auto expected = L"0123456789/*-+.,/;:\'[]{}()-_!@#$%^&*\\`~ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
		const auto actualStr = Encoding::Utf8ToUtf16("0123456789/*-+.,/;:\'[]{}()-_!@#$%^&*\\`~ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

		Assert::AreEqual(expected, actualStr.c_str());
	}

	TEST_METHOD(CanConvertUtf16ToUtf8AsciiCharacters)
	{
		const auto expected = "0123456789/*-+.,/;:\'[]{}()-_!@#$%^&*\\`~ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
		const auto actualStr = Encoding::Utf16ToUtf8(L"0123456789/*-+.,/;:\'[]{}()-_!@#$%^&*\\`~ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

		Assert::AreEqual(expected, actualStr.c_str());
	}

	TEST_METHOD(CanConvertUtf8ToUtf16NonAsciiCharacters)
	{
		const auto expected = L"àèæëáðøûþÀÈÆËÁÐØÛÞ";
		const uint8_t utf8[] =
		{
			0xc4, 0x85, 0xc4, 0x8d, 0xc4, 0x99, 0xc4, 0x97, 0xc4,
			0xaf, 0xc5, 0xa1, 0xc5, 0xb3, 0xc5, 0xab, 0xc5, 0xbe,
			0xc4, 0x84, 0xc4, 0x8c, 0xc4, 0x98, 0xc4, 0x96, 0xc4,
			0xae, 0xc5, 0xa0, 0xc5, 0xb2, 0xc5, 0xaa, 0xc5, 0xbd,
			0x00
		};

		const auto actualStr = Encoding::Utf8ToUtf16(reinterpret_cast<const char*>(utf8));

		Assert::AreEqual(expected, actualStr.c_str());
	}

	TEST_METHOD(CanConvertUtf16ToUtf8NonAsciiCharacters)
	{
		const uint8_t expected[] = 
		{ 
			0xc4, 0x85, 0xc4, 0x8d, 0xc4, 0x99, 0xc4, 0x97,	0xc4,
			0xaf, 0xc5, 0xa1, 0xc5, 0xb3, 0xc5, 0xab, 0xc5, 0xbe, 
			0xc4, 0x84, 0xc4, 0x8c, 0xc4, 0x98, 0xc4, 0x96, 0xc4, 
			0xae, 0xc5, 0xa0, 0xc5, 0xb2, 0xc5, 0xaa, 0xc5, 0xbd,
			0x00
		};

		const auto actualStr = Encoding::Utf16ToUtf8(L"àèæëáðøûþÀÈÆËÁÐØÛÞ");

		Assert::AreEqual(reinterpret_cast<const char*>(expected), actualStr.c_str());
	}
};

#endif // _TESTBUILD
