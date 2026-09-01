#include <cstdint>

#include "gtest/gtest.h"
#include "port/win32/win32_com.h"
#include "ra/wolapi/wolapi.h"

namespace {

// The canonical text form of a GUID, as the IDL and the registry write it:
// 4DD3BAF4-7579-11D1-B1C6-006097176556. The first three fields are stored
// little-endian and the last eight bytes in order, which is exactly the trap
// these tests exist to catch -- an IID whose fields are the wrong width, or
// whose bytes come out reordered, still compiles and still initializes.
IID FromString(std::uint32_t data1, std::uint16_t data2, std::uint16_t data3,
               std::uint64_t tail) {
  IID iid{data1, data2, data3, {}};
  for (int i = 0; i < 8; ++i) {
    iid.Data4[i] = static_cast<std::uint8_t>(tail >> (8U * (7 - i)));
  }
  return iid;
}

TEST(WolapiIidsTest, AGuidIsSixteenBytesWithNoPadding) {
  // wolapi_i.c declared its own IID whose first field was `unsigned long`. On
  // a 64-bit host that is eight bytes, which would push every field below it
  // out of place and change nothing the compiler would complain about.
  EXPECT_EQ(sizeof(IID), 16U);
  EXPECT_EQ(sizeof(GUID{}.Data1), 4U);
  EXPECT_EQ(sizeof(GUID{}.Data2), 2U);
  EXPECT_EQ(sizeof(GUID{}.Data3), 2U);
  EXPECT_EQ(sizeof(GUID{}.Data4), 8U);
}

TEST(WolapiIidsTest, TheInterfaceIidsMatchTheIdl) {
  EXPECT_EQ(IID_IChat,
            FromString(0x4DD3BAF4, 0x7579, 0x11D1, 0xB1C600609717'6556));
  EXPECT_EQ(IID_IChatEvent,
            FromString(0x4DD3BAF6, 0x7579, 0x11D1, 0xB1C600609717'6556));
  EXPECT_EQ(IID_IDownload,
            FromString(0x0BF5FCEB, 0x9F03, 0x11D1, 0x9DC70060'97C54321));
  EXPECT_EQ(IID_IDownloadEvent,
            FromString(0x6869E99D, 0x9FB4, 0x11D1, 0x9DC80060'97C54321));
  EXPECT_EQ(IID_INetUtil,
            FromString(0xB832B0AA, 0xA7D3, 0x11D1, 0x97C30060'9706FA0C));
  EXPECT_EQ(IID_INetUtilEvent,
            FromString(0xB832B0AC, 0xA7D3, 0x11D1, 0x97C30060'9706FA0C));
}

TEST(WolapiIidsTest, TheClassIdsMatchTheIdl) {
  EXPECT_EQ(CLSID_Chat,
            FromString(0x4DD3BAF5, 0x7579, 0x11D1, 0xB1C600609717'6556));
  EXPECT_EQ(CLSID_Download,
            FromString(0xBF6EA206, 0x9E55, 0x11D1, 0x9DC60060'97C54321));
  EXPECT_EQ(CLSID_NetUtil,
            FromString(0xB832B0AB, 0xA7D3, 0x11D1, 0x97C30060'9706FA0C));
}

TEST(WolapiIidsTest, DistinctInterfacesHaveDistinctIids) {
  EXPECT_NE(IID_IChat, IID_IChatEvent);
  EXPECT_NE(IID_IChat, CLSID_Chat) << "the interface and its class differ";
  EXPECT_NE(IID_INetUtil, IID_INetUtilEvent);
}

}  // namespace
