#include <conio.h>
#include <stdio.h>
#include <stack>
#include <string>
#include <vector>
using namespace std;

#define CLASSES 15
enum lclass {UNDEF, EQUAL, DECL, COMMA, IDENT, CONST, PLUS, MINUS, TIMES, SLASH, L_BR, R_BR, NEWL, END, UNARY};
char types[CLASSES][6] = {"UNDEF", "EQUAL", "DECL\0", "COMMA", "IDENT", "CONST", "PLUS\0", "MINUS", "TIMES", "SLASH", "L_BR\0", "R_BR\0", "NEWL\0", "END\0\0", "UNARY"};

typedef struct token {
  lclass code;
  unsigned line;
  char *data;
} token;

vector<token> tokens;
vector<token>::iterator it, ops;
vector<char*> ds;
unsigned errors;
stack<token*> operators;
vector<token*> output;

char* allocate(char* data, size_t len) {
  char *ptr = new char[len+1];
  if (ptr) {
    for (size_t i = 0; i < len; i++) {ptr[i] = data[i];}
    ptr[len] = 0;
    return ptr;
  }
  printf("\n engine: can't allocate memory");
  return 0;
}

void add(lclass type, unsigned line, char* name = 0, size_t len = 0) {
  token temp = {type, line, 0};
  if (type == IDENT || type == CONST || type == UNDEF) {temp.data = allocate(name, len);}
  tokens.push_back(temp);
}

bool is_alpha(char c) {return (c >= 'a' && c <= 'z') ? 1 : 0;}
bool is_digit(char c) {return (c >= '0' && c <= '9') ? 1 : 0;}
bool is_space(char c) {return (c == ' ' || c == '\t') ? 1 : 0;}

void lexic(FILE *io) {
  char b = fgetc(io), buffer[32], shift;
  char ops[4] = "Var";
  unsigned line = 0;
  while (b != EOF) {
    if (is_space(b)) {b = fgetc(io); continue;}
    else if (is_alpha(b) || is_digit(b) || b == 'V') {
      memset(&buffer[0], 0, sizeof(buffer));
      buffer[0] = b, shift = 1;
      if (is_alpha(b) || b == 'V') {while (is_alpha(b = fgetc(io)) && shift < 32) buffer[shift++] = b;}
      else {while (is_digit(b = fgetc(io)) && shift < 32) buffer[shift++] = b;}
      if (buffer[0] == 'V') {
        if (strcmp(&ops[0], &buffer[0])) {
          printf("\n <%d> UNDEF: %s", line, buffer);
          add(UNDEF, line, &buffer[0], shift);
        } else {
          printf("\n <%d> DECL", line);
          add(DECL, line);
        }
      } else if (is_alpha(buffer[0])) {
        printf("\n <%d> IDENT: %s", line, buffer);
        add(IDENT, line, &buffer[0], shift);
      } else {
        printf("\n <%d> CONST: %s", line, buffer);
        add(CONST, line, &buffer[0], shift);
      }
      continue;
    }
    else if (b == '+') {printf("\n <%d> PLUS: %c", line, b); add(PLUS, line);}
    else if (b == '-') {printf("\n <%d> MINUS: %c", line, b); add(MINUS, line);}
    else if (b == '*') {printf("\n <%d> TIMES: %c", line, b); add(TIMES, line);}
    else if (b == '/') {printf("\n <%d> SLASH: %c", line, b); add(SLASH, line);}
    else if (b == '=') {printf("\n <%d> EQUAL: %c", line, b); add(EQUAL, line);}
    else if (b == '(') {printf("\n <%d> L_BR: %c", line, b); add(L_BR, line);}
    else if (b == ')') {printf("\n <%d> R_BR: %c", line, b); add(R_BR, line);}
    else if (b == ',') {printf("\n <%d> COMMA: %c", line, b); add(COMMA, line);}
    else if (b == '\n') {printf("\n <%d> NEWL", line); add(NEWL, line); line++;}
    else if (b == '.') {printf("\n <%d> END: %c", line, b); add(END, line);}
    else {add(UNDEF, line, &b, 1);}
    b = fgetc(io);
  };
}

void unexpected(lclass s) {
  if (it->code != UNDEF) {printf("\n syntax: unexpected symbol at line %d: %s (expected %s)", it->line, types[it->code], types[s]);}
  else {printf("\n syntax: unexpected symbol at line %d: '%s' (expected %s)", it->line, it->data, types[s]);}
  errors++;
}

void next_token() {if (it != tokens.end()) it++;}

int equal(lclass s) {
  if (it->code == s) {next_token(); return 1;}
  return 0;
}

int expect(lclass s) {
  if (equal(s)) return 1;
  unexpected(s);
  return 0;
}

int lookup(char* d) {
  for (size_t i = 0; i < ds.size(); i++) {
    if (!strcmp(d, ds[i])) return i;
  }
  return -1;
}

void variables() {
  if (expect(IDENT)) {
    it--;
    if (lookup(it->data) == -1) {ds.push_back(it->data);}
    else {errors++; printf("\n syntax: variable redefinition at line %d (%s)", it->line, it->data);}
    it++;
    if (equal(COMMA)) {variables();}
  }
}

bool declaration() {
  if (expect(DECL)) variables();
  expect(NEWL);
  return !errors;
}

void check_declaration() {
  it--;
  if (lookup(it->data) == -1) {
    printf("\n syntax: undeclared variable: %s", it->data);
    errors++;
  }
  it++;
}

void expression();
void assembler();

void term() {
  if (it->code == MINUS) {it->code = UNARY; next_token();}
  if (equal(IDENT)) {
    check_declaration();
  } else if (equal(CONST)) {
  } else if (equal(L_BR)) {
    expression();
    expect(R_BR);
  } else {
    unexpected(IDENT); // not exactly: can be CONST, L_BR or R_BR
    next_token();
  }
}

void expression() {
  if (it->code == MINUS) {it->code = UNARY; next_token();}
  term();
  while (it->code == PLUS || it->code == MINUS || it->code == TIMES || it->code == SLASH) {
    next_token();
    term();
  }
}

void calculation() {
  do {
    if (expect(IDENT)) check_declaration();
    expect(EQUAL);
    expression();
  } while (equal(NEWL));
}

bool syntax() {
  errors = 0;
  if (tokens.empty()) {printf("\n syntax: no tokens \n"); return 0;}
  it = tokens.begin();
  declaration();
  ops = it;
  calculation();
  expect(END);
  if (!errors && it != tokens.end() && it->code != UNDEF) {
    errors++;
    printf("\n syntax: expected end at line %d \n", it->line);
  }
  if (!errors) {printf("\n Syntax check passed, no errors\n");}
  else {printf("\n Syntax check complete, errors found: %d\n", errors);}
  return !errors;
}

bool is_higher(lclass a, lclass b) { // bad until done with masks
  char res = 0;
  if (a == PLUS || a == MINUS) {res = 1;}
  else if (a == TIMES || a == SLASH) {res = 2;}
  else if (a == UNARY) {res = 4;}
  if (b == PLUS || b == MINUS) {res -= 1;}
  else if (b == TIMES || b == SLASH) {res -= 2;}
  else if (b == UNARY) {res -= 4;}
  return (res > 0);
}

void yard() { ////
  while (it->code != NEWL && it->code != END) {
    if (it->code == IDENT || it->code == CONST) {
      output.push_back(&(*it));
    } else if (it->code == PLUS || it->code == MINUS || it->code == TIMES || it->code == SLASH || it->code == UNARY) {
      if (!operators.empty()) {
        while (!operators.empty() && !is_higher(it->code, operators.top()->code)) {
          output.push_back(operators.top());
          operators.pop();
        }
      }
      operators.push(&(*it));
    } else if (it->code == L_BR) {
      operators.push(&(*it));
    } else if (it->code == R_BR) {
      while (!operators.empty() && operators.top()->code != L_BR) {
        output.push_back(operators.top());
        operators.pop();
      }
      if (!operators.empty()) {operators.pop();}
      else {printf("\n engine: yard operators stack underflow"); errors++;}
    }
    next_token();
  }
  while (!operators.empty()) {
    output.push_back(operators.top());
    operators.pop();
  }
  for (vector<token*>::iterator i = output.begin(); i != output.end(); ++i) {
    if ((*i)->code == IDENT || (*i)->code == CONST) {printf("%s ", (*i)->data);}
    else if ((*i)->code == PLUS) {printf("+ ");}
    else if ((*i)->code == MINUS) {printf("- ");}
    else if ((*i)->code == TIMES) {printf("* ");}
    else if ((*i)->code == SLASH) {printf("/ ");}
    else if ((*i)->code == UNARY) {printf("~ ");}
    else if ((*i)->code == EQUAL) {printf("= ");}
  }
  assembler();
  output.clear();
}

void postfix() {
  it = ops;
  do {
    printf("\n");
    output.push_back(&(*it)), next_token();
    output.push_back(&(*it)), next_token();
    yard();
    next_token();
  } while (tokens.end() != it && it->code != NEWL && it->code != END && it->code != UNDEF);
}

void assembler() {
  for (vector<token*>::iterator i = output.begin() + 2; i != output.end(); ++i) {
    if ((*i)->code == IDENT) {printf("\n LOAD %d ", lookup((*i)->data));}
    else if ((*i)->code == CONST){printf("\n LIT %s ", (*i)->data);}
    else if ((*i)->code == PLUS) {printf("\n ADD ");}
    else if ((*i)->code == MINUS) {printf("\n SUB ");}
    else if ((*i)->code == TIMES) {printf("\n MUL ");}
    else if ((*i)->code == SLASH) {printf("\n DIV ");}
    else if ((*i)->code == UNARY) {printf("\n NOT \n LIT 1 \n ADD ");}
  }
  printf("\n STO %d \n", lookup((*output.begin())->data));
}

void deallocate(){
  for (it = tokens.begin(); it != tokens.end(); ++it) {delete[] it->data;}
  tokens.clear();
}

void main() {
  char buf[256] = "in.txt";
  FILE *io;
  //printf("\n input: "); gets(buf);
  if (io = fopen(buf, "r")) {
    lexic(io);
    fclose(io);
    if (syntax()) postfix();
    deallocate();
    if (!errors) {printf("\n\n Build succeeded");}
    else {printf("\n\n Threre were build errors");}
  } else {
    printf("\n error: can't open input file");
  }
  getch();
}