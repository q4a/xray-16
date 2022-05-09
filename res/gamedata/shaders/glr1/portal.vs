#include "common.h"
#include "iostructs\v_portal.h"

v2p _main (v_vert v)
{
	v2p 		o;

	o.hpos 		= mul			(m_VP, v.pos);			// xform, input in world coords
	o.c 		= v.color;
	o.fog 		= calc_fogging 		(v.pos);				// fog, input in world coords

	return o;
}
