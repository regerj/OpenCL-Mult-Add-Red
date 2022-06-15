kernel
void
ArrayMult( global const float *dA, global const float *dB, global float *dD )
{
	int gid = get_global_id( 0 );

	dD[gid] = dA[gid] * dB[gid];
    // TODO refactor this function to work for mult + add
}