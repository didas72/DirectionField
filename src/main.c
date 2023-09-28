#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

#include "raylib/raylib.h"

//General settings
#define PX_WIDTH 512
#define DSP_RANGE 1.0//3.0
#define MAX_LOCAL_VARS 64

//Vector settings
#define DO_VECTORS
#define VECTOR_STEP 0.05//0.2
#define VECTOR_LENGTH 0.02//0.1
#define FLAT_MARGIN 0.05

//Line settings
//#define DO_CENTRAL_LINES
#define DO_LEFT_EDGE_LINES
#define DO_RIGHT_EDGE_LINES
#define LINE_SPACING 0.035//0.1
#define LINE_STEP 0.001
#define LINE_RANGE_EXTEND 20
#define MAX_DERIV 50



bool GetDerivative(double t, double y, double *ret);
bool GetDerivativeF(char *formula, double t, double y, double *ret);
int TToPx(double spc);
int VToPx(double spc);
void DrawVectors();
void DrawLines();



int main(int argc, char *argv[])
{
	InitWindow(PX_WIDTH, PX_WIDTH, "Direction Field viewer");

	//TODO: Parse formula
	(void)argc;
	(void)argv;

	while (!WindowShouldClose())
	{
		ClearBackground(BLACK);
		BeginDrawing();

		DrawLine(PX_WIDTH / 2, 0, PX_WIDTH / 2, PX_WIDTH, GRAY);
		DrawLine(0, PX_WIDTH / 2, PX_WIDTH, PX_WIDTH / 2, GRAY);

		#ifdef DO_VECTORS
		DrawVectors();
		#endif
		DrawLines();

		EndDrawing();
	}
}



bool GetDerivative(double t, double y, double *ret)
{
	//double result = (t * y) / (1 + t * t); //b)
	//double result = (2 - y) * (y - 1); //c)
	double result = (y + t) / (y - t); //f)

	*ret = result;

	if (isfinite(result))
		return true;

	return false;
}

bool GetDerivativeF(char *formula, double t, double y, double *ret)
{
	// ^ pow(2)
	// { sqrt(1)
	// $ log(1)
	// # abs(1)
	//Maybe factorial

	//Shorthands:
	// ( sin(1)
	// ) cos(1)
	// \ tan(1)

	double vars[MAX_LOCAL_VARS], num;
	int varHead = 0;
	int fHead = 0;

	char ch;
	bool lastWasNum = false, lastWasConstant = false;

	//TODO: Verify valid ops (sqrt(-1), /0, etc)

	while ((ch = formula[fHead++]))
	{
		if (lastWasConstant)
		{
			printf("Cte\n");
			lastWasConstant = false;
			
			switch (ch)
			{
				case 'e':
					vars[varHead] = M_E;
					break;

				case 'P':
					vars[varHead] = M_PI;
					break;

				default:
					break; //TODO: Err
			}
		}

		if (isdigit(ch))
		{
			printf("DIG\n");
			if (!lastWasNum) { num = ch - '0'; }
			else { num *= 10; num += ch - '0'; } //FIXME: Can only handle ints
			continue;
		}
		else lastWasNum = false;

		if (isalpha(ch) && ch != 't' && ch != 'y')
		{
			printf("FUNC\n");
			//TODO: Function name, append
			continue;
		}

		switch (ch)
		{
			case '_':
				lastWasConstant = true;
				break;

			case 't':
				vars[varHead] = t;
				break;

			case 'y':
				vars[varHead] = y;
				break;

			case '<':
				varHead--; //TODO: Error if underflow
				break;

			case '>':
				varHead++; //TODO: Error if overflow
				break;

			case ']':
				vars[varHead + 1] = vars[varHead];
				varHead++; //TODO: Error if overflow
				break;

			case '[':
				vars[varHead - 1] = vars[varHead];
				varHead--; //TODO: Error if overflow
				break;

			case '+':
				vars[varHead] = vars[varHead - 1] + vars[varHead];
				break;

			case '-':
				vars[varHead] = vars[varHead - 1] - vars[varHead];
				break;

			case '*':
				vars[varHead] = vars[varHead - 1] * vars[varHead];
				break;

			case '/':
				if (vars[varHead] == 0) return false;
				vars[varHead] = vars[varHead - 1] / vars[varHead];
				break;

			case '^':
				vars[varHead] = pow(vars[varHead - 1], vars[varHead]);
				break;

			case '{':
				if (vars[varHead] < 0) return false;
				vars[varHead] = sqrt(vars[varHead]);
				break;

			case '$':
				if (vars[varHead] <= 0) return false;
				vars[varHead] = log(vars[varHead]);
				break;

			case '#':
				vars[varHead] = fabs(vars[varHead]);
				break;

			case '(':
				vars[varHead] = sin(vars[varHead]);
				break;

			case ')':
				vars[varHead] = cos(vars[varHead]);
				break;

			case '\\':
				vars[varHead] = tan(vars[varHead]);
				break;

			default:
				printf("ERR: %c\n", ch);
				break;
		}
	}

	*ret = vars[varHead];

	return true;
}



int TToPx(double spc)
{
	return (int)round((PX_WIDTH / 2) + spc * (PX_WIDTH / DSP_RANGE) * 0.5);
}
int VToPx(double spc)
{
	return (int)round((PX_WIDTH / 2) - spc * (PX_WIDTH / DSP_RANGE) * 0.5);
}

void DrawVectors()
{
	for (double t = -DSP_RANGE; t <= DSP_RANGE; t += VECTOR_STEP)
	{
		for (double y = -DSP_RANGE; y <= DSP_RANGE; y += VECTOR_STEP)
		{
			double a, x, v;
			//bool valid = GetDerivative(t, y, &v);
			bool valid = GetDerivativeF("y>t+>y>t-[/", t, y, &v);
			a = atan(v);
			x = cos(a) * VECTOR_LENGTH;
			v = sin(a) * VECTOR_LENGTH;

			int cornerX = TToPx(t-x/2);
			int cornerY = VToPx(y-v/2);
			int tipX = TToPx(t+x);
			int tipY = VToPx(y+v);

			if (valid)
			{
				v *= VECTOR_LENGTH; x *= VECTOR_LENGTH;
				DrawLine(cornerX, cornerY, tipX, tipY, fabs(a) < FLAT_MARGIN ? GRAY : GREEN);
			}
			else
			{
				DrawCircle(cornerX, cornerY, 2.0, RED);
			}
		}
	}
}

void DrawLines()
{
	double curV;

	for (double y = -DSP_RANGE - LINE_RANGE_EXTEND; y <= DSP_RANGE + LINE_RANGE_EXTEND; y += LINE_SPACING)
	{
		#ifdef DO_CENTRAL_LINES
		//Start point is (0, y) to +t
		curV = y;

		for (double t = LINE_STEP; t <= DSP_RANGE + LINE_STEP; t += LINE_STEP)
		{
			double nextV;
			if (!GetDerivative(t - LINE_STEP, curV, &nextV))
				break;
			nextV = nextV * LINE_STEP + curV;
			
			if (fabs(curV) <= DSP_RANGE && fabs(nextV) <= DSP_RANGE)
				DrawLine(TToPx(t - LINE_STEP), VToPx(curV),
					TToPx(t), VToPx(nextV), BLUE);

			curV = nextV;
		}

		//Start point is (0, y) to -t
		curV = y;

		for (double t = 0; t >= -DSP_RANGE - LINE_STEP; t -= LINE_STEP)
		{
			double nextV;
			if (!GetDerivative(t - LINE_STEP, curV, &nextV))
				break;
			nextV = curV - nextV * LINE_STEP;
			
			if (fabs(curV) <= DSP_RANGE && fabs(nextV) <= DSP_RANGE)
				DrawLine(TToPx(t - LINE_STEP), VToPx(nextV),
					TToPx(t), VToPx(curV), BLUE);

			curV = nextV;
		}
		#endif

		#ifdef DO_RIGHT_EDGE_LINES
		//Start point is (DSP_RANGE, y) to 0
		curV = y;

		for (double t = DSP_RANGE + LINE_STEP; t >= 0; t -= LINE_STEP)
		{
			double nextV;
			if (!GetDerivative(t - LINE_STEP, curV, &nextV))
				break;

			if (fabs(nextV) > MAX_DERIV)
				break;			
			nextV = curV - nextV * LINE_STEP;
			
			if (fabs(curV) <= DSP_RANGE && fabs(nextV) <= DSP_RANGE)
				DrawLine(TToPx(t - LINE_STEP), VToPx(nextV),
					TToPx(t), VToPx(curV), ORANGE);

			curV = nextV;
		}
		#endif

		#ifdef DO_LEFT_EDGE_LINES
		//Start  point is (-DSP_RANGE, y) to 0
		curV = y;

		for (double t = -DSP_RANGE - LINE_STEP; t <= 0; t += LINE_STEP)
		{
			double nextV;
			if (!GetDerivative(t - LINE_STEP, curV, &nextV))
				break;

			if (fabs(nextV) > MAX_DERIV)
				break;	
			nextV = nextV * LINE_STEP + curV;
			
			if (fabs(curV) <= DSP_RANGE && fabs(nextV) <= DSP_RANGE)
				DrawLine(TToPx(t - LINE_STEP), VToPx(curV),
					TToPx(t), VToPx(nextV), VIOLET);

			curV = nextV;
		}
		#endif
	}
}
