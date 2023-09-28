#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "raylib/raylib.h"

#define PX_WIDTH 512
#define DSP_RANGE 1.0
#define VECTOR_STEP 0.05
#define VECTOR_LENGTH 0.02

#define LINE_SPACING 0.03
#define LINE_STEP 0.002
#define LINE_RANGE_EXTEND 20
#define MAX_DERIV 20

#define FLAT_MARGIN 0.05

bool GetDerivative(double t, double y, double *ret);
int TToPx(double spc);
int VToPx(double spc);
void DrawVectors();
void DrawLines();

int main(int argc, char *argv[])
{
	InitWindow(PX_WIDTH, PX_WIDTH, "Direction Field viewer");

	//TODO: Parse formula

	while (!WindowShouldClose())
	{
		ClearBackground(BLACK);
		BeginDrawing();

		DrawLine(PX_WIDTH / 2, 0, PX_WIDTH / 2, PX_WIDTH, GRAY);
		DrawLine(0, PX_WIDTH / 2, PX_WIDTH, PX_WIDTH / 2, GRAY);

		DrawVectors();
		DrawLines();

		EndDrawing();
	}
}

bool GetDerivative(double t, double y, double *ret)
{
	double result = (y + t) / (y - t);

	*ret = result;

	if (isfinite(result))
		return true;

	return false;
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
				DrawCircle(cornerX, cornerY, 2.0, RED);
			}
		}
	}
}

void DrawLines()
{
	for (double y = -DSP_RANGE - LINE_RANGE_EXTEND; y <= DSP_RANGE + LINE_RANGE_EXTEND; y += LINE_SPACING)
	{
		//Start point is (0, y) to +t
		double curV = y;

		/*
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
		*/

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
	}
}
