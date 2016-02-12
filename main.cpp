#include <conio.h>
#include <stdio.h>
#include <string>
#include <vector>

// lexem classes
enum lexem_class {UNDEFINED, KEYWORD, OPERATOR, IDENTIFIER, CONSTANT};

class lexem {
public:
  lexem() {set("", UNDEFINED);}
  lexem(std::string lexem, lexem_class type) {set(lexem, type);}
  ~lexem() {}
  void set(std::string lexem, lexem_class type) {lexem_ = lexem, type_ = type;}
  std::string lexem_;
  lexem_class type_; // lexem class
};

std::vector<lexem> lexems; // list of lexems

bool is_alpha(char c) {return (c >= 'a' && c <= 'z') ? 1 : 0;}
bool is_digit(char c) {return (c >= '0' && c <= '9') ? 1 : 0;}
bool is_decl(char *s) {return (*s == 'V' && *(s + 1) == 'a' && *(s + 2) == 'r') ? 1 : 0;}
bool is_sum_op(char c) {return (c == '+' || c == '-') ? 1 : 0;}
bool is_mul_op(char c) {return (c == '*' || c == '/') ? 1 : 0;}
bool is_space(char c) {return (c == ' ' || c == '\t') ? 1 : 0;}
void skip_space(char *s) {while (is_space(*s) && *s != 0) s++;}


char* get_name(char *s) {
  std::string t = "";
  while (is_alpha(*s)) {
    t += *s;
    s++;
  }
  printf("\n found id: %s", t.c_str());
  lexems.push_back(lexem(t, IDENTIFIER));
  skip_space(s);
  return s;
}

char* get_num(char *s) {
  std::string t = "";
  while (is_digit(*s)) {
    t += *s;
    s++;
  }
  printf("\n found integer: %s", t.c_str());
  lexems.push_back(lexem(t, CONSTANT));
  skip_space(s);
  return s;
}

void split(char *b) {
  char lex;
  
  while(*b != '\n' && *b != NULL) {
    lex = *b;
    if (is_space(lex)) {skip_space(b);}
    else if (lex == 'V' && is_decl(b)) {
      printf("\n found declaration of variables (Var)");
      lexems.push_back(lexem("Var", OPERATOR));
      b += 3;
      continue;
    } else if (is_alpha(lex)) {
      b = get_name(b);
      continue;
    } else if (is_digit(lex)) {
      b = get_num(b);
      continue;
    } else if (is_sum_op(lex) || is_mul_op(lex) || lex == '=' || lex == '(' || lex == ')' || lex == ',' || lex == '.') {
      printf("\n found operator: %c", lex);
      //lexems.push_back(lexem(lex, OPERATOR)); // doesn't work yet
    } else {return;}
//    if(isOperand(lex)) push(lex); // write lexem to stack
//    if(isOperator(lex)) push(performOperation(lex, pop(), pop()));
    b++;
  }
  return;
}

void main() {
  char buf[256] = "in";
  FILE *io;
  //printf("Enter filename: ");
  //gets(buf);
  //if (strlen(buf) == 0) {return;}
  if (io = fopen(buf, "r")) {
    while(!feof(io)) {
      fgets(buf, sizeof(buf), io);
      split(&buf[0]);
      printf("\n ----- -----");
    }
    fclose(io);
  }else{/* no file */}
  getch();
}