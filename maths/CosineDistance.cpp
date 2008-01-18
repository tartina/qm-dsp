/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2008 Kurt Jacobson.
    All rights reserved.
*/

#include "CosineDistance.h"

#include <iostream>
#include <limits>

using std::cerr;

double CosineDistance::distance(const vector<double> &v1,
                                const vector<double> &v2)
{
    dist = 1.0; dDenTot = 0; dDen1 = 0; dDen2 = 0; dSum1 =0;

    //check if v1, v2 same size
    if (v1.size() != v2.size())
    {
        cerr << "CosineDistance::distance: ERROR: vectors not the same size\n";
        return 1.0;
    }
    else
    {
        for(int i=0; i<v1.size(); i++)
        {
            dSum1 += v1[i]*v2[i];
            dDen1 += v1[i]*v1[i];
            dDen2 += v2[i]*v2[i];
        }
        dDenTot = sqrt(fabs(dDen1*dDen2));
        if(dDenTot == 0)
        {
            cerr << "CosineDistance::distance: WARNING: dividing by zero in cosine dist\n";
            return 1.0;
        }
	
        dist = 1-((dSum1)/dDenTot);
        return dist;
    }
}
