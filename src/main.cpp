// main.cpp
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <format>
#include <limits>
#include <optional>
#include <print>
#include <utility>

namespace ds_ob {

using usize = std::size_t;

constexpr bool k_verbose = true;
constexpr int max_number_live_orders = 100;

constexpr int missing_id = -1;
constexpr int missing_price = -1;
constexpr int missing_quantity = -1;

struct BestQuote {
    int price;
    int quantity; // aggregated quantity at that price
};

enum class Side {
    Bid,
    Ask,
};

std::string to_string(Side side) {
    switch(side) {
        case Side::Bid:
            return "Bid";
            break;
        case Side::Ask:
            return "Ask";
            break;
        std::unreachable();
    }
}

struct Order {
    int id{missing_id};
    Side side{Side::Bid};
    int price{missing_price};
    int quantity{missing_quantity};

    std::string to_string() const {
        return std::format("Order(id={},side={},price={},quantity={})", id, ds_ob::to_string(side), price, quantity);
    }
};

class OrderBook {
public:
    void on_add(int order_id, Side side, int price, int quantity) {
        // No need for checks as we never overflow by problem statement
        Order new_order{
            .id=order_id,
            .side=side,
            .price=price,
            .quantity=quantity
        };
        usize price_block_idx{0zu};
        int current_price{-1};
        if(side == Side::Ask) {
            const auto end = orders_ask_.begin() + orders_filled_ask_;
            auto it = std::lower_bound(
                orders_ask_.begin(),
                end,
                new_order,
                [](const Order& order1, const Order& order2) -> bool {
                    return (order1.price < order2.price);
                }
            );
            // Start index of price level
            usize idx = static_cast<usize>(it - orders_ask_.begin());
            if(it == end) {
                orders_ask_[orders_filled_ask_++] = new_order;
            } else {
                Order& order = orders_ask_[idx];
                assert(order.price <= price && "lower_bound didn't find the right price.");
                do {
                    assert(order.id != missing_id && "Malformed Ask");
                    order = orders_ask_[++idx];
                } while(order.price == price && idx < orders_filled_ask_);
                for(usize j = idx; j < orders_filled_ask_; ++j) {

                }
            }

        } else if (side == Side::Bid) {
            while(price_block_idx < orders_filled_bid_) {
                ++price_block_idx;
            }
        } else {
            std::unreachable();
        }

        if(k_verbose) {
            std::println("Added new order {}", new_order.to_string());
        }
        assert(false && "Not Implemented");
    }

    void on_add(Order order) {
        on_add(order.id, order.side, order.price, order.quantity);
    }

    void on_cancel(int order_id) {
        if(k_verbose) {
            std::println("Cancelling order with id {}", order_id);
        }
        assert(false && "Not Implemented");
    }

    void on_modify(int order_id, int new_quantity) {
        // Increases causes loss of time priority
        // Decrease Preserves Time Priority
        assert(false && "Not Implemented");
    }

    void on_trade(int order_id, int trade_quantity) {
        assert(false && "Not Implemented");
    }

    std::optional<BestQuote> best_bid() const {
        assert(false && "Not Implemented");
        return std::nullopt;
    }

    std::optional<BestQuote> best_ask() const {
        assert(false && "Not Implemented");
        return std::nullopt;
    }

    int depth_at(Side side, int price) const {
        assert(false && "Not Implemented");
        0;
    }

    void print() const {
        std::println("Printing Orderbook:");
        std::println("\tAsk Orders:");
        if(orders_filled_ask_ == 0zu) {
            std::println("\t\t<None>");
        }
        for(usize i = 0; i < orders_filled_ask_; ++i) {
            std::println("\t\t[{:03}] {}", i, orders_ask_[orders_filled_ask_].to_string());
        }
        std::println("\tBid Orders:");
        if(orders_filled_bid_ == 0zu) {
            std::println("\t\t<None>");
        }
        for(usize i = 0; i < orders_filled_bid_; ++i) {
            std::println("\t\t[{:03}] {}", i, orders_bid_[orders_filled_bid_].to_string());
        }
    }

    void validate() const {
        { // Bid prices should be monotonically decreasing 
            int current_price = 0;
            for(usize i = 0; i < orders_filled_bid_; ++i) {
                const double price = orders_bid_[i].price;
                assert(price <= current_price && "Bid price monotonicity violated");
                current_price = price;
            }
        }
        { // Ask prices should be monotonically increasing 
            int current_price = 0;
            for(usize i = 0; i < orders_filled_bid_; ++i) {
                const double price = orders_bid_[i].price;
                assert(price >= current_price && "Ask price monotonicity violated");
                current_price = price;
            }
        }
    }

private:
    std::array<Order, max_number_live_orders> orders_ask_{};
    std::array<Order, max_number_live_orders> orders_bid_{};

    usize orders_filled_bid_ = 0zu;
    usize orders_filled_ask_ = 0zu;

    usize find_order_by_id(int order_id, Side side) const {
        const auto& orders = (side == Side::Ask) ? orders_ask_ : orders_bid_;
        const auto& orders_filled = (side == Side::Ask) ? orders_filled_ask_ : orders_filled_bid_;

        usize found_idx = 0zu;
        while(found_idx < orders_filled) {
            if(orders[found_idx].id == order_id) {
                break;
            }
            ++found_idx;
        }
        assert(found_idx != orders_filled && "Order not Found. By problem statement there must not be misformed events");
        return found_idx;
    }
};
}

int main() {
    using namespace ds_ob;
    OrderBook ob{};

    ob.print();
}














