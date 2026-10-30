"""Packed analysis boundaries and recovered song-generation invariants."""
import copy
import importlib.util
import json
import math
import os
from pathlib import Path
import struct
import sys
import unittest
import zlib

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'NativeLinux'))
from song_analysis import decode_song,catalog,load_bundled
from song_track import SongGenerator,SongSpline,OriginalRandom,volume,detect_drops,tempo_value,compose,IDENTITY,song_seed
from splines import length,sub

spec=importlib.util.spec_from_file_location('song_comparison',ROOT/'scripts/compare-song-track.py')
comparison=importlib.util.module_from_spec(spec);spec.loader.exec_module(comparison)


def fixture():
    return {'song':{'id':'Example','title':'Test'}},{'track':{'tempo':120},'beats':[0.,.5,1.,1.5],
           'beatVols':[0.,.2,.5,1.],'sections':[{'beat':0.,'start':0.,'duration':2.,'loudness':.5}]}


def pack(profile,analysis):
    a=json.dumps(profile).encode()+b'\0';b=json.dumps(analysis).encode()+b'\0';x=zlib.compress(a);y=zlib.compress(b)
    return struct.pack('<5I',0x534e4730,len(x),len(y),len(a),len(b))+x+y


class SongTests(unittest.TestCase):
    def test_packed_roundtrip(self):
        p,a=fixture();self.assertEqual(decode_song(pack(p,a)),(p,a))

    def test_reject_bad_container(self):
        data=pack(*fixture())
        for broken in (b'',data[:19],b'JSON'+data[4:],data[:-1],data+b'x',data[:12]+struct.pack('<I',0x7fffffff)+data[16:]):
            with self.subTest(size=len(broken)):
                with self.assertRaises(ValueError):decode_song(broken)

    def test_reject_extra_compressed_stream(self):
        p,a=fixture();data=pack(p,a);magic,c1,c2,u1,u2=struct.unpack_from('<5I',data)
        extra=zlib.compress(b'bad')
        bad=struct.pack('<5I',magic,c1+len(extra),c2,u1,u2)+data[20:20+c1]+extra+data[20+c1:]
        with self.assertRaises(ValueError):decode_song(bad)

    def test_reject_invalid_analysis(self):
        p,a=fixture()
        for key,value in (('beats',[0,.5,.5,1.5]),('beatVols',[0,.5]),('beatVols',[0,.2,1.1,.5]),('sections',[])):
            bad=copy.deepcopy(a);bad[key]=value
            with self.subTest(key=key):
                with self.assertRaises(ValueError):decode_song(pack(p,bad))

    def test_original_mt19937_known_vector(self):
        rng=OriginalRandom(5489)
        self.assertEqual([rng.integer() for _ in range(5)],[3499211612,581869302,3890346734,3586334585,545404204])

    def test_integer_range_and_uniform_boundaries(self):
        r=OriginalRandom(17)
        self.assertEqual({r.between(-1,1) for _ in range(100)},{-1,0,1})
        self.assertTrue(all(-2.5<=r.uniform(-2.5,7.25)<=7.25 for _ in range(1000)))
        self.assertEqual(r.between(3,3),3)

    def test_rms_volume(self):
        self.assertAlmostEqual(volume([0.,1.],0,2,True),math.sqrt(.5),places=6)
        self.assertAlmostEqual(volume([0.,1.],0,2),math.sqrt(.5)**.8,places=6)
        self.assertEqual(volume([1.],-2,1),0)
        self.assertEqual(volume([1.],20,4),0)

    def test_drop_detection_and_separation(self):
        values=[1.]*20+[0.]*20+[1.]*20+[0.]*20
        result=detect_drops(values)
        self.assertEqual(set(result),{19,59})
        self.assertEqual(detect_drops([.3]*100),[])

    def test_tempo_clamp_and_seed(self):
        endpoints=[('',160.),('',200.)]
        self.assertEqual(tempo_value(endpoints,60),160)
        self.assertEqual(tempo_value(endpoints,140),180)
        self.assertEqual(tempo_value(endpoints,220),200)
        self.assertEqual(song_seed('A'),0x12345678+130)

    def test_matrix_translation_order(self):
        translate=(*IDENTITY[:9],3.,4.,5.)
        self.assertEqual(compose(translate,IDENTITY),translate)
        self.assertEqual(compose(translate,translate)[9:],(6.,8.,10.))

    def test_comparison_rejects_linux_export_as_original(self):
        with self.assertRaises(ValueError):comparison.compare({'nodes':[]},{'schema':'reriff-song-nodes-v1','nodes':[]})

    def test_comparison_detects_geometry_and_gap_changes(self):
        node=dict(position=[0.,0.,0.],up=[0.,1.,0.],widths=[1.,1.],walls=[1.,1.],flags=0,visible=1)
        native=dict(song={'id':'Example'},genre='dance',nodes=[node])
        reference=dict(source='original-game',executable_sha256=comparison.HASH,song_id='Example',genre='dance',nodes=[copy.deepcopy(node)])
        self.assertTrue(comparison.compare(native,reference)['passed'])
        reference['analysis_sha256']='different-input'
        with self.assertRaisesRegex(ValueError,'input identity'):comparison.compare(native,reference)
        del reference['analysis_sha256']
        reference['nodes'][0]['position'][0]=.25;reference['nodes'][0]['visible']=0
        result=comparison.compare(native,reference)
        self.assertFalse(result['passed']);self.assertEqual(result['max_position_error'],.25)
        self.assertEqual(result['visibility_mismatches'],[0])


class InstalledSongTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.game=Path(os.environ.get('RERIFF_GAME_DIR',str(Path.home()/'.local/share/Steam/steamapps/common/Riff Racer')))
        if not (cls.game/'vfs.zip').exists():raise unittest.SkipTest('Original installed songs unavailable')

    def test_all_bundled_analysis(self):
        songs=catalog(self.game);self.assertEqual(len(songs),25)
        for entry in songs:
            with self.subTest(song=entry['id']):
                _,p,a,_=load_bundled(self.game,entry['id'])
                self.assertGreater(len(a['beats']),100)

    def test_rolling_on_generation(self):
        _,p,a,_=load_bundled(self.game,'Rolling On')
        first=SongGenerator(self.game,p,a,'dance').generate();second=SongGenerator(self.game,p,a,'dance').generate()
        self.assertEqual(first,second)
        self.assertEqual(len(a['beats']),601);self.assertEqual(first['node_count'],635)
        self.assertEqual(first['start_node'],10);self.assertEqual(first['finish_node'],610)
        segments=first['segments'];self.assertEqual(segments[0]['start'],0);self.assertEqual(segments[-1]['end'],634)
        # Golden capture: the original subdivides the finish padding too.
        self.assertEqual(len(segments),132)
        tail=[s for s in segments if s['start']>=610]
        self.assertEqual([(s['start'],s['end'],s['type']) for s in tail],
                         [(610,616,4),(616,619,0),(619,620,0),(620,621,0),(621,622,0),(622,629,0),(629,630,0),(630,631,0),(631,634,0)])
        self.assertTrue(all(x['end']==y['start'] for x,y in zip(segments,segments[1:])))
        self.assertTrue(any(s['type'] in (6,8,9) for s in segments))
        spline=SongSpline(first)
        # Actual analysis timing determines spacing; it is not the old 20-unit fixture.
        distance=length(sub(first['nodes'][18]['position'],first['nodes'][17]['position']))
        self.assertGreater(distance,30);self.assertLess(distance,40)
        self.assertTrue(any(not n['visible'] for n in first['nodes']))
        self.assertTrue(any(n['widths'][0]<.9 or n['widths'][1]<.9 for n in first['nodes']))
        self.assertFalse(first['parity']['full_original_node_dump_compared'])
        for t in (0.,10.,30.5,100.3,500.75):
            _,frame=spline.frame(t)
            for axis in frame:self.assertAlmostEqual(length(axis),1,places=5)


if __name__=='__main__':unittest.main()
