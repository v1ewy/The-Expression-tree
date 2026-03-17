#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

static size_t malloc_count = 0;
static size_t realloc_count = 0;
static size_t free_count = 0;

void* my_malloc(size_t size) {
    malloc_count++;
    return malloc(size);
}

void* my_realloc(void* ptr, size_t size) {
    if (ptr == NULL)
        return my_malloc(size);
    else
        realloc_count++;
    return realloc(ptr, size);
}
void my_free(void* ptr) {
    if (ptr) {
        free_count++;
        free(ptr);
    }
}

#define malloc my_malloc
#define realloc my_realloc
#define free my_free
#define INITIAL_BUFFER_SIZE 256
#define BUFFER_GROWTH_FACTOR 2

typedef enum {
    CONST,
    VAR,
    UNARY,
    BINARY
} NodeType;

typedef struct Node {
    NodeType type;
    union {
        int const_val;
        char var_name;
        struct {
            char op;
            struct Node* operand;
        } unary;
        struct {
            char op;
            struct Node* left;
            struct Node* right;
        } binary;
    } data;
} Node;

Node* create_const(int val) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = CONST;
    node->data.const_val = val;
    return node;
}

Node* create_var(char name) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = VAR;
    node->data.var_name = name;
    return node;
}

Node* create_unary(char op, Node* operand) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = UNARY;
    node->data.unary.op = op;
    node->data.unary.operand = operand;
    return node;
}

Node* create_binary(char op, Node* left, Node* right) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = BINARY;
    node->data.binary.op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;
    return node;
}

void free_node(Node* node) {
    if (!node) return;
    if (node->type == UNARY)
        free_node(node->data.unary.operand);
    else if (node->type == BINARY) {
        free_node(node->data.binary.left);
        free_node(node->data.binary.right);
    }
    free(node);
}

void skip_spaces(const char* s, int* pos) {
    while (s[*pos] && isspace((unsigned char)s[*pos]))
        (*pos)++;
}

static Node* parse_sum(const char* s, int* pos);
static Node* parse_mult(const char* s, int* pos);
static Node* parse_pow(const char* s, int* pos);
static Node* parse_unary_minus(const char* s, int* pos);
static Node* parse_factorial(const char* s, int* pos);
static Node* parse_primary(const char* s, int* pos);

static Node* parse_primary(const char* s, int* pos) {
    skip_spaces(s, pos);
    char c = s[*pos];
    if (c == '(') {
        (*pos)++;
        Node* expr = parse_sum(s, pos);
        if (!expr) return NULL;
        skip_spaces(s, pos);
        if (s[*pos] != ')') {
            free_node(expr);
            return NULL;
        }
        (*pos)++;
        return expr;
    }
    else if (isdigit((unsigned char)c)) {
        long val = 0;
        int start = *pos;
        while (isdigit((unsigned char)s[*pos])) {
            val = val * 10 + (s[*pos] - '0');
            if (val > INT_MAX) return NULL;
            (*pos)++;
        }
        if (*pos == start) return NULL;
        return create_const((int)val);
    }
    else if (islower((unsigned char)c)) {
        (*pos)++;
        return create_var(c);
    }
    return NULL;
}

static Node* parse_factorial(const char* s, int* pos) {
    Node* node = parse_primary(s, pos);
    if (!node) return NULL;
    skip_spaces(s, pos);
    while (s[*pos] == '!') {
        (*pos)++;
        Node* newnode = create_unary('!', node);
        if (!newnode) {
            free_node(node);
            return NULL;
        }
        node = newnode;
        skip_spaces(s, pos);
    }
    return node;
}

static Node* parse_unary_minus(const char* s, int* pos) {
    skip_spaces(s, pos);
    if (s[*pos] == '-') {
        (*pos)++;
        Node* sub = parse_unary_minus(s, pos);
        if (!sub) return NULL;
        return create_unary('-', sub);
    }
    return parse_factorial(s, pos);
}

static Node* parse_pow(const char* s, int* pos) {
    Node* left = parse_unary_minus(s, pos);
    if (!left) return NULL;
    skip_spaces(s, pos);
    while (s[*pos] == '^') {
        char op = s[*pos];
        (*pos)++;
        Node* right = parse_unary_minus(s, pos);
        if (!right) {
            free_node(left);
            return NULL;
        }
        Node* newnode = create_binary(op, left, right);
        if (!newnode) {
            free_node(left);
            free_node(right);
            return NULL;
        }
        left = newnode;
        skip_spaces(s, pos);
    }
    return left;
}

static Node* parse_mult(const char* s, int* pos) {
    Node* left = parse_pow(s, pos);
    if (!left) return NULL;
    skip_spaces(s, pos);
    while (s[*pos] == '*' || s[*pos] == '/' || s[*pos] == '%') {
        char op = s[*pos];
        (*pos)++;
        Node* right = parse_pow(s, pos);
        if (!right) {
            free_node(left);
            return NULL;
        }
        Node* newnode = create_binary(op, left, right);
        if (!newnode) {
            free_node(left);
            free_node(right);
            return NULL;
        }
        left = newnode;
        skip_spaces(s, pos);
    }
    return left;
}

static Node* parse_sum(const char* s, int* pos) {
    Node* left = parse_mult(s, pos);
    if (!left) return NULL;
    skip_spaces(s, pos);
    while (s[*pos] == '+' || s[*pos] == '-') {
        char op = s[*pos];
        (*pos)++;
        Node* right = parse_mult(s, pos);
        if (!right) {
            free_node(left);
            return NULL;
        }
        Node* newnode = create_binary(op, left, right);
        if (!newnode) {
            free_node(left);
            free_node(right);
            return NULL;
        }
        left = newnode;
        skip_spaces(s, pos);
    }
    return left;
}

Node* parse_infix(const char* s) {
    int pos = 0;
    Node* root = parse_sum(s, &pos);
    if (root) {
        skip_spaces(s, &pos);
        if (s[pos] != '\0') {
            free_node(root);
            root = NULL;
        }
    }
    return root;
}

static Node* parse_prefix_rec(const char* s, int* pos) {
    skip_spaces(s, pos);
    char c = s[*pos];
    if (c == '\0') return NULL;
    if (isdigit((unsigned char)c)) {
        long val = 0;
        while (isdigit((unsigned char)s[*pos])) {
            val = val * 10 + (s[*pos] - '0');
            if (val > INT_MAX) return NULL;
            (*pos)++;
        }
        return create_const((int)val);
    }
    else if (islower((unsigned char)c)) {
        (*pos)++;
        return create_var(c);
    }
    else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^' || c == '!') {
        char op = c;
        (*pos)++;
        skip_spaces(s, pos);
        if (s[*pos] != '(') return NULL;
        (*pos)++;
        Node* arg1 = parse_prefix_rec(s, pos);
        if (!arg1) return NULL;
        skip_spaces(s, pos);
        if (s[*pos] == ',') {
            (*pos)++;
            Node* arg2 = parse_prefix_rec(s, pos);
            if (!arg2) {
                free_node(arg1);
                return NULL;
            }
            skip_spaces(s, pos);
            if (s[*pos] != ')') {
                free_node(arg1);
                free_node(arg2);
                return NULL;
            }
            (*pos)++;
            return create_binary(op, arg1, arg2);
        }
        else if (s[*pos] == ')') {
            (*pos)++;
            return create_unary(op, arg1);
        }
        else {
            free_node(arg1);
            return NULL;
        }
    }
    return NULL;
}

Node* parse_prefix(const char* s) {
    int pos = 0;
    Node* root = parse_prefix_rec(s, &pos);
    if (root) {
        skip_spaces(s, &pos);
        if (s[pos] != '\0') {
            free_node(root);
            root = NULL;
        }
    }
    return root;
}

static Node* parse_postfix_rec(const char* s, int* pos) {
    skip_spaces(s, pos);
    char c = s[*pos];
    if (c == '\0') return NULL;
    if (isdigit((unsigned char)c)) {
        long val = 0;
        while (isdigit((unsigned char)s[*pos])) {
            val = val * 10 + (s[*pos] - '0');
            if (val > INT_MAX) return NULL;
            (*pos)++;
        }
        return create_const((int)val);
    }
    else if (islower((unsigned char)c)) {
        (*pos)++;
        return create_var(c);
    }
    else if (c == '(') {
        (*pos)++;
        Node* arg1 = parse_postfix_rec(s, pos);
        if (!arg1) return NULL;
        skip_spaces(s, pos);
        if (s[*pos] == ',') {
            (*pos)++;
            Node* arg2 = parse_postfix_rec(s, pos);
            if (!arg2) {
                free_node(arg1);
                return NULL;
            }
            skip_spaces(s, pos);
            if (s[*pos] != ')') {
                free_node(arg1);
                free_node(arg2);
                return NULL;
            }
            (*pos)++;
            skip_spaces(s, pos);
            char op = s[*pos];
            if (op != '+' && op != '-' && op != '*' && op != '/' && op != '%' && op != '^' && op != '!') {
                free_node(arg1);
                free_node(arg2);
                return NULL;
            }
            (*pos)++;
            return create_binary(op, arg1, arg2);
        }
        else if (s[*pos] == ')') {
            (*pos)++;
            skip_spaces(s, pos);
            char op = s[*pos];
            if (op != '+' && op != '-' && op != '*' && op != '/' && op != '%' && op != '^' && op != '!') {
                free_node(arg1);
                return NULL;
            }
            (*pos)++;
            return create_unary(op, arg1);
        }
        else {
            free_node(arg1);
            return NULL;
        }
    }
    return NULL;
}

Node* parse_postfix(const char* s) {
    int pos = 0;
    Node* root = parse_postfix_rec(s, &pos);
    if (root) {
        skip_spaces(s, &pos);
        if (s[pos] != '\0') {
            free_node(root);
            root = NULL;
        }
    }
    return root;
}

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} StringBuilder;

void sb_init(StringBuilder* sb) {
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

void sb_append_char(StringBuilder* sb, char c) {
    if (sb->len + 1 >= sb->cap) {
        size_t new_cap = sb->cap == 0 ? 16 : sb->cap * 2;
        char* new_data = (char*)realloc(sb->data, new_cap);
        sb->data = new_data;
        sb->cap = new_cap;
    }
    sb->data[sb->len++] = c;
    sb->data[sb->len] = '\0';
}

void sb_append_str(StringBuilder* sb, const char* str) {
    while (*str)
        sb_append_char(sb, *str++);
}

void sb_free(StringBuilder* sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len = sb->cap = 0;
}

void node_to_prefix(Node* node, StringBuilder* sb) {
    if (!node) return;
    if (node->type == CONST) {
        char buf[32];
        sprintf(buf, "%d", node->data.const_val);
        sb_append_str(sb, buf);
    }
    else if (node->type == VAR) {
        sb_append_char(sb, node->data.var_name);
    }
    else if (node->type == UNARY) {
        sb_append_char(sb, node->data.unary.op);
        sb_append_char(sb, '(');
        node_to_prefix(node->data.unary.operand, sb);
        sb_append_char(sb, ')');
    }
    else {
        sb_append_char(sb, node->data.binary.op);
        sb_append_char(sb, '(');
        node_to_prefix(node->data.binary.left, sb);
        sb_append_str(sb, ",");
        node_to_prefix(node->data.binary.right, sb);
        sb_append_char(sb, ')');
    }
}

void node_to_postfix(Node* node, StringBuilder* sb) {
    if (!node) return;
    if (node->type == CONST) {
        char buf[32];
        sprintf(buf, "%d", node->data.const_val);
        sb_append_str(sb, buf);
    }
    else if (node->type == VAR) {
        sb_append_char(sb, node->data.var_name);
    }
    else if (node->type == UNARY) {
        sb_append_char(sb, '(');
        node_to_postfix(node->data.unary.operand, sb);
        sb_append_char(sb, ')');
        sb_append_char(sb, node->data.unary.op);
    }
    else {
        sb_append_char(sb, '(');
        node_to_postfix(node->data.binary.left, sb);
        sb_append_str(sb, ",");
        node_to_postfix(node->data.binary.right, sb);
        sb_append_char(sb, ')');
        sb_append_char(sb, node->data.binary.op);
    }
}

int safe_add(int a, int b, int* res) {
    if (b > 0 && a > INT_MAX - b) return 1;
    if (b < 0 && a < INT_MIN - b) return 1;
    *res = a + b;
    return 0;
}

int safe_sub(int a, int b, int* res) {
    if (b > 0 && a < INT_MIN + b) return 1;
    if (b < 0 && a > INT_MAX + b) return 1;
    *res = a - b;
    return 0;
}

int safe_mul(int a, int b, int* res) {
    if (a == 0 || b == 0) {
        *res = 0;
        return 0;
    }
    if (a > 0 && b > 0) {
        if (a > INT_MAX / b) return 1;
    }
    else if (a > 0 && b < 0) {
        if (b < INT_MIN / a) return 1;
    }
    else if (a < 0 && b > 0) {
        if (a < INT_MIN / b) return 1;
    }
    else {
        if (-a > INT_MAX / -b) return 1;
    }
    *res = a * b;
    return 0;
}

int safe_factorial(int n, int* res) {
    if (n < 0) return 1;
    int fact = 1;
    int i;
    for (i = 2; i <= n; i++) {
        if (fact > INT_MAX / i) return 1;
        fact *= i;
    }
    *res = fact;
    return 0;
}

int safe_pow(int a, int b, int* res) {
    if (b < 0) {
        return 1;
    }
    if (b == 0) {
        *res = 1;
        return 0;
    }
    long long t = a;
    while (--b) {
        t *= a;
    }
    if (t > INT_MAX) return 1;
    *res = (int)t;
    
    return 0;
}

int eval_node(Node* node, int* var_values, int* var_set, int* error) {
    if (*error) return 0;
    if (!node) {
        *error = 1;
        return 0;
    }
    if (node->type == CONST) {
        return node->data.const_val;
    }
    else if (node->type == VAR) {
        int idx = node->data.var_name - 'a';
        if (!var_set[idx]) {
            *error = 1;
            return 0;
        }
        return var_values[idx];
    }
    else if (node->type == UNARY) {
        int val = eval_node(node->data.unary.operand, var_values, var_set, error);
        if (*error) return 0;
        if (node->data.unary.op == '-') {
            return -val;
        }
        else if (node->data.unary.op == '!') {
            int res;
            if (safe_factorial(val, &res)) {
                *error = 1;
                return 0;
            }
            return res;
        }
        else {
            *error = 1;
            return 0;
        }
    }
    else {
        int left = eval_node(node->data.binary.left, var_values, var_set, error);
        if (*error) return 0;
        int right = eval_node(node->data.binary.right, var_values, var_set, error);
        if (*error) return 0;
        char op = node->data.binary.op;
        if (op == '+') {
            int res;
            if (safe_add(left, right, &res)) { *error = 1; return 0; }
            return res;
        }
        else if (op == '-') {
            int res;
            if (safe_sub(left, right, &res)) { *error = 1; return 0; }
            return res;
        }
        else if (op == '*') {
            int res;
            if (safe_mul(left, right, &res)) { *error = 1; return 0; }
            return res;
        }
        else if (op == '/') {
            if (right == 0) { *error = 1; return 0; }
            return left / right;
        }
        else if (op == '%') {
            if (right == 0) { *error = 1; return 0; }
            return left % right;
        }
        else if (op == '^') {
            int res;
            if (safe_pow(left, right, &res)) { *error = 1; return 0; }
            return res;
        }
        else {
            *error = 1;
            return 0;
        }
    }
}

void collect_vars(Node* node, int* vars) {
    if (!node) return;
    if (node->type == VAR) {
        int idx = node->data.var_name - 'a';
        vars[idx] = 1;
    }
    else if (node->type == UNARY) {
        collect_vars(node->data.unary.operand, vars);
    }
    else if (node->type == BINARY) {
        collect_vars(node->data.binary.left, vars);
        collect_vars(node->data.binary.right, vars);
    }
}

char* trim(char* s) {
    while (*s == ' ') s++;
    char* end = s + strlen(s) - 1;
    while (end > s && *end == ' ') {
        *end = '\0';
        end--;
    }
    return s;
}

char* read_dynamic_line(FILE* input, size_t* line_length) {
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char* buffer = (char*)malloc(buffer_size);
    if (!buffer) return NULL;
    buffer[0] = '\0';
    size_t pos = 0;

    while (1) {
        if (fgets(buffer + pos, (int)(buffer_size - pos), input) == NULL) {
            if (pos == 0) {
                free(buffer);
                return NULL;
            }
            break;
        }
        size_t len = strlen(buffer + pos);
        pos += len;
        if (len > 0 && buffer[pos - 1] == '\n') break;
        if (pos >= buffer_size - 1) {
            size_t new_size = buffer_size * BUFFER_GROWTH_FACTOR;
            char* new_buffer = (char*)realloc(buffer, new_size);
            if (!new_buffer) {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
            buffer_size = new_size;
        }
    }
    *line_length = pos;
    return buffer;
}

void read_input(FILE* input, FILE* output, Node** current_tree) {
    char* line;
    size_t line_length;
    while ((line = read_dynamic_line(input, &line_length)) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        line = trim(line);
        if (line[0] == '\0') {
            free(line);
            continue;
        }

        char* p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') continue;

        char cmd[32];
        int n = 0;
        if (sscanf(p, "%31s%n", cmd, &n) != 1) {
            fprintf(output, "incorrect\n");
            continue;
        }
        p += n;
        while (*p && isspace((unsigned char)*p)) p++;

        if (strcmp(cmd, "parse") == 0) {
            Node* new_tree = parse_infix(p);
            if (new_tree) {
                if (current_tree) free_node(*current_tree);
                *current_tree = new_tree;
                fprintf(output, "success\n");
            }
            else {
                fprintf(output, "incorrect\n");
            }
        }
        else if (strcmp(cmd, "load_prf") == 0) {
            Node* new_tree = parse_prefix(p);
            if (new_tree) {
                if (current_tree) free_node(*current_tree);
                *current_tree = new_tree;
                fprintf(output, "success\n");
            }
            else {
                fprintf(output, "incorrect\n");
            }
        }
        else if (strcmp(cmd, "load_pst") == 0) {
            Node* new_tree = parse_postfix(p);
            if (new_tree) {
                if (current_tree) free_node(*current_tree);
                *current_tree = new_tree;
                fprintf(output, "success\n");
            }
            else {
                fprintf(output, "incorrect\n");
            }
        }
        else if (strcmp(cmd, "save_prf") == 0) {
            if (*p != '\0') {
                fprintf(output, "incorrect\n");
            }
            else if (!current_tree) {
                fprintf(output, "not_loaded\n");
            }
            else {
                StringBuilder sb;
                sb_init(&sb);
                node_to_prefix(*current_tree, &sb);
                fprintf(output, "%s\n", sb.data);
                sb_free(&sb);
            }
        }
        else if (strcmp(cmd, "save_pst") == 0) {
            if (*p != '\0') {
                fprintf(output, "incorrect\n");
            }
            else if (!current_tree) {
                fprintf(output, "not_loaded\n");
            }
            else {
                StringBuilder sb;
                sb_init(&sb);
                node_to_postfix(*current_tree, &sb);
                fprintf(output, "%s\n", sb.data);
                sb_free(&sb);
            }
        }
        else if (strcmp(cmd, "eval") == 0) {
            if (!current_tree) {
                fprintf(output, "not_loaded\n");
                continue;
            }

            int var_set[26] = { 0 };
            int var_values[26] = { 0 };

            if (*p == '\0') {
                int vars_present[26] = { 0 };
                collect_vars(*current_tree, vars_present);
                int has_vars = 0;
                int i;
                for (i = 0; i < 26; i++) {
                    if (vars_present[i]) {
                        has_vars = 1;
                        break;
                    }
                }
                if (has_vars) {
                    fprintf(output, "no_var_values\n");
                }
                else {
                    int error = 0;
                    int res = eval_node(*current_tree, var_values, var_set, &error);
                    if (error)
                        fprintf(output, "error\n");
                    else
                        fprintf(output, "%d\n", res);
                }
                continue;
            }

            char* arg = p;
            int incorrect = 0;

            while (*arg) {
                while (*arg && isspace((unsigned char)*arg)) arg++;
                if (*arg == '\0') break;

                if (!islower((unsigned char)*arg)) { incorrect = 1; break; }
                char var = *arg;
                arg++;

                if (*arg != '=') { incorrect = 1; break; }
                arg++;

                int sign = 1;
                if (*arg == '-') { sign = -1; arg++; }
                else if (*arg == '+') arg++;

                if (!isdigit((unsigned char)*arg)) { incorrect = 1; break; }
                long val = 0;
                while (isdigit((unsigned char)*arg)) {
                    val = val * 10 + (*arg - '0');
                    if (val > INT_MAX) { incorrect = 1; break; }
                    arg++;
                }
                if (incorrect) break;
                val *= sign;
                if (val < INT_MIN || val > INT_MAX) { incorrect = 1; break; }

                int idx = var - 'a';
                if (var_set[idx]) { incorrect = 1; break; }
                var_set[idx] = 1;
                var_values[idx] = (int)val;

                while (*arg && isspace((unsigned char)*arg)) arg++;
                if (*arg == ',') {
                    arg++;
                    continue;
                }
                else if (*arg == '\0') {
                    break;
                }
                else {
                    incorrect = 1;
                    break;
                }
            }

            if (incorrect) {
                fprintf(output, "incorrect\n");
                continue;
            }

            int vars_present[26] = { 0 };
            collect_vars(*current_tree, vars_present);
            int missing = 0;
            int extra = 0;
            int i;
            for (i = 0; i < 26; i++) {
                if (vars_present[i] && !var_set[i]) missing = 1;
                if (var_set[i] && !vars_present[i]) extra = 1;
            }
            if (missing) {
                fprintf(output, "no_var_values\n");
                continue;
            }
            if (extra) {
                fprintf(output, "incorrect\n");
                continue;
            }

            int error = 0;
            int res = eval_node(*current_tree, var_values, var_set, &error);
            if (error)
                fprintf(output, "error\n");
            else
                fprintf(output, "%d\n", res);
        }
        else {
            fprintf(output, "incorrect\n");
        }
        
        free(line);
    }
}

int main(void) {
    FILE* input = fopen("input.txt", "r");
    FILE* output = fopen("output.txt", "w");
    if (!input || !output) {
        if (input) fclose(input);
        if (output) fclose(output);
        return 1;
    }

    Node* current_tree = NULL;
    
    read_input(input, output, &current_tree);
    
    if (current_tree) free_node(current_tree);
    fclose(input);
    fclose(output);

    FILE* memstat = fopen("memstat.txt", "w");
    if (memstat) {
        fprintf(memstat, "malloc:%lu\n", (unsigned long)malloc_count);
        fprintf(memstat, "realloc:%lu\n", (unsigned long)realloc_count);
        fprintf(memstat, "free:%lu\n", (unsigned long)free_count);
        fclose(memstat);
    }

    return 0;
}
