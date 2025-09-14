#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ==================== SClang-like structures ====================
typedef enum {
    SC_RESULT_OK,
    SC_RESULT_ERROR
} sc_result;

typedef struct {
    char* addr;
    void* data;
    char* type;
} sc_memory_entry;

typedef struct {
    sc_memory_entry* entries;
    size_t size;
    size_t capacity;
} sc_memory_context;

void sc_memory_init(sc_memory_context* ctx, size_t initial_capacity) {
    ctx->entries = malloc(initial_capacity * sizeof(sc_memory_entry));
    ctx->size = 0;
    ctx->capacity = initial_capacity;
}

void sc_memory_store(sc_memory_context* ctx, const char* addr, void* data, const char* type) {
    // Check if addr already exists and update
    for (size_t i = 0; i < ctx->size; i++) {
        if (strcmp(ctx->entries[i].addr, addr) == 0) {
            ctx->entries[i].data = data;
            ctx->entries[i].type = strdup(type);
            return;
        }
    }

    // Resize if necessary
    if (ctx->size >= ctx->capacity) {
        ctx->capacity *= 2;
        ctx->entries = realloc(ctx->entries, ctx->capacity * sizeof(sc_memory_entry));
    }

    // Add new entry
    ctx->entries[ctx->size].addr = strdup(addr);
    ctx->entries[ctx->size].data = data;
    ctx->entries[ctx->size].type = strdup(type);
    ctx->size++;
}

void* sc_memory_get(sc_memory_context* ctx, const char* addr, const char* type) {
    for (size_t i = 0; i < ctx->size; i++) {
        if (strcmp(ctx->entries[i].addr, addr) == 0) {
            if (strcmp(ctx->entries[i].type, type) != 0) {
                fprintf(stderr, "Type mismatch for SC element: %s\n", addr);
                exit(EXIT_FAILURE);
            }
            return ctx->entries[i].data;
        }
    }
    return NULL;
}

void sc_log_event(const char* msg) {
    printf("[SC] %s\n", msg);
}

// ==================== domains ====================
typedef struct {
    double value;
    int is_known;
} angle;

typedef struct {
    angle angles[3]; // angles[0] - A, angles[1] - B, angles[2] - C
} triangle;

// ==================== agents ====================
sc_result calculate_angles_agent_execute(sc_memory_context* ctx) {
    triangle* tri = sc_memory_get(ctx, "input_triangle", "triangle");
    // rules_set not used in this agent

    int unknown_count = 0;
    double sum_known = 0.0;

    for (int i = 0; i < 3; i++) {
        if (tri->angles[i].is_known) {
            sum_known += tri->angles[i].value;
        } else {
            unknown_count++;
        }
    }

    if (unknown_count == 1) {
        for (int i = 0; i < 3; i++) {
            if (!tri->angles[i].is_known) {
                tri->angles[i].value = 180.0 - sum_known;
                tri->angles[i].is_known = 1;
                char msg[100];
                snprintf(msg, sizeof(msg), "Calculated angle: %.2f°", tri->angles[i].value);
                sc_log_event(msg);
                return SC_RESULT_OK;
            }
        }
    }

    sc_log_event("Angle calculation error");
    return SC_RESULT_ERROR;
}

sc_result check_right_angle_agent_execute(sc_memory_context* ctx) {
    triangle* tri = sc_memory_get(ctx, "input_triangle", "triangle");
    // rules_set not used in this agent

    for (int i = 0; i < 3; i++) {
        if (tri->angles[i].is_known && fabs(tri->angles[i].value - 90.0) < 0.001) {
            int is_right = 1;
            sc_memory_store(ctx, "is_right_triangle", &is_right, "int");
            sc_log_event("Right angle detected (90°)");
            return SC_RESULT_OK;
        }
    }

    int is_right = 0;
    sc_memory_store(ctx, "is_right_triangle", &is_right, "int");
    return SC_RESULT_OK;
}

sc_result triangle_processing_agent_execute(sc_memory_context* ctx) {
    sc_log_event("Starting triangle processing");

    if (calculate_angles_agent_execute(ctx) != SC_RESULT_OK) {
        return SC_RESULT_ERROR;
    }

    if (check_right_angle_agent_execute(ctx) != SC_RESULT_OK) {
        return SC_RESULT_ERROR;
    }

    int* is_right = sc_memory_get(ctx, "is_right_triangle", "int");
    sc_log_event(*is_right ? "Triangle is right-angled" : "Triangle is not right-angled");
    
    return SC_RESULT_OK;
}

// ==================== CLI UI SClang-like ====================
void print_sc_memory(sc_memory_context* ctx) {
    printf("=== SC Memory Dump ===\n");
    for (size_t i = 0; i < ctx->size; i++) {
        printf("%20s: ", ctx->entries[i].addr);
        if (strcmp(ctx->entries[i].type, "triangle") == 0) {
            triangle* tri = ctx->entries[i].data;
            printf("Triangle(");
            for (int j = 0; j < 3; j++) {
                printf(tri->angles[j].is_known ? "%.2f " : "? ", tri->angles[j].value);
            }
            printf(")");
        } else if (strcmp(ctx->entries[i].type, "int") == 0) {
            int* val = ctx->entries[i].data;
            printf(*val ? "true" : "false");
        } else if (strcmp(ctx->entries[i].type, "rules_set") == 0) {
            printf("RulesSet");
        }
        printf("\n");
    }
    printf("======================\n");
}

// ==================== Testing ====================
int main() {
    // Initialize SC memory
    sc_memory_context ctx;
    sc_memory_init(&ctx, 10);

    // Test triangle 1 (90°, 45°, ?)
    triangle triangle1 = {
        { {90.0, 1}, {45.0, 1}, {0.0, 0} }
    };
    // rules_set not used in agents
    
    sc_memory_store(&ctx, "input_triangle", &triangle1, "triangle");
    // Store a dummy rules_set (not used)
    int dummy_rules = 0;
    sc_memory_store(&ctx, "rules_set", &dummy_rules, "rules_set");

    printf("=== Test 1: Right-angled triangle ===\n");
    print_sc_memory(&ctx);
    
    sc_result result = triangle_processing_agent_execute(&ctx);
    
    print_sc_memory(&ctx);
    printf("Result: %s\n\n", result == SC_RESULT_OK ? "SC_RESULT_OK" : "SC_RESULT_ERROR");

    // Test triangle 2 (60°, 60°, ?)
    triangle triangle2 = {
        { {60.0, 1}, {60.0, 1}, {0.0, 0} }
    };
    sc_memory_store(&ctx, "input_triangle", &triangle2, "triangle");

    printf("=== Test 2: Non-right-angled triangle ===\n");
    print_sc_memory(&ctx);
    
    result = triangle_processing_agent_execute(&ctx);
    
    print_sc_memory(&ctx);
    printf("Result: %s\n", result == SC_RESULT_OK ? "SC_RESULT_OK" : "SC_RESULT_ERROR");

    // Free memory
    for (size_t i = 0; i < ctx.size; i++) {
        free(ctx.entries[i].addr);
        free(ctx.entries[i].type);
    }
    free(ctx.entries);

    return 0;
}
