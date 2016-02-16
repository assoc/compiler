#include <conio.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stack>

enum lexem_class {UNDEFINED, DECL, IDENT, CONST, NEWLINE, PLUS, MINUS, TIMES, SLASH, EQUAL, COMMA, PERIOD, L_PAR, R_PAR};
unsigned actual_line;

class lexem {
public:
  lexem() {set("", UNDEFINED, 0);}
  lexem(std::string lexem, lexem_class type, unsigned line) {set(lexem, type, line);}
  ~lexem() {}
  void set(std::string lexem, lexem_class type, unsigned line) {lexem_ = lexem, type_ = type; line_ = line;}
  std::string lexem_;
  lexem_class type_;
  unsigned line_;
};

std::vector<lexem> lexems;

bool is_alpha(char c) {return (c >= 'a' && c <= 'z') ? 1 : 0;}
bool is_digit(char c) {return (c >= '0' && c <= '9') ? 1 : 0;}
bool is_decl(char *s) {return (*s == 'V' && *(s + 1) == 'a' && *(s + 2) == 'r') ? 1 : 0;}
bool is_space(char c) {return (c == ' ' || c == '\t') ? 1 : 0;}
void skip_space(char *s) {while (is_space(*s) && *s != 0) s++;}

char* get_name(char *s) {
  std::string t = "";
  while (is_alpha(*s)) {t += *s; s++;}
  printf("\n found IDENT: %s", t.c_str());
  lexems.push_back(lexem(t, IDENT, actual_line));
  skip_space(s);
  return s;
}

char* get_num(char *s) {
  std::string t = "";
  while (is_digit(*s)) {t += *s; s++;}
  printf("\n found CONST: %s", t.c_str());
  lexems.push_back(lexem(t, CONST, actual_line));
  skip_space(s);
  return s;
}

void split(char *b) {
  char lex;
  
  while(*b != '\n' && *b != NULL) {
    lex = *b;
    if (is_space(lex)) {skip_space(b);}
    else if (lex == 'V' && is_decl(b)) {
      printf("\n found DECL");
      lexems.push_back(lexem("", DECL, actual_line));
      b += 3;
      continue;
    } else if (is_alpha(lex)) {
      b = get_name(b);
      continue;
    } else if (is_digit(lex)) {
      b = get_num(b);
      continue;
    } else if (lex == '+') {
      printf("\n found PLUS: %c", lex);
      lexems.push_back(lexem("", PLUS, actual_line));
    } else if (lex == '-') {
      printf("\n found MINUS: %c", lex);
      lexems.push_back(lexem("", MINUS, actual_line));
    } else if (lex == '*') {
      printf("\n found TIMES: %c", lex);
      lexems.push_back(lexem("", TIMES, actual_line));
    } else if (lex == '/') {
      printf("\n found SLASH: %c", lex);
      lexems.push_back(lexem("", SLASH, actual_line));
    } else if (lex == '=') {
      printf("\n found EQUAL: %c", lex);
      lexems.push_back(lexem("", EQUAL, actual_line));
    } else if (lex == '(') {
      printf("\n found L_PAR: %c", lex);
      lexems.push_back(lexem("", L_PAR, actual_line));
    } else if (lex == ')') {
      printf("\n found R_PAR: %c", lex);
      lexems.push_back(lexem("", R_PAR, actual_line));
    } else if (lex == ',') {
      printf("\n found COMMA: %c", lex);
      lexems.push_back(lexem("", COMMA, actual_line));
    } else if (lex == '.') {
      printf("\n found PERIOD: %c", lex);
      lexems.push_back(lexem("", PERIOD, actual_line));
    } else {return;}
//    if(isOperand(lex)) push(lex); // write lexem to stack
//    if(isOperator(lex)) push(performOperation(lex, pop(), pop()));
    b++;
  }
  lexems.push_back(lexem("", NEWLINE, actual_line));
  return;
}

/*
program = declaration operation "."
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
lexem_class symbol;
int iterator, errors;

void unexpected() {
  printf("\n expect: unexpected symbol at line %d (%d)", actual_line, symbol);
  errors++;
}

void next_symbol() {
  if (iterator < lexems.size()) {
    symbol = lexems[iterator].type_;
    actual_line = lexems[iterator].line_;
    iterator++;
  } else {symbol = UNDEFINED;}
}

int equal(lexem_class s) {
  if (symbol == s) {next_symbol(); return 1;}
  return 0;
}
int expect(lexem_class s) {
  if (equal(s)) return 1;
  unexpected();
  return 0;
}

void variables() {
  if (expect(IDENT)) {
    // add to vartable
    if (equal(COMMA)) {
      variables();
    }
  } else {unexpected();}
}

void declaration() {
  if (expect(DECL)) {
    variables();
  } else {unexpected();}
  if (!expect(NEWLINE)) {unexpected();}
}
void expression();
void factor() {
  if (symbol == MINUS) {next_symbol();}
  if (equal(IDENT)) {
    // check vartable
  } else if (equal(CONST)) {
  } else if (equal(L_PAR)) {
    expression();
    expect(R_PAR);
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
  if (symbol == MINUS) {next_symbol();}
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
  } while(equal(NEWLINE));
}

void syntax() {
  next_symbol();
  declaration();
  operations();
  expect(PERIOD);
}

void postfix() {
  postfix();
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
    iterator = 0;
    errors = 0;
    syntax();
    if (!errors) {printf("\n Syntax check passed, no errors");}
    else {printf("\n Syntax check passed, errors found: %d", errors);}
  }else{/* no file */}
  getch();
}