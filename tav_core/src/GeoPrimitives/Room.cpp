//
//  Room.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 16.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "Room.h"

namespace tav
{
Room::Room(float _width, float _height, float _depth) :
		GeoPrimitive()
{
	width = _width;
	height = _height;
	depth = _depth;
	nrSubDiv = 0;
	init();
}

Room::Room(float _width, float _height, float _depth, int _nrSubDiv) :
		GeoPrimitive()
{
	width = _width;
	height = _height;
	depth = _depth;
	nrSubDiv = _nrSubDiv;
	init();
}

Room::~Room()
{
//        for (int i=0; i<4; i++) delete walls[i];
//        delete walls;
}

void Room::init()
{
	/*
	 // make space
	 walls = new ofPlane*[4];
	 for (int i=0;i<4;i++) walls[i] = new ofPlane(nrSubDiv);
	 
	 // back wall
	 walls[0]->scale(width, height, 1.0f);
	 walls[0]->translate(0.0f, height * 0.5f, depth * -1.0f);
	 
	 // floor
	 walls[1]->scale(width, depth, 1.0f);
	 walls[1]->rotate(-M_PI * 0.5f, 1.0f, 0.0f, 0.0f);
	 walls[1]->translate(0.0f, 0.0f, depth * -0.5f);
	 
	 // wall left
	 walls[2]->scale(depth, height, 1.0f);
	 walls[2]->rotate(-M_PI * 0.5f, 0.0f, 1.0f, 0.0f);
	 walls[2]->translate(width * 0.5f, height * 0.5f, depth * -0.5f);
	 
	 // wall right
	 walls[3]->scale(depth, height, 1.0f);
	 walls[3]->rotate(M_PI * 0.5f, 0.0f, 1.0f, 0.0f);
	 walls[3]->translate(width * -0.5f, height * 0.5f, depth * -0.5f);
	 */
}

void Room::draw(TFO* _tfo)
{
	//for (int i=0;i<4;i++) walls[i]->draw(_tfo);
}

void Room::draw(GLenum _type, TFO* _tfo)
{
	// for (int i=0;i<4;i++) walls[i]->draw(_type, _tfo);
}

void Room::remove()
{
	/*
	 for (int i=0;i<4;i++) {
	 walls[i]->remove();
	 delete walls[i];
	 }
	 delete [] walls;
	 */
}
}
