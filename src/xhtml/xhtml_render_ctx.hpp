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

#pragma once

#include <memory>
#include <string>
#include <stack>

#include "font_freetype.hpp"

namespace xhtml
{
	class RenderContext
	{
	public:
		// Returns the render context instance.
		static RenderContext& get();

		void pushFont(const std::string& name, double size);
		void pushFont(const std::vector<std::string>& name, double size);
		void popFont();
		const std::string& getFontName() const { return fh_.top()->getFontName(); }
		double getFontSize() const { return fh_.top()->getFontSize(); }
		double getFontXHeight() const { return fh_.top()->getFontXHeight(); }

		KRE::FontHandlePtr getFont() const { return fh_.top(); }

		double getDPI() const { return dpi_scale_; }
		void setDPI(double dpi) { dpi_scale_ = dpi; }
	private:
		explicit RenderContext(const std::string& font_name, double font_size);
		std::stack<KRE::FontHandlePtr> fh_;
		double dpi_scale_;
	};
}
