#include "common.h"
#include "iostructs\v_sky.h"

v2p _main (vi v)
{
	v2p 		o;

	float4	tpos	    = mul	(1000, v.p);
        o.hpos              = mul       (m_WVP, tpos);						// xform, input in world coords, 1000 - magic number
	o.hpos.z	    = o.hpos.w;
	o.c		= v.c;				// copy color
	o.tc0		= v.tc0;			// copy tc
	o.tc1		= v.tc1;			// copy tc

	return o;
}
