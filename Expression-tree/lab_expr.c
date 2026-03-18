#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define INITIAL_BUFFER_SIZE 256
#define BUFFER_GROWTH_FACTOR 2


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

typedef struct Node {
    NodeType type;
    union {
        int num;
        char var;
        struct {
            char op;
            struct Node* child;
        } unary;
        struct {
            char op;
            struct Node* left;
            struct Node* right;
        } binary;
    } data;
} Node;

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
} ParserInfix;

void skip_spaces_inf(ParserInfix* p) {
    while (p->s[p->pos] && isspace(p->s[p->pos])) p->pos++;
}

char current_char_inf(ParserInfix* p) {
    skip_spaces_inf(p);
    return p->s[p->pos];
}

int eof_inf(ParserInfix* p) {
    skip_spaces_inf(p);
    return p->s[p->pos] == '\0';
}

Node* parse_addition(ParserInfix* p);
Node* parse_multiplication(ParserInfix* p);
Node* parse_power(ParserInfix* p);
Node* parse_unary(ParserInfix* p);
Node* parse_factor(ParserInfix* p);
Node* parse_primary(ParserInfix* p);

Node* parse_primary(ParserInfix* p) {
    char c = current_char_inf(p);
    if (c == '(') {
        p->pos++;
        Node* node = parse_addition(p);
        if (!node) return NULL;
        if (current_char_inf(p) != ')') {
            free_tree(node);
            return NULL;
        }
        p->pos++;
        return node;
    }
    else if (isdigit(c)) {
        long long val = 0;
        int digits = 0;
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

Node* parse_factor(ParserInfix* p) {
    Node* node = parse_primary(p);
    if (!node) return NULL;
    skip_spaces_inf(p);
    while (current_char_inf(p) == '!') {
        p->pos++;
        Node* newnode = create_unary('!', node);
        if (!newnode) {
            free_tree(node);
            return NULL;
        }
        node = newnode;
        skip_spaces_inf(p);
    }
    return node;
}

Node* parse_unary(ParserInfix* p) {
    skip_spaces_inf(p);
    if (current_char_inf(p) == '-') {
        p->pos++;
        Node* sub = parse_unary(p);
        if (!sub) return NULL;
        return create_unary('-', sub);
    }
    return parse_factor(p);
}

Node* parse_power(ParserInfix* p) {
    Node* left = parse_unary(p);
    if (!left) return NULL;
    skip_spaces_inf(p);
    while (current_char_inf(p) == '^') {
        p->pos++;
        Node* right = parse_unary(p);
        if (!right) {
            free_tree(left);
            return NULL;
        }
        Node* newnode = create_binary('^', left, right);
        if (!newnode) {
            free_tree(left);
            free_tree(right);
            return NULL;
        }
        left = newnode;
        skip_spaces_inf(p);
    }
    return left;
}

Node* parse_multiplication(ParserInfix* p) {
    Node* left = parse_power(p);
    if (!left) return NULL;
    skip_spaces_inf(p);
    while (1) {
        char op = current_char_inf(p);
        if (op == '*' || op == '/' || op == '%') {
            p->pos++;
            Node* right = parse_power(p);
            if (!right) {
                free_tree(left);
                return NULL;
            }
            Node* newnode = create_binary(op, left, right);
            if (!newnode) {
                free_tree(left);
                free_tree(right);
                return NULL;
            }
            left = newnode;
            skip_spaces_inf(p);
        } else {
            break;
        }
    }
    return left;
}

Node* parse_addition(ParserInfix* p) {
    Node* left = parse_multiplication(p);
    if (!left) return NULL;
    while (1) {
        skip_spaces_inf(p);
        char op = current_char_inf(p);
        if (op == '+' || op == '-') {
            p->pos++;
            Node* right = parse_multiplication(p);
            if (!right) {
                free_tree(left);
                return NULL;
            }
            Node* newnode = create_binary(op, left, right);
            if (!newnode) {
                free_tree(left);
                free_tree(right);
                return NULL;
            }
            left = newnode;
            skip_spaces_inf(p);
        } else {
            break;
        }
    }
    return left;
}

Node* parse_infix(const char* str) {
    ParserInfix p;
    p.s = str;
    p.pos = 0;
    Node* res = parse_addition(&p);
    if (res && !eof_inf(&p)) {
        free_tree(res);
        return NULL;
    }
    return res;
}

typedef struct {
    const char* s;
    int pos;
} ParserPrefix;

void skip_spaces_pref(ParserPrefix* p) {
    while (p->s[p->pos] && isspace(p->s[p->pos])) p->pos++;
}

Node* parse_prefix_sub(ParserPrefix* p) {
    skip_spaces_pref(p);
    char c = p->s[p->pos];
    if (c == '\0') return NULL;
    if (isdigit(c)) {
        long long val = 0;
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
        skip_spaces_pref(p);
        if (p->s[p->pos] != '(') return NULL;
        p->pos++;
        Node* left = parse_prefix_sub(p);
        if (!left) return NULL;
        skip_spaces_pref(p);
        if (p->s[p->pos] == ',') {
            p->pos++;
            Node* right = parse_prefix_sub(p);
            if (!right) {
                free_tree(left);
                return NULL;
            }
            skip_spaces_pref(p);
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
    ParserPrefix p;
    p.s = str;
    p.pos = 0;
    Node* res = parse_prefix_sub(&p);
    if (res) {
        skip_spaces_pref(&p);
        if (p.s[p.pos] != '\0') {
            free_tree(res);
            return NULL;
        }
    }
    return res;
}

typedef struct {
    const char* s;
    int pos;
} ParserPostfix;

void skip_spaces_post(ParserPostfix* p) {
    while (p->s[p->pos] && isspace(p->s[p->pos])) p->pos++;
}

Node* parse_postfix_sub(ParserPostfix* p) {
    skip_spaces_post(p);
    char c = p->s[p->pos];
    if (c == '\0') return NULL;
    if (isdigit(c)) {
        long long val = 0;
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
        skip_spaces_post(p);
        if (p->s[p->pos] == ',') {
            p->pos++;
            Node* right = parse_postfix_sub(p);
            if (!right) {
                free_tree(left);
                return NULL;
            }
            skip_spaces_post(p);
            if (p->s[p->pos] != ')') {
                free_tree(left);
                free_tree(right);
                return NULL;
            }
            p->pos++;
            skip_spaces_post(p);
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
            skip_spaces_post(p);
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
    ParserPostfix p;
    p.s = str;
    p.pos = 0;
    Node* res = parse_postfix_sub(&p);
    if (res) {
        skip_spaces_post(&p);
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
        if (t > INT_MAX || t < INT_MIN) return 1;
        t *= a;
    }
    if (t > INT_MAX || t < INT_MIN) return 1;
    *res = (int)t;
    
    return 0;
}

int eval_tree(Node* node, int* vars_ap, int* vars_val, int* error) {
    if (*error) return 0;
    if (!node) {
        *error = 1;
        return 0;
    }
    if (node->type == NODE_NUM) {
        return node->data.num;
    }
    else if (node->type == NODE_VAR) {
        int idx = node->data.var - 'a';
        if (!vars_ap[idx]) {
            *error = 1;
            return 0;
        }
        return vars_val[idx];
    }
    else if (node->type == NODE_UNARY) {
        int val = eval_tree(node->data.unary.child, vars_ap, vars_val, error);
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
        int left = eval_tree(node->data.binary.left, vars_ap, vars_val, error);
        if (*error) return 0;
        int right = eval_tree(node->data.binary.right, vars_ap, vars_val, error);
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

Node* root = NULL;

void print_incorrect(FILE* out) {
    fprintf(out, "incorrect\n");
}

void print_not_loaded(FILE* out) {
    fprintf(out, "not_loaded\n");
}

void print_error(FILE* out) {
    fprintf(out, "error\n");
}

void handle_parse(const char* args, FILE* out) {
    if (*args == '\0') {
        print_incorrect(out);
        return;
    }
    
    if (!is_valid_expr(args)) {
        fprintf(out, "incorrect\n");
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
        fprintf(out, "incorrect\n");
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
        fprintf(out, "incorrect\n");
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
    int vars_ap[26] = { 0 };
    int vars_val[26] = { 0 };
    const char* p = args;
    while (*p) {
        while (isspace(*p)) p++;
        if (*p == '\0') break;
        if (!islower(*p)) { print_error(out); return; }
        char name = *p;
        p++;
        
        while (isspace(*p)) p++;
        if (*p != '=') { print_error(out); return; }
        p++;
        
        while (isspace(*p)) p++;
        int sign = 1;
        if (*p == '-') { sign = -1; p++; }
        else if (*p == '+') p++;
        
        if (!isdigit(*p)) { print_error(out); return; }
        long long val = 0;
        while (isdigit(*p)) {
            val = val * 10 + (*p - '0');
            if (val > INT_MAX) { print_error(out); return; }
            p++;
        }
        
        val *= sign;
        int idx = name - 'a';
        if (vars_ap[idx]) {
            print_error(out);
            return;
        }
        
        vars_ap[idx] = 1;
        vars_val[idx] = (int)val;
        
        while (isspace(*p)) p++;
        if (*p == ',') {
            p++;
            continue;
        }
        else if (*p == '\0') {
            break;
        } else {
            print_error(out);
            return;
        }
    }
    
    int error = 0;
    long long res = eval_tree(root, vars_ap, vars_val, &error);
    if (error) {
        print_error(out);
    }
    else {
        fprintf(out, "%lld\n", res);
    }
}

char* trim(char* s) {
    while (*s == ' ')
        s++;

    char* end = s + strlen(s) - 1;
    while (end > s && *end == ' ') {
        *end = '\0';
        end--;
    }

    return s;
}

char* read_dynamic_line(FILE* input, size_t* line_length) {
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char* buffer = (char*)my_malloc(buffer_size);
    if (!buffer) {
        return NULL;
    }

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

        if (len > 0 && buffer[pos - 1] == '\n') {
            break;
        }

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
        if (!line_copy) continue;
        strcpy(line_copy, line);

        char* cmd = strtok(line_copy, " \t");
        if (!cmd) {
            my_free(line_copy);
            print_incorrect(output);
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
            if (*args != '\0') {
                print_incorrect(output);
            }
            else {
                handle_save_prf(output);
            }
            handled = 1;
        }
        else if (strcmp(cmd, "save_pst") == 0) {
            if (*args != '\0') {
                print_incorrect(output);
            }
            else {
                handle_save_pst(output);
            }
            handled = 1;
        }
        else if (strcmp(cmd, "eval") == 0) {
            if (*args == '\0') {
                print_incorrect(output);
            }
            else {
                handle_eval(args, output);
            }
            handled = 1;
        }

        if (!handled) {
            print_incorrect(output);
        }
       
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
