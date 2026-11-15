#!/usr/bin/env python3
"""Export native resources from the supplied /images originals; never edit them."""
from pathlib import Path
import argparse
import struct
from PIL import Image, ImageFilter, ImageOps
ROOT=Path(__file__).resolve().parents[1]

def write_bitmap(logo,path):
    width,height=logo.size;pixels=bytearray()
    for y in range(height-1,-1,-1):
        for x in range(width):
            r,g,b,a=logo.getpixel((x,y));pixels.extend(((b*a+127)//255,(g*a+127)//255,(r*a+127)//255,a))
    # Premultiplied BGRA for LoadImage(LR_CREATEDIBSECTION) + AlphaBlend.
    header=struct.pack('<2sIHHI',b'BM',54+len(pixels),0,0,54)+struct.pack('<IiiHHIIiiII',40,width,height,1,32,0,len(pixels),0,0,0,0)
    path.write_bytes(header+pixels)

def build(output):
    output.mkdir(parents=True,exist_ok=True)
    frames=[]
    for source in sorted((ROOT/'images').glob('reriff-icon-*x*.png')):
        with Image.open(source) as im:
            w,h=im.size
            if w!=h or not 1<=w<=256:raise ValueError('Expected square icon up to 256 pixels')
        frames.append((w,source.read_bytes()))
    if not frames:raise ValueError('Missing application icons')
    frames.sort();offset=6+16*len(frames);directory=bytearray(struct.pack('<3H',0,1,len(frames)))
    for size,data in frames:
        directory.extend(struct.pack('<4B2H2I',size%256,size%256,0,0,1,32,len(data),offset));offset+=len(data)
    (output/'reriff.ico').write_bytes(directory+b''.join(data for _,data in frames))
    with Image.open(ROOT/'images/reriff-logo.png') as source:
        logo=source.convert('RGBA');bounds=logo.getchannel('A').getbbox()
        if not bounds:raise ValueError('Logo is entirely transparent')
        logo=logo.crop(bounds);logo.thumbnail((480,80),Image.Resampling.LANCZOS)
        preview=Image.new('RGBA',logo.size,(26,26,26,255));preview.alpha_composite(logo);preview.save(output/'reriff-logo.png')
        write_bitmap(logo,output/'reriff-logo.bmp')
        # About one screen pixel of outline at the launcher's button size.
        padded=ImageOps.expand(logo,border=4,fill=(0,0,0,0))
        outline=Image.new('RGBA',padded.size,(0,0,0,0))
        outline.putalpha(padded.getchannel('A').filter(ImageFilter.MaxFilter(9)))
        outline.alpha_composite(padded)
        outline.save(output/'reriff-logo-button.png')
        write_bitmap(outline,output/'reriff-logo-button.bmp')
    return output/'reriff.ico',output/'reriff-logo.bmp'
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'build/ui-assets');args=parser.parse_args();print(*build(args.output),sep='\n')
