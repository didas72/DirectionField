//formulas.c - 

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "formulas.h"



static bool GetFunctionCode(const char *name, uint64_t *store, int srcHead);



bool CompileFormula(const char *src, uint64_t *store)
{
	double literalNum = 0;
	char funcName[MAX_FUNCTION_NAME + 1], ch;
	int srcHead = 0, funcHead = 0, storeHead = 0, decimalCounter = -1;
	bool lastWasLiteral = false, lastWasConstant = false, lastWasFunc = false;

	while ((ch = src[srcHead++]))
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

			continue;
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

bool EvaluateFormula(const uint64_t *src, FormulaVariable *variables, int variableC, double *ret)
{
	int srcHead = 0, bufferHead = 0;
	double buffers[MAX_BUFFERS], clip = 0.0;
	uint64_t instruction;

	while ((instruction = src[srcHead++]) != FORMULA_RET)
	{
		//TODO: Test
		if (FORMULA_VAR_BASE <= instruction && instruction <= FORMULA_VAR_TOP)
		{
			char name = instruction - FORMULA_VAR_BASE + 'a';

			for (int i = 0; i < variableC; i++)
			{
				if (variables[i].name == name)
				{
					buffers[bufferHead] = variables[i].value;
					break;
				}
			}

			continue;
		}

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

			case FORMULA_ADD:
				if (bufferHead < 1)
				{
					fprintf(stderr, "BufferHead underflow at instruction %d.\n", srcHead);
					return false;
				}
				buffers[bufferHead] = buffers[bufferHead - 1] + buffers[bufferHead];
				break;

			case FORMULA_SUBTRACT:
				if (bufferHead < 1)
				{
					fprintf(stderr, "BufferHead underflow at instruction %d.\n", srcHead);
					return false;
				}
				buffers[bufferHead] = buffers[bufferHead - 1] - buffers[bufferHead];
				break;

			case FORMULA_MULTIPLY:
				if (bufferHead < 1)
				{
					fprintf(stderr, "BufferHead underflow at instruction %d.\n", srcHead);
					return false;
				}
				buffers[bufferHead] = buffers[bufferHead - 1] * buffers[bufferHead];
				break;

			case FORMULA_DIVIDE:
				if (bufferHead < 1)
				{
					fprintf(stderr, "BufferHead underflow at instruction %d.\n", srcHead);
					return false;
				}
				buffers[bufferHead] = buffers[bufferHead - 1] / buffers[bufferHead];
				break;

			case FORMULA_REMAINDER:
				if (bufferHead < 1)
				{
					fprintf(stderr, "BufferHead underflow at instruction %d.\n", srcHead);
					return false;
				}
				buffers[bufferHead] = fmod(buffers[bufferHead - 1], buffers[bufferHead]);
				break;

			case FORMULA_POW:
				if (bufferHead < 1)
				{
					fprintf(stderr, "BufferHead underflow at instruction %d.\n", srcHead);
					return false;
				}
				buffers[bufferHead] = pow(buffers[bufferHead - 1], buffers[bufferHead]);
				break;

			case FORMULA_SQUARE:
				buffers[bufferHead] *= buffers[bufferHead];
				break;

			case FORMULA_SQRT:
				buffers[bufferHead] = sqrt(buffers[bufferHead]);
				break;

			case FORMULA_LOGN:
				buffers[bufferHead] = log(buffers[bufferHead]);
				break;

			case FORMULA_LOGD:
				buffers[bufferHead] = log10(buffers[bufferHead]);
				break;

			case FORMULA_LOGB:
				buffers[bufferHead] = log2(buffers[bufferHead]);
				break;

			case FORMULA_ABS:
				buffers[bufferHead] = fabs(buffers[bufferHead]);
				break;

			case FORMULA_SIN:
				buffers[bufferHead] = sin(buffers[bufferHead]);
				break;

			case FORMULA_COS:
				buffers[bufferHead] = cos(buffers[bufferHead]);
				break;

			case FORMULA_TAN:
				buffers[bufferHead] = tan(buffers[bufferHead]);
				break;

			case FORMULA_ASIN:
				buffers[bufferHead] = asin(buffers[bufferHead]);
				break;

			case FORMULA_ACOS:
				buffers[bufferHead] = acos(buffers[bufferHead]);
				break;

			case FORMULA_ATAN:
				buffers[bufferHead] = atan(buffers[bufferHead]);
				break;

			case FORMULA_SINH:
				buffers[bufferHead] = sinh(buffers[bufferHead]);
				break;

			case FORMULA_COSH:
				buffers[bufferHead] = cosh(buffers[bufferHead]);
				break;

			case FORMULA_TANH:
				buffers[bufferHead] = tanh(buffers[bufferHead]);
				break;

			case FORMULA_ASINH:
				buffers[bufferHead] = asinh(buffers[bufferHead]);
				break;

			case FORMULA_ACOSH:
				buffers[bufferHead] = acosh(buffers[bufferHead]);
				break;

			case FORMULA_ATANH:
				buffers[bufferHead] = atanh(buffers[bufferHead]);
				break;

			case FORMULA_SIGN:
				buffers[bufferHead] = 
					buffers[bufferHead] == 0.0 ? 0.0 :
					(buffers[bufferHead] > 0.0 ? 1.0 : - 1.0);
				break;

			case FORMULA_CEIL:
				buffers[bufferHead] = ceil(buffers[bufferHead]);
				break;

			case FORMULA_FLOOR:
				buffers[bufferHead] = floor(buffers[bufferHead]);
				break;

			case FORMULA_ROUND:
				buffers[bufferHead] = round(buffers[bufferHead]);
				break;

			case FORMULA_NEGATIVE:
				buffers[bufferHead] = -buffers[bufferHead];
				break;

			case FORMULA_RET://Should never get here
				fprintf(stderr, "Unexpected return at instruction %d.\n", srcHead);
				return false;

			default:
				fprintf(stderr, "Invalid instruction %ld at index %d.\n", src[srcHead], srcHead);
				return false;
		}
	}

	//Check if nan or +-infinity
	if (buffers[bufferHead] != buffers[bufferHead] ||
		isinf(buffers[bufferHead])) return false;

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
	else if (!strcmp(name, "log"))		*store = FORMULA_LOGD;
	else if (!strcmp(name, "logb"))		*store = FORMULA_LOGB;
	else if (!strcmp(name, "sign"))		*store = FORMULA_SIGN;
	else if (!strcmp(name, "ceil"))		*store = FORMULA_CEIL;
	else if (!strcmp(name, "floor"))	*store = FORMULA_FLOOR;
	else if (!strcmp(name, "round"))	*store = FORMULA_ROUND;
	else if (!strcmp(name, "sqrt"))		*store = FORMULA_SQRT;
	else
	{
		fprintf(stderr, "Invalid function '%s' at character %d.\n", name, srcHead);
		return false;
	}

	return true;
}
