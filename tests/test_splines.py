"""Spline invariants and road/vehicle checks, independent of proprietary assets."""
import math
from pathlib import Path
import random
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'NativeLinux'))
from assets import FormatError
from splines import NativeMath,RoadSpline,dot,length,sub,transform
from curved_scene import surface,geometry_validation,place_vehicle,intersect_clearance


class SplineTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.math=NativeMath()

    def test_independent_catmull_rom_basis(self):
        rng=random.Random(41)
        points=[tuple(rng.uniform(-20,20) for _ in range(3)) for _ in range(8)]
        for t in (1.125,1.5,2.25,3.75,4.875):
            i=int(t);u=t-i
            weights=(-u**3/2+u**2-u/2,1.5*u**3-2.5*u**2+1,-1.5*u**3+2*u**2+u/2,u**3/2-u**2/2)
            derivative=(-1.5*u**2+2*u-.5,4.5*u**2-5*u,-4.5*u**2+4*u+.5,1.5*u**2-u)
            for tangent,basis in ((False,weights),(True,derivative)):
                expected=[sum(basis[j]*points[i-1+j][k] for j in range(4)) for k in range(3)]
                actual=self.math.evaluate(points,t,tangent)
                for a,b in zip(actual,expected):self.assertAlmostEqual(a,b,delta=2e-5)

    def test_original_linear_lead_in_and_extrapolation(self):
        points=[(0,0,0),(1,2,3),(4,8,12),(5,10,15),(6,12,18),(7,14,21)]
        self.assertEqual(self.math.evaluate(points,-2),(-2,-4,-6))
        self.assertEqual(self.math.evaluate(points,.5),(.5,1,1.5))
        self.assertEqual(self.math.evaluate(points,1,True),(1,2,3))
        # The original's branch is not generically C1 at t=1; preserve it.
        after=self.math.evaluate(points,1.00001,True)
        self.assertAlmostEqual(after[0],2,delta=1e-3)
        self.assertEqual(self.math.evaluate(points,100),points[3])
        # Tangent is evaluated at the clamped parameter, not returned as zero.
        self.assertGreater(length(self.math.evaluate(points,100,True)),0)

    def test_original_orientation_and_fallback(self):
        right,up,forward=self.math.orientation((1,2,3),(0,1,0))
        for v in (right,up,forward):self.assertAlmostEqual(length(v),1,places=6)
        for a,b in ((right,up),(right,forward),(up,forward)):self.assertAlmostEqual(dot(a,b),0,places=6)
        self.assertEqual(self.math.orientation((0,0,0),(0,1,0)),((1,0,0),(0,1,0),(0,0,1)))
        # Parallel guides remain degenerate in the raw original math.
        self.assertEqual(self.math.orientation((0,0,1),(0,0,1))[1],(0,0,0))

    def test_checked_boundary_rejects_invalid_inputs(self):
        points=[(0,0,i) for i in range(8)]
        for bad,t in ((points[:3],1),(points,float('nan')),([(float('inf'),0,0)]*8,2)):
            with self.assertRaises(ValueError):self.math.evaluate(bad,t)
        with self.assertRaises(ValueError):self.math.orientation((float('nan'),0,0),(0,1,0))

    def test_all_sections_width_continuity_normals_and_winding(self):
        for kind in ('curved','uphill','downhill','banked','combined'):
            with self.subTest(kind=kind):
                spline=RoadSpline(kind)
                parameters,rows,frames,centers=surface(spline)
                result=geometry_validation(spline,parameters,rows,frames,centers)
                self.assertLess(result['max_width_error'],1e-4)
                self.assertEqual(result['degenerate_triangles'],0)
                self.assertGreater(result['minimum_face_to_shading_normal_dot'],.95)
                self.assertLess(result['max_normal_length_error'],1e-5)

    def test_fixture_shapes(self):
        self.assertGreater(RoadSpline('curved').frame(4)[0][0],0)
        self.assertGreater(RoadSpline('uphill').frame(4)[1][2][1],0)
        self.assertLess(RoadSpline('downhill').frame(4)[1][2][1],0)
        bank=RoadSpline('banked');position,frame=bank.frame(4)
        self.assertGreater(abs(frame[0][1]),.4)
        self.assertLess(abs(sub(bank.at(4,12),bank.at(4,-12))[0]),24)
        combined=RoadSpline('combined')
        self.assertGreater(combined.frame(4)[1][2][1],0)
        self.assertLess(combined.frame(8)[1][2][1],0)

    def test_bad_geometry_fails_validation(self):
        spline=RoadSpline('curved');spline.points=[(0,0,0)]*len(spline.points)
        parameters,rows,frames,centers=surface(spline)
        with self.assertRaises(FormatError):geometry_validation(spline,parameters,rows,frames,centers)

    def test_vehicle_pose_on_tilted_plane(self):
        frame=self.math.orientation((0,.2,1),(-.3,1,0))
        corners=[transform(frame,p) for p in ((-10,0,-10),(-10,0,10),(10,0,-10),(10,0,10))]
        triangles=[(corners[0],corners[1],corners[2]),(corners[1],corners[3],corners[2])]
        class Plane:
            def frame(self,t):return (0,0,0),frame
        supports={f'wheel_{i}':[(x,0,z)] for i,(x,z) in enumerate(((-.5,-1),(.5,-1),(-.5,1),(.5,1)))}
        source=[(x,0,z,0,1,0,1,0,0,.7,.25) for x,z in ((-.5,-1),(.5,-1),(-.5,1),(.5,1))]
        vertices,indices=[],[]
        report=place_vehicle(Plane(),0,source,[0,1,2,1,3,2],supports,triangles,vertices,indices)
        for value in report['wheel_support_clearances'].values():self.assertAlmostEqual(value,.002,places=6)
        for v in vertices:
            self.assertAlmostEqual(dot(v[:3],frame[1]),.002,places=6)
            for a,b in zip(v[3:6],frame[1]):self.assertAlmostEqual(a,b,places=6)
        self.assertAlmostEqual(length(sub(vertices[1][:3],vertices[0][:3])),1,places=6)
        self.assertEqual(indices,[0,1,2,1,3,2])

    def test_support_ray_above_below_and_miss(self):
        triangles=[((-2,0,-2),(-2,0,2),(2,0,-2))]
        self.assertEqual(intersect_clearance((0,1,0),(0,1,0),triangles),1)
        self.assertEqual(intersect_clearance((0,-1,0),(0,1,0),triangles),-1)
        with self.assertRaises(FormatError):intersect_clearance((10,0,0),(0,1,0),triangles)


if __name__=='__main__':unittest.main()
