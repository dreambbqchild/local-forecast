#include "DrawServiceObjC.h"
#include "ImageCache.h"
#include "ImageCacheInternal.h"

#include <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <Foundation/Foundation.h>
#include <UniformTypeIdentifiers/UTCoreTypes.h>

#include <string>

const char* FontRegular = "Ubuntu_Mono/UbuntuMono-Regular.ttf",
          * FontItalic = "Ubuntu_Mono/UbuntuMono-Italic.ttf",
          * FontBold = "Ubuntu_Mono/UbuntuMono-Bold.ttf",
          * FontBoldItalic = "Ubuntu_Mono/UbuntuMono-BoldItalic.ttf";

CFStringRef pngIdentifier = (CFStringRef)CFBridgingRetain(UTTypePNG.identifier);
SafeReleaseInt32 imageDpi(144);
const double dpiScaleFactor = 2.0;

class DrawTextContext : public IDrawTextContextObjC {
    private:
        double fontSize, textShadowBlur, lineSpacing;
        CGSize textShadowOffset;
        const char* fontString;

        NSFont* font;
        NSColor* foregroundColor;
        NSColor* strokeColor = nullptr;
        double strokeThickness = 0.0;
        NSMutableAttributedString* workingString;

    public:
        DrawTextContext() : textShadowOffset({}), textShadowBlur(-1), lineSpacing(0)
        {
            workingString = [[NSMutableAttributedString alloc] initWithString:@""];
            SelectFontFileAtPath(FontRegular, 12);
            SetTextFillColor(PredefinedColors::black);
        }

        void SelectFontFileAtPath(const char* path, double fontSize)
        {
            this->fontSize = fontSize;
            fontString = path;

            NSString *nsPath = [[NSString alloc] initWithUTF8String:path];
            NSURL *nsFontUrl = [NSURL fileURLWithPath:nsPath];
                
            CFArrayRef arrFontDescriptors = CTFontManagerCreateFontDescriptorsFromURL((CFURLRef)nsFontUrl);
            CTFontDescriptorRef fontDescriptor = (CTFontDescriptorRef)CFArrayGetValueAtIndex( arrFontDescriptors, 0 );
                
            font = (__bridge_transfer  NSFont*)CTFontCreateWithFontDescriptor(fontDescriptor, fontSize, nil);
            CFRelease(arrFontDescriptors);
        }

        void SetFontSize(double fontSize)
        {
            SelectFontFileAtPath(fontString, fontSize);
        }

        void SetLineSpacing(double lineSpacing)
        {
            this->lineSpacing = lineSpacing;
        }

        void SetTextFillColor(DSColor color)
        {
            foregroundColor = [NSColor colorWithRed:ToComponentDouble(color.r) green:ToComponentDouble(color.g) blue:ToComponentDouble(color.b) alpha:ToComponentDouble(color.a)];
        }

        void SetTextStrokeColorWithThickness(DSColor color, double thickness)
        {
            if(thickness)
            {
                strokeColor = [NSColor colorWithRed:ToComponentDouble(color.r) green:ToComponentDouble(color.g) blue:ToComponentDouble(color.b) alpha:ToComponentDouble(color.a)];
                strokeThickness = -thickness;
            }
            else 
            {                
                strokeColor = nullptr;
                strokeThickness = 0.0;
            }
        }

        void SetTextShadow(DSSize offset, double blur) 
        {
            textShadowOffset = ToCG(offset);
            textShadowBlur = blur;
        }

        void SetFontOptions(FontOptions options)
        {    
            if(options == FontOptions::Bold)
                SelectFontFileAtPath(FontBold, fontSize);
            else if(options == FontOptions::Italic)
                SelectFontFileAtPath(FontItalic, fontSize);
            else if(options == FontOptions::BoldItalic)
                SelectFontFileAtPath(FontBoldItalic, fontSize);
            else
                SelectFontFileAtPath(FontRegular, fontSize);
        }

        void AddText(const char* text)
        {
            NSString* str = [[NSString alloc] initWithUTF8String: text];
            
            NSMutableParagraphStyle *paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
            paragraphStyle.lineSpacing = lineSpacing;

            NSDictionary* textAttributes = strokeThickness 
                ? @{
                    NSFontAttributeName: font,
                    NSStrokeWidthAttributeName: [NSNumber numberWithDouble:strokeThickness],
                    NSStrokeColorAttributeName: strokeColor,
                    NSForegroundColorAttributeName: foregroundColor,
                    NSParagraphStyleAttributeName: paragraphStyle
                }
                : @{
                    NSFontAttributeName: font,
                    NSForegroundColorAttributeName: foregroundColor,
                    NSParagraphStyleAttributeName: paragraphStyle
                } ;

            NSAttributedString *concating = [[NSAttributedString alloc] initWithString:str attributes: textAttributes];

            [workingString appendAttributedString: concating];
        }

        void AddImage(const char* path)
        {
            AddImage(path, {0, 0, fontSize, fontSize});
        }

        void AddImage(const char* path, DSRect bounds)
        {
            NSTextAttachment *textAttachment = [[NSTextAttachment alloc] init];
            auto cachedImage = GetCachedImageInternal(path);
            if(!cachedImage)
            {
                NSString *strPath = [[NSString alloc] initWithUTF8String: path];
                textAttachment.image = [[NSImage alloc] initWithContentsOfFile:strPath];
            }
            else
                textAttachment.image = [[NSImage alloc] initWithCGImage:cachedImage->GetImageRef() size: ToCG(cachedImage->GetSize())];

            textAttachment.bounds = ToCG(bounds);
            NSAttributedString *attachmentString = [NSAttributedString attributedStringWithAttachment:textAttachment];
            [workingString appendAttributedString: attachmentString];
        }

        DSSize Bounds()
        {
            return FromCG([workingString size]);
        }

        CGImageRef CreateImageRef()
        {
            auto size = [workingString size];
            size.width *= dpiScaleFactor;
            size.height *= dpiScaleFactor;

            NSImage *image = [NSImage imageWithSize:{size.width, size.height} flipped:NO drawingHandler:^BOOL(NSRect dstRect) 
            {
                CGContextRef imageContext = [[NSGraphicsContext currentContext] CGContext];
                CGContextScaleCTM(imageContext, dpiScaleFactor, dpiScaleFactor);

                if(textShadowBlur >= 0)
                    CGContextSetShadow(imageContext, textShadowOffset, textShadowBlur);

                [workingString drawAtPoint:NSMakePoint(0, 0)];    
                return YES;
            }];
            
            NSRect imageRect = NSMakeRect(0, 0, image.size.width, image.size.height);
            return [image CGImageForProposedRect:&imageRect context:NULL hints: nil];
        }
};

CGImageRef RunDrawTextContext(std::function<DSRect(IDrawTextContext*)> callback, DSRect& textBounds)
{
    CGImageRef result = nullptr;
    @autoreleasepool
    {
        DrawTextContext drawTextContext;
        textBounds = callback(&drawTextContext);
        result = drawTextContext.CreateImageRef();
        CFRetain(result);
    }
    return result;
}