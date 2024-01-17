#pragma once
#include <boost/qvm/mat.hpp>
#include <boost/qvm/mat_operations.hpp>
#include <boost/qvm/vec.hpp>
#include <boost/qvm/vec_operations.hpp>
#include <iostream>

typedef boost::qvm::vec<double, 2> Vector2d;

struct CheaterMatrix {
    double a[6];
};

//For this calculation there would need to be a row of all 1s in a 3x3 matrix. Well. Why multiply a bunch of 1s since that special case is the norm?
#define SetupCheater(p1, p2, p3) CheaterMatrix {\
    p1.a[0], p2.a[0], p3.a[0],\
    p1.a[1], p2.a[1], p3.a[1] \
}

#define BoundsCheck(iv, ia, ib, edge) if(resultWeights[iv] > 1 - edgeSlop) \
{ \
    resultWeights[iv] = 1.0; \
    resultWeights[ia] = resultWeights[ib] = 0.0; \
    return CoordForTriangleResult::Vertex; \
} \
else if(resultWeights[iv] < edgeSlop) \
{ \
    resultWeights[iv] = 0.0; \
    return edge;\
}

enum CoordForTriangleResult
{
    Vertex,
    LegEdge,
    HypotenuseEdge,
    Inside,
    Outside
};

inline double CheaterDeterminant(const CheaterMatrix& mat)
{    
    const int32_t a = 0, b = 1, c = 2, d = 3, e = 4, f = 5;
    auto& m = mat.a;
    return (m[a] * (m[e] - m[f])) - (m[b] * (m[d] - m[f])) + (m[c] * (m[d] - m[e]));
}

//Assumes a triangle defined as leg, leg, hypotenuse. resultWeights also assumed to be 3.
static CoordForTriangleResult BarycentricCoordinatesForTriangle(const Vector2d& pq, const Vector2d* v, double* resultWeights, double edgeSlop = 0.001)
{
    using namespace boost::qvm;
    auto invdet = 1.0 / CheaterDeterminant(SetupCheater(v[0], v[1], v[2]));

    resultWeights[0] = CheaterDeterminant(SetupCheater(pq, v[1], v[2])) * invdet;
    if(resultWeights[0] < 0)
        return CoordForTriangleResult::Outside;
    
    resultWeights[1] = CheaterDeterminant(SetupCheater(v[0], pq, v[2])) * invdet;
    if(resultWeights[1] < 0)
        return CoordForTriangleResult::Outside;
    
    resultWeights[2] = 1.0 - resultWeights[0] - resultWeights[1];
    if(resultWeights[2] < 0)
        return CoordForTriangleResult::Outside;

    BoundsCheck(0, 1, 2, CoordForTriangleResult::LegEdge)
    else BoundsCheck(1, 0, 2, CoordForTriangleResult::HypotenuseEdge)
    else BoundsCheck(2, 0, 1, CoordForTriangleResult::LegEdge)

    return CoordForTriangleResult::Inside;
}

inline double CoTangent(const Vector2d& a, const Vector2d& b, const Vector2d& c)
{
    using namespace boost::qvm;

    Vector2d ba = a - b,
             bc = c - b;

    auto crossResult = cross(bc, ba);
    return crossResult ? dot(bc, ba) / abs(crossResult) : 0;
}

//Note: This function is only good for points strictly inside the polygon.
//It also doesn't do nice things like detect if the point is in or out of the polygon reliably, unlike the triangle version.
template <int N>
static void GeneralBarycentric2d(const Vector2d& p, const Vector2d v[N], double w[N])
{
    using namespace boost::qvm;

	double weightSum = 0.f;

	for(auto i = 0; i < N; i++)
	{
		auto prev = (i + N - 1) % N;
		auto next = (i + 1) % N;
        w[i] = (CoTangent(p, v[i], v[prev]) + CoTangent(p, v[i], v[next])) / mag_sqr(p - v[i]);
        weightSum += w[i];
	}
    
	for(auto i = 0; i < N; i++)
		w[i] /= weightSum;
}

static bool BarycentricCoordinatesForCWTetrahedron(const Vector2d& p, const Vector2d v[4], double w[4], double edgeSlop = 0.001)
{
    auto result = BarycentricCoordinatesForTriangle(p, v, w, edgeSlop);
    if(result < CoordForTriangleResult::HypotenuseEdge)
    {
        w[3] = 0;
        return true;
    }

    if(result == CoordForTriangleResult::Outside)
    {
        //01
        //32
        Vector2d tv[3] = {v[0], v[3], v[2]};
        double wv[3] = {0};
        result = BarycentricCoordinatesForTriangle(p, tv, wv);
        if(result < CoordForTriangleResult::HypotenuseEdge)
        {
            w[0] = wv[0];
            w[1] = 0;
            w[2] = wv[2];
            w[3] = wv[1];
            return true;
        }
    }

    if(result == CoordForTriangleResult::Outside)
        return false;

    GeneralBarycentric2d<4>(p, v, w);

    return true;
}

template <typename T>
inline T WeightValues4(T values[4], double resultWeights[4])
{
    return static_cast<T>(values[0] * resultWeights[0] + 
        values[1] * resultWeights[1] + 
        values[2] * resultWeights[2] +
        values[3] * resultWeights[3]);
};