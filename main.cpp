#include <conio.h>
#include <stdio.h>
#include <stack>
#include <string>
#include <vector>
#define CLASSES 15

enum lclass {UNDEF, EQUAL, DECL, COMMA, IDENT, CONST, PLUS, MINUS, TIMES, SLASH, L_BR, R_BR, NEWL, END, UNARY};
char types[CLASSES][6] = {"UNDEF", "EQUAL", "DECL\0", "COMMA", "IDENT", "CONST", "PLUS\0", "MINUS", "TIMES", "SLASH", "L_BR\0", "R_BR\0", "NEWL\0", "END\0\0", "UNARY"};

typedef struct token {
  lclass code;
  unsigned line;
  char *data;
} token;

std::vector<token> tokens;
std::vector<char*> ds; // declared variables
unsigned actual_line; ////
lclass symbol; ////
int errors, iterator; ////
int opstart;
std::stack<token> operators; ////
std::vector<token> output; ////

char* allocate(char* data, size_t len) {
  char *ptr = new char[len+1];
  if (ptr) {
    for (size_t i = 0; i < len; i++) {ptr[i] = data[i];}
    ptr[len] = 0;
    return ptr;
  }
  return 0;
}

void add(lclass type, char* name = 0, size_t len = 0) {
  token temp = {type, actual_line, 0}; //// danger?
  if (type == IDENT || type == CONST) {temp.data = allocate(name, len);}
  tokens.push_back(temp);
}

bool is_alpha(char c) {return (c >= 'a' && c <= 'z') ? 1 : 0;}
bool is_digit(char c) {return (c >= '0' && c <= '9') ? 1 : 0;}
bool is_space(char c) {return (c == ' ' || c == '\t') ? 1 : 0;}
void skip_space(char *s) {while (is_space(*s) && *s != 0) s++;}

void lexic(FILE *io) {
  char b = fgetc(io);
  actual_line = 0;
  do {
    if (is_space(b)) {b = fgetc(io); continue;}
    else if (b == 'V') {
      char buffer[32] = {0}, shift = 1;
      char example[4] = "Var";
      buffer[0] = 'V';
      while (is_alpha(b = fgetc(io)) && shift < 32) {buffer[shift++] = b;}
      if (strcmp(&example[0], &buffer[0])) {
        printf("\n found UNDEF: %s", buffer);
        add(UNDEF, &buffer[0], shift);
      } else {
	      printf("\n found DECL");
        add(DECL);
      }
      continue;
    } 
    else if (is_alpha(b)) {
      char buffer[32] = {0}, shift = 1;
      buffer[0] = b;
      while (is_alpha(b = fgetc(io)) && shift < 32) {buffer[shift++] = b;}
      printf("\n found IDENT: %s", buffer);
      add(IDENT, &buffer[0], shift);
      continue;
    }
    else if (is_digit(b)) {
      char buffer[32] = {0}, shift = 1;
      buffer[0] = b;
      while (is_digit(b = fgetc(io)) && shift < 32) {buffer[shift++] = b;}
      printf("\n found CONST: %s", buffer);
      add(CONST, &buffer[0], shift);
      continue;
    }
    else if (b == '+') {printf("\n found PLUS: %c", b); add(PLUS);}
    else if (b == '-') {printf("\n found MINUS: %c", b); add(MINUS);}
    else if (b == '*') {printf("\n found TIMES: %c", b); add(TIMES);}
    else if (b == '/') {printf("\n found SLASH: %c", b); add(SLASH);}
    else if (b == '=') {printf("\n found EQUAL: %c", b); add(EQUAL);}
    else if (b == '(') {printf("\n found L_BR: %c", b); add(L_BR);}
    else if (b == ')') {printf("\n found R_BR: %c", b); add(R_BR);}
    else if (b == ',') {printf("\n found COMMA: %c", b); add(COMMA);}
    else if (b == '\n') {printf("\n found NEWL"); add(NEWL); actual_line++;}
    else if (b == '.') {printf("\n found END: %c", b); add(END);}
    else {add(UNDEF);}
    b = fgetc(io);
  } while(b != EOF);
}

void unexpected(lclass s) {
  printf("\n syntax: unexpected symbol at line %d (%s)", actual_line, types[symbol]);
  errors++;
}

void next_symbol() {
  if (iterator < tokens.size()) {
    symbol = tokens[iterator].code;
    actual_line = tokens[iterator].line;
    iterator++;
  } else {symbol = UNDEF;}
}

int equal(lclass s) {
  if (symbol == s) {next_symbol(); return 1;}
  return 0;
}

int expect(lclass s) {
  if (equal(s)) return 1;
  unexpected(s);
  return 0;
}

bool lookup(char* d) {
  for (int i = 0; i < ds.size(); i++) {
    if (!strcmp(d, ds[i])) return 1;
  }
  return 0;
}

void variables() {
  if (expect(IDENT)) {
	  if (!lookup(tokens[iterator-2].data)) {ds.push_back(tokens[iterator-2].data);} //// add to vartable
	  else {errors++; printf("\n syntax: variable redefinition at line %d (%s)", actual_line, tokens[iterator-2].data);}
    if (equal(COMMA)) {variables();}
  } else {unexpected(IDENT);}
}

int declaration() {
  if (expect(DECL)) {variables();}
  if (!expect(NEWL)) {unexpected(NEWL);}
  return !errors;
}

void unary() {tokens[iterator-1].code = UNARY;} ////

void expression();

void factor() {
  if (symbol == MINUS) {unary(); next_symbol();}
  if (equal(IDENT)) {
    if (!lookup(tokens[iterator-2].data)) {
      printf("\n syntax: undeclared variable: %s", tokens[iterator-2].data);
      errors++;
    }
  } else if (equal(CONST)) {
  } else if (equal(L_BR)) {
    expression();
    expect(R_BR);
  } else {
    next_symbol();
    unexpected(symbol);
  }
}

void term() {
  factor();
  while (symbol == TIMES || symbol == SLASH) {
    next_symbol();
    factor();
  }
}

void expression() {
  if (symbol == MINUS) {unary(); next_symbol();}
  term();
  while (symbol == PLUS || symbol == MINUS) {
    next_symbol();
    term();
  }
}

void operations() {
  do {
    expect(IDENT);
    expect(EQUAL);
    expression();
  } while(equal(NEWL));
}

int syntax() {
  errors = iterator = 0;
  next_symbol();
  if (!declaration()) return !errors; //// check UNDEF
  opstart = iterator - 1;
  operations();
  expect(END);
  //expect(NEWL);
  if (!errors && iterator != tokens.size()) {
    errors++;
    printf("\n syntax: expected end at line %d \n", actual_line-1);
  }
  if (!errors) {printf("\n Syntax check passed, no errors\n");}
  else {printf("\n Syntax check complete, errors found: %d\n", errors);}
  return !errors;
}

int is_higher(lclass a, lclass b) { //// BAD
  char res;
  if (a == PLUS || a == MINUS) {res = 1;}
  else if (a == TIMES || a == SLASH) {res = 2;}
  else if (a == L_BR) {res = 0;}
  else {res = 4;}
  if (b == PLUS || b == MINUS) {res -= 1;}
  else if (b == TIMES || b == SLASH) {res -= 2;}
  else if (b == L_BR) {res -= 0;}
  else {res -= 4;}
  return (res > 0);
}

void assembler();

void use_stack() { ////
  while (symbol != NEWL && symbol != END) {
    if (symbol == IDENT || symbol == CONST) {
      output.push_back(tokens[iterator-1]);
    } else if (symbol == PLUS || symbol == MINUS || symbol == TIMES || symbol == SLASH || symbol == UNARY) {
      if (!operators.empty()) {
        while (!operators.empty() && !is_higher(symbol, operators.top().code)) { // less priority
          output.push_back(operators.top()); // !
          operators.pop();
        }
      }
      operators.push(tokens[iterator-1]);
    } else if (symbol == L_BR) {
      operators.push(tokens[iterator-1]);
    } else if (symbol == R_BR) {
      while (operators.top().code != L_BR) {
        output.push_back(operators.top());
        operators.pop();
      }
      operators.pop(); ////
    }
    next_symbol();
  }
  while (!operators.empty()) {
    output.push_back(operators.top());
    operators.pop();
  }
  for (int i = 0; i < output.size(); i++) { // 2 to 0
    if (output[i].code == IDENT || output[i].code == CONST) {printf("%s ", output[i].data);}
    else if (output[i].code == PLUS) {printf("+ ");}
    else if (output[i].code == MINUS) {printf("- ");}
    else if (output[i].code == TIMES) {printf("* ");}
    else if (output[i].code == SLASH) {printf("/ ");}
    else if (output[i].code == UNARY) {printf("~ ");}
    else if (output[i].code == EQUAL) {printf("= ");}
  }
  assembler();
  output.clear();
}

void postfix() {
  iterator = opstart;
  next_symbol();
  do {
    printf("\n");
    output.push_back(tokens[iterator-1]);
    next_symbol();
    output.push_back(tokens[iterator-1]);
    next_symbol();
    use_stack();
    next_symbol();
  } while (symbol != NEWL && symbol != END && symbol != UNDEF);
}

void assembler() {
  for (int i = 2; i < output.size(); i++) {
    if (output[i].code == IDENT) {printf("\n LOAD %s ", output[i].data);}
    else if (output[i].code == CONST){printf("\n LIT %s ", output[i].data);}
    else if (output[i].code == PLUS) {printf("\n ADD ");}
    else if (output[i].code == MINUS) {printf("\n SUB ");}
    else if (output[i].code == TIMES) {printf("\n MUL ");}
    else if (output[i].code == SLASH) {printf("\n DIV ");}
    else if (output[i].code == UNARY) {printf("\n NOT \n LIT 1 \n ADD ");}
  }
  printf("\n STO %s \n", output[0].data);
}
void deallocate(){
  while (!tokens.empty()) {
    delete[] tokens[tokens.size()-1].data;
    tokens.pop_back();
  }
}
void main() {
  char buf[256] = "in";
  FILE *io;
  //printf("\n input: "); gets(buf);
  if (io = fopen(buf, "r")) {
    lexic(io);
    fclose(io);
    
    if (syntax()) postfix();
    deallocate();
    printf("\n\n DONE");
  } else {
    printf("\n error: can't open input file");
  }
  getch();
}