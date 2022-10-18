/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include "roq/client.hpp"

#include "roq/samples/example-3/instrument.hpp"
#include "roq/samples/example-3/model.hpp"

namespace roq {
namespace samples {
namespace example_3 {

class Strategy final : public client::Handler {
 public:
  explicit Strategy(client::Dispatcher &);

  Strategy(Strategy &&) = default;
  Strategy(Strategy const &) = delete;

 protected:
  void operator()(Event<Timer> const &) override;
  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;
  void operator()(Event<DownloadBegin> const &) override;
  void operator()(Event<DownloadEnd> const &) override;
  void operator()(Event<GatewayStatus> const &) override;
  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;
  void operator()(Event<MarketByPriceUpdate> const &) override;
  void operator()(Event<OrderAck> const &) override;
  void operator()(Event<OrderUpdate> const &) override;
  void operator()(Event<TradeUpdate> const &) override;
  void operator()(Event<PositionUpdate> const &) override;
  void operator()(Event<FundsUpdate> const &) override;

  // helper - dispatch event to instrument
  template <typename T>
  void dispatch(T const &event) {
    assert(event.message_info.source == 0);
    instrument_(event.value);
  }

  void update_model();

  void try_trade(Side, double price);

 private:
  client::Dispatcher &dispatcher_;
  Instrument instrument_;
  uint32_t max_order_id_ = {};
  Model model_;
  std::chrono::nanoseconds next_sample_ = {};
  uint32_t working_order_id_ = {};
  Side working_side_ = {};
};

}  // namespace example_3
}  // namespace samples
}  // namespace roq
