/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/samples/example-3/strategy.h"

#include <limits>

#include "roq/logging.h"

#include "roq/samples/example-3/flags.h"
#include "roq/samples/example-3/utilities.h"

using namespace roq::literals;

namespace roq {
namespace samples {
namespace example_3 {

Strategy::Strategy(client::Dispatcher &dispatcher)
    : dispatcher_(dispatcher), instrument_(Flags::exchange(), Flags::symbol(), Flags::account()) {
}

void Strategy::operator()(const Event<Timer> &event) {
  // note! using system clock (*not* the wall clock)
  if (event.value.now < next_sample_)
    return;
  if (next_sample_ != next_sample_.zero())  // initialized?
    update_model();
  auto now = std::chrono::duration_cast<std::chrono::seconds>(event.value.now);
  next_sample_ = now + std::chrono::seconds{Flags::sample_freq_secs()};
  // possible extension: reset request timeout
}

void Strategy::operator()(const Event<Connection> &event) {
  dispatch(event);
}

void Strategy::operator()(const Event<DownloadBegin> &event) {
  dispatch(event);
}

void Strategy::operator()(const Event<DownloadEnd> &event) {
  dispatch(event);
  if (update(max_order_id_, event.value.max_order_id)) {
    LOG(INFO)(R"(max_order_id={})"_fmt, max_order_id_);
  }
}

void Strategy::operator()(const Event<MarketDataStatus> &event) {
  dispatch(event);
}

void Strategy::operator()(const Event<OrderManagerStatus> &event) {
  dispatch(event);
}

void Strategy::operator()(const Event<ReferenceData> &event) {
  dispatch(event);
}

void Strategy::operator()(const Event<MarketStatus> &event) {
  dispatch(event);
}

void Strategy::operator()(const Event<MarketByPriceUpdate> &event) {
  dispatch(event);
}

void Strategy::operator()(const Event<OrderAck> &event) {
  LOG(INFO)(R"(OrderAck={})"_fmt, event.value);
  auto &order_ack = event.value;
  if (is_request_complete(order_ack.status)) {
    // possible extension: reset request timeout
  }
}

void Strategy::operator()(const Event<OrderUpdate> &event) {
  LOG(INFO)(R"(OrderUpdate={})"_fmt, event.value);
  dispatch(event);  // update position
  auto &order_update = event.value;
  if (is_order_complete(order_update.status)) {
    working_order_id_ = 0u;
    working_side_ = Side::UNDEFINED;
  }
}

void Strategy::operator()(const Event<TradeUpdate> &event) {
  LOG(INFO)(R"(TradeUpdate={})"_fmt, event.value);
}

void Strategy::operator()(const Event<PositionUpdate> &event) {
  dispatch(event);
}

void Strategy::operator()(const Event<FundsUpdate> &event) {
  LOG(INFO)(R"(FundsUpdate={})"_fmt, event.value);
}

void Strategy::update_model() {
  if (instrument_.is_ready()) {
    auto side = model_.update(instrument_);
    switch (side) {
      case Side::UNDEFINED:
        // nothing to do
        break;
      case Side::BUY:
        try_trade(side, instrument_.best_bid());
        break;
      case Side::SELL:
        try_trade(side, instrument_.best_ask());
        break;
    }
  } else {
    model_.reset();
  }
}

void Strategy::try_trade(Side side, double price) {
  if (!Flags::enable_trading()) {
    LOG(WARNING)("Trading *NOT* enabled"_fmt);
    return;
  }
  // if buy:
  //   if sell order outstanding
  //     cancel old order
  //   if position not long
  //     send buy order
  //
  if (working_order_id_) {
    LOG(INFO)("*** ANOTHER ORDER IS WORKING ***"_fmt);
    if (side != working_side_) {
      LOG(INFO)("*** CANCEL WORKING ORDER ***"_fmt);
      dispatcher_.send(
          CancelOrder{
              .account = Flags::account(),
              .order_id = working_order_id_,
          },
          0u);
    }
    return;
  }
  if (!instrument_.can_trade(side)) {
    LOG(INFO)("*** CAN'T INCREASE POSITION ***"_fmt);
    return;
  }
  auto order_id = ++max_order_id_;
  dispatcher_.send(
      CreateOrder{
          .account = Flags::account(),
          .order_id = order_id,
          .exchange = Flags::exchange(),
          .symbol = Flags::symbol(),
          .side = side,
          .quantity = instrument_.min_trade_vol(),
          .order_type = OrderType::LIMIT,
          .price = price,
          .time_in_force = TimeInForce::GTC,
          .position_effect = PositionEffect::UNDEFINED,
          .execution_instruction = ExecutionInstruction::UNDEFINED,
          .stop_price = NaN,
          .max_show_quantity = NaN,
          .order_template = {},
      },
      0u);
  working_order_id_ = order_id;
  working_side_ = side;
  // possible extension: monitor for request timeout
}

}  // namespace example_3
}  // namespace samples
}  // namespace roq
