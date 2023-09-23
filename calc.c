#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define SIZE 256

char *shift(int *argc, char ***argv) {
	if (*argc <= 0) {
		// fputs("[ERROR]: not enough arguments provided\n", stderr);
		return NULL;
	}
	char *result = **argv;
	(*argc) -= 1;
	(*argv) += 1;
	return result;
}

typedef enum {
	FLOAT= -2,
	INT = -1,
	UNKNOWN = 0,
	//
	PLUS = '+',
	MINUS = '-',
	MUL = '*',
	DIV = '/',
	// POWER = '^',
	// SQRT = '',
	// SUM = '',
	// PROD = '',
} TOKEN_NAME;

typedef struct {
	TOKEN_NAME t;
	char *v;
} Token;

void to_string(Token *token) {
	switch (token->t) {
		case INT:
			printf("INT(%s)\n",token->v);
			break;
		case FLOAT:
			printf("FLOAT(%s)\n",token->v);
			break;
		default:
			printf("SYMBOL(%s)\n", token->v);
	}
}

void printer(Token **tokens, size_t n) {
	for (int i=0;i<n;i++) {
		to_string(tokens[i]);
	}
}

void freemy(Token **tokens, size_t n) {
	for (int i=0; i<n; ++i) {
		free(tokens[i]->v);
		free(tokens[i]);
	}
	free(tokens);
}

Token **lex(char *buf, size_t size, size_t *token_count) {
	if (size == 0) {
		return 0;
	}

	Token **tokens = calloc(size, sizeof(Token *));
	int str_size = SIZE;
	int tokens_size = 0;
	for (int i=0; i < size; ++i) {
		Token *new = calloc(1, sizeof(Token));
		new->t = UNKNOWN;
		char *val = calloc(str_size, sizeof(char));
		if (buf[i] >= '0' && buf[i] <= '9') {
			char b_i;
			new->t = INT;
			int dot_count = 0;
			int j = 0;
			do {
				if (buf[i] == '.') {
					char b_ip1;
					if (i+1 < size && !((b_ip1 = buf[i+1]) >= '0' && b_ip1 <= '9')) {
						break;
					}
					if (!dot_count) {
						new->t = FLOAT;
					} else {
						new->t = UNKNOWN;
					}
					++dot_count;
				}
				val[j] = buf[i];
				++i;
				++j;
			} while (((b_i = buf[i]) >= '0' && b_i <= '9' || b_i == '.') && i < size);
			--i;
			val = realloc(val, j);			
		} else {
			new->t = buf[i];
			val[0] = buf[i];
			val = realloc(val, 1);
		}
		new->v = val;
		tokens[tokens_size] = new;
		++tokens_size;
	}
	(*token_count) = tokens_size;
	// printer(tokens, tokens_size);
	// freemy(tokens, tokens_size);
	return tokens;
}

Token **s_isfifo(size_t *token_count) { // piped in
	char *buf = calloc(SIZE, sizeof(char));
	char c;
	size_t i = 0;
	size_t size = SIZE;
	while ((c = fgetc(stdin)) != EOF) {
		buf[i] = c;
		++i;
		if (i >= size) {
			size *= 2;
			buf = realloc(buf, size);
		}
	}
	Token **tokens = lex(buf, i, token_count);
	free(buf);
	return tokens;
}

void my_memset(char *buf, int c, size_t n) {
	for (int i=0; i<n; ++i) {
		buf[i] = c;
	}
}

Token **s_ischr(size_t *token_count) { // REPL
	char *buf = calloc(SIZE, sizeof(char));
	size_t size = SIZE;
	char c;
	
	size_t i = 0;
	while ((c = fgetc(stdin)) != '\n' && c != EOF) {
		buf[i] = c;
		++i;
		if (i >= size) {
			size *= 2;
			buf = realloc(buf, size);
		}
	}
	Token **tokens = lex(buf, i, token_count);
	free(buf);
	return tokens;
}

Token **s_isreg(size_t *token_count) { // file directed as stdin, eg ./a.out < file
	fseek(stdin, 0, SEEK_END);
	int size = ftell(stdin);
	rewind(stdin);
	char *buf = calloc(size, sizeof(char));
	fread(buf, sizeof(char), size, stdin);
	Token **tokens = lex(buf, size, token_count);
	free(buf);
	return tokens;
}

Token **w_args(int argc, char **argv, size_t *token_count) {
	char *buf = calloc(SIZE, sizeof(char));
	char *arg;
	size_t i = 0;
	size_t size = SIZE;
	while (arg = shift(&argc, &argv)) {
		while ( *arg != '\0') {
			buf[i] = *arg;
			++i;
			arg += 1;
			if (i+1 >= size) {
				size *= 2;
				buf = realloc(buf, size);
			}
			buf[i] = ' ';
			++i;
		}		
	}
	Token **tokens = lex(buf, i, token_count);
	free(buf);
	return tokens;
}

int main(int argc, char **argv) {
	char *prog_name = shift(&argc, &argv); // shift program name
	Token **tokens;
	size_t *token_count = calloc(1, sizeof(size_t *));
	if (argc > 0) {
		tokens = w_args(argc, argv, token_count);
	} else {
		struct stat stats;
		fstat(fileno(stdin), &stats);
		int stats_mode = stats.st_mode;
		if (S_ISFIFO(stats_mode)) {
			tokens = s_isfifo(token_count);
		} else if (S_ISCHR(stats_mode)) {
			while (1) {
				tokens  = s_ischr(token_count);
			}		
		} else if (S_ISREG(stats_mode)) {
			tokens = s_isreg(token_count);
		} else {
			fputs("[ERROR]: unsupported filetype\n", stderr);
			return 1;
		}
	}

	printer(tokens, *token_count);
	freemy(tokens, *token_count);
	free(token_count);
	return 0;
}