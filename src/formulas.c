//formulas.c - 

#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include "formulas.h"



static bool GetFunctionCode(const char *name, uint64_t *store, int srcHead);



bool CompileFormula(const char *src, uint64_t *store)
{
	double literalNum = 0;
	char funcName[MAX_FUNCTION_NAME + 1], ch;
	int srcHead = 0, funcHead = 0, storeHead = 0, decimalCounter = -1;
	bool lastWasLiteral = false, lastWasConstant = false, lastWasFunc = false;

	while ((ch = src[srcHead]))
	{
		iter:

		//NOPs
		if (isblank(ch)) continue;
		
		//Constants
		if (lastWasConstant)
		{
			lastWasConstant = false;
			store[storeHead++] = FORMULA_CONSTANT;

			switch (ch) //constant code
			{
				case 'e':
					((double *)store)[storeHead++] = M_E;
					break;

				case 'p':
					((double *)store)[storeHead++] = M_PI;
					break;

				default:
					fprintf(stderr, "Invalid constant '%c' at character %d.\n", ch, srcHead);
					return false;
			}

			continue;
		}
	
		//Functions
		if (lastWasFunc)
		{
			//Append char
			if (isalpha(ch))
			{
				if (funcHead >= MAX_FUNCTION_NAME)
				{
					fprintf(stderr, "Function name too long at character %d.\n", srcHead);
					return false;
				}
				funcName[funcHead++] = ch;
				funcName[funcHead] = '\0';
				continue;
			}
		
			lastWasFunc = false;
			funcHead = 0;

			//Set function code
			if (!GetFunctionCode(funcName, &store[storeHead++], srcHead))
			{
				fprintf(stderr, "Invalid function name '%s' at character %d.\n", funcName, srcHead);
				return false;
			}
		}
	
		//Literals
		if (isdigit(ch))
		{
			if (!lastWasLiteral) { literalNum = ch - '0'; }
			else { literalNum *= 10; literalNum += ch - '0'; }
			if (decimalCounter != -1) decimalCounter++;
			lastWasLiteral = true;
			continue;
		}
		else if (lastWasLiteral)
		{
			if (ch == '.')
			{
				decimalCounter = 0;
				continue;
			}
			else
			{
				if (decimalCounter != -1) while(decimalCounter--) literalNum /= 10.0;

				store[storeHead++] = FORMULA_LITERAL;
				((double *)store)[storeHead++] = literalNum;
				decimalCounter = false;
				decimalCounter = -1;
			}
		}
	
		//Variables
		if (isalpha(ch))
		{
			if (islower(ch))
				store[storeHead++] = FORMULA_VAR_BASE + ch - 'a';
			else
			{
				fprintf(stderr, "Variables must be lower case. Invalid variable '%c' at character %d.\n", ch, srcHead);
				return false;
			}
		}

		//Operations / shortcut funcs
		switch (ch)
		{
			//Used chars: ._,;<>[]+*-/^{}*#()\%~=	avoid using ! and " because of shell character escaping
			//Free chars: &'?@`'|

			case '_':
				lastWasConstant = true;
				break;

			case '=':
				lastWasFunc = true;
				break;

			case ',':
				store[storeHead++] = FORMULA_CLIP_WRITE;
				break;

			case ';':
				store[storeHead++] = FORMULA_CLIP_READ;
				break;

			case '<':
				store[storeHead++] = FORMULA_SEEK_LEFT;
				break;

			case '>':
				store[storeHead++] = FORMULA_SEEK_RIGHT;
				break;

			case '[':
				store[storeHead++] = FORMULA_COPY_LEFT;
				break;

			case ']':
				store[storeHead++] = FORMULA_COPY_RIGHT;
				break;

			case '+':
				store[storeHead++] = FORMULA_ADD;
				break;

			case '-':
				store[storeHead++] = FORMULA_SUBTRACT;
				break;

			case '*':
				store[storeHead++] = FORMULA_MULTIPLY;
				break;

			case '/':
				store[storeHead++] = FORMULA_DIVIDE;
				break;

			case '%':
				store[storeHead++] = FORMULA_REMAINDER;
				break;

			case '^':
				store[storeHead++] = FORMULA_POW;
				break;

			case '{':
				store[storeHead++] = FORMULA_SQRT;
				break;

			case '}':
				store[storeHead++] = FORMULA_SQUARE;
				break;

			case '$':
				store[storeHead++] = FORMULA_LOGN;
				break;

			case '#':
				store[storeHead++] = FORMULA_ABS;
				break;

			case '(':
				store[storeHead++] = FORMULA_SIN;
				break;

			case ')':
				store[storeHead++] = FORMULA_COS;
				break;

			case '\\':
				store[storeHead++] = FORMULA_TAN;
				break;

			case '~':
				store[storeHead++] = FORMULA_NEGATIVE;
				break;

			default:
				printf("Invalid operation: '%c'.\n", ch);
				return false;
		}
	}

	//Finish up if needed
	if (lastWasLiteral || lastWasConstant || lastWasFunc)
	{
		ch = ' ';
		srcHead--;

		goto iter;
	}

	//Return (like '\0' in strings)
	store[storeHead] = FORMULA_RET;

	return true;
}

bool EvaluateFormula(const uint64_t *src, FormulaVariable *variables, size_t variableC, double *ret)
{
	int srcHead = 0, bufferHead = 0;
	double buffers[MAX_BUFFERS], clip;
	uint64_t instruction;

	while ((instruction = src[srcHead++]) != FORMULA_RET)
	{
		switch (instruction)
		{
			case FORMULA_NOP: break;
			case FORMULA_CONSTANT:
			case FORMULA_LITERAL:
				buffers[bufferHead] = ((double *)src)[srcHead++];
				break;

			case FORMULA_CLIP_WRITE:
				clip = buffers[bufferHead];
				break;

			case FORMULA_CLIP_READ:
				buffers[bufferHead] = clip;
				break;

			case FORMULA_SEEK_LEFT:
				if (--bufferHead < 0)
				{
					fprintf(stderr, "BufferHead underflow at instruction %d.\n", srcHead);
					return false;
				}
				break;

			case FORMULA_SEEK_RIGHT:
				if (++bufferHead >= MAX_BUFFERS)
				{
					fprintf(stderr, "BufferHead overflow at instruction %d.\n", srcHead);
					return false;
				}
				break;

			case FORMULA_COPY_LEFT:
				if (--bufferHead < 0)
				{
					fprintf(stderr, "BufferHead underflow at instruction %d.\n", srcHead);
					return false;
				}
				buffers[bufferHead] = buffers[bufferHead + 1];
				break;

			case FORMULA_COPY_RIGHT:
				if (++bufferHead < 0)
				{
					fprintf(stderr, "BufferHead overflow at instruction %d.\n", srcHead);
					return false;
				}
				buffers[bufferHead] = buffers[bufferHead - 1];
				break;
		}
	}

	*ret = buffers[bufferHead];
	return true;
}


static bool GetFunctionCode(const char *name, uint64_t *store, int srcHead)
{
	if (!strcmp(name, "sin"))			*store = FORMULA_SIN;
	else if (!strcmp(name, "cos"))		*store = FORMULA_COS;
	else if (!strcmp(name, "tan"))		*store = FORMULA_TAN;
	else if (!strcmp(name, "asin"))		*store = FORMULA_ASIN;
	else if (!strcmp(name, "acos"))		*store = FORMULA_ACOS;
	else if (!strcmp(name, "atan"))		*store = FORMULA_ATAN;
	else if (!strcmp(name, "sinh"))		*store = FORMULA_SINH;
	else if (!strcmp(name, "cosh"))		*store = FORMULA_COSH;
	else if (!strcmp(name, "tanh"))		*store = FORMULA_TANH;
	else if (!strcmp(name, "asinh"))	*store = FORMULA_ASINH;
	else if (!strcmp(name, "acosh"))	*store = FORMULA_ACOSH;
	else if (!strcmp(name, "atanh"))	*store = FORMULA_ATANH;
	else if (!strcmp(name, "pow"))		*store = FORMULA_POW;
	else if (!strcmp(name, "abs"))		*store = FORMULA_ABS;
	else if (!strcmp(name, "ln"))		*store = FORMULA_LOGN;
	else if (!strcmp(name, "log"))		*store = FORMULA_LOG10;
	else if (!strcmp(name, "log2"))		*store = FORMULA_LOG2;
	else if (!strcmp(name, "sign"))		*store = FORMULA_SIGN;
	else if (!strcmp(name, "ceil"))		*store = FORMULA_CEIL;
	else if (!strcmp(name, "floor"))	*store = FORMULA_FLOOR;
	else if (!strcmp(name, "round"))	*store = FORMULA_ROUND;
	else
	{
		fprintf(stderr, "Invalid function '%s' at character %d.\n", name, srcHead);
		return false;
	}

	return true;
}
