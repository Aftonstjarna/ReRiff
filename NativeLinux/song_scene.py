"""Build and measure a complete song road using the recovered native splines."""
import hashlib
import importlib.util
from pathlib import Path
import json
import math

from assets import need,unpack
from curved_scene import place_vehicle,wheel_supports
from song_analysis import load_bundled
from song_track import SongGenerator,SongSpline
from splines import dot,length,sub,transform,f32
from track_scene import append_section,write_mesh,normal,wall_profile


def build_song_geometry(game_dir,identifier,genre,model,vehicle_output,output,reference_nodes=None):
    entry,profile,analysis,source_hash=load_bundled(game_dir,identifier)
    track=SongGenerator(game_dir,profile,analysis,genre).generate()
    track['bundled_reference']={k:v for k,v in entry.items() if k!='analysis_path'}
    track['analysis_sha256']=source_hash
    comparison=None
    if reference_nodes is not None:
        spec=importlib.util.spec_from_file_location('song_comparison',Path(__file__).resolve().parents[1]/'scripts/compare-song-track.py')
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        comparison=module.compare(track,json.loads(reference_nodes.read_text()))
        comparison['reference_sha256']=hashlib.sha256(reference_nodes.read_bytes()).hexdigest()
        need(comparison['passed'],'generated song differs from original node capture')
        track['parity'].update(full_original_node_dump_compared=True,comparison=comparison)
        (output/'original-comparison.json').write_text(json.dumps(comparison,indent=2)+'\n')
    (output/'song-profile.json').write_text(json.dumps(profile,indent=2)+'\n')
    (output/'song-analysis.json').write_text(json.dumps(analysis,indent=2)+'\n')
    (output/'song-nodes.json').write_text(json.dumps(track,indent=2)+'\n')
    spline=SongSpline(track);parameters=spline.parameters();frames=[];centers=[];rows=[];widths=[];heights=[];visible=[]
    for t in parameters:
        p,frame=spline.frame(t);centers.append(p);frames.append(frame)
        w=spline.widths(t);h=spline.walls(t)
        factors=[f32(x*0.6666666865348816+0.3333333432674408) for x in w]
        widths.append(factors);heights.append(h);visible.append(spline.row_visible(t))
        rows.append([tuple(f32(p[k]+f32(frame[0][k]*f32((j*3-12)*factors[1 if j<4 else 0]))) for k in range(3)) for j in range(9)])
    vertices=[];indices=[];positions=[p for row in rows for p in row];road_indices=[]
    for i in range(len(rows)-1):
        if visible[i] and visible[i+1]:
            for j in range(8):
                a=i*9+j;b=a+9;road_indices.extend((a,b,a+1,b,b+1,a+1))
    uvs=[(f32((j*3-12)*widths[i][1 if j<4 else 0]/24)+0.5,f32(t*1.600000023841858)) for i,t in enumerate(parameters) for j in range(9)]
    append_section(vertices,indices,positions,[frame[1] for frame in frames for _ in range(9)],uvs,road_indices,(1,1,1),0,0.75,1)
    # Original seven-point visual wall profile, with interpolated lane widths.
    for side in (-1,1):
        slot=0 if side==1 else 1
        def wall_row(i):
            p,frame=centers[i],frames[i]
            points=[]
            for x,y in wall_profile(side,heights[i][slot]):
                x=f32(x*widths[i][slot]);delta=transform(frame,(x,y,0))
                points.append(tuple(f32(p[k]+delta[k]) for k in range(3)))
            return points
        previous=wall_row(0);wall_u=[-0.4,0.4,0.55,0.65,0.75,0.85,1]
        for i in range(len(rows)-1):
            following=wall_row(i+1)
            if visible[i] and visible[i+1]:
                for j in range(6):
                    pts=[previous[j],following[j],previous[j+1],following[j+1]];n=normal(*pts[:3])
                    uv=[(wall_u[j],parameters[i]),(wall_u[j],parameters[i+1]),(wall_u[j+1],parameters[i]),(wall_u[j+1],parameters[i+1])]
                    append_section(vertices,indices,pts,[n]*4,uv,(0,1,2,1,3,2),(1,1,1),0.2,0.5,2)
            previous=following
    report=validate_geometry(spline,parameters,rows,widths,frames,positions,road_indices,visible)
    car=(vehicle_output/'vehicle.rmesh').read_bytes();_,vc,ic,*_=unpack(car,'<8sII6f',0)
    local_vertices=[unpack(car,'<11f',40+i*44) for i in range(vc)];local_indices=unpack(car,f'<{ic}I',40+vc*44)
    supports=wheel_supports(model);cars=[]
    # Poses are selected on intact road away from ramps and narrow sections.
    candidates=[12.]+[float(s['start']+2) for s in track['segments'] if s['end']-s['start']>=6 and s['type'] in (0,4)]
    for t in candidates:
        if len(cars)>=4:break
        if any(abs(t-c['parameter'])<25 for c in cars):continue
        row=min(range(len(parameters)),key=lambda i:abs(parameters[i]-t))
        if not all(visible[max(0,row-4):row+5]) or min(widths[row])<0.9:continue
        local=[]
        for j in range(0,len(road_indices),3):
            ids=road_indices[j:j+3]
            if abs(ids[0]//9-row)<=5:local.append(tuple(positions[i] for i in ids))
        # Build into temporary buffers: an unsupported rigid pose must not leave
        # partial vehicle geometry in the exported scene.
        cv=[];ci=[]
        try:car=place_vehicle(spline,t,local_vertices,local_indices,supports,local,cv,ci)
        except ValueError:continue
        first=len(vertices);vertices.extend(cv);indices.extend(first+i for i in ci);cars.append(car)
    need(bool(cars),'no valid vehicle inspection pose on song road')
    write_mesh(output/'scene.rmesh',vertices,indices)
    if comparison is not None:report['original_comparison']=comparison
    report['vehicle_poses']=len(cars)
    report['max_output_vertex_normal_length_error']=max(abs(length(v[3:6])-1) for v in vertices)
    need(report['max_output_vertex_normal_length_error']<1e-4,'song output normals invalid')
    (output/'song-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    return dict(section='song',song=dict(profile['song'],bundled_audio=entry['path']),analysis_sha256=source_hash,
                node_count=track['node_count'],segments=len(track['segments']),generation=track['generation'],
                road=dict(rows=len(rows),triangles=len(road_indices)//3,width='Interpolated original per-side width factors, remapped by 2/3*w+1/3',
                          hidden_rows=visible.count(False),start_parameter=parameters[0],end_parameter=parameters[-1]),
                validation=report,vehicles=cars,viewer_focus=[cars[0]['position'][k]+cars[0]['frame']['up'][k] for k in range(3)],
                viewer_distance=55,viewer_elevation=45,total_vertices=len(vertices),total_triangles=len(indices)//3,
                native_math=['elSpline.evaluate','elSpline.evaluateTangent','elMatrix.setOrientation'],
                limitations=([] if comparison is not None else ['Full generated layout has not been compared with an original game node dump.'])+['Static inspection materials; original animated effects, ramp assets, props, physics and audio playback pending.'])


def validate_geometry(spline,parameters,rows,widths,frames,positions,indices,visible):
    tolerance=max(1e-4,max(abs(x) for p in positions for x in p)*2e-6)
    width_error=max(abs(length(sub(row[-1],row[0]))-12*sum(w)) for row,w in zip(rows,widths))
    normal_error=max(abs(length(frame[1])-1) for frame in frames)
    ortho=max(abs(dot(frame[a],frame[b])) for frame in frames for a,b in ((0,1),(0,2),(1,2)))
    min_area=float('inf');alignment=1.;inverted=0;degenerate=0
    for j in range(0,len(indices),3):
        a,b,c=(positions[i] for i in indices[j:j+3]);u,v=sub(b,a),sub(c,a)
        cross=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]);area=length(cross)*0.5
        min_area=min(area,min_area)
        if area<1e-8:degenerate+=1;continue
        frame=frames[indices[j]//9];match=dot(normal(a,b,c),frame[1]);alignment=min(alignment,match)
        if match<=0:inverted+=1
    joins=[]
    for i in range(1,len(spline.points)-3):
        left=f32(i-1e-4);right=f32(i+1e-4)
        p=spline.math.evaluate(spline.packed_points,left);q=spline.math.evaluate(spline.packed_points,right)
        a=spline.math.evaluate(spline.packed_points,left,True);b=spline.math.evaluate(spline.packed_points,right,True)
        jump=length(tuple(p[k]+a[k]*(i-left)-(q[k]-b[k]*(right-i)) for k in range(3)))
        joins.append(jump)
    need(width_error<tolerance,'song road width differs from sampled width spline')
    need(normal_error<1e-5 and ortho<1e-5,'invalid song road orientation')
    need(degenerate==0,'degenerate song road triangles')
    need(inverted==0,'inverted song road triangles')
    need(max(joins)<tolerance*2,'discontinuous song spline')
    return dict(max_width_error=width_error,world_space_tolerance=tolerance,max_normal_length_error=normal_error,max_frame_axis_dot=ortho,
                minimum_triangle_area=min_area,degenerate_triangles=degenerate,inverted_face_count=inverted,
                minimum_face_to_row_normal_dot=alignment,max_knot_position_jump_estimate=max(joins),
                visible_rows=sum(visible),hidden_rows=len(visible)-sum(visible),
                topology='Indexed adjacent rows; jump gaps use the original visibility flags.',
                original_comparison=dict(full_track_layout='NOT VALIDATED: original node dump unavailable',
                    spline_math='Use scripts/verify-track-math.py for direct original x86 parity.',
                    generator='Ported from original IL and original PC configuration, not an independently captured layout.'))
