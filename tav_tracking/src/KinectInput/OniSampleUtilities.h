/*****************************************************************************
 *                                                                            *
 *  OpenNI 2.x Alpha                                                          *
 *  Copyright (C) 2012 PrimeSense Ltd.                                        *
 *                                                                            *
 *  This file is part of OpenNI.                                              *
 *                                                                            *
 *  Licensed under the Apache License, Version 2.0 (the "License");           *
 *  you may not use this file except in compliance with the License.          *
 *  You may obtain a copy of the License at                                   *
 *                                                                            *
 *      http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                            *
 *  Unless required by applicable law or agreed to in writing, software       *
 *  distributed under the License is distributed on an "AS IS" BASIS,         *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 *  See the License for the specific language governing permissions and       *
 *  limitations under the License.                                            *
 *                                                                            *
 *****************************************************************************/
#ifndef _ONI_SAMPLE_UTILITIES_H_
#define _ONI_SAMPLE_UTILITIES_H_

#include <stdio.h>
#include <OpenNI.h>

#ifdef WIN32
#include <conio.h>
int wasKeyboardHit()
{
	return (int)_kbhit();
}

#else // linux

#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
int wasKeyboardHit()
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	// don't echo and don't wait for ENTER
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);

	// make it non-blocking (so we can check without waiting)
	if (0 != fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK))
	{
		return 0;
	}

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	if (0 != fcntl(STDIN_FILENO, F_SETFL, oldf))
	{
		return 0;
	}

	if (ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}

void Sleep(int millisecs)
{
	usleep(millisecs * 1000);
}
#endif // WIN32

void calculateHistogram(float* pHistogram, int histogramSize,
		const openni::VideoFrameRef& frame)
{
	// get a pointer to the actual depth data
	const openni::DepthPixel* pDepth =
			(const openni::DepthPixel*) frame.getData();

	// reset all histogram data
	// each index of the histogram corresponds a depth value
	memset(pHistogram, 0, histogramSize * sizeof(float));

	// Gives the length of one row of pixels in pixels - the width of the frame
	// = if the frame is cropped the size of the crop area
	// if no cropping this is zero
	int restOfRow = frame.getStrideInBytes() / sizeof(openni::DepthPixel)
			- frame.getWidth();
	int height = frame.getHeight();
	int width = frame.getWidth();

	// go through the whole depth data from the device
	// count the number of pixels processed
	unsigned int nNumberOfPoints = 0;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x, ++pDepth)
		{
			if (*pDepth != 0)
			{
				// if the depth value at this position is not black
				// take the depth value as an index and add one to the value at this index
				// e.g. if there are a lot absolut bright values, the value at the index of
				// this brightness value will be high
				pHistogram[*pDepth]++;
				nNumberOfPoints++;
			}
		}
		pDepth += restOfRow;
	}

	// run through all the histogram
	// add the value at [0] to [1]
	// add the value at [1] to [2]
	for (int nIndex = 1; nIndex < histogramSize; nIndex++)
		pHistogram[nIndex] += pHistogram[nIndex - 1];

	// when there was data processed
	// run through the whole histogram
	// normalize
	if (nNumberOfPoints)
		for (int nIndex = 1; nIndex < histogramSize; nIndex++)
			pHistogram[nIndex] = (256
					* (1.0f - (pHistogram[nIndex] / nNumberOfPoints)));
}

#endif // _ONI_SAMPLE_UTILITIES_H_
