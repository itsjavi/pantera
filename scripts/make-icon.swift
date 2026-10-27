#!/usr/bin/env swift
// Draws the layer of Pantera's app icon (AppIcon.icon/Assets/head.png) on a 1024 pt canvas: a
// one-color panther (DECISION-10).
// Usage: swift scripts/make-icon.swift branding/AppIcon.icon [<svg output folder>]
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
        (850, 1460, 725, 1835, 815, 1580),   // chest, up to the throat
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
        (1246, 660, 1214, 520, 1244, 590),   // back of the skull
        (1560, 2100, 1300, 850, 1480, 1350), // nape, down the back of the neck to the shoulder
    ]
    let path = NSBezierPath()
    path.move(to: fromPhoto(650, 2100))
    for s in segments {
        path.curve(to: fromPhoto(s.0, s.1), controlPoint1: fromPhoto(s.2, s.3), controlPoint2: fromPhoto(s.4, s.5))
    }
    path.close()
    return path
}

/// A hair-like line along a cubic curve, tapering from `width` at its root to a point.
func taperedCurve(_ p0: NSPoint, _ c1: NSPoint, _ c2: NSPoint, _ p1: NSPoint, width: CGFloat) -> NSBezierPath {
    func point(_ t: CGFloat) -> NSPoint {
        let u = 1 - t
        let a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, d = t * t * t
        let x = a * p0.x + b * c1.x + c * c2.x + d * p1.x
        let y = a * p0.y + b * c1.y + c * c2.y + d * p1.y
        return NSPoint(x: x, y: y)
    }
    let steps = 64
    var left: [NSPoint] = [], right: [NSPoint] = []
    for i in 0...steps {
        let t = CGFloat(i) / CGFloat(steps)
        let p = point(t), ahead = point(min(t + 0.01, 1)), behind = point(max(t - 0.01, 0))
        let dx = ahead.x - behind.x, dy = ahead.y - behind.y
        let length = max(hypot(dx, dy), 0.0001), half = width * (1 - t) / 2
        left.append(NSPoint(x: p.x - dy / length * half, y: p.y + dx / length * half))
        right.append(NSPoint(x: p.x + dy / length * half, y: p.y - dx / length * half))
    }
    let path = NSBezierPath()
    path.move(to: left[0])
    for p in left.dropFirst() { path.line(to: p) }
    for p in right.reversed() { path.line(to: p) }
    path.close()
    return path
}

/// Long whiskers fanning back from the muzzle across the cheek, as in the photo.
func whiskerPaths() -> [NSBezierPath] {
    let whiskers: [(CGFloat, CGFloat, CGFloat, CGFloat, CGFloat, CGFloat, CGFloat, CGFloat)] = [
        (600, 1298, 760, 1226, 920, 1196, 1080, 1212),  // upper
        (606, 1324, 770, 1288, 940, 1288, 1110, 1318),  // middle
        (612, 1350, 770, 1352, 930, 1384, 1080, 1432),  // lower
    ]
    return whiskers.map { w in
        taperedCurve(fromPhoto(w.0, w.1), fromPhoto(w.2, w.3), fromPhoto(w.4, w.5), fromPhoto(w.6, w.7), width: 12)
    }
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

/// The whole panther as one even-odd shape: the head, with the eye and whiskers cut out to the
/// background and the pupil left inside the eye.
func pantherPath() -> NSBezierPath {
    let path = NSBezierPath()
    path.windingRule = .evenOdd
    for part in [headPath(), eyePath(), pupilPath()] + whiskerPaths() { path.append(part) }
    return path
}

// One layer, drawn in its light-appearance colors; icon.json overrides them for dark.
let layers: [(String, NSBitmapImageRep)] = [
    ("head", bitmap { fill(pantherPath(), "#1A1C20", "#060708") }),
]

let assets = URL(fileURLWithPath: iconOutput).appendingPathComponent("Assets")
try? FileManager.default.removeItem(at: assets)
try FileManager.default.createDirectory(at: assets, withIntermediateDirectories: true)
for (name, rep) in layers {
    try rep.representation(using: .png, properties: [:])!.write(to: assets.appendingPathComponent("\(name).png"))
}
print("Wrote \(layers.count) layers to \(assets.path)")

/// SVG path data for `path`, flipped to SVG's y-down coordinates.
func svgPathData(_ path: NSBezierPath) -> String {
    func point(_ p: NSPoint) -> String { String(format: "%.1f %.1f", p.x, canvas - p.y) }
    var data = "", points = [NSPoint](repeating: .zero, count: 3)
    for i in 0..<path.elementCount {
        switch path.element(at: i, associatedPoints: &points) {
        case .moveTo: data += "M\(point(points[0]))"
        case .lineTo: data += "L\(point(points[0]))"
        case .cubicCurveTo: data += "C\(point(points[0])) \(point(points[1])) \(point(points[2]))"
        case .quadraticCurveTo: data += "Q\(point(points[0])) \(point(points[1]))"
        case .closePath: data += "Z"
        @unknown default: fatalError("Unknown path element")
        }
    }
    return data
}

// With a second argument, also write the panther as a one-color SVG (panther.svg) for the UI,
// where CSS masks it in a theme color.
if CommandLine.arguments.count > 2 {
    let svgOutput = URL(fileURLWithPath: CommandLine.arguments[2])
    try FileManager.default.createDirectory(at: svgOutput, withIntermediateDirectories: true)
    let svg = """
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 \(Int(canvas)) \(Int(canvas))">\
        <path fill-rule="evenodd" d="\(svgPathData(pantherPath()))"/></svg>

        """
    try svg.write(to: svgOutput.appendingPathComponent("panther.svg"), atomically: true, encoding: .utf8)
    print("Wrote panther.svg to \(svgOutput.path)")
}
