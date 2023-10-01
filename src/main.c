#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <errno.h>
#include <time.h>

#include "raylib/raylib.h"

//TODO: Document NOPS
//TODO: Document new instructions
//TODO: Document no negative literals (use negative '~')
//TODO: Verify formula before computing
//TODO: Precompile formula to ints to avoid strcmp and string building
//TODO: More visualization settings via command line
//TODO: MAX filter when scaling down

//Default settings
#define DEFAULT_FORMULA "y>t+>y>t-[/"
#define DEFAULT_PX_WIDTH 512
#define DEFAULT_SAMPLE_POW 0
#define DEFAULT_DRAW_FLAGS DRAW_VECTORS | DRAW_LEFT_EDGE_LINES | DRAW_RIGHT_EDGE_LINES
#define DEFAULT_DSP_RANGE 1.0
#define DEFAULT_PRINT_PERF false

//Formula settings
#define MAX_LOCAL_VARS 64
#define MAX_FORMULA 4096
#define MAX_FUNCTION_NAME 16

//Vector settings
#define DRAW_VECTORS 0b1
#define VECTOR_STEP 0.05
#define VECTOR_LENGTH 0.02
#define UNDEF_RADIUS 2.0
#define FLAT_MARGIN 0.01

//Line settings
#define DRAW_CENTRAL_LINES 0b10
#define DRAW_LEFT_EDGE_LINES 0b100
#define DRAW_RIGHT_EDGE_LINES 0b1000
#define LINE_SPACING 0.035
#define LINE_STEP 0.001
#define LINE_RANGE_EXTEND 20
#define MAX_DERIV 40

//Export settings
#define MAX_PATH 4096



//Settings
char _formula[MAX_FORMULA + 1];
unsigned char _drawFlags;
int _pxWidth;
int _samplePow, _sampleMult;
double _dspRange;
bool printPerf;
char _exportPath[MAX_PATH + 1];

//Other globals
Image renderedImg;
Texture renderedTxt;



int ParseArgs(int argc, char *argv[]);
void WriteUsageMessage();
bool GetDerivative(double t, double y, double *ret);
void GenerateTexture();
void DrawAxis();
void DrawVectors();
void DrawLines();
void PlotResult(double bottom, double top, double spacing, double left, double right, double step, Color color);



int main(int argc, char *argv[])
{
	int ret = ParseArgs(argc, argv);
	if (ret) return ret;

	SetTraceLogLevel(LOG_NONE);
	InitWindow(_pxWidth, _pxWidth, "Direction Field viewer");
	SetTargetFPS(5);

	BeginDrawing();
	DrawText("Generating...", 10, 10, 20, WHITE);
	EndDrawing();

	GenerateTexture();

	Vector2 zero = { .x = 0.0, .y = 0.0 };

	while (!WindowShouldClose())
	{
		BeginDrawing();
		DrawTextureEx(renderedTxt, zero, 0, 1.0/* / _sampleMult*/, WHITE);
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
	_samplePow = DEFAULT_SAMPLE_POW;
	_dspRange = DEFAULT_DSP_RANGE;
	printPerf = DEFAULT_PRINT_PERF;
	strcpy(_formula, DEFAULT_FORMULA);
	strcpy(_exportPath, "");

	static struct option long_options[] = {
		{"help",		no_argument,		NULL, 'h'},
		{"formula",		required_argument,	NULL, 'f'},
		{"display",		required_argument,	NULL, 'd'},
		{"width",		required_argument,	NULL, 'w'},
		{"range",		required_argument,	NULL, 'r'},
		{"performance",	no_argument,		NULL, 'p'},
		{"sampling",	required_argument,	NULL, 's'},
		{"export",		required_argument,	NULL, 'e'},
	};

	while ((opt = getopt_long(argc, argv, "hf:d:w:s:r:pe:", long_options, &optId)) != -1)
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

			case 's':
				int samplePow = strtol(optarg, NULL, 10);
				if (errno || samplePow < 0 || samplePow > 8)
				{
					fprintf(stderr, "Invalid sampling power '%s'. Must be an integer between 0 and 8 inclusive.\n", optarg);
					return -1;
				}
				
				_samplePow = samplePow;
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

			case 'p':
				printPerf = true;
				break;

			case 'e':
				if (strlen(optarg) > MAX_PATH)
				{
					fprintf(stderr, "Export path is too long. Max allowed length is %d.\n", MAX_PATH);
					return -1;
				}
				strcpy(_exportPath, optarg);
				break;

			case '?':
				//getopt_long already wrote error message
				WriteUsageMessage();
				return -1;
		}
	}

	_sampleMult = 1 << _samplePow;

	return 0;
}

void WriteUsageMessage()
{
	printf("CLI usage:\n\tdfv [options]\n\nOptions:\n"
		"\t-h, --help\n\t\tShows usage message.\n"
		"\t-f, --formula <formula>\n\t\tSpecifies the formula to be used during derivative calculation.\n"
		"\t-d, --draw <mode>\n\t\tSpecifies what draw mode to use. Calculate by adding the requested flags: 1=vectors, 2=central, 4=left, 8=right\n"
		"\t-w, --width <width>\n\t\tSpecifies what the window width should be. The window is aways square. Must be between 1 and 4096 inclusive.\n"
		"\t-r, --range <range>\n\t\tSpecifies what number range to use when drawing. Interval will be [-range,range]. Must be between 0.001 and 1000.\n"
		"\t-p, --performance\n\t\tEnables printing of performance metrics.\n"
		"\t-s, --sampling <mult>\n\t\tSpecifies what sampling power to use when rendering. Must be between 0 and 8 inclusive.\n");
}



bool GetDerivative(double t, double y, double *ret)
{
	double vars[MAX_LOCAL_VARS], num = 0, clip = 0;
	char functionName[MAX_FUNCTION_NAME + 1];
	int varHead = 0, formulaHead = 0, funcHead = 0;
	int decimalCounter = -1;

	char ch;
	bool lastWasNum = false, lastWasConstant = false, lastWasFunc = false;

	while ((ch = _formula[formulaHead++]))
	{
		iter:

		if (lastWasConstant)
		{
			lastWasConstant = false;
			
			switch (ch)
			{
				case 'e':
					vars[varHead] = M_E;
					break;

				case 'p':
					vars[varHead] = M_PI;
					break;

				default:
					fprintf(stderr, "Invalid constant '%c'.\n", ch);
					exit(-1);
					break;
			}

			continue;
		}
		else if (lastWasFunc)
		{
			if (isalpha(ch))
			{
				if (funcHead >= MAX_FUNCTION_NAME)
				{
					fprintf(stderr, "Function name too long at character %d.\n", formulaHead);
					return false;
				}
				functionName[funcHead++] = ch;
				functionName[funcHead] = '\0';
				continue;
			}
			
			lastWasFunc = false;
			funcHead = 0;

			if (!strcmp(functionName, "sin"))			vars[varHead] = sin(vars[varHead]);
			else if (!strcmp(functionName, "cos"))		vars[varHead] = cos(vars[varHead]);
			else if (!strcmp(functionName, "tan"))		vars[varHead] = tan(vars[varHead]);
			else if (!strcmp(functionName, "asin"))		vars[varHead] = asin(vars[varHead]);
			else if (!strcmp(functionName, "acos"))		vars[varHead] = acos(vars[varHead]);
			else if (!strcmp(functionName, "atan"))		vars[varHead] = atan(vars[varHead]);
			else if (!strcmp(functionName, "sinh"))		vars[varHead] = sinh(vars[varHead]);
			else if (!strcmp(functionName, "cosh"))		vars[varHead] = cosh(vars[varHead]);
			else if (!strcmp(functionName, "tanh"))		vars[varHead] = tanh(vars[varHead]);
			else if (!strcmp(functionName, "asinh"))	vars[varHead] = asinh(vars[varHead]);
			else if (!strcmp(functionName, "acosh"))	vars[varHead] = acosh(vars[varHead]);
			else if (!strcmp(functionName, "atanh"))	vars[varHead] = atanh(vars[varHead]);
			else if (!strcmp(functionName, "pow"))
			{
				if (varHead <= 0) fprintf(stderr, "BufferHead underflow at character %d.\n", formulaHead);
				vars[varHead] = pow(vars[varHead - 1], vars[varHead]);
			}
			else if (!strcmp(functionName, "abs"))		vars[varHead] = fabs(vars[varHead]);
			else if (!strcmp(functionName, "ln"))		vars[varHead] = log(vars[varHead]);
			else if (!strcmp(functionName, "log"))		vars[varHead] = log10(vars[varHead]);
			else if (!strcmp(functionName, "log2"))		vars[varHead] = log2(vars[varHead]);
			else if (!strcmp(functionName, "sign"))		vars[varHead] = vars[varHead] == 0.0 ? 0.0 : (vars[varHead] > 0.0 ? 1.0 : -1.0);
			else if (!strcmp(functionName, "ceil"))		vars[varHead] = ceil(vars[varHead]);
			else if (!strcmp(functionName, "floor"))	vars[varHead] = floor(vars[varHead]);
			else if (!strcmp(functionName, "round"))	vars[varHead] = round(vars[varHead]);
			else
			{
				fprintf(stderr, "Invalid function: '%s'.\n", functionName);
				return false;
			}

			if (vars[varHead] != vars[varHead] || isinf(vars[varHead])) //Check if nan or +-infinity
				return false;

			//Flow through
		}

		if (isdigit(ch))
		{
			if (!lastWasNum) { num = ch - '0'; }
			else { num *= 10; num += ch - '0'; }
			if (decimalCounter != -1) decimalCounter++;
			lastWasNum = true;
			continue;
		}
		else if (ch == '.' && lastWasNum)
		{
			decimalCounter = 0;
			continue;
		}
		else if (lastWasNum)
		{
			if (decimalCounter != -1) while(decimalCounter--) num /= 10.0;

			vars[varHead] = num;
			lastWasNum = false;
			decimalCounter = -1;
		}

		switch (ch)
		{
			//Used chars: ._,;<>[]+*-/^{*#()\%		avoid using ! and " because of shell character escaping
			//Free chars: &'=?@`'|~

			case ' '://NOP
				break;

			case '_':
				lastWasConstant = true;
				break;

			case '=':
				lastWasFunc = true;
				break;

			case ',':
				clip = vars[varHead];
				break;

			case ';':
				vars[varHead] = clip;
				break;

			case 't':
				vars[varHead] = t;
				break;

			case 'y':
				vars[varHead] = y;
				break;

			case '<':
				varHead--;
				if (varHead < 0) { fprintf(stderr, "BufferHead underflow at character %d.\n", formulaHead); return false; }
				break;

			case '>':
				varHead++;
				if (varHead >= MAX_LOCAL_VARS) { fprintf(stderr, "BufferHead overflow at character %d.\n", formulaHead); return false; }
				break;

			case '[':
				if (varHead < 0) { fprintf(stderr, "BufferHead underflow at character %d.\n", formulaHead); return false; }
				vars[varHead - 1] = vars[varHead];
				varHead--;
				break;

			case ']':
				if (varHead + 1 >= MAX_LOCAL_VARS) { fprintf(stderr, "BufferHead overflow at character %d.\n", formulaHead); return false; }
				vars[varHead + 1] = vars[varHead];
				varHead++;
				break;

			case '+':
				if (varHead < 0) { fprintf(stderr, "BufferHead underflow at character %d.\n", formulaHead); return false; }
				vars[varHead] = vars[varHead - 1] + vars[varHead];
				break;

			case '-':
				if (varHead < 0) { fprintf(stderr, "BufferHead underflow at character %d.\n", formulaHead); return false; }
				vars[varHead] = vars[varHead - 1] - vars[varHead];
				break;

			case '*':
				if (varHead < 0) { fprintf(stderr, "BufferHead underflow at character %d.\n", formulaHead); return false; }
				vars[varHead] = vars[varHead - 1] * vars[varHead];
				break;

			case '/':
				if (varHead < 0) { fprintf(stderr, "BufferHead underflow at character %d.\n", formulaHead); return false; }
				if (vars[varHead] == 0) return false;
				vars[varHead] = vars[varHead - 1] / vars[varHead];
				break;

			case '%':
				if (varHead < 0) { fprintf(stderr, "BufferHead underflow at character %d.\n", formulaHead); return false; }
				if (vars[varHead] == 0) return false;
				vars[varHead] = fmod(vars[varHead - 1], vars[varHead]);
				break;

			case '^':
				if (varHead < 0) { fprintf(stderr, "BufferHead underflow at character %d.\n", formulaHead); return false; }
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
				printf("Invalid operation: '%c'.\n", ch);
				return false;
		}
	}

	if (lastWasNum || lastWasConstant || lastWasFunc)
	{
		ch = ' ';
		formulaHead--;

		goto iter;
	}

	*ret = vars[varHead];

	return true;
}



int TToPx(double spc)
{
	return (int)round((_pxWidth * _sampleMult / 2) + spc * (_pxWidth * _sampleMult / _dspRange) * 0.5);
}
int VToPx(double spc)
{
	return (int)round((_pxWidth * _sampleMult / 2) - spc * (_pxWidth * _sampleMult / _dspRange) * 0.5);
}



void GenerateTexture()
{
	renderedImg = GenImageColor(_pxWidth * _sampleMult, _pxWidth * _sampleMult, BLACK);

	clock_t start = clock(), diff;
	DrawAxis();
	DrawVectors();
	DrawLines();
	diff = clock() - start;

	if (strlen(_exportPath)) ExportImage(renderedImg, _exportPath);

	//Scale back down if super sampled
	if (_samplePow)
	{
		//Apply some corrections
		ImageColorBrightness(&renderedImg, +64);
		ImageBlurGaussian(&renderedImg, _samplePow);
		ImageResize(&renderedImg, _pxWidth, _pxWidth);
		ImageColorContrast(&renderedImg, 30);
	}
	
	renderedTxt = LoadTextureFromImage(renderedImg);
	UnloadImage(renderedImg);

	if (printPerf) printf("Total time elapsed: %.2fms.\n", diff * 1000.0 / CLOCKS_PER_SEC);
}
void DrawAxis()
{
	ImageDrawLine(&renderedImg, _pxWidth * _sampleMult / 2, 0, _pxWidth * _sampleMult / 2, _pxWidth * _sampleMult, GRAY);
	ImageDrawLine(&renderedImg, 0, _pxWidth * _sampleMult / 2, _pxWidth * _sampleMult, _pxWidth * _sampleMult / 2, GRAY);
}
void DrawVectors()
{
	if (!(_drawFlags & DRAW_VECTORS))
		return;

	clock_t start = clock(), diff;

	for (double t = -_dspRange; t <= _dspRange; t += VECTOR_STEP)
	{
		for (double y = -_dspRange; y <= _dspRange; y += VECTOR_STEP)
		{
			double a, x, v = 0;
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
				ImageDrawLine(&renderedImg, cornerX, cornerY, tipX, tipY, fabs(a) < FLAT_MARGIN ? GRAY : GREEN);
			}
			else
			{
				ImageDrawCircle(&renderedImg, cornerX, cornerY, UNDEF_RADIUS, RED);
			}
		}
	}

	diff = clock() - start;
	if (printPerf) printf("Vectors time elapsed: %.2fms.\n", diff * 1000.0 / CLOCKS_PER_SEC);
}
void DrawLines()
{
	clock_t start = clock(), diff;

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

	diff = clock() - start;
	if (printPerf) printf("Lines time elapsed: %.2fms.\n", diff * 1000.0 / CLOCKS_PER_SEC);
}



void PlotResult(double bottom, double top, double spacing, double start, double end, double step, Color color)
{
	bool leftToRight = start < end;
	double s = step * (leftToRight ? 1 : -1) / _sampleMult;

	for (double y = bottom; y <= top; y += spacing)
	{
		double curV = y;

		for (double t = start; leftToRight ? t <= end : t >= end; t += s)
		{
			double nextV = 0;
			if (!GetDerivative(t - s, curV, &nextV))
				break;

			//derivative limiter
			if (fabs(nextV) > MAX_DERIV)
					break;	
			nextV = nextV * s + curV;
			
			if (fabs(curV) <= _dspRange && fabs(nextV) <= _dspRange)
				ImageDrawLine(&renderedImg, TToPx(t - s), VToPx(curV),
					TToPx(t), VToPx(nextV), color);

			curV = nextV;
		}
	}
}
