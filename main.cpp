#include <stack>
#include <vector>
#include <string>

using std::stack;
using std::vector;

enum lclass {UNDEF, COMMA, NUMBER, PLUS, MINUS, MULTIPLY, DIVIDE, POWER, L_BR, R_BR, NEWL, UNARY};
char types[][6] = {"UNDEF", "COMMA", "NUMB\0", "PLUS\0", "MINUS", "MULT", "DIVID", "POWER", "L_BR\0", "R_BR\0", "NEWL\0", "UNARY"};

typedef struct token {
  lclass code;
  long long number;
  unsigned line;

  bool is_number() { return code == NUMBER; }
  bool is_left_operator() { return (code == PLUS || code == MINUS || code == MULTIPLY || code == DIVIDE); }
  bool is_right_operator() { return code == POWER; }
  bool is_binary_operator(){ return is_left_operator() || is_right_operator(); }
  bool is_unary_operator() { return code == UNARY; }
  bool is_operator() { return is_binary_operator() || is_unary_operator(); }
} token;

vector<token>			tokens;
vector<token>::iterator tokens_it;

unsigned				errors;

stack<token*>			operators;
vector<token*>			output;

void add(lclass type, unsigned line, char* name = 0, size_t len = 0)
{
  token temp = {type, 0, 0};
  if (type == NUMBER)
	  temp.number = std::stoi(name);

  tokens.push_back(temp);
}

bool is_alpha(char c) {return (c >= 'a' && c <= 'z') ? 1 : 0;}
bool is_digit(char c) {return (c >= '0' && c <= '9') ? 1 : 0;}
bool is_space(char c) {return (c == ' ' || c == '\t') ? 1 : 0;}

char next_char(char** in)
{
	if (in == 0 || *in == 0)
		return 0;

	char c = (**in);
	(*in)++;

	return c;
}

void lexic(char* formula) {
	char b = next_char(&formula), buffer[32], shift;
	unsigned line = 1;
	while (b != 0) {
		if (is_space(b)) { b = next_char(&formula); continue; }
		else if (is_digit(b)) {
			memset(&buffer[0], 0, sizeof(buffer));
			buffer[0] = b, shift = 1;

			while (is_digit(b = next_char(&formula)) && shift < 32)
				buffer[shift++] = b;

			add(NUMBER, line, &buffer[0], shift);

			continue;
		}
		else if (b == '+') { add(PLUS, line); }
		else if (b == '-') { add(MINUS, line); }
		else if (b == '*') { add(MULTIPLY, line); }
		else if (b == '/') { add(DIVIDE, line); }
		else if (b == '^') { add(POWER, line); }
		else if (b == '(') { add(L_BR, line); }
		else if (b == ')') { add(R_BR, line); }
		else if (b == ',') { add(COMMA, line); }
		else if (b == '\n') { add(NEWL, line); line++; }
		else {
			add(UNDEF, line, &b, 1);
			printf("\n (%d) lexer: unknown character '%c' [0x%x]", line, b, b);
		}
		b = next_char(&formula);
	};
}

void unexpected(lclass s)
{
  printf("\n (%d) parser: unexpected symbol: %s [expected %s]", tokens_it->line, types[tokens_it->code], types[s]);
  errors++;
}

void next_token()
{
	if (tokens_it != tokens.end())
		++tokens_it;
}

bool peek(lclass s) {return (tokens_it->code == s);}

int equal(lclass s)
{
  if (tokens_it->code == s)
  {
	  next_token();
	  return 1;
  }
  return 0;
}

int expect(lclass s)
{
  if (equal(s))
	  return 1;

  unexpected(s);
  return 0;
}

void expression();

void term() {
  if (tokens_it->code == MINUS) {tokens_it->code = UNARY; next_token();}
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
  if (tokens_it->code == MINUS)
  {
	  tokens_it->code = UNARY;
	  next_token();
  }

  term();

  while (tokens_it->is_binary_operator())
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
  if (tokens.empty()) {printf("\n [i] parser: no tokens \n"); return 0;}
  tokens_it = tokens.begin();
  calculation();
  //expect(NEWL);

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

void shunting_yard_error()
{
  printf("\n (%d) postfix: brackets missmatch", (*output.begin())->line);
  errors++;

  while (!operators.empty())
	  operators.pop();

  output.clear();

  while (tokens_it->code != NEWL) {next_token();}
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

void apply_operation()
{
	token* operation = operators.top();
	if (operation->is_binary_operator())
	{
		if (output.size() < 2)
			__debugbreak();

		token* op2 = output.back();
		output.pop_back();
		token* op1 = output.back();
		output.pop_back();

		switch (operation->code)
		{
		case PLUS:		op1->number += op2->number;	break;
		case MINUS:		op1->number -= op2->number;	break;
		case MULTIPLY:	op1->number *= op2->number;	break;
		case DIVIDE:	op1->number /= op2->number;	break; // TODO: /0
		case POWER:		op1->number = pow(op1->number, op2->number); break;
		}

		output.push_back(op1);
	}
	else
	{
		if (output.size() < 1)
			__debugbreak();
		output.back()->number = -output.back()->number;
	}
}

void shunting_yard()
{
  while (tokens_it->code != NEWL /*&& it->code != END*/) {
    if (tokens_it->is_number())
	{
      output.push_back(&(*tokens_it));
    }
	else if (tokens_it->is_operator()) {
		if (!operators.empty())
		{
			while (!operators.empty() && !is_higher(tokens_it->code, operators.top()->code) && tokens_it->is_left_operator())
			{
				apply_operation(); // output.push_back(operators.top());
				operators.pop();
			}
		}
		operators.push(&(*tokens_it));
    }
	else if (tokens_it->code == L_BR) {
      operators.push(&(*tokens_it));
    } else if (tokens_it->code == R_BR) {
      bool bracket_pair_found = false;
	  while (!operators.empty() && !bracket_pair_found)
	  {
        if (operators.top()->code != L_BR)
		{
			apply_operation();
		}
        else
		{
			bracket_pair_found = true;
		}
        operators.pop();
      }
	  if (!bracket_pair_found)
	  {
		  shunting_yard_error();
		  return;
	  }
    }
    next_token();
  }

  while (!operators.empty())
  {
    if (operators.top()->code != L_BR) {
		apply_operation();
      operators.pop();
    } else {
      shunting_yard_error();
      return;
    }
  }

  // Output as string
  for (auto i = output.begin(); i != output.end(); ++i)
  {
    if ((*i)->code == NUMBER) {printf("%d", (*i)->number);}
    else if ((*i)->code == PLUS) {printf("+ ");}
    else if ((*i)->code == MINUS) {printf("- ");}
    else if ((*i)->code == MULTIPLY) {printf("* ");}
    else if ((*i)->code == DIVIDE) {printf("/ ");}
	else if ((*i)->code == POWER) { printf("^ "); }
    else if ((*i)->code == UNARY) {printf("~ ");}
  }
}

void postfix()
{
	tokens_it = tokens.begin();
	do
	{
		printf("\n");
		shunting_yard();
		output.clear();
		next_token();
	} while (tokens.end() != tokens_it && tokens_it->code != NEWL && tokens_it->code != UNDEF);
}

void deallocate()
{
	tokens.clear();
}

int main(int argc, char* argv[])
{
	char buf_input[] =	"3 + 4 * 2 / ( 1 - 5 ) ^ 2 ^ 3"	"\n"
						"2 + 2 * 2"						"\n"
						"100 * ( 2 + 12 ) / 14"			"\n"; // TODO: parser failed when 14 deleted

	lexic(buf_input);

	if (parser())
		postfix();

	deallocate();

	if (errors)
	{
		printf("\n [i] compiler: %d error(s)", errors);
		printf("\n\n Threre were build errors");
	}
	
	return errors;
}