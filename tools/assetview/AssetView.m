#import <Cocoa/Cocoa.h>
#include <math.h>

// All file offsets are big-endian Atari offsets, never host pointers.
static void Need(BOOL ok, NSString *why) {
    if (!ok) @throw [NSException exceptionWithName:@"AssetError" reason:why userInfo:nil];
}
static void Range(NSData *d, NSUInteger off, NSUInteger n) {
    Need(off <= d.length && n <= d.length-off, @"Truncated asset or invalid offset.");
}
static unsigned U16(NSData *d, NSUInteger o) { Range(d,o,2); const uint8_t *p=d.bytes; return (p[o]<<8)|p[o+1]; }
static uint32_t U32(NSData *d, NSUInteger o) { return ((uint32_t)U16(d,o)<<16)|U16(d,o+2); }
static NSData *Read(NSString *path) {
    NSDictionary *a=[[NSFileManager defaultManager] attributesOfItemAtPath:path error:nil];
    Need(a && [a fileSize]<=64*1024*1024, @"Cannot read file, or file exceeds 64 MiB.");
    NSData *d=[NSData dataWithContentsOfFile:path]; Need(d!=nil,@"Cannot read file."); return d;
}
@interface AVFrame : NSObject
@property NSUInteger width,height,planes,maskPlanes;
@property NSData *pixels,*mask;
@end
@implementation AVFrame
@end
@interface AVAsset : NSObject
@property NSString *path,*kind,*paletteName;
@property NSMutableArray<AVFrame *> *frames;
@property NSArray<NSNumber *> *sequence;
@property NSData *charmap;
@property NSArray<NSValue *> *regions;
@property NSUInteger first,last,space,kerning;
@property double fps;
@property NSData *palette;
@end
@implementation AVAsset
@end
static NSData *DefaultPalette(void) {
    uint8_t p[48]; for(int i=0;i<16;i++) for(int c=0;c<3;c++) p[i*3+c]=i*17;
    return [NSData dataWithBytes:p length:48];
}
static NSData *Palette(NSData *d, NSUInteger off) {
    Range(d,off,32); uint8_t p[48];
    for(int i=0;i<16;i++) { unsigned w=U16(d,off+i*2);
        for(int c=0;c<3;c++) {unsigned n=(w>>(8-c*4))&15; p[i*3+c]=(((n&7)<<1)|(n>>3))*17;}
    } return [NSData dataWithBytes:p length:48];
}
static NSData *LoadPalette(NSString *path) {
    NSData *d=Read(path);
    if(d.length==32) return Palette(d,0);
    Need(d.length>=32034 && U16(d,0)==0,@"Palette must be a 32-byte PAL or uncompressed low-resolution PI1.");
    return Palette(d,2);
}
static AVFrame *Decode(NSData *d, NSUInteger o) {
    Range(d,o,16); NSUInteger w=U16(d,o+8),h=U16(d,o+10),p=U16(d,o+12),mp=U16(d,o+14);
    Need(w && h && w<=4096 && h<=4096 && w*h<=4*1024*1024 && p>=1 && p<=4 && mp<=4,@"Unsupported sprite dimensions or plane count (expected 1–4 graphics planes).");
    NSUInteger words=(w+15)/16, go=o+(NSUInteger)U32(d,o),mo=o+(NSUInteger)U32(d,o+4);
    Range(d,go,words*h*p*2); if(mp)Range(d,mo,words*h*2);
    NSMutableData *pix=[NSMutableData dataWithLength:w*h],*mask=[NSMutableData dataWithLength:w*h];
    uint8_t *ip=pix.mutableBytes,*ap=mask.mutableBytes;
    for(NSUInteger y=0;y<h;y++) for(NSUInteger x=0;x<w;x++) {
        NSUInteger wi=y*words+x/16,bit=0x8000>>(x&15),i=y*w+x;
        for(NSUInteger plane=0;plane<p;plane++) if(U16(d,go+(wi*p+plane)*2)&bit) ip[i]|=1<<plane;
        // Old RG files omit mask storage entirely when the plane count is zero.
        ap[i]=(mp && (U16(d,mo+wi*2)&bit))?1:0;
    }
    AVFrame *f=[AVFrame new]; f.width=w;f.height=h;f.planes=p;f.maskPlanes=mp;f.pixels=pix;f.mask=mask;return f;
}
static NSArray<NSNumber *> *Numbers(NSArray<NSString *> *lines) {
    NSMutableArray *a=[NSMutableArray new];
    for(NSString *line in lines) for(NSString *s in [line componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceCharacterSet]) {
        if(!s.length)continue; NSScanner *scanner=[NSScanner scannerWithString:s]; long long n;
        Need([scanner scanLongLong:&n] && scanner.isAtEnd && n>=0 && n<=65535,@"Invalid ANIM8R number."); [a addObject:@(n)];
    }
    return a;
}
static void LoadAnim(AVAsset *a, NSData *script) {
    NSString *text=[[NSString alloc] initWithData:script encoding:NSUTF8StringEncoding];
    if(!text)text=[[NSString alloc] initWithData:script encoding:NSISOLatin1StringEncoding];
    Need(text!=nil,@"Cannot decode ANIM8R script.");
    NSMutableDictionary<NSString *,NSMutableArray *> *tags=[NSMutableDictionary new]; NSString *tag=nil;
    NSSet *known=[NSSet setWithArray:@[@"GRAPHIC",@"WIDTH",@"HEIGHT",@"X",@"Y",@"FRAMECOUNT",@"FRAMES",@"TIME",@"EOF"]];
    for(NSString *raw in [text componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet]) {
        NSString *line=[raw stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
        if(!line.length || [line hasPrefix:@"#"] || [line hasPrefix:@";"])continue;
        if([line hasPrefix:@"["] && [line hasSuffix:@"]"]) {
            tag=[[[line substringWithRange:NSMakeRange(1,line.length-2)] stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet] uppercaseString];
            Need([known containsObject:tag],@"Unknown ANIM8R command."); if([tag isEqual:@"EOF"])break;
            Need(tags[tag]==nil,@"Duplicate ANIM8R command.");tags[tag]=[NSMutableArray new];
        } else {Need(tag!=nil,@"ANIM8R parameter without a command.");[tags[tag] addObject:line];}
    }
    Need([tags[@"GRAPHIC"] count]==1,@"ANIM8R needs one [GRAPHIC] filename.");
    NSString *graphic=[tags[@"GRAPHIC"][0] stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"\""]];
    graphic=[graphic stringByReplacingOccurrencesOfString:@"\\" withString:@"/"];
    NSString *path=graphic.isAbsolutePath?graphic:[[a.path stringByDeletingLastPathComponent] stringByAppendingPathComponent:graphic];
    // Atari names are case-insensitive, including when run on case-sensitive hosts.
    if(![[NSFileManager defaultManager] fileExistsAtPath:path]) {
        NSString *dir=[path stringByDeletingLastPathComponent],*name=path.lastPathComponent;
        for(NSString *candidate in [[NSFileManager defaultManager] contentsOfDirectoryAtPath:dir error:nil])
            if([candidate caseInsensitiveCompare:name]==NSOrderedSame){path=[dir stringByAppendingPathComponent:candidate];break;}
    }
    NSData *d=Read(path);Need(d.length>=32034 && U16(d,0)==0,@"ANIM8R [GRAPHIC] must be uncompressed low-resolution PI1.");
    NSArray *wv=Numbers(tags[@"WIDTH"]),*hv=Numbers(tags[@"HEIGHT"]),*cv=Numbers(tags[@"FRAMECOUNT"]),*tv=Numbers(tags[@"TIME"]);
    Need(wv.count==1 && hv.count==1 && cv.count==1 && tv.count==1,@"ANIM8R needs WIDTH, HEIGHT, FRAMECOUNT and TIME.");
    NSUInteger w=[wv[0] unsignedIntegerValue],h=[hv[0] unsignedIntegerValue],count=[cv[0] unsignedIntegerValue],time=[tv[0] unsignedIntegerValue];
    Need(w && w<=320 && h && h<=200 && count && count<=4096 && time,@"Invalid ANIM8R dimensions, frame count or TIME.");
    NSArray *seq=Numbers(tags[@"FRAMES"]); Need(seq.count>0 && seq.count<=4096,@"ANIM8R needs a nonempty [FRAMES] sequence.");
    seq=[seq subarrayWithRange:NSMakeRange(0,MIN(count,seq.count))];
    NSUInteger max=0;for(NSNumber *n in seq)max=MAX(max,n.unsignedIntegerValue);
    NSUInteger cols=320/w, stride=((w+15)/16)*16;
    Need(max<4096 && (max/cols+1)*h<=200 && MIN(cols,max+1)*stride<=320,@"ANIM8R frame lies outside the PI1 sheet.");
    for(NSUInteger n=0;n<=max;n++) {
        NSUInteger ox=(n%cols)*stride,oy=(n/cols)*h;
        Need(ox+w<=320 && oy+h<=200,@"ANIM8R frame outside PI1.");
        NSMutableData *pix=[NSMutableData dataWithLength:w*h],*mask=[NSMutableData dataWithLength:w*h];uint8_t *p=pix.mutableBytes;
        for(NSUInteger y=0;y<h;y++)for(NSUInteger x=0;x<w;x++){
            NSUInteger off=34+(oy+y)*160+((ox+x)/16)*8,bit=0x8000>>((ox+x)&15);
            for(int plane=0;plane<4;plane++)if(U16(d,off+plane*2)&bit)p[y*w+x]|=1<<plane;
        }
        AVFrame *f=[AVFrame new];f.width=w;f.height=h;f.planes=4;f.pixels=pix;f.mask=mask;[a.frames addObject:f];
    }
    a.sequence=seq;a.fps=50.0/time;a.palette=Palette(d,2);a.paletteName=path.lastPathComponent;a.kind=@"ANIM8R / PI1";
    // X/Y were explicitly unsupported by the original ANIM8R, and remain ignored.
}
static AVAsset *Load(NSString *path) {
    NSData *d=Read(path);AVAsset *a=[AVAsset new];a.path=path;a.frames=[NSMutableArray new];a.palette=DefaultPalette();a.paletteName=@"Default grayscale — load PAL/PI1";a.fps=10;
    Range(d,0,4);const uint8_t *b=d.bytes;
    if(!memcmp(b,"BSBK",4) || !memcmp(b,"KBSB",4)) {
        Need(U16(d,4)==0,@"Unsupported BSB version."); NSUInteger count=U16(d,6);Need(count && count<=4096,@"Invalid BSB sprite count.");Range(d,8,count*4);
        NSUInteger total=0;for(NSUInteger i=0;i<count;i++){AVFrame *f=Decode(d,U32(d,8+i*4));total+=f.width*f.height;Need(total<=16*1024*1024,@"Decoded sprite block exceeds 16 million pixels.");[a.frames addObject:f];}a.kind=@"BSB";
    } else if(!memcmp(b,"TNOF",4) || !memcmp(b,"FONT",4)) {
        Range(d,0,32);Need(U32(d,4)==0,@"Unsupported BFB version.");
        a.first=b[8];a.last=b[9];a.kerning=b[10];a.space=b[11];NSUInteger count=U16(d,16),table=U32(d,24),regions=U32(d,28),map=U32(d,20);
        Need(a.last>=a.first && count && count<=256,@"Invalid BFB character range/count.");Range(d,table,count*16);Range(d,regions,count*8);Range(d,map,a.last-a.first+1);
        a.charmap=[d subdataWithRange:NSMakeRange(map,a.last-a.first+1)];NSMutableArray *rs=[NSMutableArray new];NSUInteger total=0;
        for(NSUInteger i=0;i<count;i++) {AVFrame *f=Decode(d,table+i*16);total+=f.width*f.height;Need(total<=16*1024*1024,@"Decoded font exceeds 16 million pixels.");[a.frames addObject:f];
            NSUInteger o=regions+i*8;unsigned x0=U16(d,o),y0=U16(d,o+2),x1=U16(d,o+4),y1=U16(d,o+6);
            Need(x0<=x1 && y0<=y1 && x1<f.width && y1<f.height,@"Invalid BFB glyph region.");[rs addObject:[NSValue valueWithRect:NSMakeRect(x0,y0,x1-x0+1,y1-y0+1)]];
        }
        const uint8_t *mapBytes=a.charmap.bytes;for(NSUInteger i=0;i<a.charmap.length;i++)Need(mapBytes[i]==255 || mapBytes[i]<count,@"BFB character points outside glyph table.");
        a.regions=rs;a.kind=@"BFB";
    } else {LoadAnim(a,d);}
    if(!a.sequence){NSMutableArray *s=[NSMutableArray new];for(NSUInteger i=0;i<a.frames.count;i++)[s addObject:@(i)];a.sequence=s;
        NSString *base=[path stringByDeletingPathExtension];for(NSString *ext in @[@"PAL",@"pal",@"PI1",@"pi1"]){NSString *p=[base stringByAppendingPathExtension:ext];if([[NSFileManager defaultManager]fileExistsAtPath:p]){a.palette=LoadPalette(p);a.paletteName=p.lastPathComponent;break;}}
    }return a;
}
static NSBitmapImageRep *Bitmap(AVFrame *f, NSData *palette, BOOL masks) {
    NSBitmapImageRep *r=[[NSBitmapImageRep alloc]initWithBitmapDataPlanes:NULL pixelsWide:f.width pixelsHigh:f.height bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:f.width*4 bitsPerPixel:32];
    const uint8_t *p=f.pixels.bytes,*m=f.mask.bytes,*pal=palette.bytes;uint8_t *out=r.bitmapData;
    for(NSUInteger i=0;i<f.width*f.height;i++){for(int c=0;c<3;c++)out[i*4+c]=masks?(m[i]?0:255):pal[p[i]*3+c];out[i*4+3]=masks?255:(m[i]?0:255);}return r;
}
static void DrawFrame(AVFrame *f, NSData *palette, BOOL masks, NSRect target, NSRect source) {
    NSBitmapImageRep *rep=Bitmap(f,palette,masks);NSImage *image=[[NSImage alloc]initWithSize:NSMakeSize(f.width,f.height)];[image addRepresentation:rep];
    NSGraphicsContext.currentContext.imageInterpolation=NSImageInterpolationNone;
    [image drawInRect:target fromRect:source operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
}
static NSDictionary *Info(AVAsset *a) {
    NSMutableArray *frames=[NSMutableArray new];for(AVFrame *f in a.frames)[frames addObject:@{@"width":@(f.width),@"height":@(f.height),@"planes":@(f.planes),@"mask_planes":@(f.maskPlanes)}];
    return @{@"type":a.kind,@"frames":frames,@"sequence":a.sequence,@"fps":@(a.fps),@"palette":a.paletteName,@"char_first":@(a.first),@"char_last":@(a.last)};
}

@class AVController;
@interface AVCanvas : NSView <NSDraggingDestination>
@property AVController *owner;
@end
@interface AVController : NSObject <NSApplicationDelegate,NSWindowDelegate,NSTextFieldDelegate>
@property NSWindow *window;
@property AVCanvas *canvas;
@property NSScrollView *scroll;
@property NSTextField *status,*sample;
@property NSButton *play,*sheet,*mask;
@property NSSlider *speed;
@property NSPopUpButton *zoom;
@property AVAsset *asset;
@property NSUInteger position;
@property NSTimer *timer;
@property BOOL playing;
@property NSString *initial;
- (void)openPath:(NSString *)path;
- (void)refresh;
- (void)step:(NSInteger)amount;
- (void)toggle:(id)sender;
@end
@implementation AVCanvas
- (BOOL)isFlipped{return YES;}
- (BOOL)acceptsFirstResponder{return YES;}
- (instancetype)initWithFrame:(NSRect)frame {if((self=[super initWithFrame:frame]))[self registerForDraggedTypes:@[NSPasteboardTypeFileURL]];return self;}
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)s{return NSDragOperationCopy;}
- (BOOL)performDragOperation:(id<NSDraggingInfo>)s {NSArray *urls=[s.draggingPasteboard readObjectsForClasses:@[NSURL.class]options:@{NSPasteboardURLReadingFileURLsOnlyKey:@YES}];if(!urls.count)return NO;[self.owner openPath:[urls[0] path]];return YES;}
- (void)keyDown:(NSEvent *)e {if(e.keyCode==123)[self.owner step:-1];else if(e.keyCode==124)[self.owner step:1];else if([e.charactersIgnoringModifiers isEqual:@" "])[self.owner toggle:nil];else [super keyDown:e];}
- (void)drawRect:(NSRect)dirty {
    [[NSColor colorWithWhite:.13 alpha:1]setFill];NSRectFill(dirty);
    // A checker makes opaque black pixels and transparent pixels distinguishable.
    NSInteger x0=floor(NSMinX(dirty)/16),y0=floor(NSMinY(dirty)/16),x1=ceil(NSMaxX(dirty)/16),y1=ceil(NSMaxY(dirty)/16);
    [[NSColor colorWithWhite:.19 alpha:1]setFill];for(NSInteger y=y0;y<y1;y++)for(NSInteger x=x0;x<x1;x++)if((x+y)&1)NSRectFill(NSMakeRect(x*16,y*16,16,16));
    AVAsset *a=self.owner.asset;if(!a){[@"Open a BSB, BFB or ANIM8R script — or drop it here." drawAtPoint:NSMakePoint(24,24)withAttributes:@{NSForegroundColorAttributeName:NSColor.whiteColor}];return;}
    double z=self.owner.zoom.selectedItem.title.doubleValue;BOOL sheet=self.owner.sheet.state==NSControlStateValueOn,masks=self.owner.mask.state==NSControlStateValueOn;
    NSMutableArray<NSNumber *> *indices=[NSMutableArray new];if(sheet)for(NSUInteger i=0;i<a.frames.count;i++)[indices addObject:@(i)];else[indices addObject:a.sequence[self.owner.position]];
    NSUInteger mw=1,mh=1;for(AVFrame *f in a.frames){mw=MAX(mw,f.width);mh=MAX(mh,f.height);}
    NSUInteger cols=sheet?MAX(1,(NSUInteger)(self.bounds.size.width/(mw*z+24))):1;
    CGFloat top=12;
    if(a.charmap){
        NSData *text=[self.owner.sample.stringValue dataUsingEncoding:NSISOLatin1StringEncoding allowLossyConversion:YES];const uint8_t *chars=text.bytes,*map=a.charmap.bytes;CGFloat x=12;
        for(NSUInteger n=0;n<text.length;n++){
            unsigned c=chars[n];if(c==32){x+=a.space*z;continue;}
            if(c<a.first || c>a.last || map[c-a.first]==255)continue;
            NSUInteger idx=map[c-a.first];AVFrame *f=a.frames[idx];NSRect region=a.regions[idx].rectValue;
            // Draw the full glyph shifted left by its region minimum, as GodLib does.
            DrawFrame(f,a.palette,masks,NSMakeRect(x-region.origin.x*z,12,f.width*z,f.height*z),NSZeroRect);
            x+=(region.size.width+a.kerning)*z;
            if(x>self.bounds.size.width)break;
        }
        top+=mh*z+30;
    }
    for(NSUInteger n=0;n<indices.count;n++) {
        NSUInteger idx=indices[n].unsignedIntegerValue;AVFrame *f=a.frames[idx];NSPoint origin=NSMakePoint(12+(n%cols)*(mw*z+24),top+(n/cols)*(mh*z+28));
        NSRect target=NSMakeRect(origin.x,origin.y,f.width*z,f.height*z);if(!NSIntersectsRect(target,dirty))continue;
        DrawFrame(f,a.palette,masks,target,NSZeroRect);
        NSString *label=[NSString stringWithFormat:@"#%lu",(unsigned long)idx];
        if(a.charmap){const uint8_t *map=a.charmap.bytes;NSMutableString *chars=[NSMutableString new];for(NSUInteger c=a.first;c<=a.last;c++)if(map[c-a.first]==idx)[chars appendFormat:@" $%02lX",(unsigned long)c];label=[label stringByAppendingString:chars];}
        [label drawAtPoint:NSMakePoint(origin.x,origin.y+f.height*z+3)withAttributes:@{NSForegroundColorAttributeName:NSColor.whiteColor,NSFontAttributeName:[NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular]}];
    }
}
@end

static NSButton *Button(NSString *title,id target,SEL action,CGFloat x,CGFloat w) {
    NSButton *b=[NSButton buttonWithTitle:title target:target action:action];b.frame=NSMakeRect(x,0,w,30);return b;
}
@implementation AVController
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app{return YES;}
- (void)applicationDidFinishLaunching:(NSNotification *)note {
    NSMenu *bar=[NSMenu new];NSMenuItem *app=[NSMenuItem new];[bar addItem:app];NSMenu *menu=[NSMenu new];[menu addItemWithTitle:@"About GodLib AssetView" action:@selector(orderFrontStandardAboutPanel:)keyEquivalent:@""];[menu addItem:NSMenuItem.separatorItem];[menu addItemWithTitle:@"Quit AssetView" action:@selector(terminate:)keyEquivalent:@"q"];app.submenu=menu;
    NSMenuItem *file=[NSMenuItem new];[bar addItem:file];NSMenu *fm=[[NSMenu alloc]initWithTitle:@"File"];NSMenuItem *op=[fm addItemWithTitle:@"Open…" action:@selector(open:)keyEquivalent:@"o"];op.target=self;NSMenuItem *pal=[fm addItemWithTitle:@"Load Palette…" action:@selector(palette:)keyEquivalent:@"p"];pal.target=self;NSMenuItem *ex=[fm addItemWithTitle:@"Export Frame PNG…" action:@selector(export:)keyEquivalent:@"s"];ex.target=self;file.submenu=fm;
    NSMenuItem *edit=[NSMenuItem new];[bar addItem:edit];NSMenu *em=[[NSMenu alloc]initWithTitle:@"Edit"];for(NSArray *spec in @[@[@"Cut",@"cut:",@"x"],@[@"Copy",@"copy:",@"c"],@[@"Paste",@"paste:",@"v"],@[@"Select All",@"selectAll:",@"a"]])[em addItemWithTitle:spec[0]action:NSSelectorFromString(spec[1])keyEquivalent:spec[2]];edit.submenu=em;NSApp.mainMenu=bar;
    self.window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,1040,720)styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];self.window.title=@"GodLib AssetView";self.window.minSize=NSMakeSize(1020,500);self.window.delegate=self;
    NSView *root=self.window.contentView;CGFloat height=root.bounds.size.height;
    NSView *tools=[[NSView alloc]initWithFrame:NSMakeRect(8,height-40,1020,32)];tools.autoresizingMask=NSViewMinYMargin;[root addSubview:tools];
    [tools addSubview:Button(@"Open…",self,@selector(open:),0,78)];[tools addSubview:Button(@"Palette…",self,@selector(palette:),82,86)];[tools addSubview:Button(@"◀",self,@selector(prev:),174,38)];
    self.play=Button(@"Play",self,@selector(toggle:),216,60);[tools addSubview:self.play];[tools addSubview:Button(@"▶",self,@selector(next:),280,38)];
    self.sheet=[NSButton checkboxWithTitle:@"Sheet" target:self action:@selector(changed:)];self.sheet.frame=NSMakeRect(328,3,70,24);self.sheet.state=NSControlStateValueOn;[tools addSubview:self.sheet];
    self.mask=[NSButton checkboxWithTitle:@"Mask" target:self action:@selector(changed:)];self.mask.frame=NSMakeRect(400,3,66,24);[tools addSubview:self.mask];
    self.zoom=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(475,1,70,28)pullsDown:NO];[self.zoom addItemsWithTitles:@[@"1×",@"2×",@"3×",@"4×",@"6×",@"8×"]];[self.zoom selectItemAtIndex:2];self.zoom.target=self;self.zoom.action=@selector(changed:);[tools addSubview:self.zoom];
    NSTextField *fps=[NSTextField labelWithString:@"FPS"];fps.frame=NSMakeRect(556,7,30,18);[tools addSubview:fps];self.speed=[NSSlider sliderWithValue:10 minValue:50.0/65535 maxValue:50 target:self action:@selector(speedChanged:)];self.speed.frame=NSMakeRect(587,5,135,20);[tools addSubview:self.speed];[tools addSubview:Button(@"Export PNG…",self,@selector(export:),736,116)];
    self.sample=[[NSTextField alloc]initWithFrame:NSMakeRect(12,42,1016,26)];self.sample.placeholderString=@"BFB sample text (byte characters / ISO-8859-1)";self.sample.stringValue=@"THE QUICK BROWN FOX 0123456789";self.sample.delegate=self;self.sample.autoresizingMask=NSViewWidthSizable;[root addSubview:self.sample];
    self.status=[NSTextField labelWithString:@"Open BSB / BFB / ANIM8R TXT. Space: play/pause · ←/→: frame"];self.status.frame=NSMakeRect(12,8,1016,28);self.status.autoresizingMask=NSViewWidthSizable;self.status.lineBreakMode=NSLineBreakByTruncatingMiddle;[root addSubview:self.status];
    self.scroll=[[NSScrollView alloc]initWithFrame:NSMakeRect(8,78,1024,height-126)];self.scroll.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;self.scroll.hasVerticalScroller=YES;self.scroll.hasHorizontalScroller=YES;self.canvas=[[AVCanvas alloc]initWithFrame:self.scroll.contentView.bounds];self.canvas.owner=self;self.scroll.documentView=self.canvas;[root addSubview:self.scroll];
    [self.window center];[self.window makeKeyAndOrderFront:nil];[self.window makeFirstResponder:self.canvas];[NSApp activateIgnoringOtherApps:YES];
    if(!self.initial)self.initial=[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"Demo/ROTATE.BSB"];
    if(self.initial)[self openPath:self.initial];
}
- (BOOL)application:(NSApplication *)app openFile:(NSString *)filename {if(self.window)[self openPath:filename];else self.initial=filename;return YES;}
- (void)open:(id)sender {NSOpenPanel *p=[NSOpenPanel openPanel];p.message=@"Open BSB, BFB or ANIM8R text script";if([p runModal]==NSModalResponseOK)[self openPath:p.URL.path];}
- (void)error:(NSString *)text {NSAlert *alert=[NSAlert new];alert.messageText=@"Cannot open asset";alert.informativeText=text;[alert runModal];}
- (void)openPath:(NSString *)path {
    if([@[@"pal",@"pi1"]containsObject:path.pathExtension.lowercaseString] && self.asset){@try{self.asset.palette=LoadPalette(path);self.asset.paletteName=path.lastPathComponent;[self refresh];}@catch(NSException *e){[self error:e.reason];}return;}
    @try{AVAsset *asset=Load(path);[self.timer invalidate];self.timer=nil;self.playing=NO;self.play.title=@"Play";self.asset=asset;self.position=0;self.speed.doubleValue=asset.fps;self.window.title=[@"GodLib AssetView — " stringByAppendingString:path.lastPathComponent];[self refresh];[self.window makeFirstResponder:self.canvas];}@catch(NSException *e){[self error:e.reason];}
}
- (void)palette:(id)sender {if(!self.asset)return;NSOpenPanel *p=[NSOpenPanel openPanel];p.message=@"Palette: 32-byte PAL or uncompressed PI1";if([p runModal]==NSModalResponseOK)@try{self.asset.palette=LoadPalette(p.URL.path);self.asset.paletteName=p.URL.lastPathComponent;[self refresh];}@catch(NSException *e){[self error:e.reason];}}
- (void)refresh {
    if(!self.asset)return;AVAsset *a=self.asset;NSUInteger mw=1,mh=1;for(AVFrame *f in a.frames){mw=MAX(mw,f.width);mh=MAX(mh,f.height);}double z=self.zoom.selectedItem.title.doubleValue;
    CGFloat width=MAX(self.scroll.contentSize.width,mw*z+24);NSUInteger cols=MAX(1,(NSUInteger)(width/(mw*z+24))),rows=self.sheet.state?(a.frames.count+cols-1)/cols:1;
    self.canvas.frame=NSMakeRect(0,0,width,MAX(self.scroll.contentSize.height,rows*(mh*z+28)+12+(a.charmap?mh*z+30:0)));[self.canvas setNeedsDisplay:YES];
    AVFrame *f=a.frames[[a.sequence[self.position]unsignedIntegerValue]];self.status.stringValue=[NSString stringWithFormat:@"%@ · %lu frames · step %lu/%lu → #%@ · %lu×%lu · %lu planes · %.2f FPS · %@",a.kind,(unsigned long)a.frames.count,(unsigned long)self.position+1,(unsigned long)a.sequence.count,a.sequence[self.position],(unsigned long)f.width,(unsigned long)f.height,(unsigned long)f.planes,self.speed.doubleValue,a.paletteName];
    self.sample.hidden=a.charmap==nil;
}
- (void)changed:(id)sender {[self refresh];[self.window makeFirstResponder:self.canvas];}
- (void)windowDidResize:(NSNotification *)n {[self refresh];}
- (void)step:(NSInteger)amount {if(!self.asset)return;NSInteger n=self.asset.sequence.count;self.position=(self.position+n+amount)%n;[self refresh];}
- (void)prev:(id)sender {[self step:-1];[self.window makeFirstResponder:self.canvas];}
- (void)next:(id)sender {[self step:1];[self.window makeFirstResponder:self.canvas];}
- (void)tick:(id)sender {[self step:1];}
- (void)startTimer {[self.timer invalidate];if(self.playing){self.timer=[NSTimer timerWithTimeInterval:1.0/self.speed.doubleValue target:self selector:@selector(tick:)userInfo:nil repeats:YES];[[NSRunLoop mainRunLoop]addTimer:self.timer forMode:NSRunLoopCommonModes];}}
- (void)toggle:(id)sender {if(!self.asset)return;self.playing=!self.playing;self.play.title=self.playing?@"Pause":@"Play";if(self.playing)self.sheet.state=NSControlStateValueOff;else[self.timer invalidate];[self startTimer];[self refresh];[self.window makeFirstResponder:self.canvas];}
- (void)speedChanged:(id)sender {[self startTimer];[self refresh];}
- (void)controlTextDidChange:(NSNotification *)n {[self.canvas setNeedsDisplay:YES];}
- (void)export:(id)sender {if(!self.asset)return;NSSavePanel *p=[NSSavePanel savePanel];p.nameFieldStringValue=@"frame.png";if([p runModal]!=NSModalResponseOK)return;AVFrame *f=self.asset.frames[[self.asset.sequence[self.position]unsignedIntegerValue]];NSData *png=[Bitmap(f,self.asset.palette,self.mask.state==NSControlStateValueOn)representationUsingType:NSBitmapImageFileTypePNG properties:@{}];NSError *error=nil;if(![png writeToURL:p.URL options:NSDataWritingAtomic error:&error])[self error:error.localizedDescription];}
@end

int main(int argc,const char **argv) {@autoreleasepool {
    if(argc>=3 && (!strcmp(argv[1],"--inspect") || !strcmp(argv[1],"--export"))) {
        @try{AVAsset *a=Load([NSString stringWithUTF8String:argv[2]]);
            if(!strcmp(argv[1],"--inspect")){NSData *j=[NSJSONSerialization dataWithJSONObject:Info(a)options:NSJSONWritingPrettyPrinted error:nil];fwrite(j.bytes,1,j.length,stdout);puts("");return 0;}
            Need(argc>=4,@"Usage: assetview --export input output.png [palette.pal|picture.pi1] [frame-index]");
            if(argc>=5)a.palette=LoadPalette([NSString stringWithUTF8String:argv[4]]);NSUInteger index=argc>=6?strtoul(argv[5],NULL,10):0;Need(index<a.frames.count,@"Frame index out of range.");
            NSData *png=[Bitmap(a.frames[index],a.palette,NO)representationUsingType:NSBitmapImageFileTypePNG properties:@{}];Need([png writeToFile:[NSString stringWithUTF8String:argv[3]]atomically:YES],@"Cannot write PNG.");return 0;
        }@catch(NSException *e){fprintf(stderr,"assetview: %s\n",e.reason.UTF8String);return 1;}
    }
    if(argc>1 && !strcmp(argv[1],"--help")){puts("GodLib AssetView: assetview [file.bsb|font.bfb|anim.txt]\n  --inspect FILE\n  --export FILE OUTPUT.png [PALETTE.pal|PI1] [FRAME]\nNative Cocoa UI: Open, Palette, Play, arrows, Sheet, Mask, Zoom, FPS, Export PNG.");return 0;}
    NSApplication *app=[NSApplication sharedApplication];[app setActivationPolicy:NSApplicationActivationPolicyRegular];AVController *controller=[AVController new];if(argc>1)controller.initial=[[NSString stringWithUTF8String:argv[1]]stringByStandardizingPath];app.delegate=controller;[app run];return 0;
}}
