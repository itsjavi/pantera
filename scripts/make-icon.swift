#!/usr/bin/env swift
// Draws the layers of Pantera's app icon (AppIcon.icon/Assets/*.png) on a 1024 pt canvas.
// Usage: swift scripts/make-icon.swift branding/AppIcon.icon
// The .icon's icon.json sets the background, glass, and the dark-appearance colors (the head
// turns silver on a near-black background). Edit it by hand or in Icon Composer; this script
// only rewrites Assets/. scripts/build-branding.sh runs it and compiles the result.
import AppKit

let iconOutput = CommandLine.arguments.dropFirst().first ?? "branding/AppIcon.icon"
let canvas: CGFloat = 1024

func color(_ hex: String) -> NSColor {
    let v = UInt32(hex.dropFirst(), radix: 16)!
    return NSColor(srgbRed: CGFloat(v >> 16 & 0xFF) / 255, green: CGFloat(v >> 8 & 0xFF) / 255,
                   blue: CGFloat(v & 0xFF) / 255, alpha: 1)
}

func bitmap(_ draw: () -> Void) -> NSBitmapImageRep {
    let rep = NSBitmapImageRep(
        bitmapDataPlanes: nil, pixelsWide: Int(canvas), pixelsHigh: Int(canvas), bitsPerSample: 8,
        samplesPerPixel: 4, hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB,
        bytesPerRow: 0, bitsPerPixel: 0
    )!
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
    draw()
    NSGraphicsContext.restoreGraphicsState()
    return rep
}

/// Fills a path with a gentle top-to-bottom gradient. Flat only: the system adds glass and shadows.
func fill(_ path: NSBezierPath, _ top: String, _ bottom: String) {
    NSGraphicsContext.saveGraphicsState()
    path.addClip()
    NSGradient(colors: [color(top), color(bottom)])!.draw(in: path.bounds, angle: -90)
    NSGraphicsContext.restoreGraphicsState()
}

/// Erases a shape from the current layer.
func cut(_ path: NSBezierPath) {
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current?.compositingOperation = .clear
    path.fill()
    NSGraphicsContext.restoreGraphicsState()
}

/// Maps a point traced from a reference photo of a black panther (1500 × 2000 px, facing left,
/// y down) onto the canvas, mirrored so the panther faces right.
func fromPhoto(_ x: CGFloat, _ y: CGFloat) -> NSPoint {
    let scale = canvas / 1500
    return NSPoint(x: canvas - (x - 200) * scale, y: (1760 - y) * scale - 30)
}

/// The panther's head and neck in profile: long, gently convex nose bridge, blunt nose, deep jaw
/// tucked over the throat, pointed ears with the far one just showing over the brow.
/// The neck runs off the bottom and side edges.
func headPath() -> NSBezierPath {
    // Cubic segments in photo coordinates: end, control 1, control 2.
    let segments: [(CGFloat, CGFloat, CGFloat, CGFloat, CGFloat, CGFloat)] = [
        (850, 1460, 660, 1820, 800, 1560),   // chest, up to the throat
        (590, 1535, 780, 1530, 680, 1550),   // jaw, underside
        (515, 1455, 540, 1530, 515, 1490),   // chin
        (432, 1302, 476, 1420, 436, 1356),   // upper lip
        (408, 1186, 374, 1290, 366, 1220),   // nose
        (560, 780, 450, 1050, 500, 880),     // nose bridge
        (700, 650, 600, 720, 650, 670),      // brow
        (724, 548, 708, 620, 716, 570),      // far ear, front
        (812, 612, 736, 556, 790, 590),      // far ear, back
        (985, 500, 880, 580, 950, 520),      // crown
        (1086, 326, 1010, 440, 1062, 366),   // near ear, front
        (1190, 470, 1110, 340, 1166, 410),   // near ear, back
        (1800, 1300, 1270, 640, 1460, 960),  // back of the neck
    ]
    let path = NSBezierPath()
    path.move(to: fromPhoto(540, 2100))
    for s in segments {
        path.curve(to: fromPhoto(s.0, s.1), controlPoint1: fromPhoto(s.2, s.3), controlPoint2: fromPhoto(s.4, s.5))
    }
    path.line(to: fromPhoto(1800, 2100))
    path.close()
    return path
}

/// An almond eye between its two corners in the photo, enlarged so it reads at small sizes.
func eyePath() -> NSBezierPath {
    let inner = fromPhoto(688, 955), outer = fromPhoto(768, 878)
    let mid = NSPoint(x: (inner.x + outer.x) / 2, y: (inner.y + outer.y) / 2)
    let length = hypot(outer.x - inner.x, outer.y - inner.y) * 1.4
    let angle = atan2(outer.y - inner.y, outer.x - inner.x) * 180 / .pi
    let eye = NSBezierPath()
    eye.move(to: NSPoint(x: -length / 2, y: 0))
    eye.curve(to: NSPoint(x: length / 2, y: 0), controlPoint1: NSPoint(x: -length / 5, y: length * 0.42),
              controlPoint2: NSPoint(x: length / 4, y: length * 0.36))
    eye.curve(to: NSPoint(x: -length / 2, y: 0), controlPoint1: NSPoint(x: length / 4, y: -length * 0.3),
              controlPoint2: NSPoint(x: -length / 5, y: -length * 0.34))
    eye.close()
    var transform = AffineTransform(rotationByDegrees: angle)
    transform.append(AffineTransform(translationByX: mid.x, byY: mid.y))
    eye.transform(using: transform)
    return eye
}

func pupilPath() -> NSBezierPath {
    let eye = eyePath().bounds
    return NSBezierPath(ovalIn: NSRect(x: eye.midX - 7, y: eye.midY - 20, width: 14, height: 40))
}

// icon.json lists these front to back: its first layer is drawn on top.
// The head is drawn in its light-appearance color; icon.json overrides it for dark.
let layers: [(String, NSBitmapImageRep)] = [
    ("eye", bitmap {
        fill(eyePath(), "#FFD84D", "#F2A516")
        fill(pupilPath(), "#050506", "#000000")
    }),
    ("head", bitmap {
        fill(headPath(), "#1A1C20", "#060708")
        cut(eyePath())
    }),
]

let assets = URL(fileURLWithPath: iconOutput).appendingPathComponent("Assets")
try? FileManager.default.removeItem(at: assets)
try FileManager.default.createDirectory(at: assets, withIntermediateDirectories: true)
for (name, rep) in layers {
    try rep.representation(using: .png, properties: [:])!.write(to: assets.appendingPathComponent("\(name).png"))
}
print("Wrote \(layers.count) layers to \(assets.path)")
