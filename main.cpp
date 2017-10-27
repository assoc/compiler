#include <iostream>
#include <stack>
#include <string>
#include <vector>

using std::cin;
using std::stack;
using std::string;
using std::vector;

enum lexem_class {
	UNDEF,	// undefined symbol
	COMMA,	// comma (dot)
	NUMBER,	// digit/number
	PLUS,	// addition
	MINUS,	// subtraction
	MULT,	// multiplication
	DIVIDE,	// division
	POWER,	// power (exponential)
	L_BR,	// left bracket
	R_BR,	// right bracket
	NEWL,	// new line or EOF
	UNARY	// unary minus
};

typedef struct token
{
	lexem_class code;
	long long	number;
	unsigned	line;

	bool is_number()
	{ return code == NUMBER; }
	bool is_left_operator()
	{ return (code == PLUS || code == MINUS || code == MULT || code == DIVIDE); }
	bool is_right_operator()
	{ return code == POWER; }
	bool is_binary_operator(){ return is_left_operator() || is_right_operator(); }
	bool is_unary_operator()
	{ return code == MINUS || code == UNARY; }
	bool is_operator()
	{ return is_binary_operator() || is_unary_operator(); }
} token;





namespace lexer
{
	bool is_alpha(char c)
	{
		return (c >= 'a' && c <= 'z');
	}
	bool is_digit(char c)
	{
		return (c >= '0' && c <= '9');
	}
	bool is_space(char c)
	{
		return (c == ' ' || c == '\t');
	}

	vector<token> parse(string& input)
	{
		vector<token>	tokens;
		string			number_as_text;
		unsigned		current_line = 1;

		for (auto& it = input.begin(); it != input.end(); ++it)
		{
			// skip all spaces
			if (is_space(*it))
				continue;
			
			if (is_digit(*it))
			{
				// numbers

				number_as_text.clear();
				number_as_text += *it;

				// add digits until something else is found
				for (++it; it != input.end() && is_digit(*it); ++it)
					number_as_text += *it;

				try
				{
					// convert string to actual number and add it to the list
					tokens.push_back({ NUMBER, std::stoll(number_as_text.c_str()), current_line });
				}
				catch (std::out_of_range&)
				{
					// if number is too big or too small mark it as undefined
					tokens.push_back({ UNDEF, 0, current_line });
					printf("\n (%d) lexer: out of range number '%s'", current_line, number_as_text.c_str());
				}

				// return back to last non-digit symbol
				--it;

				continue;
			}
			else
			{
				// operators

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

		// last symbol
		tokens.push_back({ NEWL, 0, current_line });
		return tokens;
	}
};

namespace parser
{
	vector<token>::iterator current_token;
	vector<token>::iterator end_token;

	void consume()
	{
		if (current_token != end_token)
			++current_token;
	}

	token& next()
	{
		return *current_token;
	}

	void error()
	{
		throw std::exception("Parsing error");
	}

	void expect(lexem_class tok)
	{
		if (next().code == tok)
			consume();
		else
			error();
	}

	void expression();
	void term()
	{
		if (next().is_number())
		{
			consume();
		}
		else if (next().code == L_BR)
		{
			consume();
			expression();
			expect(R_BR);
		}
		else if (next().is_unary_operator())
		{
			next().code = UNARY;
			consume();
			term();
		}
		else
		{
			error();
		}
	}

	/*
		digit -> "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
		number -> digit | number {digit}
		b_op -> "+" | "-" | "*" | "/" | "^"
		expression -> term | term b_op term
		term -> number | "(" expression ")" | "-" term
	*/

	void expression()
	{
		term();

		while (next().is_binary_operator())
		{
			consume();
			term();
		}
	}

	void parse(vector<token>& tokens)
	{
		if (tokens.empty())
			return;

		current_token	= tokens.begin();
		end_token		= tokens.end();

		do 
		{
			expression();
			expect(NEWL);
		} while (current_token != end_token && next().code != NEWL);
	}
};

namespace yard
{
	stack<token*>			operators;
	vector<token*>			output;

	vector<token>::iterator current_token;
	vector<token>::iterator end_token;
	unsigned				errors;

	void next_token()
	{
		if (current_token != end_token)
			++current_token;
	}

	bool is_higher(lexem_class a, lexem_class b)
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

	void apply_operation()
	{
		token* operation = operators.top();
		if (operation->is_binary_operator())
		{
			if (output.size() < 2)
				throw std::exception("Not enough operands for binary operator");

			token* op2 = output.back();
			output.pop_back();
			token* op1 = output.back();
			output.pop_back();

			switch (operation->code)
			{
			case PLUS:		op1->number += op2->number;	break;
			case MINUS:		op1->number -= op2->number;	break;
			case MULT:		op1->number *= op2->number;	break;
			case DIVIDE:
				if (op2->number == 0)
					throw std::exception("Division by 0");
				op1->number /= op2->number;
				break;

			case POWER:		op1->number = pow(op1->number, op2->number); break;
			}

			output.push_back(op1);
		}
		else
		{
			if (output.size() < 1)
				throw std::exception("Not enough operands for unary operator");

			output.back()->number = -output.back()->number;
		}
	}

	void shunting_yard()
	{
		// while there are tokens to be read...
		while (current_token->code != NEWL /*&& it->code != END*/)
		{
			// if the token is a number, then push it to the output queue.
			if (current_token->is_number())
			{
				output.push_back(&(*current_token));
			}
			// if the token is an operator, then:
			else if (current_token->is_operator())
			{
				// while there is an operator at the top of the operator stack
				// with greater than or equal to precedence and the operator is left associative:
				while (		!operators.empty()
						&& 	!is_higher(current_token->code, operators.top()->code)
						&& 	current_token->is_left_operator())
				{
					// pop operators from the operator stack, onto the output queue.
					apply_operation();
					operators.pop();
				}

				// push the read operator onto the operator stack.
				operators.push(&(*current_token));
			}
			// if the token is a left bracket (i.e. "("), then:
			else if (current_token->code == L_BR)
			{
				// push it onto the operator stack.
				operators.push(&(*current_token));
			}
			// if the token is a right bracket (i.e. ")"), then:
			else if (current_token->code == R_BR)
			{
				bool bracket_pair_found = false;

				// while the operator at the top of the operator stack is not a left bracket:
				while (!operators.empty() && !bracket_pair_found)
				{
					// pop operators from the operator stack onto the output queue.
					if (operators.top()->code != L_BR)
					{
						apply_operation();
					}
					else
					{
						bracket_pair_found = true;
					}

					// pop the left bracket from the stack.
					operators.pop();
				}

				// (if the stack runs out without finding a left bracket, then there are mismatched parentheses.)
				if (!bracket_pair_found)
				{
					shunting_yard_error();
					return;
				}
			}

			// ... read a token
			next_token();
		}

		// if there are no more tokens to read:

		// while there are still operator tokens on the stack:
		while (!operators.empty())
		{
			// ( if the operator token on the top of the stack is a bracket, then there are mismatched parentheses. )
			if (operators.top()->code != L_BR)
			{
				// pop the operator onto the output queue.
				apply_operation();
				operators.pop();
			}
			else
			{
				shunting_yard_error();
				return;
			}
		}

		// output result of the calculation
		for (auto& i : output)
		{
			if (i->code == NUMBER) { printf("%d", i->number); }
			else if (i->code == PLUS) { printf("+ "); }
			else if (i->code == MINUS) { printf("- "); }
			else if (i->code == MULT) { printf("* "); }
			else if (i->code == DIVIDE) { printf("/ "); }
			else if (i->code == POWER) { printf("^ "); }
			else if (i->code == UNARY) { printf("~ "); }
		}
	}

	unsigned postfix(vector<token>& tokens, vector<token*>& results)
	{
		current_token	= tokens.begin();
		end_token		= tokens.end();
		errors			= 0;

		do
		{
			printf("\n");
			shunting_yard();

			results.insert(results.end(), output.begin(), output.end());

			output.clear();
			next_token();
		} while (end_token != current_token && current_token->code != NEWL);
	
		return errors;
	}
};

bool test()
{
	char buf_input[] =	"-3 + 4 * 2 / ( 1 - 5 ) ^ 2 ^ 3""\n"
						"2 + 2 * 2"						"\n"
						"100 * ( 2 + 12 ) / 14"			"\n"
						"- (14/2+3) * -( -3 + 8 / 2 )"	"\n";

	vector<long long> control({ -3, 6, 100, 10 });
	vector<token> tokens = lexer::parse(string(buf_input));
	vector<token*> results;

	try
	{
		parser::parse(tokens);
		printf("\n [i] parser: no errors");
		yard::postfix(tokens, results);

		if (control.size() != results.size())
			return false;

		for (size_t i = 0; i < control.size(); ++i)
			if (control[i] != results[i]->number)
				return false;

		return true;
	}
	catch (std::exception&)
	{
		return false;
	}
}

int main(int argc, char* argv[])
{
	string str_input;

	if (argc < 2)
	{
		getline(cin, str_input);
	}
	else
	{
		str_input = argv[1];
	}

	vector<token> tokens = lexer::parse(string(str_input));
	vector<token*> results;

	try
	{
		parser::parse(tokens);
		yard::postfix(tokens, results);
	}
	catch (std::exception&)
	{
		return 1;
	}

	return 0;
}