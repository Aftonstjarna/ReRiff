"""Sample recovered native splines, assemble surfaces, and validate test scenes."""
import math

from assets import need,unpack
from splines import RoadSpline,dot,sub,length,transform,f32
from track_scene import road_profile,wall_profile,strip_indices,normal,append_section,write_mesh


def surface(spline):
    parameters=spline.parameters();rows=[];frames=[];centers=[]
    for t in parameters:
        position,frame=spline.frame(t)
        need(dot(frame[0],frame[0])>0.99 and dot(frame[1],frame[1])>0.99,'degenerate road orientation')
        frames.append(frame);centers.append(position)
        rows.append([spline.at(t,x) for x in road_profile()])
    return parameters,rows,frames,centers


def geometry_validation(spline,parameters,rows,frames,centers):
    widths=[length(sub(row[-1],row[0])) for row in rows]
    width_error=max(abs(w-24) for w in widths)
    normal_error=max(abs(length(frame[1])-1) for frame in frames)
    orthogonality=max(abs(dot(frame[i],frame[j])) for frame in frames for i,j in ((0,1),(0,2),(1,2)))
    frame_handedness=min(dot(normal((0,0,0),frame[0],frame[1]),frame[2]) for frame in frames)
    shading_alignment=1.0;degenerate=0;minimum_area=float('inf')
    for i in range(len(rows)-1):
        for j in range(8):
            for ids in ((rows[i][j],rows[i+1][j],rows[i][j+1]),(rows[i+1][j],rows[i+1][j+1],rows[i][j+1])):
                a,b,c=ids;u,v=sub(b,a),sub(c,a)
                area=0.5*length((u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]))
                minimum_area=min(minimum_area,area)
                if area<1e-8:degenerate+=1
                n=normal(a,b,c)
                shading_alignment=min(shading_alignment,dot(n,frames[i][1]),dot(n,frames[i+1][1]))
    joins=[];epsilon=1e-4
    for knot in range(1,int(spline.end)):
        left=spline.math.evaluate(spline.points,knot-epsilon);right=spline.math.evaluate(spline.points,knot+epsilon)
        dl=spline.math.evaluate(spline.points,knot-epsilon,True);dr=spline.math.evaluate(spline.points,knot+epsilon,True)
        # Extrapolate both nearby samples back to the knot to estimate the jump.
        position_jump=length(tuple(left[k]+dl[k]*epsilon-(right[k]-dr[k]*epsilon) for k in range(3)))
        tangent_change=length(sub(dl,dr))
        _,left_frame=spline.frame(knot-epsilon);_,right_frame=spline.frame(knot+epsilon)
        frame_change=max(length(sub(a,b)) for a,b in zip(left_frame,right_frame))
        joins.append(dict(parameter=knot,position_jump_estimate=position_jump,tangent_change_near_join=tangent_change,
                          frame_axis_change_near_join=frame_change))
    need(width_error<1e-4,'road width validation failed')
    need(normal_error<1e-5 and orthogonality<1e-5 and frame_handedness>0.999,'road frame validation failed')
    need(degenerate==0 and shading_alignment>0.85,'road folds, degenerate triangles or normal inversion detected')
    need(all(j['position_jump_estimate']<0.002 and j['tangent_change_near_join']<0.03 and j['frame_axis_change_near_join']<0.001 for j in joins),'fixture continuity validation failed')
    center_steps=[length(sub(a,b)) for a,b in zip(centers,centers[1:])]
    need(all(0<step<6 for step in center_steps),'disconnected road sample detected')
    return dict(max_width_error=width_error,max_normal_length_error=normal_error,
                max_frame_axis_dot=orthogonality,min_frame_handedness=frame_handedness,
                minimum_triangle_area=minimum_area,degenerate_triangles=degenerate,
                minimum_face_to_shading_normal_dot=shading_alignment,
                maximum_face_to_shading_normal_angle_degrees=math.degrees(math.acos(min(1,shading_alignment))),
                max_center_sample_distance=max(center_steps),joins=joins,
                topology='Adjacent rows share the same vertex indices; no duplicated boundary seams.')


def intersect_clearance(point,direction,triangles):
    """Signed separation along the vehicle up axis from the actual road triangles."""
    best=None
    for a,b,c in triangles:
        u,v=sub(b,a),sub(c,a)
        n=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
        denominator=dot(n,direction)
        if abs(denominator)<1e-10:continue
        along=dot(n,sub(a,point))/denominator
        hit=tuple(point[k]+direction[k]*along for k in range(3));w=sub(hit,a)
        uu,uv,vv=dot(u,u),dot(u,v),dot(v,v)
        denominator=uu*vv-uv*uv
        if abs(denominator)<1e-12:continue
        wu,wv=dot(w,u),dot(w,v)
        s=(vv*wu-uv*wv)/denominator;t=(uu*wv-uv*wu)/denominator
        if s>=-1e-6 and t>=-1e-6 and s+t<=1.000001:
            clearance=-along
            if best is None or abs(clearance)<abs(best):best=clearance
    need(best is not None,'wheel support ray missed the road')
    return best


def wheel_supports(model):
    supports={}
    for node in model['nodes']:
        if not node['name'].startswith('wheel_'):continue
        t=node['transform'];points=set()
        for mesh in node['meshes']:
            for p in model['meshes'][mesh]['positions']:
                points.add(tuple(f32(t[k]*p[0]+t[3+k]*p[1]+t[6+k]*p[2]+t[9+k]) for k in range(3)))
        # A bottom contact patch, rather than one arbitrary triangle vertex.
        supports[node['name']]=sorted(points,key=lambda p:p[1])[:32]
    need(len(supports)==4,'expected four wheel nodes')
    return supports


def place_vehicle(spline,t,local_vertices,local_indices,supports,triangles,vertices,indices):
    position,frame=spline.frame(t);right,up,forward=frame
    def world(point,lift=0):
        delta=transform(frame,point)
        return tuple(f32(position[k]+delta[k]+up[k]*lift) for k in range(3))
    initial={name:min(intersect_clearance(world(p),up,triangles) for p in points) for name,points in supports.items()}
    lift=-min(initial.values())+0.002
    final={name:min(intersect_clearance(world(p,lift),up,triangles) for p in points) for name,points in supports.items()}
    need(min(final.values())>=-1e-5 and max(final.values())<0.2,'vehicle support placement failed')
    first=len(vertices)
    for v in local_vertices:
        p=world(v[:3],lift);n=transform(frame,v[3:6])
        vertices.append((*p,*(f32(x) for x in n),*v[6:],0,0,0))
    indices.extend(first+i for i in local_indices)
    return dict(parameter=t,position=list(position),local_up_lift=lift,
                frame=dict(right=right,up=up,forward=forward),wheel_support_clearances=final,
                clearance_method='32 lowest authored vertices per wheel, projected along vehicle up onto sampled road triangles',
                note='Rigid inspection pose. Suspension, tire collision and driving physics are not implemented.')


def build_geometry(model,vehicle_output,section,output):
    spline=RoadSpline(section)
    parameters,rows,frames,centers=surface(spline)
    validation=geometry_validation(spline,parameters,rows,frames,centers)
    vertices,indices=[],[]
    road_positions=[p for row in rows for p in row]
    road_indices=strip_indices(len(rows),9)
    road_normals=[frame[1] for frame in frames for _ in range(9)]
    uvs=[(x/24+0.5,t*1.600000023841858) for t in parameters for x in road_profile()]
    append_section(vertices,indices,road_positions,road_normals,uvs,road_indices,(1,1,1),0,0.75,1)
    # World coordinates for placement checks use exactly the float32 positions
    # serialized for the native GPU buffers.
    triangles=[tuple(road_positions[i] for i in road_indices[j:j+3]) for j in range(0,len(road_indices),3)]
    for side in (-1,1):
        profile=wall_profile(side);wall_u=[-0.4,0.4,0.55,0.65,0.75,0.85,1]
        for i in range(len(rows)-1):
            for j in range(6):
                x,y=profile[j];nx,ny=profile[j+1]
                points=[spline.at(parameters[i],x,y),spline.at(parameters[i+1],x,y),
                        spline.at(parameters[i],nx,ny),spline.at(parameters[i+1],nx,ny)]
                n=normal(*points[:3])
                wall_uv=[(wall_u[j],parameters[i]),(wall_u[j],parameters[i+1]),
                         (wall_u[j+1],parameters[i]),(wall_u[j+1],parameters[i+1])]
                append_section(vertices,indices,points,[n]*4,wall_uv,(0,1,2,1,3,2),(1,1,1),0.2,0.5,2)
    car=(vehicle_output/'vehicle.rmesh').read_bytes()
    _,vc,ic,*_=unpack(car,'<8sII6f',0)
    local_vertices=[unpack(car,'<11f',40+i*44) for i in range(vc)]
    local_indices=unpack(car,f'<{ic}I',40+vc*44)
    supports=wheel_supports(model)
    locations=[2.6,4.7,6.7,8.8] if section=='combined' else [3.5]
    cars=[place_vehicle(spline,t,local_vertices,local_indices,supports,triangles,vertices,indices) for t in locations]
    validation['max_output_vertex_normal_length_error']=max(abs(length(tuple(f32(x) for x in v[3:6]))-1) for v in vertices)
    need(validation['max_output_vertex_normal_length_error']<1e-4,'output vertex normal validation failed')
    write_mesh(output/'scene.rmesh',vertices,indices)
    focus=tuple(cars[0]['position'][k]+cars[0]['frame']['up'][k]*0.8 for k in range(3))
    return dict(section=section,control_points=spline.points,up_guides=spline.ups,
                native_math=['elSpline.evaluate','elSpline.evaluateTangent','elMatrix.setOrientation'],
                road=dict(width=24,rows=len(rows),cross_section_vertices=9,triangles=len(road_indices)//3,
                          start_parameter=spline.start,end_parameter=spline.end),
                validation=validation,vehicles=cars,viewer_focus=focus,viewer_distance=42,viewer_elevation=55,
                ramp=dict(included=False,reason='Ramp placement/orientation is separate from spline surface validation.'),
                total_vertices=len(vertices),total_triangles=len(indices)//3,
                limitations=['Control points and bank guides are deliberate test fixtures, not a recovered song track.',
                             'Original row-up shading normals are preserved; geometric normals can differ on changing banks.',
                             'Vehicle support placement is geometric inspection, not original suspension or collision.',
                             'Static road-mask colors; original music effects and shaders remain pending.'])
