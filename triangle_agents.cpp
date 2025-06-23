#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <memory>
#include <iomanip>
#include <stdexcept>

// ==================== SClang-like structures ====================
namespace sc {
    enum class Result {
        Ok,
        Error
    };

    struct MemoryContext {
        std::map<std::string, void*> data;
        std::map<std::string, std::string> types;
        
        template<typename T>
        void store(const std::string& addr, T* value) {
            data[addr] = static_cast<void*>(value);
            types[addr] = typeid(T).name();
        }
        
        template<typename T>
        T* get(const std::string& addr) {
            if (types[addr] != typeid(T).name()) {
                throw std::runtime_error("Type mismatch for SC element: " + addr);
            }
            return static_cast<T*>(data[addr]);
        }
    };

    void log_sc_event(const std::string& msg) {
        std::cout << "[SC] " << msg << "\n";
    }
}

// ==================== domains ====================
struct Angle {
    double value;
    bool is_known;
};

struct Triangle {
    Angle angles[3]; // angles[0] - A, angles[1] - B, angles[2] - C
};

// ==================== agents ====================
class CalculateAnglesAgent {
public:
    sc::Result execute(sc::MemoryContext& ctx) {
        auto* triangle = ctx.get<Triangle>("input_triangle");
        auto* rules = ctx.get<std::map<std::string, std::string>>("rules_set");

        int unknown_count = 0;
        double sum_known = 0.0;

        for (const auto& angle : triangle->angles) {
            if (angle.is_known) {
                sum_known += angle.value;
            } else {
                unknown_count++;
            }
        }

        if (unknown_count == 1) {
            for (auto& angle : triangle->angles) {
                if (!angle.is_known) {
                    angle.value = 180.0 - sum_known;
                    angle.is_known = true;
                    sc::log_sc_event("Calculated angle: " + std::to_string(angle.value) + "°");
                    return sc::Result::Ok;
                }
            }
        }

        sc::log_sc_event("Angle calculation error");
        return sc::Result::Error;
    }
};

class CheckRightAngleAgent {
public:
    sc::Result execute(sc::MemoryContext& ctx) {
        auto* triangle = ctx.get<Triangle>("input_triangle");
        auto* rules = ctx.get<std::map<std::string, std::string>>("rules_set");

        for (const auto& angle : triangle->angles) {
            if (angle.is_known && std::abs(angle.value - 90.0) < 0.001) {
                bool is_right = true;
                ctx.store("is_right_triangle", &is_right);
                sc::log_sc_event("Right angle detected (90°)");
                return sc::Result::Ok;
            }
        }

        bool is_right = false;
        ctx.store("is_right_triangle", &is_right);
        return sc::Result::Ok;
    }
};

class TriangleProcessingAgent {
public:
    sc::Result execute(sc::MemoryContext& ctx) {
        sc::log_sc_event("Starting triangle processing");

        CalculateAnglesAgent angles_agent;
        if (angles_agent.execute(ctx) != sc::Result::Ok) {
            return sc::Result::Error;
        }

        CheckRightAngleAgent check_agent;
        if (check_agent.execute(ctx) != sc::Result::Ok) {
            return sc::Result::Error;
        }

        bool* is_right = ctx.get<bool>("is_right_triangle");
        sc::log_sc_event(*is_right ? "Triangle is right-angled" : "Triangle is not right-angled");
        
        return sc::Result::Ok;
    }
};

// ==================== CLI UI SClang-like ====================
void print_sc_memory(const sc::MemoryContext& ctx) {
    std::cout << "=== SC Memory Dump ===\n";
    for (const auto& [key, type] : ctx.types) {
        std::cout << std::setw(20) << key << ": ";
        if (type == typeid(Triangle).name()) {
            auto* tri = static_cast<Triangle*>(ctx.data.at(key));
            std::cout << "Triangle(";
            for (const auto& angle : tri->angles) {
                std::cout << (angle.is_known ? std::to_string(angle.value) : "?") << " ";
            }
            std::cout << ")";
        } else if (type == typeid(bool).name()) {
            auto* val = static_cast<bool*>(ctx.data.at(key));
            std::cout << (*val ? "true" : "false");
        } else if (type == typeid(std::map<std::string, std::string>).name()) {
            std::cout << "RulesSet";
        }
        std::cout << "\n";
    }
    std::cout << "======================\n";
}

// ==================== Testing ====================
int main() {
    // Initialize SC memory
    sc::MemoryContext ctx;
    
    // Test triangle 1 (90°, 45°, ?)
    Triangle triangle1 = {
        { {90.0, true}, {45.0, true}, {0.0, false} }
    };
    auto rules = std::map<std::string, std::string>{
        {"right_angle_threshold", "90.0"}
    };
    
    ctx.store("input_triangle", &triangle1);
    ctx.store("rules_set", &rules);

    std::cout << "=== Test 1: Right-angled triangle ===\n";
    print_sc_memory(ctx);
    
    TriangleProcessingAgent agent;
    auto result = agent.execute(ctx);
    
    print_sc_memory(ctx);
    std::cout << "Result: " << (result == sc::Result::Ok ? "SC_RESULT_OK" : "SC_RESULT_ERROR") << "\n\n";

    // Test triangle 2 (60°, 60°, ?)
    Triangle triangle2 = {
        { {60.0, true}, {60.0, true}, {0.0, false} }
    };
    ctx.store("input_triangle", &triangle2);

    std::cout << "=== Test 2: Non-right-angled triangle ===\n";
    print_sc_memory(ctx);
    
    result = agent.execute(ctx);
    
    print_sc_memory(ctx);
    std::cout << "Result: " << (result == sc::Result::Ok ? "SC_RESULT_OK" : "SC_RESULT_ERROR") << "\n";

    return 0;
}