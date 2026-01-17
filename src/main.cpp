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

using Price = int;
using Quantity = int;
using OrderId = int;

constexpr bool k_verbose = true;
constexpr int max_number_live_orders = 100;

constexpr OrderId missing_id = -1;
constexpr Price missing_price = -1;
constexpr Quantity missing_quantity = -1;

struct BestQuote {
    Price price;
    Quantity quantity; // aggregated quantity at that price
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
    }
}

struct Order {
    OrderId id{missing_id};
    Side side{Side::Bid};
    Price price{missing_price};
    Quantity quantity{missing_quantity};

    std::string to_string() const {
        return std::format("Order(id={},side={},price={},quantity={})", id, ds_ob::to_string(side), price, quantity);
    }
};

class OrderBook {
public:
    void on_add(OrderId order_id, Side side, Price price, Quantity quantity) {
        // No need for checks as we never overflow by problem statement
        const Order new_order{
            .id=order_id,
            .side=side,
            .price=price,
            .quantity=quantity
        };
        auto& orders = (side == Side::Ask) ? orders_ask_ : orders_bid_;
        usize& end_ptr = (side == Side::Ask) ? orders_filled_ask_ : orders_filled_bid_;

        const auto end{orders.begin() + end_ptr};
        auto cmp = (side == Side::Ask)
            ? [](const Order& order1, const Order& order2) -> bool {
                return (order1.price > order2.price);
            }
            : [](const Order& order1, const Order& order2) -> bool {
                return (order1.price < order2.price);
            };
        auto it = std::lower_bound(
            orders.begin(),
            end,
            new_order,
            cmp
        );
        if(it == end) {
            orders[end_ptr++] = new_order;
            return;
        }

        if(side == Side::Ask) {
            while(it > orders.begin() + 1 && (it - 1)->price == price) {
                --it;
            }
        } else if (side == Side::Bid) {
            while(it > orders.begin() && it->price == price) {
                --it;
            }
        }
        const usize idx = static_cast<usize>(it - orders.begin());
        ++end_ptr;
        for(usize i = end_ptr; i > idx; --i) {
            std::swap(orders[i - 1], orders[i]);
        }
        orders[idx] = new_order;

        if(k_verbose) {
            std::println("Added new order {}", new_order.to_string());
        }
    }

    void on_add(Order order) {
        on_add(order.id, order.side, order.price, order.quantity);
    }

    void on_cancel(OrderId order_id) {
        if(k_verbose) {
            std::println("Cancelling order with id {}", order_id);
        }
        assert(false && "Not Implemented");
    }

    void on_modify(OrderId order_id, Quantity new_quantity) {
        // Increases causes loss of time priority
        // Decrease Preserves Time Priority
        assert(false && "Not Implemented");
        (void)order_id;
        (void)new_quantity;
    }

    void on_trade(int order_id, int trade_quantity) {
        assert(false && "Not Implemented");
        (void)order_id;
        (void)trade_quantity;
    }

    std::optional<BestQuote> best_bid() const {
        assert(false && "Not Implemented");
        return std::nullopt;
    }

    std::optional<BestQuote> best_ask() const {
        assert(false && "Not Implemented");
        return std::nullopt;
    }

    int depth_at(Side side, Price price) const {
        assert(false && "Not Implemented");
        (void)side;
        (void)price;
        0;
    }

    void print() const {
        std::println("Printing Orderbook:");
        std::println("\tAsk Orders:");
        if(orders_filled_ask_ == 0zu) {
            std::println("\t\t<None>");
        }
        for(usize i{0zu}; i < orders_filled_ask_; ++i) {
            std::println("\t\t[{:03}] {}", i, orders_ask_[i].to_string());
        }
        std::println("\tBid Orders:");
        if(orders_filled_bid_ == 0zu) {
            std::println("\t\t<None>");
        }
        for(usize i{0zu}; i < orders_filled_bid_; ++i) {
            std::println("\t\t[{:03}] {}", i, orders_bid_[i].to_string());
        }
    }

    void validate() const {
        { // Bid prices should be monotonically decreasing 
            int current_price = 0;
            for(usize i{0zu}; i < orders_filled_bid_; ++i) {
                const int price = orders_bid_[i].price;
                assert(price <= current_price && "Bid price monotonicity violated");
                current_price = price;
            }
        }
        { // Ask prices should be monotonically increasing 
            int current_price = 0;
            for(usize i{0zu}; i < orders_filled_bid_; ++i) {
                const int price = orders_bid_[i].price;
                assert(price >= current_price && "Ask price monotonicity violated");
                current_price = price;
            }
        }
    }

    int n_ask() const { return static_cast<int>(orders_filled_ask_); }
    int n_bid() const { return static_cast<int>(orders_filled_bid_); }

    const std::array<Order, max_number_live_orders>&
    orders_ask() {
        return orders_ask_;
    }
    const std::array<Order, max_number_live_orders>&
    orders_bid() {
        return orders_bid_;
    }

private:
    std::array<Order, max_number_live_orders> orders_ask_{};
    std::array<Order, max_number_live_orders> orders_bid_{};

    usize orders_filled_bid_{0zu};
    usize orders_filled_ask_{0zu};

    usize find_order_by_id(int order_id, Side side) const {
        const auto& orders = (side == Side::Ask) ? orders_ask_ : orders_bid_;
        const usize orders_filled = (side == Side::Ask) ? orders_filled_ask_ : orders_filled_bid_;

        usize found_idx{0zu};
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

void _test_on_add() {
    using namespace ds_ob;
    OrderBook ob{};

    ob.on_add(0, Side::Ask, Price{2}, 1);
    ob.on_add(1, Side::Ask, Price{4}, 1);
    ob.on_add(2, Side::Ask, Price{3}, 1);
    ob.on_add(3, Side::Ask, Price{1}, 1);
    ob.on_add(4, Side::Ask, Price{3}, 1);

    ob.on_add(5, Side::Bid, Price{1}, 1);
    ob.on_add(6, Side::Bid, Price{2}, 1);
    ob.on_add(7, Side::Bid, Price{1}, 1);

    assert(ob.n_ask() == 5);
    assert(ob.n_bid() == 3);

    auto asks = ob.orders_ask();
    auto bids = ob.orders_bid();

    assert(asks[0].id == 1);
    assert(asks[1].id == 4);
    assert(asks[2].id == 2);
    assert(asks[3].id == 0);
    assert(asks[4].id == 3);

    assert(bids[0].id == 7);
    assert(bids[1].id == 5);
    assert(bids[2].id == 6);
}

void tests() {
    _test_on_add();
}

int main() {
    using namespace ds_ob;
    tests();

    const OrderBook ob{};
    ob.print();
}














