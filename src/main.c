#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <errno.h>

#include "raylib/raylib.h"

//Default settings
#define DEFAULT_FORMULA "y>t+>y>t-[/"
#define DEFAULT_PX_WIDTH 512
#define DEFAULT_DRAW_FLAGS DRAW_VECTORS | DRAW_LEFT_EDGE_LINES | DRAW_RIGHT_EDGE_LINES
#define DEFAULT_DSP_RANGE 1.0

//Formulas
#define MAX_LOCAL_VARS 64
#define MAX_FORMULA 4096

//Vector settings
#define DRAW_VECTORS 0b1
#define VECTOR_STEP 0.05
#define VECTOR_LENGTH 0.02
#define UNDEF_RADIUS 2.0
#define FLAT_MARGIN 0.05

//Line settings
#define DRAW_CENTRAL_LINES 0b10
#define DRAW_LEFT_EDGE_LINES 0b100
#define DRAW_RIGHT_EDGE_LINES 0b1000
#define LINE_SPACING 0.035
#define LINE_STEP 0.001
#define LINE_RANGE_EXTEND 20
#define MAX_DERIV 50



//Settings
char _formula[MAX_FORMULA + 1];
unsigned char _drawFlags;
int _pxWidth;
double _dspRange;



int ParseArgs(int argc, char *argv[]);
void WriteUsageMessage();
bool GetDerivative(double t, double y, double *ret);
bool GetDerivativeF(char *formula, double t, double y, double *ret);
int TToPx(double spc);
int VToPx(double spc);
void DrawVectors();
void DrawLines();
void PlotResult(double bottom, double top, double spacing, double left, double right, double step, Color color);



int main(int argc, char *argv[])
{
	int ret = ParseArgs(argc, argv);
	if (ret) return ret;

	SetTraceLogLevel(LOG_NONE);
	InitWindow(_pxWidth, _pxWidth, "Direction Field viewer");

	while (!WindowShouldClose())
	{
		ClearBackground(BLACK);
		BeginDrawing();

		DrawLine(_pxWidth / 2, 0, _pxWidth / 2, _pxWidth, GRAY);
		DrawLine(0, _pxWidth / 2, _pxWidth, _pxWidth / 2, GRAY);

		DrawVectors();
		DrawLines();

		EndDrawing();
	}

	CloseWindow();
}

int ParseArgs(int argc, char *argv[])
{
	int opt, optId;

	//Init defaults
	_drawFlags = DEFAULT_DRAW_FLAGS;
	_pxWidth = DEFAULT_PX_WIDTH;
	_dspRange = DEFAULT_DSP_RANGE;
	strcpy(_formula, DEFAULT_FORMULA);

	static struct option long_options[] = {
		{"help",	no_argument,		NULL, 'h'},
		{"formula",	required_argument,	NULL, 'f'},
		{"display",	required_argument,	NULL, 'd'},
		{"width",	required_argument,	NULL, 'w'},
		{"range",	required_argument,	NULL, 'r'}
	};

	//TODO: Variable vector spacing (VECTOR_STEP)
	//TODO: Variable line spacing (LINE_SPACING)
	//TODO: Variables for other macros

	while ((opt = getopt_long(argc, argv, "hf:d:w:r:", long_options, &optId)) != -1)
	{
		switch(opt)
		{
			case 'h':
				WriteUsageMessage();
				break;

			case 'f':
				if (strlen(optarg) > MAX_FORMULA)
				{
					fprintf(stderr, "Formula is too long. Max allowed length is %d.\n", MAX_FORMULA);
					return -1;
				}
				strcpy(_formula, optarg);
				printf("Loaded formula '%s'.\n", _formula);
				break;

			case 'd':
				int drawFlags = strtol(optarg, NULL, 10);
				if (errno || drawFlags < 0 || drawFlags > 15)
				{
					fprintf(stderr, "Invalid draw flags '%s'. Must be an integer between 0 and 15 inclusive.\n", optarg);
					return -1;
				}
				
				_drawFlags = drawFlags;
				break;

			case 'w':
				int pxWidth = strtol(optarg, NULL, 10);
				if (errno || pxWidth < 1 || pxWidth > 4096)
				{
					fprintf(stderr, "Invalid width '%s'. Must be an integer between 1 and 4096 inclusive.\n", optarg);
					return -1;
				}
				
				_pxWidth = pxWidth;
				break;

			case 'r':
				double dspRange = strtod(optarg, NULL);
				if (errno || dspRange < 0.001 || dspRange > 1000)
				{
					fprintf(stderr, "Invalid display range '%s'. Must be an number between 0.001 and 1000 inclusive.\n", optarg);
					return -1;
				}

				_dspRange = dspRange;
				break;

			case '?':
				//getopt_long already wrote error message
				WriteUsageMessage();
				return -1;
		}
	}

	return 0;
}

void WriteUsageMessage()
{
	printf("CLI usage:\n\tdfv [options]\n\nOptions:\n"
		"\t-h, --help\n\t\tShows usage message.\n"
		"\t-f, --formula <formula>\n\t\tSpecifies the formula to be used during derivative calculation.\n"
		"\t-d, --draw <mode>\n\t\tSpecifies what draw mode to use. Calculate by adding the requested flags: 1=vectors, 2=central, 4=left, 8=right\n"
		"\t-w, --width <width>\n\t\tSpecifies what the window width should be. The window is aways square. Must be between 1 and 4096 inclusive.\n"
		"\t-r, --range <range>\n\t\tSpecifies what number range to use when drawing. Interval will be [-range,range]. Must be between 0.001 and 1000.\n");
}



bool GetDerivative(double t, double y, double *ret)
{
	/*//double result = (t * y) / (1 + t * t); //b)
	//double result = (2 - y) * (y - 1); //c)
	double result = (y + t) / (y - t); //f)

	*ret = result;

	if (isfinite(result))
		return true;

	return false;*/

	return GetDerivativeF(_formula, t, y, ret);
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
			if (!lastWasNum) { num = ch - '0'; }
			else { num *= 10; num += ch - '0'; } //FIXME: Can only handle ints
			continue;
		}
		else lastWasNum = false;

		if (isalpha(ch) && ch != 't' && ch != 'y')
		{
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
	return (int)round((_pxWidth / 2) + spc * (_pxWidth / _dspRange) * 0.5);
}
int VToPx(double spc)
{
	return (int)round((_pxWidth / 2) - spc * (_pxWidth / _dspRange) * 0.5);
}

void DrawVectors()
{
	if (!(_drawFlags & DRAW_VECTORS))
		return;

	for (double t = -_dspRange; t <= _dspRange; t += VECTOR_STEP)
	{
		for (double y = -_dspRange; y <= _dspRange; y += VECTOR_STEP)
		{
			double a, x, v;
			bool valid = GetDerivative(t, y, &v);
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
				DrawCircle(cornerX, cornerY, UNDEF_RADIUS, RED);
			}
		}
	}
}

void DrawLines()
{
	if (_drawFlags & DRAW_CENTRAL_LINES)
	{
		PlotResult(-_dspRange - LINE_RANGE_EXTEND, _dspRange + LINE_RANGE_EXTEND, LINE_SPACING,
			LINE_STEP, _dspRange + LINE_STEP, LINE_STEP, SKYBLUE);
		PlotResult(-_dspRange - LINE_RANGE_EXTEND, _dspRange + LINE_RANGE_EXTEND, LINE_SPACING,
			0, -_dspRange - LINE_STEP, LINE_STEP, SKYBLUE);
	}
	if (_drawFlags & DRAW_RIGHT_EDGE_LINES)
		PlotResult(-_dspRange - LINE_RANGE_EXTEND, _dspRange + LINE_RANGE_EXTEND, LINE_SPACING,
			_dspRange + LINE_STEP, 0, LINE_STEP, ORANGE);
	if (_drawFlags & DRAW_LEFT_EDGE_LINES)
		PlotResult(-_dspRange - LINE_RANGE_EXTEND, _dspRange + LINE_RANGE_EXTEND, LINE_SPACING,
			-_dspRange - LINE_STEP, 0, LINE_STEP, VIOLET);

	return;
}

void PlotResult(double bottom, double top, double spacing, double start, double end, double step, Color color)
{
	bool leftToRight = start < end;
	double s = step * (leftToRight ? 1 : -1);

	for (double y = bottom; y <= top; y += spacing)
	{
		double curV = y;

		for (double t = start; leftToRight ? t <= end : t >= end; t += s)
		{
			double nextV;
			if (!GetDerivative(t - s, curV, &nextV))
				break;

			//derivative limiter
			if (fabs(nextV) > MAX_DERIV)
					break;	
			nextV = nextV * s + curV;
			
			if (fabs(curV) <= _dspRange && fabs(nextV) <= _dspRange)
				DrawLine(TToPx(t - s), VToPx(curV),
					TToPx(t), VToPx(nextV), color);

			curV = nextV;
		}
	}
}
