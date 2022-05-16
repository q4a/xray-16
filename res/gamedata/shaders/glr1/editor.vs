#include "common.h"
#include "iostructs\v_editor.h"

/*
struct vf
{
	float4 P; //: POSITION	;
	float4 C; //: COLOR0	;
};
*/

uniform float4 		tfactor;
v2p _main (vf i)
{
	v2p 		o;

	o.P 		= mul(m_WVP, i.P);			// xform, input in world coords
	o.C 		= tfactor*i.C;

	return o;
}
