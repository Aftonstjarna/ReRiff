"""Linux port of gaTrackGen and gaSegmentManager's analysis-driven geometry.

Algorithm source: supported Game.exe IL methods (see SONG-TRACKS.md). Float
stores are explicit. End-to-end layout parity still requires an original node dump.
"""
import copy
import ctypes
import hashlib
import math
import zipfile

from assets import need
from splines import NativeMath,RoadSpline,f32,dot,sub,length
from track_scene import config
from song_analysis import SUPPORTED_GAME_SHA256

MASK=0xffffffff
IDENTITY=(1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.)


def first_map(pairs):
    result={}
    for key,value in pairs:result.setdefault(key,value)
    return result


def lerp(a,b,t):return f32((b-a)*t+a)
def tempo_value(value,tempo):
    if not isinstance(value,list):return f32(value)
    a,b=(v for _,v in value)
    return lerp(a,b,f32(min(1,max(0,f32((tempo-100)/80)))))
def rounded(value):return math.floor(f32(value+0.5))
def song_seed(identifier):
    value=0x12345678
    for i,byte in enumerate(identifier.encode('utf-8')):
        signed=byte if byte<128 else byte-256
        value=(value+(signed<<((i//2+1)&31)))&MASK
    return value


class OriginalRandom:
    """elRand: MT19937, inclusive integer range and float32 uint scaling."""
    def __init__(self,seed):
        self.state=[seed&MASK]
        for i in range(1,624):
            x=self.state[-1];self.state.append((1812433253*(x^(x>>30))+i)&MASK)
        self.index=624

    def integer(self):
        if self.index==624:
            for i in range(624):
                y=(self.state[i]&0x80000000)|(self.state[(i+1)%624]&0x7fffffff)
                self.state[i]=self.state[(i+397)%624]^(y>>1)^(0x9908b0df if y&1 else 0)
            self.index=0
        y=self.state[self.index];self.index+=1
        y^=y>>11;y^=(y<<7)&0x9d2c5680;y^=(y<<15)&0xefc60000;y^=y>>18
        return y&MASK

    def uniform(self,a=0,b=1):
        u=f32(f32(self.integer())*f32(1/4294967296))
        return f32(f32(u*f32(b-a))+a)

    def between(self,a,b):
        need(a<=b,'invalid random range')
        return a+self.integer()%(b-a+1)

    def sign(self):return -1. if self.integer()&0x8000 else 1.

    def choice(self,entries):
        need(bool(entries),'empty weighted choice')
        total=0.
        for _,weight in entries:total=f32(total+weight)
        value=self.uniform(0,total)
        for key,weight in entries[:-1]:
            value=f32(value-weight)
            if value<0:return key
        return entries[-1][0]


def volume(values,start,count,raw=False):
    if start<0:count+=start;start=0
    squared=0.;n=0
    for v in values[start:min(len(values),start+max(0,count))]:
        squared=f32(squared+f32(v*v));n+=1
    if not n:return 0.
    result=f32(math.sqrt(f32(squared/n)))
    if not raw:result=f32(result**f32(0.8))
    return min(1.,max(0.,result))


def detect_drops(values):
    candidates=[];last=-1;strength=0.
    for i in range(10,len(values)-13):
        def rms(a,b):return f32(math.sqrt(f32(f32(a*a)+f32(b*b))))
        change=f32(rms(values[i-1],values[i])-rms(values[i+1],values[i+2]))
        if change>0.5:
            if last==i-1:
                if change>strength:candidates[-1]=(i,change)
            else:candidates.append((i,change))
            last=i;strength=change
    result=[]
    # Drop::operator< sorts by strength. Retain the six strongest separated >8 beats.
    candidates.sort(key=lambda v:v[1])
    while candidates and len(result)<6:
        i,_=candidates.pop();result.append(i)
        candidates=[c for c in candidates if abs(c[0]-i)>8]
    return result


def compose(a,b,rotate_only=False):
    result=[]
    for row in range(4):
        for col in range(3):
            if row==3 and rotate_only:result.append(a[9+col]);continue
            v=f32(f32(a[row*3]*b[col])+f32(a[row*3+1]*b[col+3]))
            v=f32(v+f32(a[row*3+2]*b[col+6]))
            result.append(f32(v+b[9+col]) if row==3 else v)
    return tuple(result)


def rotation(axis,degrees):
    angle=f32(degrees*0.017453293005625408);s=f32(math.sin(angle));c=f32(math.cos(angle))
    if axis==0:return (1.,0.,0.,0.,c,s,0.,-s,c,0.,0.,0.)
    if axis==1:return (c,0.,-s,0.,1.,0.,s,0.,c,0.,0.,0.)
    return (c,s,0.,-s,c,0.,0.,0.,1.,0.,0.,0.)


def cross(a,b):return tuple(f32(a[(i+1)%3]*b[(i+2)%3]-a[(i+2)%3]*b[(i+1)%3]) for i in range(3))


class SongGenerator:
    def __init__(self,game_dir,profile,analysis,genre):
        self.profile=profile;self.analysis=analysis;self.genre=genre
        self.executable_sha256=hashlib.sha256((game_dir/'Game.exe').read_bytes()).hexdigest()
        need(self.executable_sha256==SUPPORTED_GAME_SHA256,'song generator is only verified against the supported Game.exe build')
        self.tempo=f32(analysis['track']['tempo'] or 120)
        self.seed=song_seed(profile['song']['id']);self.random=OriginalRandom(self.seed);self.segment_random=OriginalRandom(self.seed)
        with zipfile.ZipFile(game_dir/'vfs.zip') as archive:
            files={name:archive.read(name) for name in ('scripts/config/segments_PC.cfg','scripts/config/sections_PC.cfg')}
            self.config_hashes={name:hashlib.sha256(data).hexdigest() for name,data in files.items()}
            cfg=first_map(config(files['scripts/config/segments_PC.cfg']))
            sec=first_map(config(files['scripts/config/sections_PC.cfg']))
        self.configs={group:{name:first_map(value) for name,value in cfg[group]} for group in ('segments','checkpoints')}
        self.section_configs=[(name,first_map(value)) for name,value in sec['sections']]
        self.beats=[f32(v) for v in analysis['beats']];self.volumes=[f32(v) for v in analysis['beatVols']]
        self.speed=f32(tempo_value([('',160),('',200)],self.tempo)*0.44703999161720276)
        self.spacing=f32(self.speed/f32(self.tempo/60))
        self.finish=len(self.beats)+9;self.count=len(self.beats)+34
        self.nodes=[dict(matrix=tuple((*IDENTITY[:9],0.,0.,float(i))),flags=0,visible=1,segment=-1,section=0,widths=[1.,1.],walls=[1.,1.]) for i in range(self.count)]
        self.sections=copy.deepcopy(analysis['sections']);self.math=NativeMath();self.roll_sign=0.
        self.clearance=80.;self.attempts=0;self.rejections=0;self.segments=[]
        self.assign_sections()

    def value(self,cfg,key,default=0):return tempo_value(cfg.get(key,default),self.tempo)

    def assign_sections(self):
        # initFromJson selects the NEXT section from its loudness, excluding the
        # current gameplay type. The RNG is shared with subsequent segment choices.
        selected=0
        for i,section in enumerate(self.sections):
            section['config_index']=selected
            section['node']=10+rounded(section['beat'])
            if i+1<len(self.sections):
                next_volume=f32(self.sections[i+1]['loudness']);current_type=self.section_configs[selected][1]['type']
                choices=[]
                for j,(_,cfg) in enumerate(self.section_configs[1:],1):
                    lo,hi=[v for _,v in cfg.get('volRange',[('',0),('',1)])]
                    if cfg['type']!=current_type and lo<=next_volume<=hi:
                        choices.append((j,self.value(cfg,'weight_'+self.genre,self.value(cfg,'weight'))))
                selected=self.random.choice(choices)

    def node_spacing(self,start,end):
        beat=start-10;n=end-start
        if 0<=beat<len(self.beats)-1:
            available=min(n,len(self.beats)-1-beat)
            if available>0:return f32(f32(f32(self.beats[beat+available]-self.beats[beat])/available)*self.speed)
        return self.spacing

    def parameters(self,cfg,vol):
        typ=int(cfg['type']);scale=f32((vol-1)*self.value(cfg,'loudnessInfluence',1)+1)
        scale=f32(scale*self.value(cfg,'sev_'+self.genre,1))
        r=self.random
        def rand(lo,hi):return r.uniform(self.value(cfg,lo),self.value(cfg,hi))
        if typ==0:return (0.,0.,0.,1.)
        if typ==3:return (f32(self.value(cfg,'pitch')*scale),f32(self.value(cfg,'yaw')*scale),self.value(cfg,'roll'),1.)
        if typ==4:
            signs=[r.sign() if self.value(cfg,'allowRandSign'+axis) else 1. for axis in 'XYZ']
            xyz=[f32(rand('min'+axis,'max'+axis)*sign) for axis,sign in zip('XYZ',signs)]
            return (f32(xyz[0]*scale),f32(xyz[1]*scale),xyz[2],1.)
        if typ==5:
            bump=rand('bumpMin','bumpMax');steer=rand('steerMin','steerMax');tilt=rand('tiltMin','tiltMax')
            steer=f32(steer*r.sign());tilt=f32(tilt*r.sign())
            return (f32(bump*scale),f32(steer*scale),tilt,1.)
        if typ in (6,9):return (self.value(cfg,'roll') if typ==6 else 0.,rand('rampUpMin','rampUpMax'),rand('rampDownMin','rampDownMax'),self.value(cfg,'gap'))
        if typ==7:
            a=f32(self.value(cfg,'inAngle')*scale);b=f32(self.value(cfg,'outAngle')*scale)
            if r.uniform()<0.5:a=-a;b=-b
            return a,b,self.value(cfg,'mid'),1.
        if typ==8:return self.value(cfg,'gapY'),self.value(cfg,'gapZ'),0.,1.
        raise ValueError('unsupported original segment type '+str(typ))

    def apply(self,start,end,typ,params,cfg,segment_index,section):
        a,b,c,d=params;spacing=self.node_spacing(start,end);first=start+1
        for i in range(first,end+1):
            node=self.nodes[i];angles=[0.,0.,0.];extra=0.;visible=1
            if typ==3:angles=[self.segment_random.uniform(-v,v) for v in (a,b,c)]
            elif typ==4:angles=[a,b,c];self.roll_sign=-1. if c<0 else 1. if c>0 else 0.
            elif typ==5:
                u=f32((i-first)/(end-first));sign=1. if u<=0.25 or u>0.75 else -1.
                angles=[f32(sign*v) for v in (a,b,c)]
            elif typ==7:
                u=f32((i-first)/(end-first));angles[1]=f32(f32(u/c)*(0-a)+a) if u<c else f32(f32((u-c)/(1-c))*b)
            elif typ==8:
                gap=f32((a+b)/3);maxgap=f32(spacing-1)
                if gap>maxgap:
                    ratio=f32(maxgap/gap);a=f32(a*ratio);b=f32(b*ratio);gap=maxgap
                if i in (first,first+1,end):extra=-gap
                if i>first:node['flags']|=8192
                if i==first+2:extra=f32(b*0.5);angles[0]=90.
                elif i==end-2:extra=a;angles[0]=-90.
                elif i==end-1:extra=f32(b*0.5)
                visible=int(i in (first,end-1,end))
            elif typ==6:
                limit=max(0.,f32(spacing-12.5));d=min(limit,max(-limit,d));turn=f32(self.roll_sign*a)
                angles[1]=f32(turn*0.05999999865889549)
                if i==first:angles[0]=-b;angles[2]=turn
                if i==first+1:
                    angles[2]=turn;angles[0]=f32(b-c) if a else b;visible=0
                    if a:node['flags']|=4096
                if i==first+2:
                    if not a:angles[0]=-c
                    extra=d
                if i==first+3:extra=-d
            elif typ==9:
                offset=f32((1-min(1,max(0,f32((self.tempo-100)/40))))*25)
                if i<first+2:angles[0]=f32(-b*0.5)
                elif i==first+2:angles[0]=b;visible=0;extra=f32(offset*-0.5)
                elif i==first+3:angles[0]=-c;visible=0;extra=f32(offset*-0.5)
                elif i==end-1:angles[0]=c;extra=f32(offset*0.5)
                elif i==end:extra=f32(offset*0.5)
            matrix=IDENTITY
            for axis,angle in enumerate(angles):matrix=compose(matrix,rotation(axis,angle),True)
            matrix=(*matrix[:9],0.,0.,f32(extra+spacing))
            node['local']=matrix;node['visible']=visible
            for key,flag in (('stunt',32768),('selfIntersectAllow',524288),('noSuck',131072),('noBarricades',262144)):
                if cfg.get(key,0):node['flags']|=flag
        # Original relax flags apply to [start,end-2], before transforms are
        # accumulated, so they also affect the last two nodes of the prior span.
        if cfg and not cfg.get('disableRelax',0):
            for i in range(start,end-1):self.nodes[i]['flags']|=2048
        for i in range(first,end+1):
            node=self.nodes[i];matrix=compose(node['local'],self.nodes[i-1]['matrix'])
            if node['flags']&2048:
                up=list(matrix[3:6]);up[1]=f32(up[1]+f32(0.15));up=self.math.normalize(up)
                right=self.math.normalize(cross(matrix[6:9],up));forward=cross(up,right)
                matrix=(*right,*up,*forward,*matrix[9:12])
            node['matrix']=matrix
            for j in range(start-4,max(-1,i-39),-1):
                other=self.nodes[j]
                if (other['segment']<segment_index-1 or i>=start+5) and (not other['flags']&524288 or other['segment']<0 or i>=self.segments[other['segment']]['end']+6):
                    delta=[f32(x-y) for x,y in zip(matrix[9:12],other['matrix'][9:12])]
                    if f32(sum(v*v for v in delta))<f32(self.clearance*self.clearance):
                        self.rejections+=1;return False
        if cfg:
            for i in range(start,end):
                self.nodes[i]['segment']=segment_index;self.nodes[i]['section']=section
        if typ in (6,9):
            for i in range(start,end-1):self.nodes[i]['flags']|=16384
        return True

    def entries(self,section,group):
        result=[];sec=self.section_configs[self.sections[section]['config_index']][1]
        for name in ['Drop']+[v for _,v in sec['includeList']]:
            cfg=self.configs[group].get(name)
            if cfg is None:continue
            result.append(dict(name=name,cfg=cfg,minimum=rounded(self.value(cfg,'minNodes')),maximum=rounded(self.value(cfg,'maxNodes')),
                               weight=0. if name=='Drop' and not result else self.value(cfg,'weight_'+self.genre,rounded(self.value(cfg,'weight'))),
                               vol_range=[v for _,v in cfg.get('volRange',[('',0),('',1)])],vol=0.))
        return result

    def generate_span(self,start,end,section,drop,exclude,force,start_straight,ignore_vol):
        initial=len(self.segments)
        if start_straight:
            stop=min(end,start+start_straight)
            self.apply(start,stop,0,(0,0,0,1),{},len(self.segments),section)
            self.segments.append(dict(start=start,end=stop,name='StartStraight',type=0,section=section,params=[0,0,0,1]));start=stop
        groups=[self.entries(section,g) for g in ('segments','checkpoints')]
        checkpoint=section!=0 and bool(groups[1]) and not drop
        while start<end:
            remaining=end-start;selected=None;forced=False;group=groups[1 if checkpoint else 0]
            if drop:selected=groups[0][0];checkpoint=False
            elif force:
                for g in groups:
                    for entry in g:
                        if entry['name']==force:
                            forced=True
                            if entry['minimum']<=remaining:selected=entry;checkpoint=g is groups[1]
                            break
                    if forced:break
            if selected is None and not forced:
                for pass_index in range(2):
                    choices=[];group=groups[1 if checkpoint else 0]
                    for j,e in enumerate(group):
                        if e['minimum']>remaining:continue
                        vol=volume(self.volumes,start-10,(e['maximum']+e['minimum'])//2 if pass_index==0 else 6)
                        if not (checkpoint and ignore_vol) and not e['vol_range'][0]<=vol<=e['vol_range'][1]:continue
                        e['vol']=vol
                        if e['name'].casefold() not in [s.casefold() for s in exclude]:choices.append((j,e['weight']))
                    if pass_index==0 and (not choices or sum(w for _,w in choices)==0):continue
                    if choices:selected=group[self.random.choice(choices)]
                    break
            if selected is None:selected=groups[0][1];checkpoint=False
            cfg=selected['cfg'];count=min(remaining,self.random.between(selected['minimum'],selected['maximum']))
            need(count>0,'zero length song segment')
            success=False
            for attempt in range(3):
                params=self.parameters(cfg,selected['vol'])
                if self.apply(start,start+count,int(cfg['type']),params,cfg,len(self.segments),section):success=True;break
            if not success:return None
            self.segments.append(dict(start=start,end=start+count,name=selected['name'],type=int(cfg['type']),params=params,section=section,volume=selected['vol']))
            exclude=[v for _,v in cfg.get('excludeFollowList',[])];force=cfg.get('forceNext','')
            start+=count;checkpoint=False;drop=False
        return exclude,force,len(self.segments)-initial

    def generate(self):
        self.nodes[10]['flags']|=16;self.nodes[14]['flags']|=65536;self.nodes[self.finish]['flags']|=512
        spans=[]
        for i,s in enumerate(self.sections):
            start=s['node'];end=self.sections[i+1]['node'] if i+1<len(self.sections) else self.finish
            spans.append(dict(start=start,end=end,section=i,drop=False));self.nodes[start]['flags']|=1024
        drops=detect_drops(self.volumes)
        skip=False
        for beat in drops:
            if skip:skip=False;continue
            node=beat+8
            for j,span in enumerate(spans):
                if span['start']<=node and node+6<=span['end']:
                    if span['drop'] and node<span['start']+6:skip=True
                    elif node==span['start']:span['drop']=True
                    else:
                        spans[j:j+1]=[dict(span,end=node),dict(span,start=node,drop=True)]
                    break
        # makeTrack iterates the padding segment as well. Its default analysis
        # section ID is zero, so it uses Intro rules; only its final generated
        # subsegment is forced straight afterwards.
        spans.append(dict(start=self.finish,end=self.count-1,section=0,drop=False))
        self.apply(0,10,0,(0,0,0,1),{},0,0)
        self.segments=[dict(start=0,end=10,name='LeadIn',type=0,params=[0,0,0,1],section=-1)]
        # resetNodes clears transient flags after applying the original lead-in.
        for node in self.nodes:node['flags']&=-980993;node['visible']=1
        retry=[3]*len(spans);budget=[4]*len(spans);saved=[([], '') for _ in spans];i=0;exclude=[];force=''
        start_straight=rounded(tempo_value([('',2),('',8)],self.tempo))
        while i<len(spans):
            self.attempts+=1;need(self.attempts<=10000,'song generation retry budget exhausted')
            span=spans[i]
            result=self.generate_span(**span,exclude=exclude,force=force,start_straight=start_straight,ignore_vol=budget[i]==0)
            if result is None:
                while retry[i]<=0:
                    retry[i]-=1;budget[i]=max(0,budget[i]-1);retry[i]=3
                    if i==0:self.clearance=max(0,self.clearance-10)
                    else:i-=1
                retry[i]-=1
                segment_start=next((j for j,s in enumerate(self.segments) if s['start']==spans[i]['start']),len(self.segments))
                # Preserve the original's saved AFTER-span exclusions/forceNext,
                # including its empty defaults for a span that never succeeded.
                exclude,force=saved[i];exclude=exclude[:]
                self.segments=self.segments[:segment_start]
                for node in self.nodes[spans[i]['start']:]:node['flags']&=-980993;node['visible']=1
                continue
            saved[i]=(result[0][:],result[1])
            exclude,force,_=result;i+=1;start_straight=0
        last=self.segments[-1]
        self.apply(last['start'],last['end'],0,(0,0,0,1),{},len(self.segments)-1,last['section'])
        last.update(type=0,params=[0,0,0,1])
        self.modulate_widths()
        return dict(schema='reriff-song-nodes-v1',song=self.profile['song'],tempo=self.tempo,genre=self.genre,seed=self.seed,
                    executable_sha256=self.executable_sha256,config_sha256=self.config_hashes,
                    speed_metres_per_second=self.speed,default_units_per_beat=self.spacing,start_node=10,finish_node=self.finish,
                    node_count=self.count,drops=drops,sections=self.sections,segments=self.segments,
                    generation=dict(span_attempts=self.attempts,collision_rejections=self.rejections,final_clearance=self.clearance),
                    nodes=[dict(index=i,position=n['matrix'][9:12],up=n['matrix'][3:6],forward=n['matrix'][6:9],
                                flags=n['flags'],visible=n['visible'],section=n['section'],segment=n['segment'],widths=n['widths'],walls=n['walls']) for i,n in enumerate(self.nodes)],
                    parity=dict(full_original_node_dump_compared=False,limitations=[
                      'Host trigonometry replaces the original Windows math library; full layout comparison pending.']))

    def modulate_widths(self):
        values=[1.,1.,1.,1.];remaining=[0]*4;decrease=[0.]*4;increase=[0.]*4
        chance=0.;allow_wall=True;allow_lane=True;r=self.random
        first=int(f32(self.count)*0.20000000298023224);last=int(f32(self.finish)*0.949999988079071)
        def trigger(k):
            duration=r.between(2,6);decrease[k]=f32(1.100000023841858/duration);increase[k]=f32(2./duration)
            remaining[k]=duration+r.between(4,8)
        for i,node in enumerate(self.nodes):
            if node['flags']&1024:
                cfg=self.section_configs[self.sections[node['section']]['config_index']][1]
                allow_wall=bool(cfg.get('allowSidewallDropout',1));allow_lane=bool(cfg.get('allowLaneChanges',1))
            if i+1>first:
                chance=min(f32(0.1),f32(chance+f32(0.005)))
                if i+1>=last:chance=0.
                for k in range(4):
                    if remaining[k] and (allow_lane if k<2 else allow_wall):
                        remaining[k]-=1;values[k]=f32(values[k]-decrease[k])
                    else:values[k]=f32(values[k]+increase[k])
                vol=volume(self.volumes,min(len(self.volumes)-1,max(0,i-10)),3)
                vol=f32(min(1,max(0,(vol-0.5)/0.25)));lane_chance=f32(vol*chance)
                goto_right=False
                if remaining[0]==0 and remaining[2]==0 and values[0]>=1:
                    if allow_lane and r.uniform()<lane_chance and remaining[1]==0:
                        trigger(0);goto_right=True
                    elif allow_wall and r.uniform()<chance:trigger(2)
                if goto_right or remaining[1]==0:
                    if remaining[3]==0 and values[1]>=1:
                        if allow_lane and r.uniform()<lane_chance and remaining[0]==0:trigger(1)
                        elif allow_wall and r.uniform()<chance:trigger(3)
            values=[min(1.,max(0.,v)) for v in values]
            node['widths']=values[:2];node['walls']=values[2:]


class SongSpline(RoadSpline):
    def __init__(self,track):
        self.math=NativeMath();self.points=[n['position'] for n in track['nodes']];self.ups=[n['up'] for n in track['nodes']]
        self.kind='song';self.start=1.;self.end=float(len(self.points)-3);self.track=track
        def packed(points):
            flat=[v for p in points for v in p]
            return (ctypes.c_float*len(flat))(*flat)
        self.packed_points=packed(self.points);self.packed_ups=packed(self.ups)
        self.width_splines=[packed([(n['widths'][side],0,0) for n in track['nodes']]) for side in (0,1)]
        self.wall_splines=[packed([(n['walls'][side],0,0) for n in track['nodes']]) for side in (0,1)]

    def frame(self,t):
        position=self.math.evaluate(self.packed_points,t)
        tangent=self.math.evaluate(self.packed_points,t,True)
        up=self.math.normalize(self.math.evaluate(self.packed_ups,t))
        return position,self.math.orientation(tangent,up)

    def row_visible(self,t):
        i=int(t);nodes=self.track['nodes']
        return bool(nodes[i]['visible'] or (t>1 and t==math.floor(t) and nodes[i-1]['visible']))

    def widths(self,t):return [self.math.evaluate(p,t)[0] for p in self.width_splines]
    def walls(self,t):return [self.math.evaluate(p,t)[0] for p in self.wall_splines]

    def parameters(self):
        # spawnNewRoadSegment uses float32(row * 3 / 20), bounded by floor(t)
        # <= count-4. Do not stretch the final sample to an extra spline endpoint.
        return [f32(i*3./20.) for i in range(math.ceil((len(self.points)-3)/0.15))]
