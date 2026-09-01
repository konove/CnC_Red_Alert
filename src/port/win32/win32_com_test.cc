#include "port/win32/win32_com.h"

#include "gtest/gtest.h"

#ifndef _WIN32

namespace {

TEST(Win32ComTest, SeverityDecidesSuccess) {
  EXPECT_TRUE(SUCCEEDED(S_OK));
  EXPECT_TRUE(SUCCEEDED(S_FALSE));
  EXPECT_FALSE(SUCCEEDED(E_FAIL));
  EXPECT_TRUE(FAILED(E_NOINTERFACE));
  EXPECT_FALSE(FAILED(S_OK));
}

TEST(Win32ComTest, MakeHresultPacksTheWayWindowsDoes) {
  // The WOL error tables are built from this, and their values have to match
  // the ones wolapi.dll returned.
  EXPECT_EQ(MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0), S_OK);
  EXPECT_EQ(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x200),
            static_cast<HRESULT>(0x80040200U));
  EXPECT_TRUE(FAILED(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1)));
  EXPECT_TRUE(SUCCEEDED(MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 1)));
}

TEST(Win32ComTest, GuidsCompareByValue) {
  constexpr GUID kChat = {0x4DD3BAF4,
                          0x7579,
                          0x11D1,
                          {0xB1, 0xC6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};
  constexpr GUID kSame = {0x4DD3BAF4,
                          0x7579,
                          0x11D1,
                          {0xB1, 0xC6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};
  constexpr GUID kOther = {0x4DD3BAF6,
                           0x7579,
                           0x11D1,
                           {0xB1, 0xC6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};

  EXPECT_EQ(kChat, kSame);
  EXPECT_NE(kChat, kOther);
  EXPECT_NE(IID_IUnknown, IID_IConnectionPoint);
}

TEST(Win32ComTest, CreatingAnObjectFailsAndClearsTheOutParameter) {
  // Nothing can be created: there is no wolapi.dll. Callers must see both the
  // failure code and a null pointer.
  constexpr CLSID kChatClass = {
      0x4DD3BAF2,
      0x7579,
      0x11D1,
      {0xB1, 0xC6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56}};

  int marker = 0;
  void* object = &marker;  // Deliberately not null to start with.
  const HRESULT result = CoCreateInstance(
      kChatClass, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, &object);

  EXPECT_EQ(result, REGDB_E_CLASSNOTREG);
  EXPECT_TRUE(FAILED(result));
  EXPECT_EQ(object, nullptr);
}

}  // namespace

#endif  // _WIN32
