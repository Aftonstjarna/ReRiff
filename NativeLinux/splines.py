"""Python boundary to the native recovered math. No alternate spline formula."""
import ctypes as c
import math
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]


class NativeMath:
    def __init__(self):
        path=ROOT/'build/native-linux/libtrackmath.so'
        if not path.exists():raise RuntimeError('Build the recovered math with make -C NativeLinux track-math')
        self.library=c.CDLL(str(path))
        ptr=c.POINTER(c.c_float)
        self.library.reriff_spline.argtypes=[ptr,c.c_uint32,c.c_float,c.c_int,ptr]
        self.library.reriff_spline.restype=c.c_int
        self.library.reriff_orientation.argtypes=[ptr,ptr,ptr]
        self.library.reriff_orientation.restype=c.c_int

    def evaluate(self,points,t,tangent=False):
        if isinstance(points,c.Array):array=points;count=len(points)//3
        else:
            flat=[v for p in points for v in p]
            array=(c.c_float*len(flat))(*flat);count=len(points)
        out=(c.c_float*3)()
        status=self.library.reriff_spline(array,count,t,tangent,out)
        if status:raise ValueError(f'Invalid spline evaluation (status {status})')
        return tuple(out)

    def orientation(self,forward,up):
        out=(c.c_float*9)()
        status=self.library.reriff_orientation((c.c_float*3)(*forward),(c.c_float*3)(*up),out)
        if status:raise ValueError(f'Invalid orientation (status {status})')
        return tuple(tuple(out[i:i+3]) for i in (0,3,6))

    def normalize(self,vector):
        return self.orientation(vector,(0,1,0))[2]


def dot(a,b):return sum(x*y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def length(v):return math.sqrt(dot(v,v))
def transform(frame,point):return tuple(sum(frame[j][i]*point[j] for j in range(3)) for i in range(3))
def f32(v):return c.c_float(v).value


def fixture(kind):
    if kind=='combined':
        points=[(0,0,-20),(0,0,0),(0,0,20),(2,3,40),(10,10,60),(24,17,80),
                (42,20,100),(62,17,120),(82,10,140),(98,3,160),(108,0,180),(112,0,200),(116,0,220)]
        rolls=[0,0,0,5,15,25,30,20,10,5,0,0,0]
    else:
        points=[(0,0,i*20-20) for i in range(9)]
        rolls=[0]*9
        if kind=='curved':points=[(0,0,-20),(0,0,0),(0,0,20),(3,0,40),(12,0,60),(28,0,78),(48,0,92),(70,0,100),(94,0,100)]
        elif kind in ('uphill','downhill'):
            sign=1 if kind=='uphill' else -1
            points=[(0,sign*y,i*20-20) for i,y in enumerate((0,0,0,4,12,20,28,36,44))]
        elif kind=='banked':rolls=[0,0,0,15,30,35,35,35,35]
        else:raise ValueError('Unknown spline test section')
    native=NativeMath();ups=[]
    for i,roll in enumerate(rolls):
        tangent=sub(points[min(i+1,len(points)-1)],points[max(0,i-1)])
        right,up,_=native.orientation(tangent,(0,1,0))
        angle=math.radians(roll)
        ups.append(tuple(f32(up[k]*math.cos(angle)-right[k]*math.sin(angle)) for k in range(3)))
    return points,ups


class RoadSpline:
    def __init__(self,kind):
        self.math=NativeMath();self.points,self.ups=fixture(kind)
        self.kind=kind;self.start=1.0;self.end=float(len(self.points)-3)

    def frame(self,t):
        position=self.math.evaluate(self.points,t)
        tangent=self.math.evaluate(self.points,t,True)
        up=self.math.evaluate(self.ups,t)
        # The original caller normalizes the interpolated up vector first.
        up=self.math.normalize(up)
        frame=self.math.orientation(tangent,up)
        return position,frame

    def at(self,t,lateral=0,height=0,forward=0):
        p,frame=self.frame(t)
        delta=transform(frame,(lateral,height,forward))
        return tuple(f32(p[k]+delta[k]) for k in range(3))

    def parameters(self):
        values=[self.start+i*0.15 for i in range(int((self.end-self.start)/0.15)+1)]
        if abs(values[-1]-self.end)>1e-6:values.append(self.end)
        return values
