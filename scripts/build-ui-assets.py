#!/usr/bin/env python3
"""Export native resources from the supplied /images originals; never edit them."""
from pathlib import Path
import argparse
import struct
from PIL import Image, ImageFilter, ImageOps
ROOT=Path(__file__).resolve().parents[1]

PROMPT_ORDER=['a','b','x','y','dpad_up','dpad_down','dpad_left','dpad_right',
             'lb','rb','lt','rt','ls','ls_up','ls_down','ls_left',
             'ls_right','l3','rs','rs_up','rs_down','rs_left',
             'rs_right','r3','start','back','guide','misc',
             'p1','p2','p3','p4','touchpad']

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
        # A lighter outline: two asset pixels, retaining the existing canvas size.
        padded=ImageOps.expand(logo,border=4,fill=(0,0,0,0))
        outline=Image.new('RGBA',padded.size,(0,0,0,0))
        outline.putalpha(padded.getchannel('A').filter(ImageFilter.MaxFilter(5)))
        outline.alpha_composite(padded)
        outline.save(output/'reriff-logo-button.png')
        write_bitmap(outline,output/'reriff-logo-button.bmp')
    promptdir=ROOT/'images/prompts/png'
    sheet=Image.new('RGBA',(8*32,5*32),(0,0,0,0))
    for index,name in enumerate(PROMPT_ORDER):
        with Image.open(promptdir/(name+'.png')) as source:
            glyph=source.convert('RGBA')
            if glyph.size!=(32,32):
                glyph.thumbnail((32,32),Image.Resampling.LANCZOS)
        x=(index%8)*32+(32-glyph.width)//2
        y=(index//8)*32+(32-glyph.height)//2
        sheet.alpha_composite(glyph,(x,y))
    write_bitmap(sheet,output/'prompts.bmp')
    return output/'reriff.ico',output/'reriff-logo.bmp',output/'prompts.bmp'
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'build/ui-assets');args=parser.parse_args();print(*build(args.output),sep='\n')
