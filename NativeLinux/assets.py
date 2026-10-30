#!/usr/bin/env python3
"""Decode the observed PC EADB v38 and TABT v4 vehicle layouts.

Only reads original assets. Generated game material belongs under analysis/.
Pillow is required for DDS-to-PNG conversion, not mesh decoding.
"""
import argparse
import copy
import hashlib
import io
import json
import math
from pathlib import Path, PurePosixPath
import re
import struct
import zipfile


class FormatError(ValueError):
    pass


def need(condition, message):
    if not condition:
        raise FormatError(message)


def unpack(data, fmt, offset):
    need(0 <= offset <= len(data)-struct.calcsize(fmt), 'truncated structure')
    return struct.unpack_from(fmt, data, offset)


def text(data, offset, size):
    need(0 <= offset <= len(data)-size, 'string outside file')
    value = data[offset:offset+size]
    need(b'\0' in value, 'unterminated name')
    return value.split(b'\0', 1)[0].decode('ascii')


def safe_name(name):
    return re.sub(r'[^A-Za-z0-9_.-]', '_', name)


def inspection_material(material):
    """Readable PBR approximations; original shader inputs remain in extras."""
    result = copy.deepcopy(material)
    name = material['name'].lower()
    color, metallic, roughness = None, 0, 0.6
    if 'carpaint' in name:
        parameters = {p['name']:p['value'] for p in material['extras']['originalParameters']}
        color = [max(0,min(1,float(v))) for v in parameters.get('g_color0',[0.18,0.3,0.55])[:3]]
        metallic, roughness = 0.7, 0.25
    elif 'glass' in name:
        # Opaque dark glazing remains readable in any Blender preview mode.
        color, roughness = [0.025,0.045,0.065], 0.15
    elif 'wheel' in name or ('metal' in name and 'black' not in name):
        color, metallic, roughness = [0.38,0.4,0.43], 1, 0.25
    elif any(word in name for word in ('tyre','blackplastic','undercarriage','grille')):
        color, roughness = [0.018,0.022,0.028], 0.8
    elif 'blackmetal' in name:
        color, metallic, roughness = [0.045,0.05,0.06], 0.9, 0.3
    elif 'bumper' in name:
        color, roughness = [0.12,0.13,0.15], 0.5
    elif 'lights' in name:
        color, roughness = [0.8,0.82,0.85], 0.2
    if color is not None:
        # Replace the texture binding: multiplying a yellow mask by gray
        # still leaves a yellow result, so a tint alone cannot correct it.
        result['pbrMetallicRoughness'] = dict(baseColorFactor=[*color,1],
                                             metallicFactor=metallic,roughnessFactor=roughness)
        result['extras']['inspectionApproximation'] = True
    return result


def decode_tabt(data):
    magic, endian, version, payload, count, strings_size, payload_size = unpack(data, '<4s6I', 0)
    need(magic == b'TABT' and endian == 0xffee and version == 4, 'expected PC TABT v4')
    strings = 28+24*count
    need(strings+strings_size == payload and payload+payload_size == len(data), 'invalid TABT bounds')
    output = []
    names = set()
    for i in range(count):
        name_offset, offset, size, width, height, flags = unpack(data, '<6I', 28+24*i)
        need(name_offset < strings_size, 'invalid texture name offset')
        name = text(data, strings+name_offset, strings_size-name_offset)
        need(name and name not in names, 'empty/duplicate texture name')
        names.add(name)
        need(offset <= payload_size and size <= payload_size-offset, 'texture outside payload')
        dds = data[payload+offset:payload+offset+size]
        need(len(dds) >= 128 and dds[:4] == b'DDS ', 'expected DDS texture')
        need(unpack(dds, '<I', 4)[0] == 124, 'invalid DDS header size')
        dds_height, dds_width = unpack(dds, '<2I', 12)
        need((width, height) == (dds_width, dds_height), 'DDS dimensions disagree with TABT')
        output.append(dict(name=name, width=width, height=height, flags=flags,
                           fourcc=dds[84:88].decode('ascii', errors='replace'),
                           mip_levels=unpack(dds, '<I', 28)[0],
                           caps2=unpack(dds, '<I', 112)[0], dds=dds))
    return output


def decode_eadb(data):
    need(len(data) >= 128, 'truncated EADB header')
    magic, version, platform, material_count, param_count, node_count, mesh_count = unpack(data, '<4s6I', 0)
    need(magic == b'EADB' and version == 38 and platform == 1, 'expected PC EADB v38')
    _, materials_at, params_at, nodes_at, _, meshes_at, links_at, vertices_at, indices_at = unpack(data, '<9I', 64)
    need(128 <= materials_at <= params_at <= nodes_at <= meshes_at <= links_at <= vertices_at <= indices_at <= len(data), 'invalid EADB section bounds')
    need(materials_at+material_count*464 <= params_at, 'invalid material table')
    need(params_at+param_count*128 <= nodes_at, 'invalid parameter table')
    need(nodes_at+node_count*156 <= meshes_at, 'invalid node table')
    need(meshes_at+mesh_count*48 <= links_at, 'invalid mesh table')
    parameters = []
    for i in range(param_count):
        p = params_at+i*128
        kind = unpack(data,'<I',p+100)[0]
        values = unpack(data,'<4f',p+112)
        need(all(math.isfinite(v) for v in values), 'non-finite shader parameter')
        parameters.append(dict(name=text(data,p,64),runtime_word=kind,value=values))
    materials = []
    for i in range(material_count):
        p = materials_at+i*464
        slots = [text(data, p+64+j*32, 32) for j in range(8)]
        count, first, shader_id = unpack(data,'<3I',p+436)
        need(first <= param_count and count <= param_count-first, 'invalid material parameter range')
        materials.append(dict(name=text(data, p, 64), textures=slots, shader_id=shader_id,
                              parameters=parameters[first:first+count]))
    meshes = []
    for i in range(mesh_count):
        flags, stride, first_vertex, vertex_count, first_index, index_count = unpack(data, '<6I', meshes_at+48*i)
        material = flags & 0xfff
        # Observed formats: FLOAT3 position, HALF4 normal, HALF2 UV0,
        # optionally color and additional packed attributes. Preserve extras.
        need(stride in (24, 28, 32, 36, 40, 44, 48), f'unsupported vertex stride {stride}')
        need(material < material_count, 'invalid material index')
        need(vertices_at+(first_vertex+vertex_count)*stride <= indices_at, 'vertices outside buffer')
        need(indices_at+(first_index+index_count)*2 <= len(data), 'indices outside buffer')
        need(index_count % 3 == 0, 'non-triangle index count')
        positions, normals, uvs, extra = [], [], [], []
        for j in range(vertex_count):
            p = vertices_at+(first_vertex+j)*stride
            position = unpack(data, '<3f', p)
            normal = unpack(data, '<4e', p+12)
            uv = unpack(data, '<2e', p+20)
            need(all(math.isfinite(x) for x in (*position, *normal, *uv)), 'non-finite vertex')
            positions.append(position)
            normals.append(normal[:3])
            uvs.append(uv)
            extra.append(data[p+24:p+stride].hex())
        indices = [x-first_vertex for x in unpack(data, f'<{index_count}H', indices_at+first_index*2)]
        need(all(0 <= x < vertex_count for x in indices), 'index outside mesh vertex range')
        meshes.append(dict(material=material, flags=flags, stride=stride, positions=positions,
                           normals=normals, uvs=uvs, indices=indices, extra_attributes=extra))
    nodes = []
    for i in range(node_count):
        p = nodes_at+i*156
        transform = unpack(data, '<12f', p+32)
        need(all(math.isfinite(x) for x in transform), 'non-finite node transform')
        first_link = unpack(data, '<i', p+120)[0]
        need(first_link >= 0, 'negative node link offset')
        links = []
        cursor = links_at+first_link*2
        while True:
            need(cursor+2 <= vertices_at, 'unterminated mesh links')
            link = unpack(data, '<h', cursor)[0]
            cursor += 2
            if link == -1:
                break
            need(0 <= link < mesh_count, 'invalid mesh link')
            links.append(link)
        nodes.append(dict(name=text(data, p, 32), transform=transform, meshes=links))
    need(set(range(mesh_count)) == {m for n in nodes for m in n['meshes']}, 'unreferenced meshes')
    return dict(materials=materials, meshes=meshes, nodes=nodes)


def export_gltf(model, textures, output):
    """Keep nodes and local coordinates; glTF consumers assemble the vehicle."""
    gltf = dict(asset=dict(version='2.0', generator='ReRiff asset decoder'),
                scene=0, scenes=[dict(nodes=[])], nodes=[], meshes=[], materials=[],
                buffers=[], bufferViews=[], accessors=[], images=[], textures=[],
                samplers=[dict(wrapS=10497, wrapT=10497)])
    binary = bytearray()

    def accessor(values, fmt, kind, component, target, bounds=False):
        while len(binary) % 4:
            binary.append(0)
        start = len(binary)
        for value in values:
            binary.extend(struct.pack(fmt, *value))
        view = len(gltf['bufferViews'])
        gltf['bufferViews'].append(dict(buffer=0, byteOffset=start, byteLength=len(binary)-start, target=target))
        index = len(gltf['accessors'])
        item = dict(bufferView=view, componentType=component, count=len(values), type=kind)
        if bounds:
            item.update(min=[min(v[k] for v in values) for k in range(3)], max=[max(v[k] for v in values) for k in range(3)])
        gltf['accessors'].append(item)
        return index

    lookup = {}
    for i, texture in enumerate(textures):
        filename = safe_name(texture['name'])+'.png'
        gltf['images'].append(dict(uri='textures/'+filename))
        gltf['textures'].append(dict(sampler=0, source=i))
        lookup[texture['name'].lower()] = i
    for material in model['materials']:
        pbr = dict(baseColorFactor=[1, 1, 1, 1], metallicFactor=0, roughnessFactor=0.6)
        diffuse = PurePosixPath(material['textures'][0]).stem.lower()
        if diffuse in lookup:
            pbr['baseColorTexture'] = dict(index=lookup[diffuse])
        gltf['materials'].append(dict(name=material['name'], pbrMetallicRoughness=pbr, doubleSided=True,
                                      extras=dict(originalTextureSlots=material['textures'],
                                                  originalShaderId=material['shader_id'],
                                                  originalParameters=material['parameters'])))
    for mesh in model['meshes']:
        normals = []
        for normal in mesh['normals']:
            length = math.sqrt(sum(x*x for x in normal))
            need(length > 0, 'zero-length vertex normal')
            normals.append(tuple(x/length for x in normal))
        attrs = dict(POSITION=accessor(mesh['positions'], '<3f', 'VEC3', 5126, 34962, True),
                     NORMAL=accessor(normals, '<3f', 'VEC3', 5126, 34962),
                     TEXCOORD_0=accessor(mesh['uvs'], '<2f', 'VEC2', 5126, 34962))
        indices = accessor([(v,) for v in mesh['indices']], '<I', 'SCALAR', 5125, 34963)
        gltf['meshes'].append(dict(primitives=[dict(attributes=attrs, indices=indices, material=mesh['material'], mode=4)]))
    for node in model['nodes']:
        t = node['transform']
        # Stored basis vectors plus translation, expanded column-major for glTF.
        matrix = [*t[0:3],0,*t[3:6],0,*t[6:9],0,*t[9:12],1]
        parent = len(gltf['nodes'])
        gltf['scenes'][0]['nodes'].append(parent)
        gltf['nodes'].append(dict(name=node['name'], matrix=matrix, children=[]))
        for mesh in node['meshes']:
            child = len(gltf['nodes'])
            gltf['nodes'][parent]['children'].append(child)
            gltf['nodes'].append(dict(name=f'{node["name"]}_{mesh}', mesh=mesh))
    gltf['buffers'].append(dict(uri='vehicle.bin', byteLength=len(binary)))
    (output/'vehicle.bin').write_bytes(binary)
    (output/'vehicle.gltf').write_text(json.dumps(gltf, indent=2)+'\n')
    # Effects use game-specific alpha/additive shaders. Blender's opaque
    # fallback covers the detailed body, so provide a separate inspection file.
    inspection = copy.deepcopy(gltf)
    inspection['materials'] = [inspection_material(m) for m in gltf['materials']]
    roots = [i for i in gltf['scenes'][0]['nodes']
             if gltf['nodes'][i]['name'] == 'body' or gltf['nodes'][i]['name'].startswith('wheel_')]
    retained = set()

    def retain(index):
        retained.add(index)
        for child in gltf['nodes'][index].get('children',[]):
            retain(child)

    for root in roots:
        retain(root)
    mapping = {old:new for new,old in enumerate(sorted(retained))}
    inspection['nodes'] = [inspection['nodes'][old] for old in sorted(retained)]
    for node in inspection['nodes']:
        if 'children' in node:
            node['children'] = [mapping[c] for c in node['children']]
    inspection['scenes'] = [dict(name='Body and wheels inspection',nodes=[mapping[r] for r in roots])]
    (output/'vehicle-inspection.gltf').write_text(json.dumps(inspection,indent=2)+'\n')
    export_runtime_mesh(model,inspection['materials'],output/'vehicle.rmesh')


def export_runtime_mesh(model, materials, path):
    """Small explicit interchange format for the native viewer, not original data."""
    vertices, indices = [], []
    for node in model['nodes']:
        if node['name'] != 'body' and not node['name'].startswith('wheel_'):
            continue
        t = node['transform']
        for section in node['meshes']:
            mesh = model['meshes'][section]
            pbr = materials[mesh['material']]['pbrMetallicRoughness']
            color = pbr.get('baseColorFactor',[0.5,0.5,0.5,1])[:3]
            start = len(vertices)
            for p,n in zip(mesh['positions'],mesh['normals']):
                position = [t[k]*p[0]+t[3+k]*p[1]+t[6+k]*p[2]+t[9+k] for k in range(3)]
                # Observed vehicle node bases are rigid rotations/reflections.
                normal = [t[k]*n[0]+t[3+k]*n[1]+t[6+k]*n[2] for k in range(3)]
                length = math.sqrt(sum(v*v for v in normal))
                need(length > 0,'zero runtime normal')
                vertices.append((*position,*(v/length for v in normal),*color,
                                 pbr.get('metallicFactor',0),pbr.get('roughnessFactor',0.6)))
            indices.extend(start+i for i in mesh['indices'])
    need(bool(vertices) and bool(indices),'no body/wheel runtime geometry')
    bounds = [min(v[k] for v in vertices) for k in range(3)]+[max(v[k] for v in vertices) for k in range(3)]
    with path.open('wb') as file:
        file.write(struct.pack('<8sII6f',b'RRMESH1\0',len(vertices),len(indices),*bounds))
        for vertex in vertices:
            file.write(struct.pack('<11f',*vertex))
        for index in indices:
            file.write(struct.pack('<I',index))


def preview_geometry(model, output, vehicle):
    """Diagnostic geometry render; no claim to reproduce original shaders."""
    from PIL import Image, ImageDraw
    triangles = []
    for node in model['nodes']:
        if node['name'] != 'body' and not node['name'].startswith('wheel_'):
            continue
        t = node['transform']
        for index in node['meshes']:
            mesh = model['meshes'][index]
            world = [tuple(t[k]*p[0]+t[3+k]*p[1]+t[6+k]*p[2]+t[9+k] for k in range(3)) for p in mesh['positions']]
            projected = [(0.8*p[0]-0.6*p[2], -0.21*p[0]+0.937*p[1]-0.28*p[2], 0.562*p[0]+0.35*p[1]+0.75*p[2]) for p in world]
            material = model['materials'][mesh['material']]['name'].lower()
            base = (180,190,201)
            if any(s in material for s in ('tyre','black','grille','undercarriage')):
                base = (40,44,51)
            if 'glass' in material:
                base = (59,79,93)
            for j in range(0,len(mesh['indices']),3):
                ids = mesh['indices'][j:j+3]
                a,b,c = [world[i] for i in ids]
                u,v = [b[k]-a[k] for k in range(3)],[c[k]-a[k] for k in range(3)]
                normal = (u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
                length = math.sqrt(sum(x*x for x in normal))
                if length < 1e-12:
                    continue
                light = 0.3+0.7*abs(sum(normal[k]*(-0.4,0.85,0.34)[k] for k in range(3))/length)
                points = [projected[i] for i in ids]
                triangles.append((sum(p[2] for p in points)/3,points,tuple(min(255,int(c*light)) for c in base)))
    need(bool(triangles),'no body/wheel geometry for preview')
    points = [p for _,triangle,_ in triangles for p in triangle]
    low = [min(p[k] for p in points) for k in range(2)]
    high = [max(p[k] for p in points) for k in range(2)]
    scale = min(880/(high[0]-low[0]),520/(high[1]-low[1]))
    image = Image.new('RGB',(1000,700),'#202631')
    draw = ImageDraw.Draw(image)
    for _,triangle,color in sorted(triangles,key=lambda t:t[0]):
        screen = [(500+(p[0]-(low[0]+high[0])/2)*scale,355-(p[1]-(low[1]+high[1])/2)*scale) for p in triangle]
        draw.polygon(screen,fill=color)
    draw.text((25,20),'ReRiff | Decoded original vehicle: '+vehicle.upper(),fill='white')
    draw.text((25,43),'Geometry inspection | Body and wheel transforms applied | Diagnostic colors',fill='#bac4d2')
    draw.text((25,670),'Original shaders, paint and reflections are not reproduced.',fill='#bac4d2')
    image.save(output/'geometry-preview.png')


def export_vehicle(game_dir, vehicle, output):
    from PIL import Image, ImageDraw
    need(re.fullmatch('[a-z0-9]+', vehicle) is not None, 'invalid vehicle identifier')
    prefix = f'graphics/vehicles/{vehicle}/'
    with zipfile.ZipFile(game_dir/'vfs.zip') as archive:
        mesh_data = archive.read(prefix+'lod0_livery.bdae')
        texture_data = archive.read(prefix+'textures/batchTextures.brres')
    model, textures = decode_eadb(mesh_data), decode_tabt(texture_data)
    need(len({safe_name(t['name']) for t in textures}) == len(textures), 'texture filename collision')
    # Decode all images before writing, so unsupported DDS formats fail clearly.
    images = []
    cube_faces = {}
    for texture in textures:
        dds = texture['dds']
        if texture['caps2']:
            need(texture['caps2'] == 0xfe00 and texture['fourcc'] in ('DXT1','DXT5'), 'unsupported DDS cube/volume layout')
            block = 8 if texture['fourcc'] == 'DXT1' else 16
            face_size = sum(max(1, (max(1,texture['width']>>level)+3)//4)*max(1,(max(1,texture['height']>>level)+3)//4)*block
                            for level in range(max(1,texture['mip_levels'])))
            need(len(dds) == 128+6*face_size, 'invalid DDS cube payload')
            header = bytearray(dds[:128])
            struct.pack_into('<I',header,112,0)
            faces = []
            for face in range(6):
                face_dds = header+dds[128+face*face_size:128+(face+1)*face_size]
                with Image.open(io.BytesIO(face_dds)) as image:
                    faces.append(image.convert('RGBA'))
            cube_faces[texture['name']] = faces
            images.append(faces[0])
            continue
        with Image.open(io.BytesIO(dds)) as image:
            images.append(image.convert('RGBA'))
    output.mkdir(parents=True, exist_ok=True)
    texture_dir = output/'textures'
    texture_dir.mkdir(exist_ok=True)
    sheet = Image.new('RGB', (800, math.ceil(len(images)/4)*220), '#252831')
    draw = ImageDraw.Draw(sheet)
    for i, (texture, image) in enumerate(zip(textures, images)):
        name = safe_name(texture['name'])
        (texture_dir/(name+'.dds')).write_bytes(texture['dds'])
        image.save(texture_dir/(name+'.png'))
        if texture['name'] in cube_faces:
            for suffix, face in zip(('px','nx','py','ny','pz','nz'),cube_faces[texture['name']]):
                face.save(texture_dir/(name+'_'+suffix+'.png'))
        thumb = image.copy()
        thumb.thumbnail((190,180))
        x,y = (i%4)*200,(i//4)*220
        sheet.paste(thumb,(x+(200-thumb.width)//2,y),thumb)
        draw.text((x+5,y+183),texture['name'],fill='white')
        draw.text((x+5,y+199),f'{texture["width"]} x {texture["height"]} {texture["fourcc"]}',fill='#b9c1d0')
    sheet.save(output/'texture-sheet.png')
    export_gltf(model,textures,output)
    preview_geometry(model,output,vehicle)
    report = dict(vehicle=vehicle, source_mesh=prefix+'lod0_livery.bdae',
                  source_mesh_sha256=hashlib.sha256(mesh_data).hexdigest(),
                  source_texture_sha256=hashlib.sha256(texture_data).hexdigest(),
                  mesh_count=len(model['meshes']), node_count=len(model['nodes']),
                  vertices=sum(len(m['positions']) for m in model['meshes']),
                  triangles=sum(len(m['indices'])//3 for m in model['meshes']),
                  materials=model['materials'], nodes=model['nodes'],
                  textures=[{k:v for k,v in t.items() if k!='dds'} for t in textures],
                  limitations=['Original shaders, animated livery, paint overrides and transparency are not reproduced.',
                               'Cubemaps are exported as six separate faces; the unsuffixed PNG shows +X only.',
                               'UV0 and half-float normals decoded; extra packed attributes retained only in decoder output.',
                               'Coordinate handedness, face winding and non-identity node rotations require further validation.'])
    (output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ['vehicle','mesh_count','node_count','vertices','triangles']},indent=2))
    return model, textures, images


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, required=True)
    parser.add_argument('--vehicle', default='alf')
    parser.add_argument('--output', type=Path, default=Path('analysis/native-port/vehicle-alf'))
    args = parser.parse_args()
    export_vehicle(args.game_dir,args.vehicle,args.output)
