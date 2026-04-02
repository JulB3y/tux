#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/module.h"
#include "../../include/types.h"
#include "calc.h"

typedef enum {
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_FUNCTION,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    double number;
    char op;
    char function[10];
} Token;

typedef struct {
    Token *data;
    int size;
    int capacity;
} TokenStack;

typedef struct {
    double *data;
    int size;
    int capacity;
} DoubleStack;

static void initTokenStack(TokenStack *stack, int capacity) {
    stack->data = malloc((size_t)capacity * sizeof(Token));
    stack->size = 0;
    stack->capacity = capacity;
}

static void freeTokenStack(TokenStack *stack) {
    free(stack->data);
    stack->size = 0;
    stack->capacity = 0;
}

static int pushToken(TokenStack *stack, Token token) {
    if (stack->size >= stack->capacity) {
        int new_capacity = stack->capacity * 2;
        Token *new_data = realloc(stack->data, (size_t)new_capacity * sizeof(Token));
        if (!new_data)
            return 0;
        stack->data = new_data;
        stack->capacity = new_capacity;
    }
    stack->data[stack->size++] = token;
    return 1;
}

static Token popToken(TokenStack *stack) {
    if (stack->size <= 0)
        return (Token){.type = TOKEN_ERROR};
    return stack->data[--stack->size];
}

static Token peekToken(const TokenStack *stack) {
    if (stack->size <= 0)
        return (Token){.type = TOKEN_ERROR};
    return stack->data[stack->size - 1];
}

static void initDoubleStack(DoubleStack *stack, int capacity) {
    stack->data = malloc((size_t)capacity * sizeof(double));
    stack->size = 0;
    stack->capacity = capacity;
}

static void freeDoubleStack(DoubleStack *stack) {
    free(stack->data);
    stack->size = 0;
    stack->capacity = 0;
}

static int pushDouble(DoubleStack *stack, double value) {
    if (stack->size >= stack->capacity) {
        int new_capacity = stack->capacity * 2;
        double *new_data = realloc(stack->data, (size_t)new_capacity * sizeof(double));
        if (!new_data)
            return 0;
        stack->data = new_data;
        stack->capacity = new_capacity;
    }
    stack->data[stack->size++] = value;
    return 1;
}

static double popDouble(DoubleStack *stack) {
    if (stack->size <= 0)
        return NAN;
    return stack->data[--stack->size];
}

static const char *calc_input;
static int calc_pos;

static int isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^';
}

static int isDigitOrDot(char c) {
    return isdigit((unsigned char)c) || c == '.';
}

static Token nextToken(void) {
    while (isspace((unsigned char)calc_input[calc_pos]))
        calc_pos++;

    if (calc_input[calc_pos] == '\0')
        return (Token){.type = TOKEN_EOF};

    if (isDigitOrDot(calc_input[calc_pos]) ||
        (calc_input[calc_pos] == '-' && isDigitOrDot(calc_input[calc_pos + 1]))) {
        char *endptr;
        double number = strtod(calc_input + calc_pos, &endptr);
        calc_pos += (int)(endptr - (calc_input + calc_pos));
        return (Token){.type = TOKEN_NUMBER, .number = number};
    }

    if (isalpha(calc_input[calc_pos])) {
        int start = calc_pos;
        while (isalpha(calc_input[calc_pos]) && calc_pos - start < 9)
            calc_pos++;
        int len = calc_pos - start;
        Token token = {.type = TOKEN_FUNCTION};
        strncpy(token.function, calc_input + start, (size_t)len);
        token.function[len] = '\0';
        return token;
    }

    if (isOperator(calc_input[calc_pos]))
        return (Token){.type = TOKEN_OPERATOR, .op = calc_input[calc_pos++]};

    if (calc_input[calc_pos] == '(') {
        calc_pos++;
        return (Token){.type = TOKEN_LPAREN};
    }

    if (calc_input[calc_pos] == ')') {
        calc_pos++;
        return (Token){.type = TOKEN_RPAREN};
    }

    return (Token){.type = TOKEN_ERROR};
}

static int getPrecedence(char op) {
    switch (op) {
        case '^': return 4;
        case '*':
        case '/':
        case '%': return 3;
        case '+':
        case '-': return 2;
        default: return 0;
    }
}

static int isRightAssociative(char op) {
    return op == '^';
}

static int needsImplicitMultiplication(TokenType prev, TokenType current) {
    if (prev == TOKEN_ERROR || current == TOKEN_ERROR)
        return 0;
    if (prev == TOKEN_NUMBER && current == TOKEN_LPAREN)
        return 1;
    if (prev == TOKEN_RPAREN && current == TOKEN_NUMBER)
        return 1;
    if (prev == TOKEN_RPAREN && current == TOKEN_LPAREN)
        return 1;
    if (prev == TOKEN_RPAREN && current == TOKEN_FUNCTION)
        return 1;
    return 0;
}

static TokenStack *shuntingYard(void) {
    TokenStack *output = malloc(sizeof(TokenStack));
    initTokenStack(output, 32);

    TokenStack *operators = malloc(sizeof(TokenStack));
    initTokenStack(operators, 32);

    TokenType prev_type = TOKEN_ERROR;

    Token token;
    while ((token = nextToken()).type != TOKEN_EOF) {
        if (token.type == TOKEN_ERROR) {
            freeTokenStack(output);
            freeTokenStack(operators);
            free(output);
            free(operators);
            return NULL;
        }

        if (needsImplicitMultiplication(prev_type, token.type)) {
            Token mult_token = {.type = TOKEN_OPERATOR, .op = '*'};
            while (operators->size > 0) {
                Token top = peekToken(operators);
                if (top.type == TOKEN_OPERATOR &&
                    ((isRightAssociative(mult_token.op) && getPrecedence(mult_token.op) < getPrecedence(top.op)) ||
                     (!isRightAssociative(mult_token.op) && getPrecedence(mult_token.op) <= getPrecedence(top.op)))) {
                    pushToken(output, popToken(operators));
                } else {
                    break;
                }
            }
            pushToken(operators, mult_token);
        }

        if (token.type == TOKEN_NUMBER) {
            pushToken(output, token);
        } else if (token.type == TOKEN_FUNCTION) {
            pushToken(operators, token);
        } else if (token.type == TOKEN_OPERATOR) {
            while (operators->size > 0) {
                Token top = peekToken(operators);
                if (top.type == TOKEN_OPERATOR &&
                    ((isRightAssociative(token.op) && getPrecedence(token.op) < getPrecedence(top.op)) ||
                     (!isRightAssociative(token.op) && getPrecedence(token.op) <= getPrecedence(top.op)))) {
                    pushToken(output, popToken(operators));
                } else {
                    break;
                }
            }
            pushToken(operators, token);
        } else if (token.type == TOKEN_LPAREN) {
            pushToken(operators, token);
        } else if (token.type == TOKEN_RPAREN) {
            while (operators->size > 0 && peekToken(operators).type != TOKEN_LPAREN)
                pushToken(output, popToken(operators));
            if (operators->size > 0 && peekToken(operators).type == TOKEN_LPAREN)
                popToken(operators);
            if (operators->size > 0 && peekToken(operators).type == TOKEN_FUNCTION)
                pushToken(output, popToken(operators));
        }

        prev_type = token.type;
    }

    while (operators->size > 0)
        pushToken(output, popToken(operators));

    freeTokenStack(operators);
    free(operators);
    return output;
}

static double evaluatePostfix(TokenStack *postfix) {
    DoubleStack stack;
    initDoubleStack(&stack, 32);

    for (int i = 0; i < postfix->size; i++) {
        Token token = postfix->data[i];

        if (token.type == TOKEN_NUMBER) {
            pushDouble(&stack, token.number);
        } else if (token.type == TOKEN_OPERATOR) {
            if (stack.size < 2) {
                freeDoubleStack(&stack);
                return NAN;
            }
            double b = popDouble(&stack);
            double a = popDouble(&stack);
            double result;

            switch (token.op) {
                case '+': result = a + b; break;
                case '-': result = a - b; break;
                case '*': result = a * b; break;
                case '/':
                    if (fabs(b) < 1e-10) {
                        freeDoubleStack(&stack);
                        return NAN;
                    }
                    result = a / b;
                    break;
                case '%':
                    if (fabs(b) < 1e-10) {
                        freeDoubleStack(&stack);
                        return NAN;
                    }
                    result = fmod(a, b);
                    break;
                case '^': result = pow(a, b); break;
                default:
                    freeDoubleStack(&stack);
                    return NAN;
            }
            pushDouble(&stack, result);
        } else if (token.type == TOKEN_FUNCTION) {
            if (stack.size < 1) {
                freeDoubleStack(&stack);
                return NAN;
            }
            double a = popDouble(&stack);
            double result;

            if (strcmp(token.function, "sqrt") == 0) {
                result = sqrt(a);
            } else if (strcmp(token.function, "sin") == 0) {
                result = sin(a);
            } else if (strcmp(token.function, "cos") == 0) {
                result = cos(a);
            } else if (strcmp(token.function, "tan") == 0) {
                result = tan(a);
            } else if (strcmp(token.function, "log") == 0) {
                result = log(a);
            } else if (strcmp(token.function, "exp") == 0) {
                result = exp(a);
            } else {
                freeDoubleStack(&stack);
                return NAN;
            }
            pushDouble(&stack, result);
        }
    }

    double result = NAN;
    if (stack.size == 1)
        result = stack.data[0];

    freeDoubleStack(&stack);
    return result;
}

static const char *calc_operators = "+-*/%^";
static const char *calc_functions[] = {"sqrt", "sin", "cos", "tan", "log", "exp"};
static const size_t calc_function_count = sizeof(calc_functions) / sizeof(calc_functions[0]);

static int calc_match(const char *query) {
    if (!query || query[0] == '\0')
        return 0;

    if (strpbrk(query, calc_operators) != NULL)
        return 1;

    for (size_t i = 0; i < calc_function_count; i++) {
        if (strstr(query, calc_functions[i]) != NULL)
            return 1;
    }

    if (strchr(query, '(') != NULL || strchr(query, ')') != NULL)
        return 1;

    return 0;
}

static int calc_search(const char *query, Result *results, int max) {
    (void)max;
    if (!query || !results)
        return 0;

    calc_input = query;
    calc_pos = 0;

    TokenStack *postfix = shuntingYard();
    if (!postfix)
        return 0;

    double result = evaluatePostfix(postfix);
    freeTokenStack(postfix);
    free(postfix);

    if (isnan(result))
        return 0;

    results[0].type = RESULT_CALC;
    snprintf(results[0].title, sizeof(results[0].title), "%.10g", result);
    results[0].subtitle[0] = '\0';
    results[0].score = 1000;
    results[0].payload = NULL;
    results[0].flags = 0;

    return 1;
}

static void calc_execute(Result *result) {
    (void)result;
}

static void calc_destroy(Module *module) {
    (void)module;
}

Module *calc_module_create(void) {
    Module *module = malloc(sizeof(Module));
    if (!module)
        return NULL;

    memset(module, 0, sizeof(Module));
    module->name = "calc";
    module->match = calc_match;
    module->search = calc_search;
    module->execute = calc_execute;
    module->destroy = calc_destroy;

    return module;
}