using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public static class MathFunctions
{

    ///CreateTrianglesFromVerts
    ///Decomposes the polygon into triangles with a naive ear-clipping algorithm. Does not handle internal holes in the polygon.
    ///
    ///TArray<FVector> vertices: points used to triangulate
    ///
    public static List<int> CreateTrianglesFromVerts(List<Vector3> vertices)
    {
        // Can't work if not enough verts for 1 triangle
        if (vertices.Count < 3)
        {
            List<int> SimpleIndices = new List<int>();
            SimpleIndices.Add(0);
            SimpleIndices.Add(1);
            SimpleIndices.Add(2);
            // Return true because poly is already a tri
            return SimpleIndices;
        }

        List<int> TriangleIndices = new List<int>();

        for (int i = 0; i < vertices.Count; i++)
        {
            TriangleIndices.Add(vertices.Count);
            if(i + 1 < vertices.Count)
            {
                TriangleIndices.Add(i + 1);
            }
            else
            {
                TriangleIndices.Add(0);
            }
            TriangleIndices.Add(i);
        }
        return TriangleIndices;
    }



    private static bool AreEdgesMergeable(Vector3 V0, Vector3 V1, Vector3 V2)
    {

        Vector3 MergedEdgeVector = V2 - V0;
        float MergedEdgeLengthSquared = MergedEdgeVector.sqrMagnitude;
        if (MergedEdgeLengthSquared > 0.00001f)
        {
            // Find the vertex closest to A1/B0 that is on the hypothetical merged edge formed by A0-B1.
            float IntermediateVertexEdgeFraction = Vector3.Dot((V2 - V0), (V1 - V0)) / MergedEdgeLengthSquared;
            Vector3 InterpolatedVertex = Vector3.Lerp(V0, V2, IntermediateVertexEdgeFraction);

            // The edges are merge-able if the interpolated vertex is close enough to the intermediate vertex.
            return V3Equals(InterpolatedVertex, V1, 0.00002f);
        }
        else
        {
            return true;
        }


    }

    public static bool V3Equals(Vector3 vect1, Vector3 V, float Tolerance)
    {
        return Mathf.Abs(vect1.x - V.x) <= Tolerance && Mathf.Abs(vect1.y - V.y) <= Tolerance && Mathf.Abs(vect1.z - V.z) <= Tolerance;
    }

    public static bool VectorsOnSameSide(Vector3 Vec, Vector3 A, Vector3 B)
    {
        Vector3 CrossA = Vector3.Cross(Vec, A);
        Vector3 CrossB = Vector3.Cross(Vec, B);
        return (Vector3.Dot(CrossA, CrossB)) > 0f;
    }

    public static bool PointInTriangle(Vector3 A, Vector3 B, Vector3 C, Vector3 P)
    {
        if (VectorsOnSameSide(B - A, P - A, C - A) &&
            VectorsOnSameSide(C - B, P - B, A - B) &&
            VectorsOnSameSide(A - C, P - C, B - C))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}
