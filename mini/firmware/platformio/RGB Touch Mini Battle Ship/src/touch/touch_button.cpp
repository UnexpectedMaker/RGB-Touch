#include "touch/touch_button.h"
#include "audio/audio.h"

bool TouchButton::check_bounds(uint8_t x, uint8_t y)
{
	if (!check_active_in(rgbtouch.mode))
		return false;

	// Leave a second between detecting a button press
	if (millis() - last_trigger < 1000)
	{
		// info_printf("tick %d %d %d\n", millis(), last_trigger, (millis() - last_trigger));
		// info_println("last_trigger");
		return false;
	}

	bool col = (x >= _x && x < (_x + _w));
	bool row = (y >= _y && y < (_y + _h));

	if (col && row)
	{
		if (first_touch == 0)
		{
			// info_printf("x: %d ? %d-%d, y: %d ? %d-%d\n", x, _x, (_x + _w), y, _y, (_y + _h));
			first_touch = millis();
			// info_println("Start touch");
		}
		else if (millis() - first_touch >= _duration)
		{
			first_touch = 0;
			// info_println("finished touch");
			last_trigger = millis();
			return true;
		}
	}
	else
	{
		// info_printf("bad %d,%d (%d,%d,%d,%d) C %d R %d\n", x, y, _x, _y, _w, _h, col, row);
		first_touch = 0;
		last_trigger = 0;
	}

	return false;
}

bool TouchButton::press()
{
	if (callback != nullptr)
	{
		if (callback())
			return true;
	}

	return false;
}

void TouchButton::reset()
{
	first_touch = 0;
	last_trigger = 0;
}

void TouchButton::set_icon(ICON *_icon)
{
	icon = _icon;
}

void TouchButton::draw_icon(Modes m)
{
	if (!check_active_in(m))
		return;

	if (icon != nullptr)
		display.show_icon(icon, _x, _y);
}

void TouchButton::move_icon(uint8_t delta_x, uint8_t delta_y)
{
	_x += delta_x;
	_y += delta_y;
}

void TouchButton::add_extra_mode(Modes new_mode)
{
	active_in.push_back(new_mode);
}

bool TouchButton::check_active_in(Modes mode)
{
	for (size_t i = 0; i < active_in.size(); i++)
	{
		if (active_in[i] == mode)
		{
			// info_printf("active in %d === %d\n", active_in[i], mode);
			return true;
		}
	}

	return false;
}
