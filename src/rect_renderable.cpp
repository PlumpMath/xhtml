#include "DisplayDevice.hpp"
#include "Shaders.hpp"

#include "rect_renderable.hpp"

RectRenderable::RectRenderable(bool strips, bool loops)
	: SceneObject("RectRenderable")
{
	using namespace KRE;

	setShader(ShaderProgram::getProgram("simple"));
	auto ab = DisplayDevice::createAttributeSet(false, false, false);
	r_ = std::make_shared<Attribute<glm::u16vec2>>(AccessFreqHint::DYNAMIC, AccessTypeHint::DRAW);
	r_->addAttributeDesc(AttributeDesc(AttrType::POSITION, 2, AttrFormat::SHORT, false));
	ab->addAttribute(r_);

	ab->setDrawMode(strips ? DrawMode::TRIANGLE_STRIP : loops ? DrawMode::LINE_LOOP : DrawMode::POINTS);
	addAttributeSet(ab);

	ab->setBlendState(false);
}

void RectRenderable::update(const rect& r, const KRE::Color& color)
{
	setColor(color);

	std::vector<glm::u16vec2> vc;
	if(getAttributeSet().back()->getDrawMode() != KRE::DrawMode::LINE_LOOP) {
		vc.emplace_back(glm::u16vec2(r.x(), r.y()));
		vc.emplace_back(glm::u16vec2(r.x2(), r.y()));
		vc.emplace_back(glm::u16vec2(r.x(), r.y2()));
		vc.emplace_back(glm::u16vec2(r.x2(), r.y2()));
	} else {
		vc.emplace_back(glm::u16vec2(r.x(), r.y()));
		vc.emplace_back(glm::u16vec2(r.x2(), r.y()));
		vc.emplace_back(glm::u16vec2(r.x2(), r.y2()));
		vc.emplace_back(glm::u16vec2(r.x(), r.y2()));
	}

	r_->update(&vc);
}

void RectRenderable::update(int x, int y, int w, int h, const KRE::Color& color)
{
	setColor(color);

	std::vector<glm::u16vec2> vc;
	if(getAttributeSet().back()->getDrawMode() != KRE::DrawMode::LINE_LOOP) {
		vc.emplace_back(glm::u16vec2(x, y));
		vc.emplace_back(glm::u16vec2(x + w, y));
		vc.emplace_back(glm::u16vec2(x, y + h));
		vc.emplace_back(glm::u16vec2(x + w, y + h));
	} else {
		vc.emplace_back(glm::u16vec2(x, y));
		vc.emplace_back(glm::u16vec2(x + w, y));
		vc.emplace_back(glm::u16vec2(x + w, y + h));
		vc.emplace_back(glm::u16vec2(x, y + h));
	}

	r_->update(&vc);
}

void RectRenderable::update(const std::vector<glm::u16vec2>& rs, const KRE::Color& color)
{
	setColor(color);
	r_->update(rs);
}

void RectRenderable::update(std::vector<glm::u16vec2>* rs, const KRE::Color& color)
{
	setColor(color);
	r_->update(rs);
}
