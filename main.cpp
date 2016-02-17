#include <conio.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stack>
#define CLASSES 15

enum lclass {UNDEF, DECL, IDENT, CONST, NEWL, PLUS, MINUS, TIMES, SLASH, UNARY, EQUAL, COMMA, END, L_PAR, R_PAR};
std::string types[CLASSES] = {"UNDEF", "DECL", "IDENT", "CONST", "NEWL", "PLUS", "MINUS", "TIMES", "SLASH", "UNARY", "EQUAL", "COMMA", "END", "L_PAR", "R_PAR"};
unsigned amount[CLASSES] = {0};

typedef struct lexem_s {
  lclass type;
  unsigned line;
  char *name;
} lexem_s;

lexem_s temp;
std::vector<lexem_s> lexems;
unsigned actual_line;

lclass symbol;
int errors, iterator;

char* allocate_char(char* name, size_t len) {
  char *t = new char[len+1];
  if (t) {
    for (char i = 0; i < len; i++) {t[i] = name[i];}
    t[len] = 0;
    return t;
  }
  return 0;
}

void add(lclass type, char* name = 0, size_t len = 0) {
  amount[type]++;
  temp.line = actual_line;
  temp.type = type;
  temp.name = allocate_char(name, len);
  lexems.push_back(temp);
}
void deallocate_all() {
  for (unsigned i = 0; i < lexems.size(); i++) {
    if (temp.name) delete[] temp.name;
  }
}

bool is_alpha(char c) {return (c >= 'a' && c <= 'z') ? 1 : 0;}
bool is_digit(char c) {return (c >= '0' && c <= '9') ? 1 : 0;}
bool is_decl(char *s) {return (*s == 'V' && *(s + 1) == 'a' && *(s + 2) == 'r') ? 1 : 0;}
bool is_space(char c) {return (c == ' ' || c == '\t') ? 1 : 0;}
void skip_space(char *s) {while (is_space(*s) && *s != 0) s++;}

char* get_id(char *s) {
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
char* get_num(char *s) {
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
  char lex, zzz = 0;
  while(*b != '\n' && *b != NULL) {
    lex = *b;
    if (is_space(lex)) {skip_space(b);}
    else if (lex == 'V' && is_decl(b)) {
      printf("\n found DECL");
      add(DECL);
      b += 3;
      continue;
    } else if (is_alpha(lex)) {
      b = get_id(b);
      continue;
    } else if (is_digit(lex)) {
      b = get_num(b);
      continue;
    } 
    else if (lex == '+') {printf("\n found PLUS: %c", lex); add(PLUS);}
    else if (lex == '-') {printf("\n found MINUS: %c", lex); add(MINUS);}
    else if (lex == '*') {printf("\n found TIMES: %c", lex); add(TIMES);}
    else if (lex == '/') {printf("\n found SLASH: %c", lex); add(SLASH);}
    else if (lex == '=') {printf("\n found EQUAL: %c", lex); add(EQUAL);}
    else if (lex == '(') {printf("\n found L_PAR: %c", lex); add(L_PAR);}
    else if (lex == ')') {printf("\n found R_PAR: %c", lex); add(R_PAR);}
    else if (lex == ',') {printf("\n found COMMA: %c", lex); add(COMMA);}
    else if (lex == '.') {printf("\n found END: %c", lex); add(END);}
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
  printf("\n expect: unexpected symbol at line %d (%s)", actual_line, types[symbol].c_str());
  errors++;
}

void next_symbol() {
  if (iterator < lexems.size()) {
    symbol = lexems[iterator].type;
    actual_line = lexems[iterator].line;
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
    if (equal(COMMA)) {variables();}
  } else {unexpected();}
}

void declaration() {
  if (expect(DECL)) {variables();} else {unexpected();}
  if (!expect(NEWL)) {unexpected();}
}
void unary() {
  // check iterator
  lexems[iterator-1].type = UNARY;
}

void expression();
void factor() {
  if (symbol == MINUS) {unary(); next_symbol();}
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

void syntax() {
  errors = iterator = 0;
  next_symbol();
  declaration();
  opstart = iterator - 1;
  operations();
  expect(END);
  if (!errors) {printf("\n Syntax check passed, no errors\n");}
  else {printf("\n Syntax check passed, errors found: %d\n", errors);}
}

std::stack<lexem_s> operators;
std::vector<lexem_s> output;

int is_higher(lclass a, lclass b) {
  char res;
  if (a == PLUS || a == MINUS) {res = 1;}
  else if (a == TIMES || a == SLASH) {res = 2;}
  else if (a == L_PAR) {res = 0;}
  else {res = 4;}
  if (b == PLUS || b == MINUS) {res -= 1;}
  else if (b == TIMES || b == SLASH) {res -= 2;}
  else if (b == L_PAR) {res -= 0;}
  else {res -= 4;}

  return (res > 0);
}

void use_stack() { // no unary
  while (symbol != NEWL) {
    if (symbol == CONST || symbol == IDENT) {
      output.push_back(lexems[iterator-1]);
    } else if (symbol == PLUS || symbol == MINUS || symbol == TIMES || symbol == SLASH|| symbol == UNARY) {
      if (!operators.empty()) {
        while (!operators.empty() && !is_higher(symbol, operators.top().type)) { // less priority
          output.push_back(operators.top()); // !
          operators.pop();
        }
      }
      operators.push(lexems[iterator-1]);
    } else if (symbol == L_PAR) {
      operators.push(lexems[iterator-1]);
    } else if (symbol == R_PAR) {
      while (operators.top().type != L_PAR) {
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
    if (output[i].type == IDENT || output[i].type == CONST) {printf("%s ", output[i].name);}
    else if (output[i].type == PLUS) {printf("+ ");}
    else if (output[i].type == MINUS) {printf("- ");}
    else if (output[i].type == TIMES) {printf("* ");}
    else if (output[i].type == SLASH) {printf("/ ");}
    else if (output[i].type == UNARY) {printf("~ ");}
  }
  output.clear();
}

void postfix() {
  iterator = opstart;
  next_symbol();
  do {    
    printf("\n %s = ", lexems[iterator-1].name);
    expect(IDENT);
    expect(EQUAL);
    use_stack();
    next_symbol();
  } while(symbol != NEWL && symbol != COMMA);
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
    
    for (int i = 0; i < CLASSES; i++) {printf("\n %s \t %d", types[i].c_str(), amount[i]);}
    printf("\n");
    syntax();
    postfix();

    //deallocate_all(); // BIG problem here
  }else{// no file 
  }
  getch();
}