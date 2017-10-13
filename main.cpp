#include <stack>
#include <vector>

using std::stack;
using std::vector;

enum lclass {UNDEF, COMMA, NUMBER, PLUS, MINUS, MULTIPLY, DIVIDE, POWER, L_BR, R_BR, NEWL, UNARY};
char types[][6] = {"UNDEF", "COMMA", "NUMB\0", "PLUS\0", "MINUS", "MULT", "DIVID", "POWER", "L_BR\0", "R_BR\0", "NEWL\0", "UNARY"};

typedef struct token {
  lclass code;
  char *data;
  unsigned line;

  bool is_number() { return code == NUMBER; }
  bool is_left_operator() { return (code == PLUS || code == MINUS || code == MULTIPLY || code == DIVIDE); }
  bool is_right_operator() { return code == POWER; }
  bool is_binary_operator(){ return is_left_operator() || is_right_operator(); }
  bool is_unary_operator() { return code == UNARY; }
  bool is_operator() { return is_binary_operator() || is_unary_operator(); }
} token;

vector<token> tokens;
vector<token>::iterator it, ops;
unsigned errors;
bool end_reached;
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

void add(lclass type, unsigned line, char* name = 0, size_t len = 0)
{
  token temp = {type, 0, line};
  if (type == NUMBER || type == UNDEF) {temp.data = allocate(name, len);}
  tokens.push_back(temp);
}

bool is_alpha(char c) {return (c >= 'a' && c <= 'z') ? 1 : 0;}
bool is_digit(char c) {return (c >= '0' && c <= '9') ? 1 : 0;}
bool is_space(char c) {return (c == ' ' || c == '\t') ? 1 : 0;}

void lexic(FILE *io, FILE *out) {
  char b = fgetc(io), buffer[32], shift;
  unsigned line = 1;
  while (b != EOF) {
    if (is_space(b)) {b = fgetc(io); continue;}
    else if (is_digit(b)) {
      memset(&buffer[0], 0, sizeof(buffer));
      buffer[0] = b, shift = 1;

      while (is_digit(b = fgetc(io)) && shift < 32)
		  buffer[shift++] = b;
      
	  fprintf(out, "\n <%d> CONST: %s", line, buffer);
	  add(NUMBER, line, &buffer[0], shift);

      continue;
    }
    else if (b == '+') {fprintf(out, "\n <%d> PLUS: %c", line, b); add(PLUS, line);}
    else if (b == '-') {fprintf(out, "\n <%d> MINUS: %c", line, b); add(MINUS, line);}
    else if (b == '*') {fprintf(out, "\n <%d> TIMES: %c", line, b); add(MULTIPLY, line);}
    else if (b == '/') {fprintf(out, "\n <%d> SLASH: %c", line, b); add(DIVIDE, line);}
	else if (b == '^') {fprintf(out, "\n <%d> POWER: %c", line, b); add(POWER, line);}
    else if (b == '(') {fprintf(out, "\n <%d> L_BR: %c", line, b); add(L_BR, line);}
    else if (b == ')') {fprintf(out, "\n <%d> R_BR: %c", line, b); add(R_BR, line);}
    else if (b == ',') {fprintf(out, "\n <%d> COMMA: %c", line, b); add(COMMA, line);}
    else if (b == '\n') {fprintf(out, "\n <%d> NEWL", line); add(NEWL, line); line++;}
    else {
      add(UNDEF, line, &b, 1);
      fprintf(out, "\n <%d> UNDEF: %c", line, b);
      printf("\n (%d) lexer: unknown character '%c' [0x%x]", line, b, b);
    }
    b = fgetc(io);
  };
}

void unexpected(lclass s) {
  printf("\n (%d) parser: unexpected symbol: %s [expected %s]", it->line, types[it->code], types[s]);
  errors++;
}

void next_token() {
  if (++it == tokens.end()){
    it--;
    end_reached = 1;
  }
}

bool peek(lclass s) {return (it->code == s);}

int equal(lclass s) {
  if (it->code == s) {next_token(); return 1;}
  return 0;
}

int expect(lclass s) {
  if (equal(s)) return 1;
  unexpected(s);
  return 0;
}

void expression();

void term() {
  if (it->code == MINUS) {it->code = UNARY; next_token();}
  if (equal(NUMBER)) {
  } else if (equal(L_BR)) {
    expression();
    expect(R_BR);
  } else {
    unexpected(UNDEF); // TODO
    next_token();
  }
}

void expression() {
  if (it->code == MINUS)
  {
	  it->code = UNARY;
	  next_token();
  }

  term();

  while (it->is_binary_operator())
  {
    next_token();
    term();
  }
}

void calculation() {
  do {
    expression();
  } while (!equal(NEWL));
}

bool parser() {
  errors = 0;
  end_reached = 0;
  if (tokens.empty()) {printf("\n [i] parser: no tokens \n"); return 0;}
  it = tokens.begin();
  ops = it;
  calculation();
  //expect(NEWL);
  end_reached = true;
  if (!errors && !end_reached) {
    errors++;
    printf("\n (%d) parser: expected '.' as last symbol \n", it->line);
  }
  if (!errors) {printf("\n [i] parser: no errors");}
  return !errors;
}

bool is_higher(lclass a, lclass b)
{
  char priority = 0;

  if (a == PLUS || a == MINUS) {priority = 1;}
  else if (a == MULTIPLY || a == DIVIDE) {priority = 2;}
  else if (a == POWER) { priority = 4; }
  else if (a == UNARY) {priority = 8;}

  if (b == PLUS || b == MINUS) {priority -= 1;}
  else if (b == MULTIPLY || b == DIVIDE) {priority -= 2;}
  else if (b == POWER) { priority -= 4; }
  else if (b == UNARY) {priority -= 8;}

  return (priority > 0);
}

void shunting_yard_error() {
  printf("\n (%d) postfix: brackets missmatch", (*output.begin())->line); errors++;
  while (!operators.empty()) {operators.pop();}
  output.clear();
  while (it->code != NEWL /*&& it->code != END*/) {next_token();}
}

/*
while there are tokens to be read:
	read a token.
	if the token is a number, then push it to the output queue.
	if the token is an operator, then:
		while there is an operator at the top of the operator stack with
			greater than or equal to precedence and the operator is left associative:
				pop operators from the operator stack, onto the output queue.
		push the read operator onto the operator stack.
	if the token is a left bracket (i.e. "("), then:
		push it onto the operator stack.
	if the token is a right bracket (i.e. ")"), then:
		while the operator at the top of the operator stack is not a left bracket:
			pop operators from the operator stack onto the output queue.
		pop the left bracket from the stack.
		< if the stack runs out without finding a left bracket, then there are
		mismatched parentheses. >
if there are no more tokens to read:
	while there are still operator tokens on the stack:
		< if the operator token on the top of the stack is a bracket, then
		there are mismatched parentheses. >
		pop the operator onto the output queue.
exit.
*/


stack<lclass> operations;

void shunting_yard()
{
  while (it->code != NEWL /*&& it->code != END*/) {
    if (it->is_number())
	{
      output.push_back(&(*it));
    }
	else if (it->is_operator()) {
		if (!operators.empty())
		{
			while (!operators.empty() && !is_higher(it->code, operators.top()->code) && it->is_left_operator())
			{
				output.push_back(operators.top());
				operators.pop();
			}
		}
		operators.push(&(*it));
    }
	else if (it->code == L_BR) {
      operators.push(&(*it));
    } else if (it->code == R_BR) {
      bool found = false;
      while (!operators.empty() && !found) {
        if (operators.top()->code != L_BR) {output.push_back(operators.top());}
        else {found = true;}
        operators.pop();
      }
      if (!found) {shunting_yard_error(); return;}
    }
    next_token();
  }

  while (!operators.empty()) {
    if (operators.top()->code != L_BR) {
      output.push_back(operators.top());
      operators.pop();
    } else {
      shunting_yard_error();
      return;
    }
  }

  // Output as string
  for (auto i = output.begin(); i != output.end(); ++i) {
    if ((*i)->code == NUMBER) {printf("%s ", (*i)->data);}
    else if ((*i)->code == PLUS) {printf("+ ");}
    else if ((*i)->code == MINUS) {printf("- ");}
    else if ((*i)->code == MULTIPLY) {printf("* ");}
    else if ((*i)->code == DIVIDE) {printf("/ ");}
	else if ((*i)->code == POWER) { printf("^ "); }
    else if ((*i)->code == UNARY) {printf("~ ");}
  }
}

void postfix(FILE *out) {
  it = ops;
  do {
    printf("\n ");
 //   output.push_back(&(*it)), next_token();
 //   output.push_back(&(*it)), next_token();
    shunting_yard();
    //assemble(out);
    output.clear();
    next_token();
  } while (tokens.end() != it && it->code != NEWL && /*it->code != END &&*/ it->code != UNDEF);
}

void deallocate()
{
  for (auto i = tokens.begin(); i != tokens.end(); ++i)
	delete[] i->data;

  tokens.clear();
}

int main(int argc, char* argv[])
{
  char buf_input[256] = "in.txt";
  char buf_lexems[256] = "lexems.txt";
  char buf_codes[256] = "output.txt";
  FILE *io, *lexems, *codes;
 // printf("\n input: "); gets(buf_input);
  if (io = fopen(buf_input, "r")) {
    if (lexems = fopen(buf_lexems, "w")) {
      if (codes = fopen(buf_codes, "w")) {
        lexic(io, lexems);
        fclose(io), fclose(lexems);
        if (parser()){
          postfix(codes);
          fclose(codes);
        }
        deallocate();
        if (!errors) {
          printf("\n\n Build succeeded");
        } else {
          printf("\n [i] compiler: %d error(s)", errors);
          printf("\n\n Threre were build errors");
        }
      } else {printf("\n error: can't open output file");}
    } else {printf("\n error: can't open lexems file");}
  } else {printf("\n error: can't open input file");}
  return 0;
}