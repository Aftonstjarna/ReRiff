#!/usr/bin/env python3
"""Reconstruct a straight road sample and decode an original ramp for inspection.

The cross sections/UV/index rules come from gaTrackStrip. The straight centerline
and placements are explicit inspection fixtures, not a recovered song track.
"""
import argparse
import hashlib
import io
import json
import math
from pathlib import Path,PurePosixPath
import struct
import zipfile

from assets import decode_eadb,decode_tabt,export_vehicle,need,unpack,text


def config(data):
    magic,version,pool_size,count,roots=unpack(data,'<4s4I',0)
    need(magic==b'GFCF' and version==3,'expected GFCF v3')
    strings=20+count*32
    need(strings+pool_size==len(data),'invalid config size')
    seen=set()
    def nodes(first,size,depth=0):
        need(depth<128 and 0<=first<=count and 0<=size<=count-first,'invalid config range')
        out=[]
        for i in range(first,first+size):
            need(i not in seen,'overlapping config records')
            seen.add(i)
            name,_,value,_,_,kind,children,_=unpack(data,'<8I',20+i*32)
            need(name<pool_size,'bad config name')
            key=text(data,strings+name,pool_size-name)
            if kind==2:result=nodes(value,children,depth+1)
            elif kind==1:result=unpack(data,'<f',20+i*32+8)[0]
            elif kind==0:
                need(value<pool_size,'bad config value')
                result=text(data,strings+value,pool_size-value)
            else:raise ValueError('unsupported config type')
            out.append((key,result))
        return out
    result=nodes(0,roots)
    need(len(seen)==count,'unreachable config records')
    return result


def palette_from_config(data,genre):
    genres=dict(config(data))
    need(genre in genres,'visual genre missing')
    style,entries=genres[genre][0]
    road=dict(dict(entries)['road'])
    palette={name:[float(v)/255 for _,v in road[name]] for name in ('base','accent','pulse')}
    return style,palette


def road_profile(width_left=1.0,width_right=1.0):
    # gaTrackStrip.spawnNewRoadSegment: num14*3-12, per-side width remap.
    need(all(math.isfinite(w) and w>=0 for w in (width_left,width_right)),'invalid road width')
    f32=lambda v:struct.unpack('<f',struct.pack('<f',v))[0]
    width_left=f32(width_left*0.6666666865348816+0.3333333432674408)
    width_right=f32(width_right*0.6666666865348816+0.3333333432674408)
    return [(i*3-12)*(width_right if i<4 else width_left) for i in range(9)]


def wall_profile(side=1,height=1.0):
    # Visual path of spawnNewWallSegment, seven vertices. No physics proxy.
    out=[]
    for i in range(7):
        x,y=side*12.0,-1.0
        if i>=1:y+=height*0.75+1
        if i>=2:y+=height*0.25;x+=side*0.125
        if i>=3:x+=side*0.45
        if i>=4:x+=side*0.05;y-=height*0.5
        if i>=5:y-=4.5
        if i>=6:x,y=0,-8
        out.append((x,y))
    return out


def strip_indices(rows,columns):
    indices=[]
    for row in range(rows-1):
        for col in range(columns-1):
            a=row*columns+col;b=a+columns
            indices.extend((a,b,a+1,b,b+1,a+1))
    return indices


def normal(a,b,c):
    u=[b[k]-a[k] for k in range(3)];v=[c[k]-a[k] for k in range(3)]
    n=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
    length=math.sqrt(sum(x*x for x in n))
    return tuple(x/length for x in n) if length>1e-12 else (0,1,0)


def append_section(vertices,indices,positions,normals,uvs,local_indices,color,metal,rough,layer):
    first=len(vertices)
    for p,n,uv in zip(positions,normals,uvs):
        vertices.append((*p,*n,*color,metal,rough,*uv,float(layer)))
    indices.extend(first+i for i in local_indices)


def write_mesh(path,vertices,indices):
    need(bool(vertices) and bool(indices),'empty scene')
    bounds=[min(v[k] for v in vertices) for k in range(3)]+[max(v[k] for v in vertices) for k in range(3)]
    with path.open('wb') as stream:
        stream.write(struct.pack('<8sII6f',b'RRMESH2\0',len(vertices),len(indices),*bounds))
        for vertex in vertices:stream.write(struct.pack('<14f',*vertex))
        for index in indices:stream.write(struct.pack('<I',index))


def bake_mask(image,palette):
    """Static readable RGB-channel mapping; original animated shader is pending."""
    from PIL import Image
    # Keep a fixed-sized array layer; original full-resolution DDS is also saved.
    image=image.convert('RGB').resize((512,512),Image.Resampling.LANCZOS)
    raw=bytearray()
    colors=[palette[k] for k in ('base','accent','pulse')]
    for pixel in image.get_flattened_data():
        for channel in range(3):
            linear=sum(pixel[i]/255*colors[i][channel] for i in range(3))
            raw.append(round(min(1,max(0,linear))**(1/2.2)*255))
        raw.append(255)
    return Image.frombytes('RGBA',(512,512),bytes(raw))


def build_scene(game_dir,vehicle,genre,output,section='straight',song=None,reference_nodes=None):
    from PIL import Image
    need(genre in ('dance','rock','hiphop'),'unsupported track asset genre')
    output.mkdir(parents=True,exist_ok=True)
    vehicle_output=output/'vehicle'
    model,_,_=export_vehicle(game_dir,vehicle,vehicle_output)
    road_path=f'graphics/genre/{genre}/road/batchTextures.brres'
    wall_path=f'graphics/genre/{genre}/sidewall/batchTextures.brres'
    ramp_path=f'graphics/genre/{genre}/ramps/ramp_8m.bdae'
    ramp_textures_path=f'graphics/genre/{genre}/ramps/textures/batchTextures.brres'
    with zipfile.ZipFile(game_dir/'vfs.zip') as archive:
        source={p:archive.read(p) for p in (road_path,wall_path,ramp_path,ramp_textures_path,'scripts/config/visualSections.cfg')}
    style,palette=palette_from_config(source['scripts/config/visualSections.cfg'],genre)
    ramp=decode_eadb(source[ramp_path])
    texture_dir=output/'textures';texture_dir.mkdir(exist_ok=True)
    textures=[]
    for path in (road_path,wall_path,ramp_textures_path):
        category='road' if path==road_path else 'wall' if path==wall_path else 'ramp'
        for texture in decode_tabt(source[path]):
            filename=category+'-'+texture['name']
            (texture_dir/(filename+'.dds')).write_bytes(texture['dds'])
            with Image.open(io.BytesIO(texture['dds'])) as image:
                decoded=image.convert('RGBA')
            decoded.save(texture_dir/(filename+'.png'))
            textures.append((category,texture['name'],decoded))
    image_lookup={(group,name):image for group,name,image in textures}
    # White layer 0 preserves the vehicle's constant inspection material colors.
    layers=[Image.new('RGBA',(512,512),'white')]
    layers.append(bake_mask(image_lookup['road','road'],palette))
    layers.append(bake_mask(image_lookup['wall','sidewall'],palette))
    ramp_layers={}
    for material in ramp['materials']:
        texture=PurePosixPath(material['textures'][0]).stem.lower()
        need(('ramp',texture) in image_lookup,'unmapped ramp material texture')
        if texture in ramp_layers:continue
        ramp_layers[texture]=len(layers)
        base=[0.2,0.3,0.4] if 'glow' in texture else [0.3,0.35,0.4] if ('anim' in texture or 'metal' in texture) else [0.15,0.18,0.22]
        layers.append(bake_mask(image_lookup['ramp',texture],dict(base=base,accent=[0.08,0.5,0.65],pulse=[0.18,0.2,0.24])))
    for i,layer in enumerate(layers):layer.save(texture_dir/f'inspection-layer-{i}.png')
    with (output/'scene.rtex').open('wb') as stream:
        stream.write(struct.pack('<8sIII',b'RRTEX1\0\0',512,512,len(layers)))
        for image in layers:stream.write(image.tobytes())
    if section!='straight' or song:
        if song:
            from song_scene import build_song_geometry
            geometry=build_song_geometry(game_dir,song,genre,model,vehicle_output,output,reference_nodes)
        else:
            from curved_scene import build_geometry
            geometry=build_geometry(model,vehicle_output,section,output)
        report=dict(genre=genre,palette_style=style,palette=palette,
                    sources={p:hashlib.sha256(b).hexdigest() for p,b in source.items()},
                    texture_layers=dict(count=len(layers),ramp_mapping=ramp_layers),
                    **geometry)
        (output/'scene-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(report,indent=2))
        return
    vertices,indices=[],[]
    # Keep original car scale. Place the lowest wheel vertex exactly at road Y=0.
    car= (vehicle_output/'vehicle.rmesh').read_bytes()
    _,vc,ic,*_=unpack(car,'<8sII6f',0)
    car_vertices=[unpack(car,'<11f',40+i*44) for i in range(vc)]
    wheel_heights=[]
    for node in model['nodes']:
        if node['name'].startswith('wheel_'):
            t=node['transform']
            wheel_heights.extend(t[1]*p[0]+t[4]*p[1]+t[7]*p[2]+t[10] for m in node['meshes'] for p in model['meshes'][m]['positions'])
    need(bool(wheel_heights),'wheel geometry missing')
    car_y=-min(wheel_heights);car_z=-8.0
    for v in car_vertices:
        vertices.append((v[0],v[1]+car_y,v[2]+car_z,*v[3:],0,0,0))
    indices.extend(unpack(car,f'<{ic}I',40+vc*44))
    car_bounds=[min(v[k] for v in vertices) for k in range(3)]+[max(v[k] for v in vertices) for k in range(3)]
    # Straight fixture with 20-unit control spacing: original 0.15 spline step
    # becomes a 3-unit longitudinal sample. All width/height factors are 1.
    rows=13;positions=[];uvs=[]
    for row in range(rows):
        parameter=row*3/20
        for x in road_profile():
            positions.append((x,0,-18+row*3));uvs.append((x/24+0.5,parameter*1.600000023841858))
    append_section(vertices,indices,positions,[(0,1,0)]*len(positions),uvs,strip_indices(rows,9),(1,1,1),0,0.75,1)
    # Preserve separate normals at each wall face crease.
    for side in (-1,1):
        profile=wall_profile(side)
        for row in range(rows-1):
            for j in range(6):
                x,y=profile[j];nx,ny=profile[j+1];z=-18+row*3
                points=[(x,y,z),(x,y,z+3),(nx,ny,z),(nx,ny,z+3)]
                n=normal(*points[:3]);normals=[n]*4
                wall_u=[-0.4,0.4,0.55,0.65,0.75,0.85,1]
                uvs=[(wall_u[j],row*0.15),(wall_u[j],(row+1)*0.15),
                     (wall_u[j+1],row*0.15),(wall_u[j+1],(row+1)*0.15)]
                append_section(vertices,indices,points,normals,uvs,(0,1,2,1,3,2),(1,1,1),0.2,0.5,2)
    # Decode and place the authored ramp flat, ahead of the car, at original scale.
    ramp_start=len(vertices)
    for node in ramp['nodes']:
        t=node['transform']
        for section in node['meshes']:
            mesh=ramp['meshes'][section]
            positions=[tuple(t[k]*p[0]+t[3+k]*p[1]+t[6+k]*p[2]+t[9+k]+(0,0.03,16)[k] for k in range(3)) for p in mesh['positions']]
            # Transform original normals, preserving the model's details.
            normals=[tuple(t[k]*n[0]+t[3+k]*n[1]+t[6+k]*n[2] for k in range(3)) for n in mesh['normals']]
            texture=PurePosixPath(ramp['materials'][mesh['material']]['textures'][0]).stem.lower()
            layer=ramp_layers[texture]
            append_section(vertices,indices,positions,normals,mesh['uvs'],mesh['indices'],(1,1,1),0.2,0.5,layer)
    write_mesh(output/'scene.rmesh',vertices,indices)
    report=dict(genre=genre,palette_style=style,palette=palette,
                sources={p:hashlib.sha256(b).hexdigest() for p,b in source.items()},
                road=dict(width=24,length=36,cross_section_vertices=9,rows=rows,triangles=(rows-1)*16,control_spacing_fixture=20),
                car=dict(translation=[0,car_y,car_z],bounds=car_bounds,wheel_lowest_after_placement=min(wheel_heights)+car_y),
                ramp=dict(asset=ramp_path,vertices=len(vertices)-ramp_start,translation=[0,0.03,16]),
                texture_layers=dict(count=len(layers),ramp_mapping=ramp_layers),
                total_vertices=len(vertices),total_triangles=len(indices)//3,
                limitations=['Straight inspection centerline, not a recovered song track.',
                             'Road RGB mask remapping is a static approximation; no music pulses or original shader parity.',
                             'Ramp is placed flat at authored scale; dynamic ramp orientation and collision are not ported.'])
    (output/'scene-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir',type=Path,required=True)
    parser.add_argument('--vehicle',default='alf')
    parser.add_argument('--genre',choices=('dance','rock','hiphop'),default='dance')
    parser.add_argument('--section',choices=('straight','curved','uphill','downhill','banked','combined'),default='straight')
    parser.add_argument('--output',type=Path,default=Path('analysis/native-port/track-scene'))
    parser.add_argument('--song',help='Bundled song ID or exact title')
    parser.add_argument('--reference-nodes',type=Path)
    args=parser.parse_args()
    if args.reference_nodes and not args.song:parser.error('--reference-nodes requires --song')
    build_scene(args.game_dir,args.vehicle,args.genre,args.output,args.section,args.song,args.reference_nodes)
