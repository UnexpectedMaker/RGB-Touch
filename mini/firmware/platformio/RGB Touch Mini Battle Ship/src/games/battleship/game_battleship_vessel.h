#pragma once

#include <map>
#include "../../audio/audio.h"
#include "../../frameworks/mp_game.h"

enum ShipType : uint8_t
{
	EMPTY = 0,
	SUBMARINE = 1,	// 2x BLUE
	DESTROYER = 2,	// 2x GREEN
	CRUISER = 3,	// 1x PURPLE
	BATTLESHIP = 4,	// 1x CYAN
	AIRCRAFT = 5,	// 1x RED
};

const ShipType FLEET[] = {	// Reverse to give bigger ships a chance
	AIRCRAFT,
	BATTLESHIP,
	CRUISER,
	DESTROYER,
	DESTROYER,
	SUBMARINE,
	SUBMARINE,
};

const uint8_t SHIP_COLOR[7][3] = {
	{  0x00, 0x00, 0x00 },		// empty
	{  0x00, 0x00, 0xC0 },		// SUBMARINE
	{  0x00, 0xC0, 0x00 },		// DESTROYER
	{  0x40, 0x00, 0xC0 },		// CRUISER
	{  0x00, 0xC0, 0xC0 },		// BATTLESHIP
	{  0xC0, 0xC0, 0x00 },		// AIRCRAFT
};

// could be a class, but we dont need construct
class SHIP
{
		ShipType shipID; // Getter 
		uint8_t len;
		int8_t hits;
		uint32_t color;

		// Box of ship (top/left)
		int sx;
		int sy;
		bool horiz;

		// Box of ship (bottom/right)
		int b_ex;
		int b_ey;

		// End draw
		int d_ex;
		int d_ey;

		uint8_t sunkCounter = 0xff;
		bool flasher = false;

public:		
		SHIP(ShipType _shipID, uint8_t _squares, int _x, int _y, bool _horizontal, uint32_t _col, bool killShip = false) 
			: shipID(_shipID), len(_squares), sx(_x), sy(_y) , horiz(_horizontal) , color(_col) 
			{
				// hits are 1 to x count 
				hits = killShip ? 0 : _squares+1;

				// draw end, relative 0 to x 
				d_ex = sx+(horiz ? _squares : 0);
				d_ey = sy+(!horiz ? _squares : 0);

				// Bounding box end , absolute 0 to x 
				b_ex = sx+(horiz ? hits : 0);
				b_ey = sy+(!horiz ? hits : 0);

				// defaults
				sunkCounter = 0xff;
				flasher = false;				
			};

		SHIP duplicate(bool killShip) {
			return SHIP(shipID, len, sx, sy, horiz, color, killShip);
		}

		ShipType getID() {
			return shipID;
		}

		uint8_t getHitsLeft() {
			return (uint8_t) ( hits > 0 ? hits : 0);
		}

		bool isDestroyed() {
			return hits>0;
		}		

		// DEBUG : Use true to check ship no touching boundary
		void draw(bool _thickBox=false)
		{
			if ( hits <= 0 ) {
				return;
			}
	
			if ( _thickBox ) {

  				uint16_t edgeColor = RGB_COLOR(0x40,0x40,0x40);

				// instead of drawing a square , lets get lazy and draw 3 overfil lines
				if ( horiz ) {
					display.draw_line(sx-1, sy-1, b_ex-1, b_ey, edgeColor);
					display.draw_line(sx, sy-1, b_ex, b_ey, edgeColor);
					display.draw_line(sx+1, sy-1, b_ex+1, b_ey, edgeColor);
				} else {
					display.draw_line(sx-1, sy-1, b_ex, b_ey-1, edgeColor);
					display.draw_line(sx-1, sy, b_ex, b_ey, edgeColor);
					display.draw_line(sx-1, sy+1, b_ex, b_ey+1, edgeColor);
				}
			}

			// The ship is overlayed
			display.draw_line(sx, sy, d_ex, d_ey, color);
		}

		// Amin if destroyed
		void draw_destroyed()
		{
			if ( hits > 0) {
				return;
			}
			
			// Pulse, we could create an array to just walk though with indexing
			// TODO : Const cap / steps
			sunkCounter = constrain(sunkCounter + (flasher ? 3 : -3), 128, 255);
			uint32_t destoryedRCol = RGB_COLOR(sunkCounter,0,0);
			uint32_t destoryedOCol = RGB_COLOR(sunkCounter,(sunkCounter*65/100),0);

			if ( sunkCounter == 192 || sunkCounter == 255) {
				flasher = !flasher;
			}

			/* Draw dot along line with rand alt color - didnt offer much in the way of difference
			if ( horiz ) {
				for (int _step=sx; _step <= d_ex; _step++) {
					if ( std::rand() & 1 ) {
						display.draw_single(_step, sy, destoryedOCol);
					} else {
						display.draw_single(_step, sy, destoryedRCol);
					}
				}

			} else  {
				for (int _step=sy; _step <= d_ey; _step++) {

					if ( std::rand() & 1 ) {
						display.draw_single(sx, _step, destoryedOCol);
					} else {
						display.draw_single(sx, _step, destoryedRCol);
					}
				}
			}*/

			if ( std::rand() & 1 ) {
				display.draw_line(sx, sy, d_ex, d_ey, destoryedOCol);
			} else {
				display.draw_line(sx, sy, d_ex, d_ey, destoryedRCol);
			}
		}
		
		// Collision check , true is only used for placing ships if notouchingships is enabled
		// We could expand this and use true to apply splash damage
		bool lineIntersect(int _sx, int _sy, uint8_t _squares, bool _horizontal, bool _thickBox=false)
		{
			// Bounding box end is always absolute, so we add +1 on direction
			int _ex = _sx+(_horizontal ? _squares+1 : 0);
			int _ey = _sy+(!_horizontal ? _squares+1 : 0);

			if ( _thickBox ) {

				// Expand the box +/- , used for board setup could be used for splash damage 
				if(_sx > b_ex+1
					|| _ex < sx-1
					|| _sy > b_ey+1
					|| _ey < sy-1)
						return false;

			} else {

				if(_sx > b_ex
					|| _ex < sx
					|| _sy > b_ey
					|| _ey < sy)
					return false;
			}

			return true;
		}

		// thickness can be true for splash damange ?
		int8_t hitCheck(int _sx, int _sy, bool _thickBox=false)
		{
			// already dead cant hit again
			if ( hits <= 0 ) {
				return -1;
			}

			if ( horiz ) {
				if ( _sy != sy ) {
					return -1;
				}

				if ( _sx < sx || _sx > d_ex ) {
					return -1;
				}

			} else {
				if ( _sx != sx ) {
					return -1;
				}

				if ( _sy < sy || _sy > d_ey ) {
					return -1;
				}
			}

			info_printf("hit ship %d, x: %d, y: %d hits left %d\n",shipID , _sx, _sy, (hits-1));	

			// Subtract hits and return : 0 means ship dead 
			// (doesnt check for over hit , so call must know this square was not used before)
			return --hits;
		}


};
