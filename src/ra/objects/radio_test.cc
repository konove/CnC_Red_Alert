#include "ra/objects/radio.h"

#include <cstdint>
#include <optional>
#include <vector>

#include "gtest/gtest.h"
#include "ra/core/defines.h"

namespace ra {
namespace {

// A radio owner that is not a game object: it answers the handshake the way
// its radio says and everything else with `answer`.
class FakePeer : public RadioPeer {
 public:
  RadioMessageType Receive(RadioPeer* from, RadioMessageType message,
                           int32_t& /*param*/) override {
    senders.push_back(from);
    received.push_back(message);
    if (const std::optional<RadioMessageType> handled =
            radio.Receive(from, message, alive)) {
      return *handled;
    }
    return answer;
  }

  RadioMessageType Transmit(RadioMessageType message, RadioPeer* to = nullptr) {
    return radio.Transmit(*this, message, RadioClass::DiscardedParam(), to);
  }

  RadioClass radio;
  bool alive = true;
  RadioMessageType answer = RadioMessageType::RADIO_STATIC;
  std::vector<RadioPeer*> senders;
  std::vector<RadioMessageType> received;
};

using enum RadioMessageType;

TEST(RadioTest, StartsOutOfContact) {
  const RadioClass radio;
  EXPECT_FALSE(radio.InContact());
  EXPECT_EQ(radio.contact(), nullptr);
}

TEST(RadioTest, AMessageToNobodyIsStatic) {
  FakePeer alone;
  EXPECT_EQ(alone.Transmit(RADIO_REDRAW), RADIO_STATIC);
}

TEST(RadioTest, HelloPutsBothSidesInContact) {
  FakePeer caller;
  FakePeer callee;
  EXPECT_EQ(caller.Transmit(RADIO_HELLO, &callee), RADIO_ROGER);
  EXPECT_EQ(caller.radio.contact(), &callee);
  EXPECT_EQ(callee.radio.contact(), &caller);
  EXPECT_TRUE(caller.radio.InContact());
}

TEST(RadioTest, AMessageWithoutARecipientGoesToTheContact) {
  FakePeer caller;
  FakePeer callee;
  callee.answer = RADIO_ROGER;
  caller.Transmit(RADIO_HELLO, &callee);
  EXPECT_EQ(caller.Transmit(RADIO_REDRAW), RADIO_ROGER);
  EXPECT_EQ(callee.received.back(), RADIO_REDRAW);
  EXPECT_EQ(callee.senders.back(), &caller);
}

TEST(RadioTest, HelloAgainFromTheContactIsAccepted) {
  FakePeer caller;
  FakePeer callee;
  caller.Transmit(RADIO_HELLO, &callee);
  EXPECT_EQ(caller.Transmit(RADIO_HELLO, &callee), RADIO_ROGER);
  EXPECT_EQ(caller.radio.contact(), &callee);
  EXPECT_EQ(callee.radio.contact(), &caller);
}

TEST(RadioTest, ABusyRadioRefusesAThirdParty) {
  FakePeer caller;
  FakePeer callee;
  FakePeer third;
  caller.Transmit(RADIO_HELLO, &callee);
  EXPECT_EQ(third.Transmit(RADIO_HELLO, &callee), RADIO_NEGATIVE);
  EXPECT_FALSE(third.radio.InContact());
  EXPECT_EQ(callee.radio.contact(), &caller);
}

TEST(RadioTest, HelloToSomeoneElseHangsUpOnTheOldContactFirst) {
  FakePeer caller;
  FakePeer old_contact;
  FakePeer new_contact;
  caller.Transmit(RADIO_HELLO, &old_contact);
  EXPECT_EQ(caller.Transmit(RADIO_HELLO, &new_contact), RADIO_ROGER);
  EXPECT_FALSE(old_contact.radio.InContact());
  EXPECT_EQ(old_contact.received.back(), RADIO_OVER_OUT);
  EXPECT_EQ(caller.radio.contact(), &new_contact);
}

TEST(RadioTest, ARefusedHelloStillLosesTheOldContact) {
  FakePeer caller;
  FakePeer old_contact;
  FakePeer busy;
  FakePeer other;
  caller.Transmit(RADIO_HELLO, &old_contact);
  other.Transmit(RADIO_HELLO, &busy);
  EXPECT_EQ(caller.Transmit(RADIO_HELLO, &busy), RADIO_NEGATIVE);
  EXPECT_FALSE(caller.radio.InContact());
  EXPECT_FALSE(old_contact.radio.InContact());
}

TEST(RadioTest, OverAndOutEndsTheContactOnBothSides) {
  FakePeer caller;
  FakePeer callee;
  caller.Transmit(RADIO_HELLO, &callee);
  EXPECT_EQ(caller.Transmit(RADIO_OVER_OUT), RADIO_ROGER);
  EXPECT_FALSE(caller.radio.InContact());
  EXPECT_FALSE(callee.radio.InContact());
}

TEST(RadioTest, OverAndOutFromAStrangerIsLeftToTheOwner) {
  FakePeer caller;
  FakePeer callee;
  FakePeer stranger;
  caller.Transmit(RADIO_HELLO, &callee);
  EXPECT_EQ(callee.radio.Receive(&stranger, RADIO_OVER_OUT, true),
            std::nullopt);
  EXPECT_EQ(callee.radio.contact(), &caller);
}

TEST(RadioTest, AnOwnerThatCannotAnswerLeavesHelloUnhandled) {
  FakePeer caller;
  FakePeer dead;
  dead.alive = false;
  dead.answer = RADIO_STATIC;
  EXPECT_EQ(caller.Transmit(RADIO_HELLO, &dead), RADIO_NEGATIVE);
  EXPECT_FALSE(caller.radio.InContact());
  EXPECT_FALSE(dead.radio.InContact());
}

TEST(RadioTest, OffDropsTheContactWithoutTellingIt) {
  FakePeer caller;
  FakePeer callee;
  caller.Transmit(RADIO_HELLO, &callee);
  caller.radio.Off();
  EXPECT_FALSE(caller.radio.InContact());
  EXPECT_EQ(callee.radio.contact(), &caller);
}

TEST(RadioTest, SetContactRestoresALoadedContact) {
  FakePeer owner;
  FakePeer contact;
  owner.radio.set_contact(&contact);
  EXPECT_EQ(owner.radio.contact(), &contact);
}

}  // namespace
}  // namespace ra
