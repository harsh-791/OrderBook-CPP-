#include <iostream>
#include <map>
#include <deque>
#include <unordered_map>
#include <algorithm>
#include <climits>
#include <cstdint>

using namespace std;

struct Order {
    uint64_t order_id;
    bool is_buy;
    double price;
    uint32_t quantity;
    uint64_t timestamp_ns;
};

struct PriceLevel {
    deque<Order> orders;
    uint32_t total_qty = 0;
};

class OrderBook {
private:
    map<double, PriceLevel, greater<double>> bids;
    map<double, PriceLevel> asks;
    unordered_map<uint64_t, pair<double, bool>> order_lookup;
    uint64_t sequence_number = 0;

public:
    bool add_order(uint64_t id, bool buy, double price, uint32_t qty) {
        if (order_lookup.count(id)) return false;
        
        sequence_number++;
        Order new_order{id, buy, price, qty, sequence_number};
        
        if (buy) {
            bids[price].orders.push_back(new_order);
            bids[price].total_qty += qty;
        } else {
            asks[price].orders.push_back(new_order);
            asks[price].total_qty += qty;
        }
        
        order_lookup[id] = {price, buy};
        execute_matching();
        return true;
    }

    bool cancel_order(uint64_t id) {
        if (!order_lookup.count(id)) return false;
        
        auto [price, is_buy] = order_lookup[id];

        if (is_buy) {
            auto bid_iter = bids.find(price);
            if (bid_iter == bids.end()) return false;
            
            auto& order_queue = bid_iter->second.orders;
            for (auto order_iter = order_queue.begin(); order_iter != order_queue.end(); ++order_iter) {
                if (order_iter->order_id == id) {
                    bid_iter->second.total_qty -= order_iter->quantity;
                    order_queue.erase(order_iter);
                    if (order_queue.empty()) bids.erase(bid_iter);
                    order_lookup.erase(id);
                    return true;
                }
            }
        } else {
            auto ask_iter = asks.find(price);
            if (ask_iter == asks.end()) return false;
            
            auto& order_queue = ask_iter->second.orders;
            for (auto order_iter = order_queue.begin(); order_iter != order_queue.end(); ++order_iter) {
                if (order_iter->order_id == id) {
                    ask_iter->second.total_qty -= order_iter->quantity;
                    order_queue.erase(order_iter);
                    if (order_queue.empty()) asks.erase(ask_iter);
                    order_lookup.erase(id);
                    return true;
                }
            }
        }
        return false;
    }


    bool amend_order(uint64_t id, double new_price, uint32_t new_qty) {
        if (!order_lookup.count(id)) return false;
        
        auto [old_price, is_buy] = order_lookup[id];
        cancel_order(id);
        add_order(id, is_buy, new_price, new_qty);
        return true;
    }

    void execute_matching() {
        while (!bids.empty() && !asks.empty()) {
            auto& best_bid_level = bids.begin()->second;
            auto& best_ask_level = asks.begin()->second;
            double best_bid_price = bids.begin()->first;
            double best_ask_price = asks.begin()->first;

            if (best_bid_price < best_ask_price) break;

            auto& buy_order = best_bid_level.orders.front();
            auto& sell_order = best_ask_level.orders.front();
            uint32_t trade_quantity = min(buy_order.quantity, sell_order.quantity);

            cout << "TRADE EXECUTED: " << trade_quantity
                 << " @ " << best_ask_price
                 << " (BuyOrderID=" << buy_order.order_id
                 << ", SellOrderID=" << sell_order.order_id << ")\n";

            buy_order.quantity -= trade_quantity;
            sell_order.quantity -= trade_quantity;
            best_bid_level.total_qty -= trade_quantity;
            best_ask_level.total_qty -= trade_quantity;

            if (buy_order.quantity == 0) {
                order_lookup.erase(buy_order.order_id);
                best_bid_level.orders.pop_front();
            }
            if (sell_order.quantity == 0) {
                order_lookup.erase(sell_order.order_id);
                best_ask_level.orders.pop_front();
            }
            
            if (best_bid_level.orders.empty()) bids.erase(best_bid_price);
            if (best_ask_level.orders.empty()) asks.erase(best_ask_price);
        }
    }


    void print_book() {
        cout << "\n--- ASK SIDE (Sell Orders) ---\n";
        for (auto& [price, level] : asks) {
            cout << "Price: " << price << " | Total Qty: " << level.total_qty << "\n";
        }

        cout << "\n--- BID SIDE (Buy Orders) ---\n";
        for (auto& [price, level] : bids) {
            cout << "Price: " << price << " | Total Qty: " << level.total_qty << "\n";
        }
        cout << "\n";
    }
};

int main() {
    OrderBook order_book;
    
    cout << "=== OrderBook Testing Suite ===" << endl;
    
    cout << "\n1. Adding initial orders..." << endl;
    order_book.add_order(1, true, 100.0, 1000);
    order_book.add_order(2, false, 101.0, 500);
    order_book.add_order(3, true, 99.5, 750);
    order_book.add_order(4, false, 102.0, 300);
    order_book.print_book();
    
    cout << "\n2. Adding orders that should trigger trades..." << endl;
    order_book.add_order(5, true, 101.5, 200);
    order_book.add_order(6, false, 100.5, 150);
    order_book.print_book();
    
    cout << "\n3. Testing order amendment..." << endl;
    cout << "Before amendment:" << endl;
    order_book.print_book();
    order_book.amend_order(2, 100.5, 400);
    cout << "After amending order 2 (price 101.0->100.5, qty 500->400):" << endl;
    order_book.print_book();
    
    cout << "\n4. Testing order cancellation..." << endl;
    cout << "Before cancellation:" << endl;
    order_book.print_book();
    order_book.cancel_order(3);
    cout << "After cancelling order 3:" << endl;
    order_book.print_book();
    
    cout << "\n5. Testing duplicate order ID (should fail)..." << endl;
    bool result = order_book.add_order(1, true, 99.0, 100);
    cout << "Adding duplicate order ID 1: " << (result ? "SUCCESS" : "FAILED (expected)") << endl;
    
    cout << "\n6. Testing cancellation of non-existent order..." << endl;
    result = order_book.cancel_order(999);
    cout << "Cancelling non-existent order 999: " << (result ? "SUCCESS" : "FAILED (expected)") << endl;
    
    cout << "\n7. Testing amendment of non-existent order..." << endl;
    result = order_book.amend_order(999, 99.0, 100);
    cout << "Amending non-existent order 999: " << (result ? "SUCCESS" : "FAILED (expected)") << endl;
    
    cout << "\n8. Final order book state:" << endl;
    order_book.print_book();
    
    cout << "\n=== Testing Complete ===" << endl;
    
    return 0;
}
