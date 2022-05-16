#ifndef	SKIN_MAIN_INCLUDED
#define	SKIN_MAIN_INCLUDED

void main()
{
#ifdef 	SKIN_NONE
	v_model I;
#endif
#ifdef 	SKIN_0
	v_model_skinned_0 I;
#endif
#ifdef	SKIN_1
	v_model_skinned_1 I;
#endif
#ifdef	SKIN_2
	v_model_skinned_2 I;
#endif
#ifdef	SKIN_3
	v_model_skinned_3 I;
#endif
#ifdef	SKIN_4
	v_model_skinned_4 I;
	I.ind = v_model_ind;
#endif

	I.P = v_model_P;
	I.N = v_model_N;
	I.T = v_model_T;
	I.B = v_model_B;
	I.tc = v_model_tc;

	v2p O;
#ifdef 	SKIN_NONE
	O = _main(I);
#endif
#ifdef 	SKIN_0
	O = _main(skinning_0(I));
#endif
#ifdef	SKIN_1
	O = _main(skinning_1(I));
#endif
#ifdef	SKIN_2
	O = _main(skinning_2(I));
#endif
#ifdef	SKIN_3
	O = _main(skinning_3(I));
#endif
#ifdef	SKIN_4
	O = _main(skinning_4(I));
#endif

	v2p_model_tc0 = O.tc0;
	v2p_model_c0 = O.c0;
	v2p_model_fog = O.fog;
	gl_Position = O.hpos;
}
/*
//////////////////////////////////////////////////////
#ifdef	SKIN_LQ
//////////////////////////////////////////////////////

#ifdef 	SKIN_NONE
SKIN_VF	main(v_model v) 		{ return _main(v); 		}
#endif

#ifdef 	SKIN_0
SKIN_VF	main(v_model_skinned_0 v) 	{ return _main(skinning_0(v)); 	}
#endif

#ifdef	SKIN_1
SKIN_VF	main(v_model_skinned_1 v) 	{ return _main(skinning_1(v)); 	}
#endif

#ifdef	SKIN_2
SKIN_VF	main(v_model_skinned_2 v) 	{ return _main(skinning_2(v)); }
#endif

#ifdef	SKIN_3
SKIN_VF	main(v_model_skinned_3 v) 	{ return _main(skinning_3(v)); }
#endif

#ifdef	SKIN_4
SKIN_VF	main(v_model_skinned_4 v) 	{ return _main(skinning_4(v)); }
#endif

//////////////////////////////////////////////////////
#else	//	SKIN_LQ
//////////////////////////////////////////////////////

#ifdef 	SKIN_NONE
SKIN_VF	main(v_model v) 		{ return _main(v); 		}
#endif

#ifdef 	SKIN_0
SKIN_VF	main(v_model_skinned_0 v) 	{ return _main(skinning_0(v)); 	}
#endif

#ifdef	SKIN_1
SKIN_VF	main(v_model_skinned_1 v) 	{ return _main(skinning_1(v)); 	}
#endif

#ifdef	SKIN_2
SKIN_VF	main(v_model_skinned_2 v) 	{ return _main(skinning_2(v)); 	}
#endif

#ifdef	SKIN_3
SKIN_VF	main(v_model_skinned_3 v) 	{ return _main(skinning_3(v)); 	}
#endif

#ifdef	SKIN_4
SKIN_VF	main(v_model_skinned_4 v) 	{ return _main(skinning_4(v)); 	}
#endif

//////////////////////////////////////////////////////
#endif	//	SKIN_LQ
//////////////////////////////////////////////////////
*/
#endif	//	SKIN_MAIN_INCLUDED

/*
#ifndef	SKIN_MAIN_INCLUDED
#define	SKIN_MAIN_INCLUDED

//////////////////////////////////////////////////////
#ifdef	SKIN_LQ
//////////////////////////////////////////////////////

#ifdef 	SKIN_NONE
SKIN_VF	main(v_model v) 		{ return _main(v); 		}
#endif

#ifdef 	SKIN_0
SKIN_VF	main(v_model_skinned_0 v) 	{ return _main(skinning_0(v)); 	}
#endif

#ifdef	SKIN_1
SKIN_VF	main(v_model_skinned_1 v) 	{ return _main(skinning_1(v)); 	}
#endif

#ifdef	SKIN_2
SKIN_VF	main(v_model_skinned_2 v) 	{ return _main(skinning_2lq(v)); }
#endif

#ifdef	SKIN_3
SKIN_VF	main(v_model_skinned_3 v) 	{ return _main(skinning_3lq(v)); }
#endif

#ifdef	SKIN_4
SKIN_VF	main(v_model_skinned_4 v) 	{ return _main(skinning_4lq(v)); }
#endif

//////////////////////////////////////////////////////
#else	//	SKIN_LQ
//////////////////////////////////////////////////////

#ifdef 	SKIN_NONE
SKIN_VF	main(v_model v) 		{ return _main(v); 		}
#endif

#ifdef 	SKIN_0
SKIN_VF	main(v_model_skinned_0 v) 	{ return _main(skinning_0(v)); 	}
#endif

#ifdef	SKIN_1
SKIN_VF	main(v_model_skinned_1 v) 	{ return _main(skinning_1(v)); 	}
#endif

#ifdef	SKIN_2
SKIN_VF	main(v_model_skinned_2 v) 	{ return _main(skinning_2(v)); 	}
#endif

#ifdef	SKIN_3
SKIN_VF	main(v_model_skinned_3 v) 	{ return _main(skinning_3(v)); 	}
#endif

#ifdef	SKIN_4
SKIN_VF	main(v_model_skinned_4 v) 	{ return _main(skinning_4(v)); 	}
#endif

//////////////////////////////////////////////////////
#endif	//	SKIN_LQ
//////////////////////////////////////////////////////

#endif	//	SKIN_MAIN_INCLUDED
*/