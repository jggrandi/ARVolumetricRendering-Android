//
//  LookUpTable.h
//  Volume
//
//  Created by Henrique on 11/30/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#ifndef LOOKUPTABLE_H
#define LOOKUPTABLE_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define MINIMUM_VALUE (-1024)
#define MAXIMUM_VALUE 3071

GLubyte m_lookUpTable[4*256];
int m_windowCenter = 45;
int m_windowWidth = 0;

/**
 */
void GetColorMapping(int normalizedValue, unsigned char *r, unsigned char *g, unsigned char *b)
{
	int unnormalizedKeyValueArray[13] = {
		-2048,
		-1024,
		-741,
		-70,
		37,
		42,
		53,
		75,
		98,
		190,
		260,
		1376,
		3071  
	};
	unsigned char colorMappingArray[13][3] = {
		{0, 0, 0},
		{0, 0, 0},
		{70, 28, 19},
		{255, 214, 152},
		{176, 140, 156},
		{128, 42, 31},
		{128, 31, 25},
		{149, 130, 100},
		{255, 255, 255},
		{164, 150, 123},
		{161, 146, 115},
		{161, 146, 115},
		{255, 255, 255}
	};
	int unnormalizedValue,
	i;
	float t;
	
	*r = 0;
	*g = 0;
	*b = 0;

	
	unnormalizedValue = MINIMUM_VALUE + ((float)(normalizedValue)/255.0f)*(float)(MAXIMUM_VALUE - MINIMUM_VALUE);
	
	for (i=0; i<(13 - 1); i++)
		if ((unnormalizedValue >= unnormalizedKeyValueArray[i]) &&(unnormalizedValue < unnormalizedKeyValueArray[i + 1]))
		{
			t = (float)(unnormalizedValue - unnormalizedKeyValueArray[i])/(float)(unnormalizedKeyValueArray[i + 1] - unnormalizedKeyValueArray[i]);
			
			*r = (1.0f - t)*(float)(colorMappingArray[i][0]) + t*(float)(colorMappingArray[i + 1][0]);
			*g = (1.0f - t)*(float)(colorMappingArray[i][1]) + t*(float)(colorMappingArray[i + 1][1]);
			*b = (1.0f - t)*(float)(colorMappingArray[i][2]) + t*(float)(colorMappingArray[i + 1][2]);
		}
}

/**
*/ 
void InitializeLookUpTable(int windowCenter, int windowWidth)
{
	int i;
	float windowCenterNormalized, 
	windowBeginNormalized,
	windowEndNormalized;
	unsigned char level;
	
	windowCenterNormalized = 255.0f*((float)(windowCenter - MINIMUM_VALUE)/(MAXIMUM_VALUE - MINIMUM_VALUE));
	
	windowBeginNormalized = (float)(windowCenterNormalized) - 0.5f*(float)(windowWidth);
	windowEndNormalized = windowBeginNormalized + (float)(windowWidth);
	
	for (i=0; i<256; ++i)
	{
		if (i > windowEndNormalized)
			level = 255;
		else if (i > windowBeginNormalized)
			level = (unsigned char)(255.0f*(((float)(i) - windowBeginNormalized)/(float)(windowWidth)));
		else
			level = 0;
		
		GetColorMapping(i, &m_lookUpTable[4*i + 0], &m_lookUpTable[4*i + 1], &m_lookUpTable[4*i + 2]);
		m_lookUpTable[4*i + 3] = level;
	}
	
	//cout << "Window center: " << windowCenter << endl;
	//cout << "Window width: " << (int)(((float)(windowWidth)/255.0f)*5119.0f) << endl;
}

/**
 @inproceedings{1103929,
 author = {Klaus Engel and Markus Hadwiger and Joe M. Kniss and Aaron E. Lefohn and Christof Rezk Salama and Daniel Weiskopf},
 title = {Real-time volume graphics},
 booktitle = {SIGGRAPH '04: ACM SIGGRAPH 2004 Course Notes},
 year = {2004},
 pages = {29},
 location = {Los Angeles, CA},
 doi = {http://doi.acm.org/10.1145/1103900.1103929},
 publisher = {ACM},
 address = {New York, NY, USA},
 }
*/ 
GLubyte clamp(int value, int minValue, int maxValue)
{
	if (value < minValue)
		return (GLubyte)minValue;
	else if (value > maxValue)
		return (GLubyte)maxValue;
	
	return (GLubyte)value;
}

/**
 @inproceedings{1103929,
 author = {Klaus Engel and Markus Hadwiger and Joe M. Kniss and Aaron E. Lefohn and Christof Rezk Salama and Daniel Weiskopf},
 title = {Real-time volume graphics},
 booktitle = {SIGGRAPH '04: ACM SIGGRAPH 2004 Course Notes},
 year = {2004},
 pages = {29},
 location = {Los Angeles, CA},
 doi = {http://doi.acm.org/10.1145/1103900.1103929},
 publisher = {ACM},
 address = {New York, NY, USA},
 }
*/ 
GLuint createPreintegrationTable(GLubyte* Table) 
{
	double r,
	g,
	b,
	a,
	rInt[256],
	gInt[256],
	bInt[256],
	aInt[256],
	factor,
	tauc;
	int rcol,
	gcol,
	bcol,
	acol,
	smin,
	smax,
	lookupindex;
	GLubyte lookupImg[256*256*4];
	//CTimer timer;
	
	r = 0.0;
	g = 0.0;
	b = 0.0;
	a = 0.0;
	
	rInt[0] = 0.0;
	gInt[0] = 0.0;
	bInt[0] = 0.0;
	aInt[0] = 0.0;
	
	lookupindex = 0;
	
	// compute integral functions
	for (int i=1; i<256; i++)
	{
		tauc = (Table[(i - 1)*4 + 3] + Table[i*4 + 3])/2.0;
		r = r + (Table[(i - 1)*4 + 0] + Table[i*4 + 0])/2.0*tauc/255.0;
		g = g + (Table[(i - 1)*4 + 1] + Table[i*4 + 1])/2.0*tauc/255.0;
		b = b + (Table[(i - 1)*4 + 2] + Table[i*4 + 2])/2.0*tauc/255.0;
		a = a + tauc;
		
		rInt[i] = r;
		gInt[i] = g;
		bInt[i] = b;
		aInt[i] = a;
	}
	
	// compute look-up table from integral functions
	for (int sb=0; sb<256; sb++)
		for (int sf=0; sf<256; sf++)
		{
			if (sb < sf)
			{
				smin = sb;
				smax = sf;
			}
			else
			{
				smin = sf;
				smax = sb;
			}
			
			if (smax != smin)
			{
				factor = 1.0/(double)(smax - smin);
				
				rcol = (rInt[smax] - rInt[smin])*factor;
				gcol = (gInt[smax] - gInt[smin])*factor;
				bcol = (bInt[smax] - bInt[smin])*factor;
				acol = 256.0*(1.0 - exp(-(aInt[smax] - aInt[smin])*factor/255.0));
			}
			else
			{
				factor = 1.0/255.0;
				
				rcol = Table[smin*4+0]*Table[smin*4+3]*factor;
				gcol = Table[smin*4+1]*Table[smin*4+3]*factor;
				bcol = Table[smin*4+2]*Table[smin*4+3]*factor;
				acol = (1.0 - exp(-Table[smin*4 + 3]*factor))*256.0;
			}
			
			lookupImg[lookupindex++] = clamp(rcol, 0, 255);
			lookupImg[lookupindex++] = clamp(gcol, 0, 255);
			lookupImg[lookupindex++] = clamp(bcol, 0, 255);
			lookupImg[lookupindex++] = clamp(acol, 0, 255);
		}
		
	glEnable(GL_TEXTURE_2D);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //GL_REPEAT?
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //GL_REPEAT?
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_LINEAR_MIPMAP_LINEAR?
	glBlendFunc(GL_ONE, GL_SRC_COLOR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, lookupImg);
	glGenerateMipmap(GL_TEXTURE_2D);
	//cout << "Pre-integration table creation elapsed time: " << long(timer.GetElapsed()) << " ms." << endl;
	
	return texture;	
}

#endif // LOOKUPTABLE_H