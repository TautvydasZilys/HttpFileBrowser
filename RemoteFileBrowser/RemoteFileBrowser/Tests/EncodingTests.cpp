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

	TEST_METHOD(CanDecodeUrl)
	{
		const string expected = Encoding::Utf16ToUtf8(L"http://àèæëá.remoteFileBrowser.net/E:\\my docs\\C++\\memory leak.exe");
		const string actual = Encoding::DecodeUrl("http%3A%2F%2F%C4%85%C4%8D%C4%99%C4%97%C4%AF.remoteFileBrowser.net%2FE%3A%5Cmy%20docs%5CC%2B%2B%5Cmemory%20leak.exe");

		Assert::AreEqual(expected, actual);
	}

	TEST_METHOD(CanEncodeUrl)
	{
		const string expected = "http%3A%2F%2F%C4%85%C4%8D%C4%99%C4%97%C4%AF.remoteFileBrowser.net%2FE%3A%5Cmy%20docs%5CC%2B%2B%5Cmemory%20leak.exe";
		const string actual = Encoding::EncodeUrl(Encoding::Utf16ToUtf8(L"http://àèæëá.remoteFileBrowser.net/E:\\my docs\\C++\\memory leak.exe"));

		Assert::AreEqual(expected, actual);
	}

	TEST_METHOD(CanEncodeToBase64)
	{
		const string expected = "UHJlcGFyZSB5b3Vyc2VsdmVzLCB0aGUgYmVsbHMgaGF2ZSB0b2xsZWQhIFNoZWx0ZXIgeW91ciB3ZWFrLCB5"
								"b3VyIHlvdW5nIGFuZCB5b3VyIG9sZCEgRWFjaCBvZiB5b3Ugc2hhbGwgcGF5IHRoZSBmaW5hbCBzdW0hIENy"
								"eSBmb3IgbWVyY3k7IHRoZSByZWNrb25pbmcgaGFzIGNvbWUhIFRoZSBza3kgaXMgZGFyay4gVGhlIGZpcmUg"
								"YnVybnMuIFlvdSBzdHJpdmUgaW4gdmFpbiBhcyBGYXRlJ3Mgd2hlZWwgdHVybnMuIFRoZSB0b3duIHN0aWxs"
								"IGJ1cm5zLiBBIGNsZWFuc2luZyBmaXJlISBUaW1lIGlzIHNob3J0LCBJJ2xsIHNvb24gcmV0aXJlISBNeSBm"
								"bGFtZXMgaGF2ZSBkaWVkLCBsZWZ0IG5vdCBhIHNwYXJrISBJIHNoYWxsIHNlbmQgeW91IG15c2VsZiB0byB0"
								"aGUgbGlmZWxlc3MgZGFyayEgRmlyZSBjb25zdW1lcyEgWW91J3ZlIHRyaWVkIGFuZCBmYWlsZWQuIExldCB0"
								"aGVyZSBiZSBubyBkb3VidCwganVzdGljZSBwcmV2YWlsZWQhIFNvIGVhZ2VyIHlvdSBhcmUsIGZvciBteSBi"
								"bG9vZCB0byBzcGlsbC4gWWV0IHRvIHZhbnF1aXNoIG1lLCAndGlzIG15IGhlYWQgeW91IG11c3Qga2lsbCE=";

		const string actual = Encoding::EncodeBase64(
			"Prepare yourselves, the bells have tolled! "
			"Shelter your weak, your young and your old! "
			"Each of you shall pay the final sum! "
			"Cry for mercy; the reckoning has come! "

			"The sky is dark. The fire burns. "
			"You strive in vain as Fate's wheel turns. "
			"The town still burns. A cleansing fire! "
			"Time is short, I'll soon retire! "

			"My flames have died, left not a spark! "
			"I shall send you myself to the lifeless dark! "
			"Fire consumes! You've tried and failed. "
			"Let there be no doubt, justice prevailed! "

			"So eager you are, for my blood to spill. "
			"Yet to vanquish me, 'tis my head you must kill!"
		);

		Assert::AreEqual(expected, actual);
	}
};

#endif // _TESTBUILD
