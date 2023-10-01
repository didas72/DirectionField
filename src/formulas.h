//formulas.h - 

#ifndef FORMULAS_H
#define FORMULAS_H

#include <stdbool.h>
#include <stdint.h>

//Constants
#define MAX_FUNCTION_NAME 16
#define MAX_BUFFERS 64

//Operations
#define FORMULA_NOP				0
#define FORMULA_LITERAL			1
#define FORMULA_CONSTANT		2
#define FORMULA_CLIP_WRITE		3
#define FORMULA_CLIP_READ		4
#define FORMULA_SEEK_LEFT		5
#define FORMULA_SEEK_RIGHT		6
#define FORMULA_COPY_LEFT		7
#define FORMULA_COPY_RIGHT		8
//TODO: Implement below in EvaluateFormula
#define FORMULA_ADD				9
#define FORMULA_SUBTRACT		10
#define FORMULA_MULTIPLY		11
#define FORMULA_DIVIDE			12
#define FORMULA_REMAINDER		13
#define FORMULA_POW				14
#define FORMULA_SQUARE			15	//Not documented yet
#define FORMULA_SQRT			16
#define FORMULA_LOGN			17
#define FORMULA_LOG10			18
#define FORMULA_LOG2			19
#define FORMULA_ABS				20
#define FORMULA_SIN				21
#define FORMULA_COS				22
#define FORMULA_TAN				23
#define FORMULA_ASIN			24
#define FORMULA_ACOS			25
#define FORMULA_ATAN			26
#define FORMULA_SINH			27
#define FORMULA_COSH			28
#define FORMULA_TANH			29
#define FORMULA_ASINH			30
#define FORMULA_ACOSH			31
#define FORMULA_ATANH			32
#define FORMULA_SIGN			33
#define FORMULA_CEIL			34
#define FORMULA_FLOOR			35
#define FORMULA_ROUND			36
#define FORMULA_NEGATIVE		37	//Not documented yet (A *= -1)
#define FORMULA_VAR_BASE		0x1000
#define FORMULA_VAR_TOP			FORMULA_VAR_BASE + 'z' - 'a'
#define FORMULA_RET				~0ul


typedef struct 
{
	char name;
	double value;
} FormulaVariable;


bool CompileFormula(const char *src, uint64_t *store);

bool EvaluateFormula(const uint64_t *src, FormulaVariable *variables, size_t variableC, double *ret);

#endif
