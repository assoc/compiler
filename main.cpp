#include <conio.h>
#include <stdio.h>
#include <stack>
#include <string>
#include <vector>
#define CLASSES 15

enum lclass { // need better coding for faster and simpler if-sections
  UNDEF, // 0000
  DECL,  // 0001
  IDENT, // 0010
  CONST, // 0011
  NEWL,  // 0100
  PLUS,  // 0101
  MINUS, // 0110
  TIMES, // 0111
  SLASH, // 1000
  UNARY, // 1001
  EQUAL, // 1010
  COMMA, // 1011
  END,   // 1100
  L_BR,  // 1101
  R_BR}; // 1110
std::string types[CLASSES] = {"UNDEF", "DECL", "IDENT", "CONST", "NEWL", "PLUS", "MINUS", "TIMES", "SLASH", "UNARY", "EQUAL", "COMMA", "END", "L_BR", "R_BR"};

char* allocate(char* data, size_t len) {
  char *ptr = new char[len+1];
  if (ptr) {
    for (size_t i = 0; i < len; i++) {ptr[i] = data[i];}
    ptr[len] = 0;
    return ptr;
  }
  return 0;
}

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

void add(lclass type, char* name = 0, size_t len = 0) {
  token temp;
  temp.line = actual_line;
  temp.code = type;
  temp.data = 0;
  if (name) {temp.data = allocate(name, len);}
  tokens.push_back(temp);
}

bool is_alpha(char c) {return (c >= 'a' && c <= 'z') ? 1 : 0;}
bool is_digit(char c) {return (c >= '0' && c <= '9') ? 1 : 0;}
bool is_decl(char *s) {return (*s == 'V' && *(s+1) == 'a' && *(s+2) == 'r') ? 1 : 0;}
bool is_space(char c) {return (c == ' ' || c == '\t') ? 1 : 0;}
void skip_space(char *s) {while (is_space(*s) && *s != 0) s++;}

char* get_id(char *s) { ////
  char buffer[32] = {0};
  char shift = 0;
  while (is_alpha(*s)) {
    buffer[shift++] = *s; // check for owerflow
    s++;
  }
  printf("\n found IDENT: %s", buffer);
  add(IDENT, &buffer[0], shift);
  skip_space(s);
  return s;
}
char* get_num(char *s) { ////
  char buffer[32] = {0};
  char shift = 0;
  while (is_digit(*s)) {
    buffer[shift++] = *s; // check for owerflow
    s++;
  }
  printf("\n found CONST: %s", buffer);
  add(CONST, &buffer[0], shift);
  skip_space(s);
  return s;
}

void split(char *b) {
  while(*b != '\n' && *b != NULL) {
    if (is_space(*b)) {skip_space(b);}
    else if (*b == 'V' && is_decl(b)) {
      printf("\n found DECL");
      add(DECL);
      b += 3;
      continue;
    } 
    else if (is_alpha(*b)) {b = get_id(b); continue;}
    else if (is_digit(*b)) {b = get_num(b); continue;}
    else if (*b == '+') {printf("\n found PLUS: %c", *b); add(PLUS);}
    else if (*b == '-') {printf("\n found MINUS: %c", *b); add(MINUS);}
    else if (*b == '*') {printf("\n found TIMES: %c", *b); add(TIMES);}
    else if (*b == '/') {printf("\n found SLASH: %c", *b); add(SLASH);}
    else if (*b == '=') {printf("\n found EQUAL: %c", *b); add(EQUAL);}
    else if (*b == '(') {printf("\n found L_BR: %c", *b); add(L_BR);}
    else if (*b == ')') {printf("\n found R_BR: %c", *b); add(R_BR);}
    else if (*b == ',') {printf("\n found COMMA: %c", *b); add(COMMA);}
    else if (*b == '.') {printf("\n found END: %c", *b); add(END);}
    else {add(UNDEF);}
    b++;
  }
  add(NEWL);
  return;
}
/*  program = declaration operation "."
    operation = statement_list
    declaration = "Var" variables
    variables = ident | ident , variables
    statement_list = statement | statement statement_list
    statement = ident "=" expression
    expression = unary sub_expression | sub_expression
    sub_expression = "(" expression ")" | term | sub_expression ("-" | "+" | "*" | "/") sub_expression
    unary = "-" char id_buffer[32] = {0};
    term = ident | constant
    ident = [a-z] ident | [a-z]
    constant = [0-9] constant | [0-9]
    one line can contain only one declaration or one statement
*/
void unexpected() {
  printf("\n unexpected symbol at line %d (%s)", actual_line, types[symbol].c_str());
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
  unexpected();
  return 0;
}

void variables() {
  if (expect(IDENT)) {
    // add to vartable
    ds.push_back(tokens[iterator-2].data); ////
    if (equal(COMMA)) {variables();}
  } else {unexpected();}
}

void declaration() {
  if (expect(DECL)) {variables();} else {unexpected();}
  if (!expect(NEWL)) {unexpected();}
}
void unary() {////
  tokens[iterator-1].code = UNARY;
}

bool lookup() {
  for (int i = 0; i < ds.size(); i++) {
    if (!strcmp(tokens[iterator-2].data, ds[i])) return 1;
  }
  return 0;
}

void expression();
void factor() {
  if (symbol == MINUS) {unary(); next_symbol();}
  if (equal(IDENT)) {
    // check vartable
    if (!lookup()) {
      printf("\n undeclared variable: %s", tokens[iterator-2].data);
      errors++;
    }
  } else if (equal(CONST)) {
  } else if (equal(L_BR)) {
    expression();
    expect(R_BR);
  } else {
    next_symbol();
    unexpected();
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

int opstart;

int syntax() {
  errors = iterator = 0;
  next_symbol();
  declaration();
  opstart = iterator - 1;
  operations();
  expect(END);
  if (!errors) {printf("\n Syntax check passed, no errors\n");}
  else {printf("\n Syntax check passed, errors found: %d\n", errors);}
  return !errors;
}

std::stack<token> operators; ////
std::vector<token> output; ////

int is_higher(lclass a, lclass b) { //// BAD !!
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

void use_stack() { ////
  while (symbol != NEWL) {
    if (symbol == CONST || symbol == IDENT) {
      output.push_back(tokens[iterator-1]);
    } else if (symbol == PLUS || symbol == MINUS || symbol == TIMES || symbol == SLASH|| symbol == UNARY) {
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
      operators.pop(); // stack underflow check!
    }
    next_symbol();
  }
  while (!operators.empty()) {
    output.push_back(operators.top());
    operators.pop();
  }
  for (int i = 0; i < output.size(); i++) {
    if (output[i].code == IDENT || output[i].code == CONST) {printf("%s ", output[i].data);}
    else if (output[i].code == PLUS) {printf("+ ");}
    else if (output[i].code == MINUS) {printf("- ");}
    else if (output[i].code == TIMES) {printf("* ");}
    else if (output[i].code == SLASH) {printf("/ ");}
    else if (output[i].code == UNARY) {printf("~ ");}
  }
  output.clear();
}

void postfix() {
  iterator = opstart;
  next_symbol();
  do {    
    printf("\n %s = ", tokens[iterator-1].data);
    next_symbol(); // expect(IDENT);
    next_symbol(); // expect(EQUAL);
    use_stack();
    next_symbol();
  } while(symbol != NEWL && symbol != END && symbol != UNDEF);
}

void main() {
  char buf[256] = "in";
  FILE *io;
  //printf("Enter filename: ");
  //gets(buf);
  //if (strlen(buf) == 0) {return;}
  if (io = fopen(buf, "r")) {
    actual_line = 0;
    while(!feof(io)) {
      fgets(buf, sizeof(buf), io);
      actual_line++;
      split(&buf[0]);
      printf("\n ----- -----");
    }
    fclose(io);
    
    if (syntax()) postfix();
    printf("\n\n DONE");
  } else {
    printf("\n error: can't open input file");
  }
  getch();
}