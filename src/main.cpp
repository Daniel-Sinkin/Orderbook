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
constexpr usize invalid_idx = std::numeric_limits<usize>::max();

struct BestQuote {
    Price price{missing_price};
    Quantity quantity{missing_quantity};

    bool is_empty() const {
        assert((price == missing_price) == (quantity == missing_quantity));
        return price == missing_price;
    }

    void clear() {
        price = missing_price;
        quantity = missing_quantity;
    }
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
        if(k_verbose) {
            std::println("Added new order {}", new_order.to_string());
        }

        auto& orders = (side == Side::Ask) ? orders_ask_ : orders_bid_;
        usize& orders_filled = (side == Side::Ask) ? orders_filled_ask_ : orders_filled_bid_;

        const auto end{orders.begin() + orders_filled};
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
            orders[orders_filled++] = new_order;
        } else {
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
            ++orders_filled;
            for(usize i = orders_filled; i > idx; --i) {
                std::swap(orders[i - 1], orders[i]);
            }
            orders[idx] = new_order;

        }
        if(side == Side::Ask) {
            if(best_quote_ask_.is_empty()) {
                best_quote_ask_.price = price;
                best_quote_ask_.quantity = quantity;
            } else if (best_quote_ask_.price == price) {
                assert(orders_ask_[orders_filled_ask_ - 1].price == price && "If at quote price then we must have lowest ask price");
                best_quote_ask_.quantity += quantity;
            } else if (best_quote_ask_.price > price) {
                best_quote_ask_.price = price;
                best_quote_ask_.quantity = quantity;
            }
        } else {
            if(best_quote_bid_.is_empty()) {
                best_quote_bid_.price = price;
                best_quote_bid_.quantity = quantity;
            } else if (best_quote_bid_.price == price) {
                // assert(orders_bid_[orders_filled_bid_ - 1].price == price && "If at quote price then we must have lowest bid price");
                best_quote_bid_.quantity += quantity;
            } else if (best_quote_bid_.price < price) {
                best_quote_bid_.price = price;
                best_quote_bid_.quantity = quantity;
            }
        }
    }

    void on_add(const Order& order) {
        on_add(order.id, order.side, order.price, order.quantity);
    }

    void cancel_order_by_idx(usize idx, bool is_ask) {
        auto& orders = is_ask ? orders_ask_ : orders_bid_;
        const Order& order = orders[idx];

        usize& orders_filled = is_ask ? orders_filled_ask_ : orders_filled_bid_;
        BestQuote& best_quote = is_ask ? best_quote_ask_ : best_quote_bid_;

        --orders_filled;
        if(best_quote.price == order.price) {
            if(best_quote.quantity == order.quantity) {
                if(orders_filled == 0zu) {
                    best_quote.clear();
                } else {
                    best_quote.price = orders[orders_filled - 1].price;
                    best_quote.quantity = Quantity{0};
                    for(usize i = orders_filled - 1; orders[i].price == best_quote.price; ++i) {
                        best_quote.quantity += orders[i].quantity;
                    }
                }
            } else {
                best_quote.quantity -= order.quantity;
            }
        }
        for(usize i = idx; i < orders_filled; ++i) {
            std::swap(orders[i], orders[i + 1]);
        }
    }

    void on_cancel(OrderId order_id) {
        if(k_verbose) {
            std::println("Cancelling order with id {}", order_id);
        }

        const auto& [is_ask, idx] = find_order_by_id(order_id);
        if(is_ask) {
            assert(orders_filled_ask_ > 0zu);
        } else {
            assert(orders_filled_bid_ > 0zu);
        }
        cancel_order_by_idx(idx, is_ask);
    }

    void on_cancel(const Order& order) {
        on_cancel(order.id);
    }

    void on_modify(OrderId order_id, Quantity new_quantity) {
        const auto& [is_ask, idx] = find_order_by_id(order_id);

        auto& orders = is_ask ? orders_ask_ : orders_bid_;
        Order& order = orders[idx];
        const Quantity old_quantity = order.quantity;
        BestQuote& best_quote = is_ask ? best_quote_ask_ : best_quote_bid_;

        usize idx_block_start = idx;
        if(new_quantity <= order.quantity) {
            order.quantity = new_quantity;
        } else {
            const Price price = order.price;
            while(idx_block_start > 1 && orders[idx_block_start - 1].price == price) {
                std::swap(orders[idx_block_start-1], orders[idx_block_start]);
                --idx_block_start;
            }
            orders[idx_block_start].quantity = new_quantity;
        }

        if(best_quote.price == order.price) {
            best_quote.quantity += new_quantity - old_quantity;
        }
    }

    void on_trade(int order_id, int trade_quantity) {
        if(k_verbose) {
            std::println("on_trade({},{})", order_id, trade_quantity);
        }
        const auto& [is_ask, idx] = find_order_by_id(order_id);

        (void)trade_quantity;

        auto& orders = is_ask ? orders_ask_ : orders_bid_;
        Order& order = orders[idx];
        BestQuote& best_quote = is_ask ? best_quote_ask_ : best_quote_bid_;

        assert(order.quantity >= trade_quantity);

        if(order.quantity == trade_quantity) {
            cancel_order_by_idx(idx, is_ask);
        } else {
            order.quantity -= trade_quantity;
            if(order.price == best_quote.price) {
                best_quote.quantity -= trade_quantity;
            }
        }
    }

    std::optional<BestQuote> best_bid() const {
        if(best_quote_bid_.is_empty()) {
            return std::nullopt;
        }
        return best_quote_bid_;
    }

    std::optional<BestQuote> best_ask() const {
        if(best_quote_ask_.is_empty()) {
            return std::nullopt;
        }
        return best_quote_ask_;
    }

    int depth_at(Side side, Price price) const {
        assert(false && "Not Implemented");
        (void)side;
        (void)price;
        return 0;
    }

    void print() const {
        std::println("Printing Orderbook:");
        if(best_quote_bid_.is_empty()) {
            std::println("Best Quote Bid = <None>");
        } else {
            std::println("Best Quote Bid = [price={},quantity={}]", best_quote_bid_.price, best_quote_bid_.quantity);
        }
        if(best_quote_ask_.is_empty()) {
            std::println("Best Quote Ask = <None>");
        } else {
            std::println("Best Quote Ask = [price={},quantity={}]", best_quote_ask_.price, best_quote_ask_.quantity);
        }

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
        {
            int current_price{std::numeric_limits<int>::min()};
            for(usize i{0zu}; i < orders_filled_bid_; ++i) {
                const int next_price = orders_bid_[i].price;
                assert(next_price >= current_price && "Bid prices should be monotonically increasing");
                current_price = next_price;
            }
        }
        {
            int current_price{std::numeric_limits<int>::max()};
            for(usize i{0zu}; i < orders_filled_ask_; ++i) {
                const int next_price = orders_ask_[i].price;
                assert(next_price <= current_price && "Ask price should be monotonically decreasing");
                current_price = next_price;
            }
        }
        {
            const bool quote_empty = best_quote_bid_.price == missing_price;
            assert(quote_empty == (best_quote_bid_.quantity == missing_quantity));
            assert(quote_empty == (orders_filled_bid_ == 0zu));

            if(!quote_empty) {
                assert(best_quote_bid_.quantity > 0 && "Existing best quote quantity must be positive");
                assert(best_quote_bid_.price == orders_bid_[orders_filled_bid_ - 1].price);
            }
        }
        {
            const bool quote_empty = (best_quote_ask_.price == missing_price);
            assert(quote_empty == (best_quote_ask_.quantity == missing_quantity));
            assert(quote_empty == (orders_filled_ask_ == 0zu));

            if(!quote_empty) {
                assert(best_quote_ask_.quantity > 0 && "Existing best quote quantity must be positive");
                assert(best_quote_ask_.price == orders_ask_[orders_filled_ask_ - 1].price);
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

    BestQuote best_quote_ask_{};
    BestQuote best_quote_bid_{};

    std::pair<bool, usize> find_order_by_id(OrderId order_id) const {
        bool is_ask{false};
        usize idx{invalid_idx};
        for(usize i{0zu}; i < orders_filled_ask_; ++i) {
            if(orders_ask_[i].id == order_id) {
                idx = i;
                is_ask = true;
                break;
            }
        }
        assert (is_ask == (idx != invalid_idx));
        if(!is_ask) {
            for(usize i{0zu}; i < orders_filled_bid_; ++i) {
                if(orders_bid_[i].id == order_id) {
                    idx = i;
                    break;
                }
            }
        }
        assert (idx != invalid_idx);
        return {is_ask, idx};
    }
};
}

void run_tests() {
    using namespace ds_ob;
    OrderBook ob{};

    ob.on_add(0, Side::Ask, Price{2}, 1);
    ob.on_add(1, Side::Ask, Price{4}, 1);
    ob.on_add(2, Side::Ask, Price{3}, 1);
    ob.on_add(3, Side::Ask, Price{1}, 1);
    ob.on_add(4, Side::Ask, Price{3}, 6);
    ob.on_add(5, Side::Ask, Price{3}, 1);
    ob.on_add(6, Side::Ask, Price{3}, 1);
    ob.on_add(7, Side::Ask, Price{3}, 1);

    ob.on_add(8, Side::Bid, Price{1}, 1);
    ob.on_add(9, Side::Bid, Price{2}, 1);
    ob.on_add(10, Side::Bid, Price{1}, 1);

    assert(ob.n_ask() == 8);
    assert(ob.n_bid() == 3);

    ob.on_cancel(5);
    ob.on_cancel(6);
    ob.on_cancel(7);

    assert(ob.n_ask() == 5);
    assert(ob.n_bid() == 3);

    auto asks = ob.orders_ask();
    auto bids = ob.orders_bid();

    assert(asks[0].id == 1);
    assert(asks[1].id == 4);
    assert(asks[2].id == 2);
    assert(asks[3].id == 0);
    assert(asks[4].id == 3);

    assert(bids[0].id == 10);
    assert(bids[1].id == 8);
    assert(bids[2].id == 9);

    ob.on_modify(2, 2);
    asks = ob.orders_ask();
    bids = ob.orders_bid();

    assert(asks[1].id == 2);
    assert(asks[1].quantity == 2);
    assert(asks[2].id == 4);
    assert(asks[2].quantity == 6);

    ob.on_modify(4, 1);
    ob.on_modify(9, 10);
    asks = ob.orders_ask();
    bids = ob.orders_bid();

    assert(asks[1].id == 2);
    assert(asks[1].quantity == 2);
    assert(asks[2].id == 4);
    assert(asks[2].quantity == 1);

    assert(bids[2].id == 9);
    assert(bids[2].quantity== 10);

    ob.on_trade(9, 5);
    asks = ob.orders_ask();
    bids = ob.orders_bid();

    assert(bids[2].id == 9);
    assert(bids[2].quantity==5);
    assert(ob.n_bid() == 3);

    ob.on_trade(9, 5);
    assert(ob.n_bid() == 2);

    ob.print();

    ob.validate();
}

int main() {
    using namespace ds_ob;
    std::println("Running Tests");
    run_tests();
    std::println("Tests finish running\n");

    OrderBook ob{};
    ob.on_add(OrderId{1}, Side::Ask, Price{2}, Quantity{1});
    ob.on_modify(OrderId{1}, Quantity{2});
    ob.print();
}