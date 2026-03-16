#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define INITIAL_BUFFER_SIZE 256
#define BUFFER_GROWTH_FACTOR 2

char* trim(char* s) {
    while (*s == ' ') s++;
    char* end = s + strlen(s) - 1;
    while (end > s && *end == ' ') {
        *end = '\0';
        end--;
    }
    return s;
}

int malloc_count = 0;
int realloc_count = 0;
int free_count = 0;

void* my_malloc(size_t size) {
    malloc_count++;
    return malloc(size);
}

void* my_realloc(void* ptr, size_t size) {
    realloc_count++;
    return realloc(ptr, size);
}

void my_free(void* ptr) {
    if (ptr) {
        free_count++;
        free(ptr);
    }
}

typedef enum {
    NODE_NUM,
    NODE_VAR,
    NODE_UNARY,
    NODE_BINARY
} NodeType;

struct Unary {
    char op;
    struct Node* child;
};

struct Binary {
    char op;
    struct Node* left;
    struct Node* right;
};

typedef struct Node {
    NodeType type;
    union {
        int num;
        char var;
        struct Unary unary;
        struct Binary binary;
    } data;
} Node;

Node* root = NULL;

Node* create_num(int val) {
    Node* n = (Node*)my_malloc(sizeof(Node));
    n->type = NODE_NUM;
    n->data.num = val;
    return n;
}

Node* create_var(char c) {
    Node* n = (Node*)my_malloc(sizeof(Node));
    n->type = NODE_VAR;
    n->data.var = c;
    return n;
}

Node* create_unary(char op, Node* child) {
    Node* n = (Node*)my_malloc(sizeof(Node));
    n->type = NODE_UNARY;
    n->data.unary.op = op;
    n->data.unary.child = child;
    return n;
}

Node* create_binary(char op, Node* left, Node* right) {
    Node* n = (Node*)my_malloc(sizeof(Node));
    n->type = NODE_BINARY;
    n->data.binary.op = op;
    n->data.binary.left = left;
    n->data.binary.right = right;
    return n;
}

void free_tree(Node* node) {
    if (!node) return;
    if (node->type == NODE_UNARY)
        free_tree(node->data.unary.child);
    else if (node->type == NODE_BINARY) {
        free_tree(node->data.binary.left);
        free_tree(node->data.binary.right);
    }
    my_free(node);
}

int is_valid_expr_char(char c) {
    if (isdigit(c)) return 1;
    if (islower(c)) return 1;
    if (strchr("+-*/%^!(),", c)) return 1;
    if (isspace(c)) return 1;
    return 0;
}

int is_valid_expr(const char* s) {
    for (; *s; s++)
        if (!is_valid_expr_char(*s)) return 0;
    return 1;
}

typedef struct {
    const char* s;
    int pos;
} Parser;

void skip_spaces(Parser* p) {
    while (p->s[p->pos] && isspace(p->s[p->pos])) p->pos++;
}

char current_char(Parser* p) {
    skip_spaces(p);
    return p->s[p->pos];
}

int eof_inf(Parser* p) {
    skip_spaces(p);
    return p->s[p->pos] == '\0';
}

/* Прототипы парсеров */
Node* parse_primary(Parser* p);
Node* parse_unary(Parser* p);
Node* parse_factor(Parser* p);
Node* parse_term(Parser* p);
Node* parse_expression(Parser* p);

Node* parse_primary(Parser* p) {
    char c;
    long val = 0;
    int digits = 0;
    c = current_char(p);
    if (c == '(') {
        p->pos++;
        Node* node = parse_expression(p);
        if (!node) return NULL;
        if (current_char(p) != ')') {
            free_tree(node);
            return NULL;
        }
        p->pos++;
        return node;
    }
    else if (isdigit(c)) {
        while (isdigit(p->s[p->pos])) {
            val = val * 10 + (p->s[p->pos] - '0');
            if (val > INT_MAX) return NULL;
            p->pos++;
            digits++;
        }
        if (digits == 0) return NULL;
        return create_num((int)val);
    }
    else if (islower(c)) {
        p->pos++;
        return create_var(c);
    }
    return NULL;
}

Node* parse_unary(Parser* p) {
    int minus_cnt = 0;
    while (current_char(p) == '-') {
        minus_cnt++;
        p->pos++;
        skip_spaces(p);
    }
    Node* node = parse_primary(p);
    if (!node) return NULL;
    skip_spaces(p);
    if (current_char(p) == '!') {
        p->pos++;
        node = create_unary('!', node);
    }
    while (minus_cnt-- > 0) {
        node = create_unary('-', node);
    }
    return node;
}

Node* parse_factor(Parser* p) {
    Node* left = parse_unary(p);
    if (!left) return NULL;
    while (1) {
        skip_spaces(p);
        char op = current_char(p);
        if (op == '^') {
            p->pos++;
            Node* right = parse_unary(p);
            if (!right) {
                free_tree(left);
                return NULL;
            }
            left = create_binary('^', left, right);
        }
        else break;
    }
    return left;
}

Node* parse_term(Parser* p) {
    Node* left = parse_factor(p);
    if (!left) return NULL;
    while (1) {
        skip_spaces(p);
        char op = current_char(p);
        if (op == '*' || op == '/' || op == '%') {
            p->pos++;
            Node* right = parse_factor(p);
            if (!right) {
                free_tree(left);
                return NULL;
            }
            left = create_binary(op, left, right);
        }
        else break;
    }
    return left;
}

Node* parse_expression(Parser* p) {
    Node* left = parse_term(p);
    if (!left) return NULL;
    while (1) {
        skip_spaces(p);
        char op = current_char(p);
        if (op == '+' || op == '-') {
            p->pos++;
            Node* right = parse_term(p);
            if (!right) {
                free_tree(left);
                return NULL;
            }
            left = create_binary(op, left, right);
        }
        else break;
    }
    return left;
}

Node* parse_infix(const char* str) {
    Parser p;
    p.s = str;
    p.pos = 0;
    Node* res = parse_expression(&p);
    if (res && !eof_inf(&p)) {
        free_tree(res);
        return NULL;
    }
    return res;
}

Node* parse_prefix_sub(Parser* p) {
    skip_spaces(p);
    char c = p->s[p->pos];
    if (c == '\0') return NULL;

    if (isdigit(c)) {
        long val = 0;
        while (isdigit(p->s[p->pos])) {
            val = val * 10 + (p->s[p->pos] - '0');
            if (val > INT_MAX) return NULL;
            p->pos++;
        }
        return create_num((int)val);
    }
    if (islower(c)) {
        p->pos++;
        return create_var(c);
    }
    if (strchr("+-*/%^!", c)) {
        char op = c;
        p->pos++;
        skip_spaces(p);
        if (p->s[p->pos] != '(') return NULL;
        p->pos++;
        Node* left = parse_prefix_sub(p);
        if (!left) return NULL;
        skip_spaces(p);
        if (p->s[p->pos] == ',') {
            p->pos++;
            Node* right = parse_prefix_sub(p);
            if (!right) {
                free_tree(left);
                return NULL;
            }
            skip_spaces(p);
            if (p->s[p->pos] != ')') {
                free_tree(left);
                free_tree(right);
                return NULL;
            }
            p->pos++;
            return create_binary(op, left, right);
        }
        else if (p->s[p->pos] == ')') {
            p->pos++;
            return create_unary(op, left);
        }
        else {
            free_tree(left);
            return NULL;
        }
    }
    return NULL;
}

Node* parse_prefix(const char* str) {
    Parser p;
    p.s = str;
    p.pos = 0;
    Node* res = parse_prefix_sub(&p);
    if (res) {
        skip_spaces(&p);
        if (p.s[p.pos] != '\0') {
            free_tree(res);
            return NULL;
        }
    }
    return res;
}

Node* parse_postfix_sub(Parser* p) {
    skip_spaces(p);
    char c = p->s[p->pos];
    if (c == '\0') return NULL;

    if (isdigit(c)) {
        long val = 0;
        while (isdigit(p->s[p->pos])) {
            val = val * 10 + (p->s[p->pos] - '0');
            if (val > INT_MAX) return NULL;
            p->pos++;
        }
        return create_num((int)val);
    }
    if (islower(c)) {
        p->pos++;
        return create_var(c);
    }
    if (c == '(') {
        p->pos++;
        Node* left = parse_postfix_sub(p);
        if (!left) return NULL;
        skip_spaces(p);
        if (p->s[p->pos] == ',') {
            p->pos++;
            Node* right = parse_postfix_sub(p);
            if (!right) {
                free_tree(left);
                return NULL;
            }
            skip_spaces(p);
            if (p->s[p->pos] != ')') {
                free_tree(left);
                free_tree(right);
                return NULL;
            }
            p->pos++;
            skip_spaces(p);
            char op = p->s[p->pos];
            if (!strchr("+-*/%^!", op)) {
                free_tree(left);
                free_tree(right);
                return NULL;
            }
            p->pos++;
            return create_binary(op, left, right);
        }
        else if (p->s[p->pos] == ')') {
            p->pos++;
            skip_spaces(p);
            char op = p->s[p->pos];
            if (!strchr("+-*/%^!", op)) {
                free_tree(left);
                return NULL;
            }
            p->pos++;
            return create_unary(op, left);
        }
        else {
            free_tree(left);
            return NULL;
        }
    }
    return NULL;
}

Node* parse_postfix(const char* str) {
    Parser p;
    p.s = str;
    p.pos = 0;
    Node* res = parse_postfix_sub(&p);
    if (res) {
        skip_spaces(&p);
        if (p.s[p.pos] != '\0') {
            free_tree(res);
            return NULL;
        }
    }
    return res;
}

void print_prefix(Node* node, FILE* out) {
    if (!node) return;
    switch (node->type) {
    case NODE_NUM:
        fprintf(out, "%d", node->data.num);
        break;
    case NODE_VAR:
        fprintf(out, "%c", node->data.var);
        break;
    case NODE_UNARY:
        fprintf(out, "%c(", node->data.unary.op);
        print_prefix(node->data.unary.child, out);
        fprintf(out, ")");
        break;
    case NODE_BINARY:
        fprintf(out, "%c(", node->data.binary.op);
        print_prefix(node->data.binary.left, out);
        fprintf(out, ",");
        print_prefix(node->data.binary.right, out);
        fprintf(out, ")");
        break;
    }
}

void print_postfix(Node* node, FILE* out) {
    if (!node) return;
    switch (node->type) {
    case NODE_NUM:
        fprintf(out, "%d", node->data.num);
        break;
    case NODE_VAR:
        fprintf(out, "%c", node->data.var);
        break;
    case NODE_UNARY:
        fprintf(out, "(");
        print_postfix(node->data.unary.child, out);
        fprintf(out, ")%c", node->data.unary.op);
        break;
    case NODE_BINARY:
        fprintf(out, "(");
        print_postfix(node->data.binary.left, out);
        fprintf(out, ",");
        print_postfix(node->data.binary.right, out);
        fprintf(out, ")%c", node->data.binary.op);
        break;
    }
}

/* Сбор используемых переменных */
void collect_vars(Node* node, int used[]) {
    if (!node) return;
    switch (node->type) {
    case NODE_VAR:
        used[node->data.var - 'a'] = 1;
        break;
    case NODE_UNARY:
        collect_vars(node->data.unary.child, used);
        break;
    case NODE_BINARY:
        collect_vars(node->data.binary.left, used);
        collect_vars(node->data.binary.right, used);
        break;
    default:
        break;
    }
}

/* Проверка переполнения signed int */
static int would_overflow_long_long(long long x) {
    return (x > INT_MAX || x < INT_MIN);
}

long eval_node(Node* node, int* vars, int* error) {
    if (!node) { *error = 1; return 0; }
    switch (node->type) {
    case NODE_NUM:
        return node->data.num;
    case NODE_VAR: {
        int idx = node->data.var - 'a';
        if (idx < 0 || idx > 25) { *error = 1; return 0; }
        return vars[idx];
    }
    case NODE_UNARY: {
        long val = eval_node(node->data.unary.child, vars, error);
        if (*error) return 0;
        char op = node->data.unary.op;
        if (op == '-') {
            long long res = -(long long)val;
            if (would_overflow_long_long(res)) { *error = 1; return 0; }
            return (long)res;
        }
        else if (op == '!') {
            if (val < 0) { *error = 1; return 0; }
            long long fact = 1;
            long i;
            for (i = 2; i <= val; i++) {
                fact *= i;
                if (fact > INT_MAX) { *error = 1; return 0; }
            }
            return (long)fact;
        }
        *error = 1;
        return 0;
    }
    case NODE_BINARY: {
        long left = eval_node(node->data.binary.left, vars, error);
        long right = eval_node(node->data.binary.right, vars, error);
        if (*error) return 0;
        char op = node->data.binary.op;
        long long res;
        if (op == '+') {
            res = (long long)left + right;
            if (would_overflow_long_long(res)) { *error = 1; return 0; }
            return (long)res;
        }
        if (op == '-') {
            res = (long long)left - right;
            if (would_overflow_long_long(res)) { *error = 1; return 0; }
            return (long)res;
        }
        if (op == '*') {
            res = (long long)left * right;
            if (would_overflow_long_long(res)) { *error = 1; return 0; }
            return (long)res;
        }
        if (op == '/') {
            if (right == 0) { *error = 1; return 0; }
            return left / right;
        }
        if (op == '%') {
            if (right == 0) { *error = 1; return 0; }
            return left % right;
        }
        if (op == '^') {
            if (right < 0) { *error = 1; return 0; }
            if (left == 0 && right == 0) { *error = 1; return 0; }
            long long power = 1;
            long i;
            for (i = 0; i < right; i++) {
                power *= left;
                if (would_overflow_long_long(power)) { *error = 1; return 0; }
            }
            return (long)power;
        }
        *error = 1;
        return 0;
    }
    }
    *error = 1;
    return 0;
}

void print_incorrect(FILE* out) {
    fprintf(out, "incorrect\n");
}

void print_not_loaded(FILE* out) {
    fprintf(out, "not_loaded\n");
}

void handle_parse(const char* args, FILE* out) {
    if (*args == '\0') {
        print_incorrect(out);
        return;
    }
    if (!is_valid_expr(args)) {
        print_incorrect(out);
        return;
    }
    Node* new_root = parse_infix(args);
    if (new_root) {
        if (root) free_tree(root);
        root = new_root;
        fprintf(out, "success\n");
    }
    else {
        fprintf(out, "incorrect\n");
    }
}

void handle_load_prf(const char* args, FILE* out) {
    if (*args == '\0') {
        print_incorrect(out);
        return;
    }
    if (!is_valid_expr(args)) {
        print_incorrect(out);
        return;
    }
    Node* new_root = parse_prefix(args);
    if (new_root) {
        if (root) free_tree(root);
        root = new_root;
        fprintf(out, "success\n");
    }
    else {
        fprintf(out, "incorrect\n");
    }
}

void handle_load_pst(const char* args, FILE* out) {
    if (*args == '\0') {
        print_incorrect(out);
        return;
    }
    if (!is_valid_expr(args)) {
        print_incorrect(out);
        return;
    }
    Node* new_root = parse_postfix(args);
    if (new_root) {
        if (root) free_tree(root);
        root = new_root;
        fprintf(out, "success\n");
    }
    else {
        fprintf(out, "incorrect\n");
    }
}

void handle_save_prf(FILE* out) {
    if (root == NULL) {
        print_not_loaded(out);
        return;
    }
    print_prefix(root, out);
    fprintf(out, "\n");
}

void handle_save_pst(FILE* out) {
    if (root == NULL) {
        print_not_loaded(out);
        return;
    }
    print_postfix(root, out);
    fprintf(out, "\n");
}

void handle_eval(const char* args, FILE* out) {
    if (root == NULL) {
        print_not_loaded(out);
        return;
    }

    int used_vars[26] = {0};
    collect_vars(root, used_vars);

    int var_values[26] = {0};
    int seen[26] = {0};

    const char* p = args;
    while (isspace(*p)) p++;

    if (*p != '\0') {
        while (1) {
            if (!islower(*p)) { print_incorrect(out); return; }
            char name = *p;
            int idx = name - 'a';
            p++;

            if (*p != '=') { print_incorrect(out); return; }
            p++;

            int sign = 1;
            if (*p == '-') { sign = -1; p++; }
            else if (*p == '+') p++;

            if (!isdigit(*p)) { print_incorrect(out); return; }
            long long val = 0;
            while (isdigit(*p)) {
                val = val * 10 + (*p - '0');
                if (val > INT_MAX) { print_incorrect(out); return; }
                p++;
            }
            val *= sign;
            if (val < INT_MIN || val > INT_MAX) { print_incorrect(out); return; }

            if (seen[idx]) { print_incorrect(out); return; }
            seen[idx] = 1;
            var_values[idx] = (int)val;

            if (*p == '\0') break;
            if (*p == ',') {
                p++;
                if (!islower(*p)) { print_incorrect(out); return; }
                continue;
            }
            else { print_incorrect(out); return; }
        }
    }

    int i;
    for (i = 0; i < 26; i++) {
        if (used_vars[i] && !seen[i]) {
            fprintf(out, "no_var_values\n");
            return;
        }
    }

    int error = 0;
    long res = eval_node(root, var_values, &error);
    if (error) {
        fprintf(out, "error\n");
    }
    else {
        fprintf(out, "%ld\n", res);
    }
}

char* read_dynamic_line(FILE* input, size_t* line_length) {
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char* buffer = (char*)my_malloc(buffer_size);
    if (!buffer) return NULL;
    buffer[0] = '\0';
    size_t pos = 0;

    while (1) {
        if (fgets(buffer + pos, (int)(buffer_size - pos), input) == NULL) {
            if (pos == 0) {
                my_free(buffer);
                return NULL;
            }
            break;
        }
        size_t len = strlen(buffer + pos);
        pos += len;
        if (len > 0 && buffer[pos - 1] == '\n') break;
        if (pos >= buffer_size - 1) {
            size_t new_size = buffer_size * BUFFER_GROWTH_FACTOR;
            char* new_buffer = (char*)my_realloc(buffer, new_size);
            if (!new_buffer) {
                my_free(buffer);
                return NULL;
            }
            buffer = new_buffer;
            buffer_size = new_size;
        }
    }
    *line_length = pos;
    return buffer;
}

void read_input(FILE* input, FILE* output) {
    char* line;
    size_t line_length;
    while ((line = read_dynamic_line(input, &line_length)) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        line = trim(line);
        if (line[0] == '\0') {
            my_free(line);
            continue;
        }

        char* line_copy = (char*)my_malloc(strlen(line) + 1);
        if (!line_copy) {
            my_free(line);
            continue;
        }
        strcpy(line_copy, line);

        char* cmd = strtok(line_copy, " \t");
        if (!cmd) {
            my_free(line_copy);
            print_incorrect(output);
            my_free(line);
            continue;
        }

        char* args = line + strlen(cmd);
        while (*args == ' ' || *args == '\t') args++;

        int handled = 0;

        if (strcmp(cmd, "parse") == 0) {
            handle_parse(args, output);
            handled = 1;
        }
        else if (strcmp(cmd, "load_prf") == 0) {
            handle_load_prf(args, output);
            handled = 1;
        }
        else if (strcmp(cmd, "load_pst") == 0) {
            handle_load_pst(args, output);
            handled = 1;
        }
        else if (strcmp(cmd, "save_prf") == 0) {
            if (*args != '\0') print_incorrect(output);
            else handle_save_prf(output);
            handled = 1;
        }
        else if (strcmp(cmd, "save_pst") == 0) {
            if (*args != '\0') print_incorrect(output);
            else handle_save_pst(output);
            handled = 1;
        }
        else if (strcmp(cmd, "eval") == 0) {
            handle_eval(args, output);
            handled = 1;
        }

        if (!handled) print_incorrect(output);

        my_free(line);
        my_free(line_copy);
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

    read_input(input, output);

    if (root) free_tree(root);

    FILE* memstat = fopen("memstat.txt", "w");
    if (memstat) {
        fprintf(memstat, "malloc:%d\n", malloc_count);
        fprintf(memstat, "calloc:0\n");
        fprintf(memstat, "realloc:%d\n", realloc_count);
        fprintf(memstat, "free:%d\n", free_count);
        fclose(memstat);
    }

    fclose(input);
    fclose(output);
    return 0;
}
