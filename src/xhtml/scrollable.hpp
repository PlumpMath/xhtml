/*
	Copyright (C) 2016 by Kristina Simpson <sweet.kristas@gmail.com>
	
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

#include <functional>

#include "geometry.hpp"

namespace scrollable
{
	typedef std::function<void(int)> change_handler;

	class Scrollbar 
	{
	public:
		enum Direction {VERTICAL, HORIZONTAL};
		explicit Scrollbar(Direction d, change_handler onchange);
		~Scrollbar();
		int getScrollPosition() const { return scroll_pos_; }
		void setRange(int minr, int maxr) { min_range_ = minr; max_range_ = maxr; }
		int getMin() const { return min_range_; }
		int getMax() const { return max_range_; }
		// N.B. using this function doesn't trigger a change notification.
		void setScrollPosition(int pos);

		bool handleMouseMotion(bool claimed, int x, int y);
		bool handleMouseButtonDown(bool claimed, int x, int y, unsigned button);
		bool handleMouseButtonUp(bool claimed, int x, int y, unsigned button);

		void setLocation(int x, int y);
		void setDimensions(int w, int h);
	private:
		change_handler on_change_;
		Direction dir_;
		int min_range_;
		int max_range_;
		int scroll_pos_;
		rect loc_;
		Scrollbar() = delete;
	};
}
