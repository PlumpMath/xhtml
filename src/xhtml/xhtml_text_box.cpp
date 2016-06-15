/*
	Copyright (C) 2003-2013 by Kristina Simpson <sweet.kristas@gmail.com>
	
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	   1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.

	   2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.

	   3. This notice may not be removed or altered from any source
	   distribution.
*/

#include "BlendModeScope.hpp"
#include "CameraObject.hpp"
#include "RenderTarget.hpp"
#include "Shaders.hpp"
#include "WindowManager.hpp"

#include "xhtml_layout_engine.hpp"
#include "xhtml_line_box.hpp"
#include "xhtml_text_box.hpp"
#include "xhtml_text_node.hpp"

namespace xhtml
{
	TextBox::TextBox(const BoxPtr& parent, const StyleNodePtr& node, const RootBoxPtr& root)
		: Box(BoxId::TEXT, parent, node, root),
		  line_(),
		  shadows_()
	{
		auto shadows = parent->getStyleNode()->getTextShadow();
		if(shadows) {
			// Process shadows in reverse order.
			for(auto it = shadows->getShadows().crbegin(); it != shadows->getShadows().crend(); ++it) {
				const css::TextShadow& shadow = *it;
				float xo = shadow.getOffset()[0].compute() / LayoutEngine::getFixedPointScaleFloat();
				float yo = shadow.getOffset()[1].compute() / LayoutEngine::getFixedPointScaleFloat();
				float br = shadow.getBlur().compute() / LayoutEngine::getFixedPointScaleFloat();
				KRE::ColorPtr color = shadow.getColor().compute();
				shadows_.emplace_back(xo, yo, br, color);
			}
		}
	}

	std::string TextBox::toString() const
	{
		std::ostringstream ss;
		ss << "TextBox: " << getDimensions().content_;
		ss  << "\n    "
			<< (line_.offset_.x / LayoutEngine::getFixedPointScaleFloat()) 
			<< "," 
			<< (line_.offset_.y / LayoutEngine::getFixedPointScaleFloat()) 
			<< ": ";
		for(auto& word : line_.line_->line) {
			ss << " " << word.word;
		}
		ss << "\n";
		return ss.str();
	}

	std::vector<TextBoxPtr> TextBox::reflowText(LineBoxParseInfo* pi, LayoutEngine& eng, const LineBoxPtr& parent, const Dimensions& containing)
	{
		std::vector<TextBoxPtr> lines;

		point cursor = eng.getCursor();

		FixedPoint y1 = cursor.y + parent->getOffset().y;

		const FixedPoint line_height = parent->getLineHeight();

		// XXX if padding left/border left applies should reduce width and move cursor position if isFirstInlineChild() is set.
		// Simlarly the last line width should be reduced by padding right/border right.
		FixedPoint width = eng.getWidthAtPosition(y1, y1 + line_height, containing.content_.width) - cursor.x + eng.getXAtPosition(y1, y1 + line_height);

		Text::iterator last_it = pi->txt_->begin();
		Text::iterator it = last_it;

		bool done = false;
		while(it != pi->txt_->end()) {
			LinePtr line = pi->txt_->reflowText(it, width, pi->node_);
			if(line != nullptr && !line->line.empty()) {
				// is the line larger than available space and are there floats present?
				FixedPoint last_x = line->line.back().advance.back().x;
				if(last_x > width && eng.hasFloatsAtPosition(y1, y1 + line_height)) {
					cursor.y += line_height;
					y1 = cursor.y + parent->getOffset().y;
					cursor.x = eng.getXAtPosition(y1, y1 + line_height);
					it = last_it;
					width = eng.getWidthAtPosition(y1, y1 + line_height, containing.content_.width);
					continue;
				}

				lines.emplace_back(std::make_shared<TextBox>(pi->parent_, pi->node_, pi->root_));
				lines.back()->line_.line_ = line;
				lines.back()->line_.width_ = lines.back()->calculateWidth(lines.back()->line_);
				// XXX This height needs to be modified later if we have inline elements with a different lineheight
				lines.back()->line_.height_ = line_height;
				lines.back()->line_.offset_.y = cursor.y;
				cursor.x += lines.back()->line_.width_;

				if(line->is_end_line) {
					// update the cursor for the next line
					cursor.y += line_height;
					y1 = cursor.y + parent->getOffset().y;
					cursor.x = eng.getXAtPosition(y1, y1 + line_height);
				}
			}					
		}

		for(auto& line : lines) {
			//line->setContentWidth(line->line_.width_);
			//line->setContentHeight(line_height);
		}
		
		return lines;
	}

	FixedPoint TextBox::calculateWidth(const LineInfo& line) const
	{
		ASSERT_LOG(line.line_ != nullptr, "Calculating width of TextBox with no line_ (=nullptr).");
		FixedPoint width = 0;
		for(auto& word : line.line_->line) {
			width += word.advance.back().x;
		}
		width += line.line_->space_advance * line.line_->line.size();
		return width;
	}

	void TextBox::handleLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		calculateHorzMPB(containing.content_.width);
		calculateVertMPB(containing.content_.height);

		setContentX(line_.offset_.x);
		setContentY(line_.offset_.y);
	
		setContentWidth(line_.width_);
		setContentHeight(line_.height_);
	}

	void TextBox::setRightAlign(FixedPoint containing_width)
	{
		// XXX what about case of floats?
		line_.offset_.x = containing_width - line_.width_;
	}

	void TextBox::setCenterAlign(FixedPoint containing_width)
	{
		// XXX what about case of floats?
		line_.offset_.x = (containing_width - line_.width_ - line_.offset_.x) / 2;
	}

	void TextBox::postParentLayout(LayoutEngine& eng, const Dimensions& containing)
	{
		FixedPoint containing_width = containing.content_.width;
		// perform text-align calculation.
		const css::TextAlign ta = getStyleNode()->getTextAlign();
		switch(ta) {
			case css::TextAlign::RIGHT:
				setRightAlign(containing_width);
				break;
			case css::TextAlign::CENTER:	
				setCenterAlign(containing_width);
				break;
			case css::TextAlign::JUSTIFY:	
					setJustify(containing_width);
				break;
			case css::TextAlign::NORMAL:	
				if(getStyleNode()->getDirection() == css::Direction::RTL) {
					setRightAlign(containing_width);
				}
				break;
			case css::TextAlign::LEFT:
			default:
				// use default value.
				break;
		}

		// set vertical alignment
		auto& vertical_align = getStyleNode()->getVerticalAlign();
		css::CssVerticalAlign va = vertical_align->getAlign();

		FixedPoint baseline = getLineHeight();//static_cast<FixedPoint>((getStyleNode()->getFont()->getFontXHeight() + getStyleNode()->getFont()->getLineGap()) * LayoutEngine::getFixedPointScaleFloat()) - getStyleNode()->getFont()->getDescender();

		FixedPoint child_y = line_.offset_.y;
		// XXX we should implement this fully.
		switch(va) {
			case css::CssVerticalAlign::BASELINE:
				// Align the baseline of the box with the baseline of the parent box. 
				// If the box does not have a baseline, align the bottom margin edge 
				// with the parent's baseline.
				child_y += baseline;
				break;
			case css::CssVerticalAlign::MIDDLE:
				// Align the vertical midpoint of the box with the baseline of the 
				// parent box plus half the x-height of the parent.
				child_y += getParent()->getLineHeight()/2 + getParent()->getBaselineOffset();
				break;
			case css::CssVerticalAlign::BOTTOM:
				// Align the bottom of the aligned subtree with the bottom of the line box.
				child_y += getBottomOffset();
				break;
			case css::CssVerticalAlign::SUB:
				// Lower the baseline of the box to the proper position for subscripts of the 
				// parent's box. (This value has no effect on the font size of the element's text.)
			case css::CssVerticalAlign::SUPER:
				// Raise the baseline of the box to the proper position for superscripts of the 
				// parent's box. (This value has no effect on the font size of the element's text.)
			case css::CssVerticalAlign::TOP:
				// Align the top of the aligned subtree with the top of the line box.
			case css::CssVerticalAlign::TEXT_TOP:
				// Align the top of the box with the top of the parent's content area
			case css::CssVerticalAlign::TEXT_BOTTOM:
				// Align the bottom of the box with the bottom of the parent's content area
				break;
			case css::CssVerticalAlign::LENGTH: {
				// Offset align by length value. Percentages reference the line-height of the element.
				FixedPoint len = vertical_align->getLength().compute(getLineHeight());
				// 0 for len is the baseline.
				child_y += baseline - len;
			}
			default:  break;
		}

		line_.offset_.y = child_y;

		//setContentX(line_.offset_.x);
		//setContentY(line_.offset_.y);
	}

	void TextBox::setJustify(FixedPoint containing_width)
	{
		// N.B. we don't justify last line.
		/*for(auto it = lines_.begin(); it != lines_.end()-1; ++it) {
			auto& line = *it;
			int word_count = line.line_->line.size() - 1;
			if(word_count <= 2) {
				return;
			}
			line.justification_ = (containing_width - line.width_) / word_count;
		}*/
	}

	void TextBox::handleRenderBackground(const KRE::SceneTreePtr& scene_tree, const point& offset) const
	{
		Dimensions dims = getDimensions();
		dims.content_.width = line_.width_;
		dims.content_.height = line_.height_;
		point offs = line_.offset_;
		offs.y -= line_.height_;
		getBackgroundInfo().render(scene_tree, dims, offs);
	}

	void TextBox::handleRenderBorder(const KRE::SceneTreePtr& scene_tree, const point& offset) const
	{
		const Dimensions dims = getDimensions();
		//dims.content_.width = line_.width_;
		//dims.content_.height = line_.height_;
		//point offs = line_.offset_;
		//offs.y -= line_.height_;
		/*XXX if(isFirstInlineChild() && it == lines_.begin()) {
			if(!(isLastInlineChild() && it == lines_.end()-1)) {
				dims.border_.right = 0;
			}
		} else {
			if(isLastInlineChild() && it == lines_.end()-1) {
				dims.border_.left = 0;
			} else {
				dims.border_.left = dims.border_.right = 0;
			}
		}*/
		getBorderInfo().render(scene_tree, dims, offset - point(dims.content_.x, dims.content_.y));
	}

	void TextBox::handleRenderShadow(const KRE::SceneTreePtr& scene_tree, KRE::FontRenderablePtr fontr, float w, float h) const
	{
		// make a copy of the font object.
		//KRE::FontRenderablePtr shadow_font(new KRE::FontRenderable(*fontr));
		std::vector<KRE::RenderablePtr> shadow_list;
		KRE::WindowPtr wnd = KRE::WindowManager::getMainWindow();
		const int kernel_radius = 7;

		for(auto shadow : shadows_) {
			if(std::abs(shadow.blur) < FLT_EPSILON || 
				!KRE::DisplayDevice::checkForFeature(KRE::DisplayDeviceCapabilties::RENDER_TO_TEXTURE)) {
				// no blur
				KRE::FontRenderablePtr shadow_font(new KRE::FontRenderable(*fontr));
				shadow_font->setPosition(shadow.x_offset, shadow.y_offset);
				shadow_font->setColor(shadow.color != nullptr ? *shadow.color : *getStyleNode()->getColor());
				scene_tree->addObject(shadow_font);
			} else {
				using namespace KRE;
				// more complex case where we need to blur, so we render the text to a 
				// RenderTarget, then render that to another render target with a blur filter.
				const float extra_border = (kernel_radius) * 2.0f + 20.0f;
				const float width = w + extra_border * 2.0f;
				const float height = h + extra_border * 2.0f;

				const int iwidth = getRootDimensions().content_.width / LayoutEngine::getFixedPointScale();//static_cast<int>(std::round(width));
				const int iheight = getRootDimensions().content_.height / LayoutEngine::getFixedPointScale();//static_cast<int>(std::round(height));

				auto shader_blur = ShaderProgram::createGaussianShader(kernel_radius)->clone();
				const int blur_two = shader_blur->getUniform("texel_width_offset");
				const int blur_tho = shader_blur->getUniform("texel_height_offset");
				const int u_gaussian = shader_blur->getUniform("gaussian");
				std::vector<float> gaussian = generate_gaussian(shadow.blur / 2.0f, kernel_radius);

				//CameraPtr rt_cam = std::make_shared<Camera>("ortho_blur", 0, iwidth, 0, iheight);
				KRE::FontRenderablePtr shadow_font(new KRE::FontRenderable(*fontr));
				const float xheight = getStyleNode()->getFont()->getFontXHeight() / LayoutEngine::getFixedPointScaleFloat();
				shadow_font->setPosition(extra_border, extra_border + xheight);
				//shadow_font->setCamera(rt_cam);
				shadow_font->setColor(shadow.color != nullptr ? *shadow.color : *getStyleNode()->getColor());
				int u_ignore_alpha = shadow_font->getShader()->getUniform("ignore_alpha");
				UniformSetFn old_fn = shadow_font->getShader()->getUniformDrawFunction();
				shadow_font->getShader()->setUniformDrawFunction([u_ignore_alpha](ShaderProgramPtr shader) {
					shader->setUniformValue(u_ignore_alpha, 1);
				});				

				RenderTargetPtr rt_blur_h = RenderTarget::create(iwidth, iheight);
				rt_blur_h->getTexture()->setFiltering(-1, Texture::Filtering::LINEAR, Texture::Filtering::LINEAR, Texture::Filtering::POINT);
				rt_blur_h->getTexture()->setAddressModes(-1, Texture::AddressMode::CLAMP, Texture::AddressMode::CLAMP);
				rt_blur_h->setCentre(Blittable::Centre::TOP_LEFT);
				rt_blur_h->setClearColor(Color(0,0,0,0));
				{
					RenderTarget::RenderScope rs(rt_blur_h, rect(0, 0, iwidth, iheight));
					shadow_font->preRender(wnd);
					wnd->render(shadow_font.get());
				}
				shadow_font->getShader()->setUniformDrawFunction(old_fn);
				//rt_blur_h->setCamera(rt_cam);
				rt_blur_h->setShader(shader_blur);
				shader_blur->setUniformDrawFunction([blur_two, blur_tho, iwidth, gaussian, u_gaussian](ShaderProgramPtr shader){ 
					shader->setUniformValue(u_gaussian, &gaussian[0]);
					shader->setUniformValue(blur_two, 1.0f / (iwidth - 1.0f));
					shader->setUniformValue(blur_tho, 0.0f);
				});

				RenderTargetPtr rt_blur_v = RenderTarget::create(iwidth, iheight);
				rt_blur_v->getTexture()->setFiltering(-1, Texture::Filtering::LINEAR, Texture::Filtering::LINEAR, Texture::Filtering::POINT);
				rt_blur_v->getTexture()->setAddressModes(-1, Texture::AddressMode::CLAMP, Texture::AddressMode::CLAMP);
				rt_blur_v->setCentre(Blittable::Centre::TOP_LEFT);
				rt_blur_v->setClearColor(Color(0,0,0,0));
				{
					RenderTarget::RenderScope rs(rt_blur_v, rect(0, 0, iwidth, iheight));
					rt_blur_h->preRender(wnd);
					wnd->render(rt_blur_h.get());
				}
				rt_blur_v->setShader(shader_blur);
				shader_blur->setUniformDrawFunction([blur_two, blur_tho, iheight, gaussian, u_gaussian](ShaderProgramPtr shader){ 
					shader->setUniformValue(u_gaussian, &gaussian[0]);
					shader->setUniformValue(blur_two, 0.0f);
					shader->setUniformValue(blur_tho, 1.0f / (iheight - 1.0f));
				});

				const float offs_x = 0;//getLeft() / LayoutEngine::getFixedPointScaleFloat();
				const float offs_y = 0;//getTop() / LayoutEngine::getFixedPointScaleFloat();
				rt_blur_v->setPosition(offs_x + shadow.x_offset - extra_border, offs_y + shadow.y_offset - xheight - extra_border);
				scene_tree->addObject(rt_blur_v);
				// XXX isnstead of adding all the textures here, we should add them to an array, then
				// render them all to an FBO so we only have one final texture.
				/*shadow_list.emplace_back(rt_blur_v);
				rt_blur_v->setPosition(shadow.x_offset, shadow.y_offset);*/
			}
		}

		/*if(!shadow_list.empty()) {
			using namespace KRE;
			int width = shadow_list.front()->getTexture()->width();
			int height = shadow_list.front()->getTexture()->height();
			RenderTargetPtr rt_blurred = RenderTarget::create(width, height);
			rt_blurred->getTexture()->setFiltering(-1, Texture::Filtering::LINEAR, Texture::Filtering::LINEAR, Texture::Filtering::POINT);
			rt_blurred->getTexture()->setAddressModes(-1, Texture::AddressMode::CLAMP, Texture::AddressMode::CLAMP);
			rt_blurred->setCentre(Blittable::Centre::TOP_LEFT);
			rt_blurred->setClearColor(Color(0,0,0,0));
			{
				RenderTarget::RenderScope rs(rt_blurred, rect(0, 0, width, height));
				for(auto& r : shadow_list) {
					r->preRender(wnd);
					wnd->render(r.get());
				}
			}
			rt_blurred->setPosition(offset.x / LayoutEngine::getFixedPointScaleFloat() - kernel_radius,
				(offset.y + getStyleNode()->getFont()->getDescender()) / LayoutEngine::getFixedPointScaleFloat() - h - kernel_radius);
			display_list->addRenderable(rt_blurred);
		}*/
	}

	void TextBox::handleRender(const KRE::SceneTreePtr& scene_tree, const point& offset) const
	{
		//handleRenderTextDecoration -- underlines, then overlines
		KRE::FontRenderablePtr fontr = nullptr;
		std::vector<point> path;
		std::string text;
		int dim_x = offset.x + line_.offset_.x;
		int dim_y = offset.y + getStyleNode()->getFont()->getDescender() + line_.offset_.y;
		for(auto& word : line_.line_->line) {
			for(auto it = word.advance.begin(); it != word.advance.end()-1; ++it) {
				path.emplace_back(it->x + dim_x, it->y + dim_y);
			}
			dim_x += word.advance.back().x + line_.line_->space_advance + line_.justification_;
			text += word.word;
		}

		if(!text.empty()) {
			fontr = getStyleNode()->getFont()->createRenderableFromPath(nullptr, text, path);
			fontr->setColorPointer(getStyleNode()->getColor());
			scene_tree->addObject(fontr);
		}

		if(!shadows_.empty()) {
			ASSERT_LOG(false, "FIXME: handle text shadows");
			//float w = fontr->getWidth() + (line.line_->space_advance + line.justification_) * line.line_->line.size() / LayoutEngine::getFixedPointScaleFloat();
			//float h = static_cast<float>(fontr->getHeight());
			//handleRenderShadow(scene_tree, fontr, w, h);
		}

//		fontr->setColorPointer(getStyleNode()->getColor());
//		scene_tree->addObject(fontr);
		//handleRenderEmphasis -- text-emphasis
		//handleRenderTextDecoration -- line-through
	}
}
