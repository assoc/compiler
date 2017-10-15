#include <stack>
#include <vector>
#include <string>

using std::stack;
using std::string;
using std::vector;

enum lclass {UNDEF, COMMA, NUMBER, PLUS, MINUS, MULT, DIVIDE, POWER, L_BR, R_BR, NEWL, UNARY};
char types[][6] = {"UNDEF", "COMMA", "NUMB\0", "PLUS\0", "MINUS", "MULT", "DIVID", "POWER", "L_BR\0", "R_BR\0", "NEWL\0", "UNARY"};

typedef struct token {
	lclass code;
	long long number;
	unsigned line;

	bool is_number()
	{ return code == NUMBER; }
	bool is_left_operator()
	{ return (code == PLUS || code == MINUS || code == MULT || code == DIVIDE); }
	bool is_right_operator()
	{ return code == POWER; }
	bool is_binary_operator(){ return is_left_operator() || is_right_operator(); }
	bool is_unary_operator()
	{ return code == UNARY; }
	bool is_operator()
	{ return is_binary_operator() || is_unary_operator(); }
} token;


stack<token*>			operators;
vector<token*>			output;


namespace lexer
{
	bool is_alpha(char c)
	{
		return (c >= 'a' && c <= 'z') ? 1 : 0;
	}
	bool is_digit(char c)
	{
		return (c >= '0' && c <= '9') ? 1 : 0;
	}
	bool is_space(char c)
	{
		return (c == ' ' || c == '\t') ? 1 : 0;
	}

	vector<token> parse(string& input)
	{
		vector<token>	tokens;
		string			number_as_text;
		unsigned		current_line = 1;

		for (auto& it = input.begin(); it != input.end(); ++it)
		{
			if (is_space(*it))
				continue;
			
			if (is_digit(*it))
			{
				number_as_text.clear();
				number_as_text += *it;

				for (++it; it != input.end() && is_digit(*it); ++it)
					number_as_text += *it;

				// return back to non-digit symbol
				--it;

				try
				{
					tokens.push_back({ NUMBER, std::stoll(number_as_text.c_str()), current_line });
				}
				catch (std::out_of_range&)
				{
					tokens.push_back({ UNDEF, 0, current_line });
					printf("\n (%d) lexer: out of range number '%s'", current_line, number_as_text.c_str());
				}

				continue;
			}
			else
			{
				switch (*it)
				{
				case '+':	tokens.push_back({ PLUS,	0, current_line }); break;
				case '-':	tokens.push_back({ MINUS,	0, current_line }); break;
				case '*':	tokens.push_back({ MULT,	0, current_line }); break;
				case '/':	tokens.push_back({ DIVIDE,	0, current_line }); break;
				case '^':	tokens.push_back({ POWER,	0, current_line }); break;
				case '(':	tokens.push_back({ L_BR,	0, current_line }); break;
				case ')':	tokens.push_back({ R_BR,	0, current_line }); break;
				case '.':	tokens.push_back({ COMMA,	0, current_line }); break;
				case '\n':
					tokens.push_back({ NEWL, 0, current_line });
					current_line++;
					break;

				default:
					tokens.push_back({ UNDEF, 0, current_line });
					printf("\n (%d) lexer: unknown character '%c' [0x%x]", current_line, *it, *it);
					break;
				}
			}
		}

		return tokens;
	}
};

namespace parser
{

	vector<token>::iterator current_token;
	vector<token>::iterator end_token;
	unsigned				errors;

	void next_token()
	{
		if (current_token != end_token)
			++current_token;
	}

	void unexpected(lclass s)
	{
		printf("\n (%d) parser: unexpected symbol: %s [expected %s]", current_token->line, types[current_token->code], types[s]);
		errors++;
	}

	bool peek(lclass s)
	{
		return (current_token->code == s);
	}

	bool next_if_equal(lclass s)
	{
		if (current_token->code == s)
		{
			next_token();
			return 1;
		}

		return 0;
	}

	bool expect(lclass s)
	{
		if (next_if_equal(s))
			return 1;

		unexpected(s);
		return 0;
	}

	void expression();

	void term()
	{
		if (current_token->code == MINUS)
		{
			current_token->code = UNARY;
			next_token();
		}

		if (next_if_equal(NUMBER))
		{
		}
		else if (next_if_equal(L_BR))
		{
			expression();
			expect(R_BR);
		}
		else
		{
			unexpected(UNDEF); // TODO
			next_token();
		}
	}

	void expression()
	{
		if (current_token->code == MINUS)
		{
			current_token->code = UNARY;
			next_token();
		}

		term();

		while (current_token->is_binary_operator())// TODO: crashes here if there is no \n at the end
		{
			next_token();
			term();
		}
	}

	void calculation()
	{
		do
		{
			expression();
		} while (!next_if_equal(NEWL));
	}

	bool parse(vector<token>& tokens)
	{
		if (tokens.empty())
			return 0;

		errors			= 0;
		current_token	= tokens.begin();
		end_token		= tokens.end();

		calculation();

		return (errors == 0);
	}
};

namespace yard
{
	vector<token>::iterator current_token;
	vector<token>::iterator end_token;
	unsigned				errors;

	void next_token()
	{
		if (current_token != end_token)
			++current_token;
	}

	bool is_higher(lclass a, lclass b)
	{
		char priority = 0;

		if (a == PLUS || a == MINUS) { priority = 1; }
		else if (a == MULT || a == DIVIDE) { priority = 2; }
		else if (a == POWER) { priority = 4; }
		else if (a == UNARY) { priority = 8; }

		if (b == PLUS || b == MINUS) { priority -= 1; }
		else if (b == MULT || b == DIVIDE) { priority -= 2; }
		else if (b == POWER) { priority -= 4; }
		else if (b == UNARY) { priority -= 8; }

		return (priority > 0);
	}

	void shunting_yard_error()
	{
		printf("\n (%d) postfix: brackets missmatch", (*output.begin())->line);
		errors++;

		while (!operators.empty())
			operators.pop();

		output.clear();

		while (current_token->code != NEWL)
			next_token();
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
			case MULT:	op1->number *= op2->number;	break;
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
		while (current_token->code != NEWL /*&& it->code != END*/)
		{
			if (current_token->is_number())
			{
				output.push_back(&(*current_token));
			}
			else if (current_token->is_operator())
			{
				if (!operators.empty())
				{
					while (!operators.empty() && !is_higher(current_token->code, operators.top()->code) && current_token->is_left_operator())
					{
						apply_operation(); // output.push_back(operators.top());
						operators.pop();
					}
				}

				operators.push(&(*current_token));
			}
			else if (current_token->code == L_BR)
			{
				operators.push(&(*current_token));
			}
			else if (current_token->code == R_BR)
			{
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
			if (operators.top()->code != L_BR)
			{
				apply_operation();
				operators.pop();
			}
			else
			{
				shunting_yard_error();
				return;
			}
		}

		// Output as string
		for (auto i = output.begin(); i != output.end(); ++i)
		{
			if ((*i)->code == NUMBER) { printf("%d", (*i)->number); }
			else if ((*i)->code == PLUS) { printf("+ "); }
			else if ((*i)->code == MINUS) { printf("- "); }
			else if ((*i)->code == MULT) { printf("* "); }
			else if ((*i)->code == DIVIDE) { printf("/ "); }
			else if ((*i)->code == POWER) { printf("^ "); }
			else if ((*i)->code == UNARY) { printf("~ "); }
		}
	}

	unsigned postfix(vector<token>& tokens)
	{
		current_token	= tokens.begin();
		end_token		= tokens.end();
		errors			= 0;

		do
		{
			printf("\n");
			shunting_yard();
			output.clear();
			next_token();
		} while (tokens.end() != current_token && current_token->code != NEWL && current_token->code != UNDEF);
	
		return errors;
	}
};

int main(int argc, char* argv[])
{
	char buf_input[] =	"3 + 4 * 2 / ( 1 - 5 ) ^ 2 ^ 3"	"\n"
						"2 + 2 * 2"						"\n"
						"100 * ( 2 + 12 ) / 14"			"\n"; // TODO: parser failed when 14 deleted

	vector<token> tokens = lexer::parse(string(buf_input));

	if (parser::parse(tokens))
	{
		printf("\n [i] parser: no errors");
		yard::postfix(tokens);
	}

	return 0;
}