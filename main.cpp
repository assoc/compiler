#include <conio.h>
#include <stdio.h>
#include <stack>
#include <string>
#include <vector>
#define CLASSES 15

enum lclass {UNDEF, EQUAL, DECL, COMMA, IDENT, CONST, PLUS, MINUS, TIMES, SLASH, L_BR, R_BR, NEWL, END, UNARY};
char types[CLASSES][6] = {"UNDEF", "EQUAL", "DECL\0", "COMMA", "IDENT", "CONST", "PLUS\0", "MINUS", "TIMES", "SLASH", "L_BR\0", "R_BR\0", "NEWL\0", "END\0\0", "UNARY"};

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
  if (type == IDENT || type == CONST) {temp.data = allocate(name, len);}
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
    else if (*b == '+') {add(PLUS);}
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
void unexpected(lclass s) {
  printf("\n unexpected symbol at line %d (%s)", actual_line, types[symbol]);
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

void variables() {
  if (expect(IDENT)) {
    // add to vartable
    ds.push_back(tokens[iterator-2].data); ////
    if (equal(COMMA)) {variables();}
  } else {unexpected(IDENT);}
}

void declaration() {
  if (expect(DECL)) {variables();} else {unexpected();}
  if (!expect(NEWL)) {unexpected(NEWL);}
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

void assembler();

void use_stack() { ////
  while (symbol != NEWL) {
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
      operators.pop(); // stack underflow check!
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
    output.push_back(tokens[iterator-1]); next_symbol();
    output.push_back(tokens[iterator-1]); next_symbol();
    use_stack();
    next_symbol();
  } while (symbol != NEWL && symbol != END && symbol != UNDEF);
}

void assembler() {
/*  LIT const - push CONST to stack
    LOAD n - push IDENT 'n' to stack
    STO n - pop 'n'
    ADD, MUL - two top elements
    SUB - top minus second
    DIV - top divide by second
    NOT - logical not for top */
  for (int i = 2; i < output.size(); i++) {
    if (output[i].code == IDENT) {printf("\n LOAD %s ", output[i].data);}
    else if (output[i].code == CONST){printf("\n LIT %s ", output[i].data);}
    else if (output[i].code == PLUS) {printf("\n ADD ");}
    else if (output[i].code == MINUS) {printf("\n SUB ");} //// FIX HERE
    else if (output[i].code == TIMES) {printf("\n MUL ");}
    else if (output[i].code == SLASH) {printf("\n DIV ");} //// FIX HERE
    else if (output[i].code == UNARY) {printf("\n NOT \n LIT 1 \n ADD");}
  }
  printf("\n STO %s \n", output[0].data);

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