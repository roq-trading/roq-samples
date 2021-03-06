/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/samples/example-2/instrument.h"

#include "roq/logging.h"

#include "roq/format.h"

#include "roq/samples/example-2/flags.h"

using namespace roq::literals;

namespace roq {
namespace samples {
namespace example_2 {

Instrument::Instrument(const std::string_view &exchange, const std::string_view &symbol)
    : exchange_(exchange), symbol_(symbol),
      depth_builder_(client::DepthBuilderFactory::create(symbol, depth_)) {
}

void Instrument::operator()(const Connection &connection) {
  if (update(connection_status_, connection.status)) {
    LOG(INFO)
    (R"([{}:{}] connection_status={})"_fmt, exchange_, symbol_, connection_status_);
    check_ready();
  }
  switch (connection_status_) {
    case ConnectionStatus::UNDEFINED:
      LOG(FATAL)("Unexpected"_fmt);
      break;
    case ConnectionStatus::CONNECTED:
      // nothing to do for this implementation
      break;
    case ConnectionStatus::DISCONNECTED:
      // reset all cached state - await download upon reconnection
      reset();
      break;
  }
}

void Instrument::operator()(const DownloadBegin &download_begin) {
  if (!download_begin.account.empty())  // we only care about market (not account)
    return;
  assert(!download_);
  download_ = true;
  LOG(INFO)(R"([{}:{}] download={})"_fmt, exchange_, symbol_, download_);
}

void Instrument::operator()(const DownloadEnd &download_end) {
  if (!download_end.account.empty())  // we only care about market (not account)
    return;
  assert(download_);
  download_ = false;
  LOG(INFO)(R"([{}:{}] download={})"_fmt, exchange_, symbol_, download_);
  // update the ready flag
  check_ready();
}

void Instrument::operator()(const MarketDataStatus &market_data_status) {
  // update our cache
  if (update(market_data_status_, market_data_status.status)) {
    LOG(INFO)
    (R"([{}:{}] market_data_status={})"_fmt, exchange_, symbol_, market_data_status_);
  }
  // update the ready flag
  check_ready();
}

void Instrument::operator()(const ReferenceData &reference_data) {
  assert(exchange_.compare(reference_data.exchange) == 0);
  assert(symbol_.compare(reference_data.symbol) == 0);
  // update the depth builder
  depth_builder_->update(reference_data);
  // update our cache
  if (update(tick_size_, reference_data.tick_size)) {
    LOG(INFO)(R"([{}:{}] tick_size={})"_fmt, exchange_, symbol_, tick_size_);
  }
  if (update(min_trade_vol_, reference_data.min_trade_vol)) {
    LOG(INFO)
    (R"([{}:{}] min_trade_vol={})"_fmt, exchange_, symbol_, min_trade_vol_);
  }
  if (update(multiplier_, reference_data.multiplier)) {
    LOG(INFO)(R"([{}:{}] multiplier={})"_fmt, exchange_, symbol_, multiplier_);
  }
  // update the ready flag
  check_ready();
}

void Instrument::operator()(const MarketStatus &market_status) {
  assert(exchange_.compare(market_status.exchange) == 0);
  assert(symbol_.compare(market_status.symbol) == 0);
  // update our cache
  if (update(trading_status_, market_status.trading_status)) {
    LOG(INFO)
    (R"([{}:{}] trading_status={})"_fmt, exchange_, symbol_, trading_status_);
  }
  // update the ready flag
  check_ready();
}

void Instrument::operator()(const MarketByPriceUpdate &market_by_price_update) {
  assert(exchange_.compare(market_by_price_update.exchange) == 0);
  assert(symbol_.compare(market_by_price_update.symbol) == 0);
  LOG_IF(INFO, download_)
  (R"(MarketByPriceUpdate={})"_fmt, market_by_price_update);
  // update depth
  // note!
  //   market by price only gives you *changes*.
  //   you will most likely want to use the the price to look up
  //   the relative position in an order book and then modify the
  //   liquidity.
  //   the depth builder helps you maintain a correct view of
  //   the order book.
  auto depth = depth_builder_->update(market_by_price_update);
  VLOG(1)
  (R"([{}:{}] depth=[{}])"_fmt, exchange_, symbol_, roq::join(depth_, ", "_sv));
  if (depth > 0 && is_ready())
    update_model();
}

void Instrument::operator()(const MarketByOrderUpdate &market_by_order_update) {
  assert(exchange_.compare(market_by_order_update.exchange) == 0);
  assert(symbol_.compare(market_by_order_update.symbol) == 0);
  LOG_IF(INFO, download_)
  (R"(MarketByOrderUpdate={})"_fmt, market_by_order_update);
  // update depth
  // note!
  //   market by order only gives you *changes*.
  //   you will most likely want to use the the price and order_id
  //   to look up the relative position in an order book and then
  //   modify the liquidity.
  //   the depth builder helps you maintain a correct view of
  //   the order book.
  auto depth = depth_builder_->update(market_by_order_update);
  VLOG(1)
  (R"([{}:{}] depth=[{}])"_fmt, exchange_, symbol_, roq::join(depth_, ", "_sv));
  if (depth > 0 && is_ready())
    update_model();
}

void Instrument::update_model() {
  // one sided market?
  if (is_zero(depth_[0].bid_quantity) || is_zero(depth_[0].ask_quantity))
    return;
  // validate depth
  auto spread = depth_[0].ask_price - depth_[0].bid_price;
  LOG_IF(FATAL, !is_strictly_positive(spread))
  (R"([{}:{}] Probably something wrong: )"
   R"(choice price or price inversion detected. depth=[{}])"_fmt,
   exchange_,
   symbol_,
   roq::join(depth_, ", "_sv));
  // compute (weighted) mid
  double sum_1 = 0.0, sum_2 = 0.0;
  for (auto &[bid_price, bid_quantity, ask_price, ask_quantity] : depth_) {
    sum_1 += bid_price * bid_quantity + ask_price * ask_quantity;
    sum_2 += bid_quantity + ask_quantity;
  }
  mid_price_ = sum_1 / sum_2;
  // update (exponential) moving average
  if (std::isnan(avg_price_))
    avg_price_ = mid_price_;
  else
    avg_price_ = Flags::alpha() * mid_price_ + (1.0 - Flags::alpha()) * avg_price_;
  // only verbose logging
  VLOG(1)
  (R"([{}:{}] model={{mid_price={}, avg_price={}}})"_fmt,
   exchange_,
   symbol_,
   mid_price_,
   avg_price_);
}

void Instrument::check_ready() {
  auto before = ready_;
  ready_ = connection_status_ == ConnectionStatus::CONNECTED && !download_ &&
           is_strictly_positive(tick_size_) && is_strictly_positive(min_trade_vol_) &&
           is_strictly_positive(multiplier_) && trading_status_ == TradingStatus::OPEN &&
           market_data_status_ == GatewayStatus::READY;
  LOG_IF(INFO, ready_ != before)
  (R"([{}:{}] ready={})"_fmt, exchange_, symbol_, ready_);
}

void Instrument::reset() {
  connection_status_ = ConnectionStatus::DISCONNECTED;
  download_ = false;
  tick_size_ = NaN;
  min_trade_vol_ = NaN;
  trading_status_ = TradingStatus::UNDEFINED;
  market_data_status_ = GatewayStatus::DISCONNECTED;
  depth_builder_->reset();
  mid_price_ = NaN;
  avg_price_ = NaN;
  ready_ = false;
}

}  // namespace example_2
}  // namespace samples
}  // namespace roq
