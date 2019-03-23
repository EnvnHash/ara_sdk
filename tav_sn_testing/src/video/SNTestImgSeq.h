/*
 * SNTestImgSeq.h
 *
 *  Created on: Jul 23, 2018
 *      Author: sven
 */

#ifndef SRC_VIDEO_SNTESTIMGSEQ_H_
#define SRC_VIDEO_SNTESTIMGSEQ_H_

#include <SceneNode.h>
#include <ImgSeqPlayer.h>

namespace tav
{

class SNTestImgSeq : public SceneNode
{

public:
	SNTestImgSeq(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestImgSeq();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

	void bind(unsigned int texUnit);

private:
	Quad* 			quad;
	Shaders*		texShader;
	ImgSeqPlayer*	imgSeq;
};

} /* namespace tav */

#endif /* SRC_VIDEO_SNTESTIMGSEQ_H_ */
