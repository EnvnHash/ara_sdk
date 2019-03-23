//
//  RootWidget.cpp
//  tav_core
//
//  Created by Sven Hahne on 29/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "RootWidget.h"

#define STRINGIFY(A) #A

namespace tav
{
RootWidget::RootWidget(ShaderCollector* _shCol, int _scrWidth, int _scrHeight,
		int _nrViewports) :
		scrWidth(_scrWidth), scrHeight(_scrHeight), shCol(_shCol), actEvent(
				NONE), maxNrMultTouch(5), guiHasChanged(true), inited(false), nrViewports(
				_nrViewports)
{
	actCursorPos = new glm::vec2[maxNrMultTouch];
	screenSize = glm::vec2(static_cast<float>(scrWidth),
			static_cast<float>(scrHeight));

	// hier muesste irgendwie from gui die viewports zu ändern sein...
	rootViewport = glm::vec4(0.f, 0.f, screenSize.x, screenSize.y);

	// der gesamte screen, können auch mehrere screens seine -> rendern per geoshader
	viewports.push_back(&rootViewport);

	rootMat = glm::mat4(1.f);
}

//--------------------------------------------------

void RootWidget::add(Widget* widg)
{
	widgets.push_back(widg);
}

//--------------------------------------------------

void RootWidget::init()
{
	if (static_cast<int>(widgets.size()) > 0)
	{
		addColShader(nrViewports);
		addTexShader(nrViewports);
		addObjIdShader(nrViewports);
		addTouchRecShader();

		objIdFbo = new FBO(shCol, scrWidth, scrHeight, GL_R16F, GL_TEXTURE_2D,
				false, 1, 1, 1, GL_REPEAT, false);

		// setup a vao with the number of touches
		GLfloat initData[maxNrMultTouch * 3];
		for (int i = 0; i < maxNrMultTouch * 3; i++)
			initData[i] = 0.f;

		ptrVao = new VAO("position:3f", GL_DYNAMIC_DRAW);
		ptrVao->initData(maxNrMultTouch, initData);

		// setup tfo to record the touch feedback
		std::vector<std::string> recAttribNames;
		recAttribNames.push_back(stdRecAttribNames[0]);

		ptrFdbk = new TFO(maxNrMultTouch, recAttribNames);
		ptrFdbk->setVaryingsToRecord(&recAttribNames,
				touchRecShader->getProgram());
		touchRecShader->link();

		inited = true;
	}

	// loop through all widgets and assign unique ids
	// collect the widgets as a map
	int idCnt = 1;
	for (std::vector<Widget*>::iterator it = widgets.begin();
			it != widgets.end(); ++it)
	{
		widgMap[idCnt] = (*it);
		(*it)->setId(&idCnt);
		(*it)->init(&animUpdtQ, &animUpdtQToKill);
	}
}

//--------------------------------------------------

void RootWidget::draw()
{
	if (inited)
	{
		glDisable(GL_DEPTH_TEST);

		matrixStack.clear();
		matrixStack.push_back(&rootMat);

		// draw all widgets
		for (std::vector<Widget*>::iterator it = widgets.begin();
				it != widgets.end(); ++it)
		{
			(*it)->draw(&viewports, &matrixStack, guiColShader, guiTexShader);

			// delete last added matrix from the stack;
			matrixStack.pop_back();
		}

		// reset stack
		matrixStack.clear();
		matrixStack.push_back(&rootMat);

		// draw again for objId Map if gui has changed
		if (guiHasChanged)
		{
			objIdFbo->bind();

			objIdShader->begin();
			objIdShader->setIdentMatrix4fv("m_pvm");

			for (std::vector<Widget*>::iterator it = widgets.begin();
					it != widgets.end(); ++it)
			{
				(*it)->draw(&viewports, &matrixStack, objIdShader);

				// delete last added viewports from the stack;
				matrixStack.pop_back();
			}

			objIdFbo->unbind();

			guiHasChanged = false;
		}
	}
}

//--------------------------------------------------

void RootWidget::update(double time, double dt)
{
	if (inited)
	{
		// process action Queue
		for (std::vector<widgetEvent>::iterator it = reqActionQ.begin();
				it != reqActionQ.end(); ++it)
		{
			switch ((*it))
			{
			case LEFT_CLICK:
				getObjFromMap(1);
				break;
			case RIGHT_CLICK:
				getObjFromMap(1);
				break;
			case SWIPE_RIGHT:
				getObjFromMap(1);
				break;
			case SWIPE_LEFT:
				getObjFromMap(1);
				break;
			case SWIPE_UP:
				getObjFromMap(1);
				break;
			case SWIPE_DOWN:
				getObjFromMap(1);
				break;
			default:
				break;
			}
		}

		reqActionQ.clear();

		// update guianimation of the GUIObjects
		for (std::vector<GUIAnimation*>::const_iterator it = animUpdtQ.begin();
				it != animUpdtQ.end(); ++it)
			(*it)->update(time, dt);

		// erase all animation that were pushed to be killed
		for (std::vector<std::vector<GUIAnimation*>::iterator>::iterator it =
				animUpdtQToKill.begin(); it != animUpdtQToKill.end(); ++it)
			animUpdtQ.erase(*it);

		animUpdtQToKill.clear();
	}
}

//--------------------------------------------------
// get corresponding GUIObjects from map

void RootWidget::getObjFromMap(uint8_t nrPoints)
{
	// write the actual mouse position to the vao
	GLfloat* ptr = (GLfloat*) ptrVao->getMapBuffer(POSITION);

	for (uint8_t i = 0; i < nrPoints; i++)
	{
		ptr[i * 3] = static_cast<float>(actCursorPos[i].x) / screenSize.x * 2.f
				- 1.f;
		ptr[i * 3 + 1] = (1.f
				- static_cast<float>(actCursorPos[i].y) / screenSize.y) * 2.f
				- 1.f;

//            printf("getObjFromMap at scrHeight: %f scrWidth: %f %f %f \n", screenSize.x, screenSize.y,
//                   static_cast<float>(actCursorPos[i].x) / screenSize.x * 2.f - 1.f,
//                   (1.f - static_cast<float>(actCursorPos[i].y) / screenSize.y) * 2.f - 1.f);
	}

	ptrVao->unMapBuffer();

	glEnable(GL_RASTERIZER_DISCARD);

	//check primitives written
//        GLuint* ids = new GLuint[1];
//        glGenQueries(1, ids);
//        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, ids[0]);

	touchRecShader->begin();
	touchRecShader->setIdentMatrix4fv("m_pvm");
	touchRecShader->setUniform1i("idMap", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, objIdFbo->getColorImg());

	ptrFdbk->bind();
	ptrFdbk->begin(GL_POINTS);

	// zeichne den aktuellen tfo, der beim nächsten update als quelle dranhängt
	ptrVao->draw(GL_POINTS);

	ptrFdbk->end();
	ptrFdbk->unbind();

	glDisable(GL_RASTERIZER_DISCARD);

//        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
//        GLuint count;
//        glGetQueryObjectuiv(ids[0], GL_QUERY_RESULT, &count);
//        printf("written primitives: %d \n", count);

	// read the touches
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, ptrFdbk->getTFOBuf(POSITION));

	// get a pointer to the buffer
	GLfloat* readPtr = (GLfloat*) glMapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,
			GL_READ_ONLY);

	// loop through the touches, look for the corresponding objects and
	// call actions
	for (uint8_t i = 0; i < nrPoints; i++)
	{
		int objId = static_cast<int>(readPtr[i * 4]);
		int lastInd = 0;
		Widget* searchWidg = 0;

		// loop through all child widgets
		if (static_cast<int>(widgMap.size()) > 1)
		{
			for (std::map<int, Widget*>::iterator it = widgMap.begin();
					it != widgMap.end(); ++it)
			{
				if ((*it).first > objId && lastInd <= objId)
					break;

				lastInd = (*it).first;
				searchWidg = (*it).second;
			}
		}
		else
		{
			lastInd = (*widgMap.begin()).first;
			searchWidg = (*widgMap.begin()).second;
		}

		// if a widget was found for the search (has to) search the guiobjects
		if (lastInd != 0 && searchWidg->getId() != objId
				&& searchWidg->getId() <= objId)
		{
			lastInd = 0;
			GUIObject* dstGuiObj = 0;
			if (searchWidg)
			{
				if (static_cast<int>(searchWidg->getGuiMap()->size()) > 1)
				{
					for (std::map<int, GUIObject*>::iterator it =
							searchWidg->getGuiMap()->begin();
							it != searchWidg->getGuiMap()->end(); ++it)
					{
						if ((*it).first > objId && lastInd <= objId)
							break;

						lastInd = (*it).first;
						dstGuiObj = (*it).second;
					}
				}
				else
				{
					dstGuiObj = (*searchWidg->getGuiMap()->begin()).second;
				}

				dstGuiObj->callAction(reqActionQ.back());
			}
		}
		else
		{
			// call widget action
			//std::cout << "found widget with id: " << searchWidg->getId() << std::endl;

		}
	}

	glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

//        glDeleteQueries(1, ids);
}

//--------------------------------------------------

void RootWidget::onCursor(double xpos, double ypos)
{
	actCursorPos[0].x = xpos;
	actCursorPos[0].y = ypos;
}

//--------------------------------------------------

void RootWidget::onMouseButton(int button, int action, int mods, double xPos,
		double yPos)
{
	onCursor(xPos, yPos);

	if (action == GLFW_PRESS)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			actionQ.push_back(std::make_pair(glfwGetTime(), LEFT_PRESS));

		// use right button to simulate swipe
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
			actionQ.push_back(std::make_pair(glfwGetTime(), SWIPE_RIGHT));
	}

	if (action == GLFW_RELEASE)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			if (static_cast<int>(actionQ.size()) > 0
					&& actionQ.back().second == LEFT_PRESS)
			{
				actionQ.pop_back();
				if (int(reqActionQ.size()) == 0)
					reqActionQ.push_back(LEFT_CLICK);
			}

		if (button == GLFW_MOUSE_BUTTON_RIGHT)
			if (static_cast<int>(actionQ.size()) > 0
					&& actionQ.back().second == SWIPE_RIGHT)
			{
				actionQ.pop_back();
				if (int(reqActionQ.size()) == 0)
					reqActionQ.push_back(SWIPE_RIGHT);
			}
	}
}

//--------------------------------------------------

void RootWidget::onKey(int key, int scancode, int action, int mods)
{
}

//--------------------------------------------------

void RootWidget::setCmd(double xpos, double ypos, widgetEvent _event)
{
	actCursorPos[0].x = xpos;
	actCursorPos[0].y = ypos;
	reqActionQ.push_back(_event);
}

//--------------------------------------------------

void RootWidget::addColShader(int _nrViewports)
{
	std::cout << " RootWidget::addColShader: _nrViewports " << _nrViewports
			<< std::endl;

	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout(location=0) in vec4 position; layout(location=1) in vec4 normal; layout(location=2) in vec2 texCoord; layout(location=3) in vec4 color; void main() { gl_Position = position; });
	vert = "// basic gui color shader, vert\n" + shdr_Header + vert;

	std::string geom =
			STRINGIFY(
					uniform mat4 m_pvm; void main(void) { gl_ViewportIndex = gl_InvocationID; for (int i=0;i<gl_in.length();i++) { gl_Position = m_pvm * gl_in[i].gl_Position; EmitVertex(); } EndPrimitive(); });

	std::string gHeader = "// basic gui color shader, geom\n";
	gHeader += "#version 410\n";
	gHeader += "layout(triangles, invocations=" + std::to_string(_nrViewports)
			+ ") in;\n";
	gHeader += "layout(triangle_strip, max_vertices=3) out;\n";
	// gHeader += "uniform mat4 m_pvm[" +std::to_string(_nrViewports)+ "];\n";
	geom = gHeader + geom;

	std::string frag =
			STRINGIFY(
					uniform sampler2D tex; uniform vec4 col; layout (location = 0) out vec4 color; void main() { color = col; });

	frag = "// basic gui color shader, frag\n" + shdr_Header + frag;

	guiColShader = shCol->addCheckShaderText("guiColShdr", vert.c_str(),
			geom.c_str(), frag.c_str());
}

//--------------------------------------------------

void RootWidget::addTexShader(int _nrViewports)
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; out vec2 tex_coord;

					void main() { tex_coord = texCoord; gl_Position = position; });

	vert = "// basic gui texture shader, vert\n" + shdr_Header + vert;

	std::string geom =
			STRINGIFY(
					in vec2 tex_coord[]; uniform mat4 m_pvm; out vec2 gs_tex_coord; void main(void) { gl_ViewportIndex = gl_InvocationID; for (int i=0;i<gl_in.length();i++) { gs_tex_coord = tex_coord[i]; gl_Position = m_pvm * gl_in[i].gl_Position; EmitVertex(); } EndPrimitive(); });

	std::string gHeader = "// basic gui texture shader, geom\n";
	gHeader += "#version 410\n";
	gHeader += "layout(triangles, invocations=" + std::to_string(_nrViewports)
			+ ") in;\n";
	gHeader += "layout(triangle_strip, max_vertices=3) out;\n";
	//gHeader += "uniform mat4 m_pvm[" +std::to_string(_nrViewports)+ "];\n";
	geom = gHeader + geom;

	std::string frag =
			STRINGIFY(
					uniform sampler2D tex; uniform vec4 backColor; uniform vec4 frontColor; in vec2 gs_tex_coord; layout (location = 0) out vec4 color; vec4 col;

					void main() { color = texture(tex, gs_tex_coord); color.rgb = color.rgb * frontColor.rgb;
					//color = col.a + (1.0 - col.a) * backColor;
					//color = vec4(0, 0, 0, 1.0);
					});

	frag = "// basic gui texture shader, frag\n" + shdr_Header + frag;

	guiTexShader = shCol->addCheckShaderText("guiTexShdr", vert.c_str(),
			geom.c_str(), frag.c_str());
}

//--------------------------------------------------

void RootWidget::addObjIdShader(int _nrViewports)
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; out vec4 col; void main() { col = color; gl_Position = position; });

	vert = "// objId shader, vert\n" + shdr_Header + vert;

	std::string geom =
			STRINGIFY(
					uniform mat4 m_pvm; void main() { gl_ViewportIndex = gl_InvocationID; for (int i=0;i<gl_in.length();i++) { gl_Position = m_pvm * gl_in[i].gl_Position; EmitVertex(); } EndPrimitive(); });

	std::string gHeader = "// basic objId shader, geom\n";
	gHeader += "#version 410\n";
	gHeader += "layout(triangles, invocations=" + std::to_string(_nrViewports)
			+ ") in;\n";
	gHeader += "layout(triangle_strip, max_vertices=3) out;\n";
	// gHeader += "uniform mat4 m_pvm[" +std::to_string(_nrViewports)+ "];\n";
	geom = gHeader + geom;

	std::string frag =
			STRINGIFY(
					uniform int objId; in vec4 col; layout (location = 0) out vec4 color;

					void main() { color = vec4(float(objId), 0, 0, 1); });

	frag = "// objId color shader, frag\n" + shdr_Header + frag;

	objIdShader = shCol->addCheckShaderText("guiObjID", vert.c_str(),
			geom.c_str(), frag.c_str());
}

//--------------------------------------------------

void RootWidget::addTouchRecShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; uniform mat4 m_pvm; void main() { gl_Position = m_pvm * position; });
	vert = "// guiTouchRec color shader, vert\n" + shdr_Header + vert;

	std::string geom =
			STRINGIFY(layout (points) in; layout (points, max_vertices = 1) out;

			uniform sampler2D idMap; layout(location=0) out vec4 rec_position;

			vec4 outV;

			void main() {
			// in dem definierten auschnitt gehe durch die Emit Textur durch
					outV = gl_in[0].gl_Position;

					vec4 objId = texture(idMap, vec2(outV.x * 0.5 + 0.5, outV.y * 0.5 + 0.5));

					rec_position = vec4(objId.r, 0.0, 0.0, 1.0); gl_Position = gl_in[0].gl_Position;

					EmitVertex(); EndPrimitive(); });
	geom = "// guiTouchRec color shader, frag\n" + shdr_Header + geom;

	std::string frag =
			STRINGIFY(
					layout (location = 0) out vec4 color; void main() { color = vec4(1.0, 0.0, 0.0, 1.0); });
	frag = "// guiTouchRec color shader, frag\n" + shdr_Header + frag;

	touchRecShader = shCol->addCheckShaderTextNoLink("guiTouchRec",
			vert.c_str(), geom.c_str(), frag.c_str());
}

//--------------------------------------------------

RootWidget::~RootWidget()
{
	for (std::vector<Widget*>::iterator it = widgets.begin();
			it != widgets.end(); ++it)
		delete (*it);
	widgets.clear();
	widgets.resize(0);
}
}
