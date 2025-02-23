// Copyright 2025 Mitchell Xu <mitchell@sdf.org>

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char vars[10000];
int counter = 0;

typedef enum {
    OP_AND = '&',
    OP_OR = '|',
    OP_NOT = '~',
    OP_XOR = '^',
    OP_IMPLY = '>',
    OP_EQUAL = '=',
    OP_LPAR = '(',
    OP_RPAR = ')',
    OP_END = '\0',
    OP_VAR,
} Op;

typedef struct Token {
    Op op;
    char var; // Variable name
} Token;

Token tokens[10000];
int pos = 0;

typedef struct ASTNode {
    Token token;
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

ASTNode *mknode(int op, ASTNode *left, ASTNode *right, char name)
{
    ASTNode *n = malloc(sizeof(ASTNode));
    if (n == NULL) {
	fprintf(stderr, "Unable to malloc in mknode()\n");
	exit(1);
    }

    n->token.op = op;
    n->left = left;
    n->right = right;
    n->token.var = name;

    return n;
}

ASTNode *mkleaf(int op, char name)
{
    return mknode(op, NULL, NULL, name);
}

ASTNode *mkunary(int op, ASTNode *left, char name)
{
    return mknode(op, left, NULL, name);
}

Token *tokenize(char *str)
{
    Token *t = malloc((strlen(str)+2) * sizeof(Token));
    if (t == NULL) {
	fprintf(stderr, "Unable to malloc in tokenize()\n");
	exit(1);
    }
    int pos = 0;
    while (*str) {
	switch (*str) {
	case ' ': break;
	case '&': t[pos++] = (Token){OP_AND, 0}; break;
	case '|': t[pos++] = (Token){OP_OR, 0}; break;
	case '^': t[pos++] = (Token){OP_XOR, 0}; break;
	case '~': t[pos++] = (Token){OP_NOT, 0}; break;
	case '(': t[pos++] = (Token){OP_LPAR, 0}; break;
	case ')': t[pos++] = (Token){OP_RPAR, 0}; break;
	case '>': t[pos++] = (Token){OP_IMPLY, 0}; break;
	case '=': t[pos++] = (Token){OP_EQUAL, 0}; break;
	default:
	    if (isalpha(*str)) {
		t[pos++] = (Token){OP_VAR, *str};
	    } else {
		fprintf(stderr, "Illegal character %c, expect a Token\n", *str);
		exit(1);
	    }
	}
	str++;
    }
    t[pos++] = (Token){OP_END, 0};
    return t;
}

ASTNode *parexp(int pre);
ASTNode *parse();

int prec(Op op) {
    switch (op) {
    case OP_NOT: return 6;
    case OP_AND: return 5;
    case OP_XOR: return 4;
    case OP_OR:  return 3;
    case OP_IMPLY:  return 2;
    case OP_EQUAL: return 1;
    case OP_LPAR: return -1;
    default: return -1;
    }
}

ASTNode *parexp(int m_prec) {
    ASTNode *left = parse();

    while (1) {
	Token t = tokens[pos];
	int pre = prec(t.op);
	if (pre < m_prec) break;
	pos++;
	ASTNode *right = parexp(pre + 1);
	left = mknode(t.op, left, right, 0);
    }
    return left;
}

ASTNode *parse()
{
    Token t = tokens[pos];
    switch (t.op) {
    case OP_VAR: {
	pos++;
	return mkleaf(t.op, t.var);
    }
    case OP_NOT: {
	pos++;
	return mkunary(t.op, parse(), 0);
    }
    case OP_LPAR: {
	pos++;
	ASTNode *expr = parexp(0);
	if (tokens[pos].op != OP_RPAR) {
	    fprintf(stderr, "Mismatched Parentheses at %d\n", pos);
	    exit(1);
	}
	pos++;
	return expr;
    }
    default:
	fprintf(stderr, "Expect an expression\n");
	exit(1);
    }
}

void collect(ASTNode *node, char *vars, int *count)
{
    if (!node) return;
    if (!node->left && !node->right) {
	int exist = 0;
	for (int i = 0; i < *count; i++) {
	    if (vars[i] == node->token.var) exist = 1;
	}
	if (!exist) vars[(*count)++] = node->token.var;
    }
    collect(node->left, vars, count);
    collect(node->right, vars, count);
}

int evaluate(ASTNode *node, int *values, char *vars, int count)
{
    if (!node->left && !node->right) {
	for (int i = 0; i < count; i++) {
	    if (vars[i] == node->token.var) return values[i];
	}
	return -1;
    }

    int left = node->left ? evaluate(node->left, values, vars, count) : 0;
    int right = node->right ? evaluate(node->right, values, vars, count) : 0;

    switch (node->token.op) {
    case OP_AND: return left && right;
    case OP_OR: return left || right;
    case OP_NOT: return !left;
    case OP_XOR: return left ^ right;
    case OP_IMPLY: return !left || right;
    case OP_EQUAL: return left == right;
    default: return -1;
    }
}

void print_table(char *str, ASTNode *expr)
{
    printf("R = %s\n", str);
    for (int m = 0; m < counter; m++) {
	printf("| %c ", vars[m]);
    }
    printf("| R |\n");
    for (int i = 0; i < (1 << counter); i++) {
	int values[counter];

	for (int j = 0; j < counter; j++) {
	    values[j] = (i >> (counter - 1 - j)) & 1;
	    printf("| %d ", values[j]);
	}

	int result = evaluate(expr, values, vars, counter);
	printf("| %d |\n", result);
    }
}

void print_ast(ASTNode *node, int depth, int left)
{
    if (!node) return;
    print_ast(node->right, depth+1, 0);

    for (int i = 0; i < depth - 1; i++)
	printf("│   ");
    if (depth > 0)
	printf("%s---", left ? "└" : "├");

    switch (node->token.op) {
	case OP_VAR:
	    printf("VAR('%c')\n", node->token.var);
	    break;
	case OP_AND:
	    printf("AND(&)\n");
	    break;
	case OP_OR:
	    printf("OR(|)\n");
	    break;
	case OP_XOR:
	    printf("XOR(^)\n");
	    break;
	case OP_NOT:
	    printf("NOT(~)\n");
	    break;
	case OP_IMPLY:
	    printf("IMPLY(>)\n");
	    break;
	case OP_EQUAL:
	    printf("EQUAL(=)\n");
	    break;
	default:
	    printf("UNKNOWN\n");
    }

    print_ast(node->left, depth + 1, 1);
}

int main()
{
    char str[9999];
    printf("Input the Boolean Expression: ");
    scanf("%s", str);
    Token *tokenized = tokenize(str);
    memcpy(tokens, tokenized, sizeof(Token) * (strlen(str) + 2));
    free(tokenized);
    ASTNode *expr = parexp(0);
    collect(expr, vars, &counter);
    print_table(str, expr);
    printf("\n");
    print_ast(expr, 0, 0);
    printf("\n");
    return 0;
}
