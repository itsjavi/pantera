#!/usr/bin/env swift
// Draws the layer of Pantera's app icon (AppIcon.icon/Assets/head.png) on a 1024 pt canvas: a
// one-color panther (DECISION-10).
// Usage: swift scripts/make-icon.swift branding/AppIcon.icon [branding/overlay]
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

/// A square of the canvas, in y-down coordinates, drawn into a `size` × `size` box.
struct Frame {
    var x: CGFloat = 0, y: CGFloat = 0, side: CGFloat = canvas, size: CGFloat = canvas

    func map(_ p: NSPoint) -> NSPoint {
        NSPoint(x: (p.x - x) / side * size, y: (canvas - p.y - y) / side * size)
    }
}

/// The elements of `path` with their points mapped into `frame` (y down).
func elements(_ path: NSBezierPath, _ frame: Frame) -> [(NSBezierPath.ElementType, [NSPoint])] {
    var points = [NSPoint](repeating: .zero, count: 3)
    return (0..<path.elementCount).map { i in
        let type = path.element(at: i, associatedPoints: &points)
        return (type, points.map(frame.map))
    }
}

/// SVG path data for `path` in `frame`.
func svgPathData(_ path: NSBezierPath, _ frame: Frame = Frame(), digits: Int = 1) -> String {
    func point(_ p: NSPoint) -> String { String(format: "%.\(digits)f %.\(digits)f", p.x, p.y) }
    return elements(path, frame).map { type, p in
        switch type {
        case .moveTo: return "M\(point(p[0]))"
        case .lineTo: return "L\(point(p[0]))"
        case .cubicCurveTo: return "C\(point(p[0])) \(point(p[1])) \(point(p[2]))"
        case .quadraticCurveTo: return "Q\(point(p[0])) \(point(p[1]))"
        case .closePath: return "Z"
        @unknown default: fatalError("Unknown path element")
        }
    }.joined()
}

/// Chromium's vector icon commands (.icon files) for `path` in `frame`. Their default fill rule
/// is even-odd, like the panther's.
func iconCommands(_ path: NSBezierPath, _ frame: Frame) -> String {
    func point(_ p: NSPoint) -> String { String(format: "%.2ff, %.2ff", p.x, p.y) }
    return elements(path, frame).compactMap { type, p in
        switch type {
        case .moveTo: return "MOVE_TO, \(point(p[0])),"
        case .lineTo: return "LINE_TO, \(point(p[0])),"
        case .cubicCurveTo: return "CUBIC_TO, \(point(p[0])), \(point(p[1])), \(point(p[2])),"
        case .closePath: return "CLOSE,"
        default: fatalError("Unsupported path element")
        }
    }.joined(separator: "\n") + "\n"
}

let glyphFrame = Frame(x: 175, y: 60, side: 760, size: 24)

/// The panther's head with the eye cut out, framed as a bust and clipped to `glyphFrame`: the
/// glyph that replaces Chromium's logo in small icons. The whiskers and pupil are too fine for
/// 16–24 px.
func glyphPath() -> NSBezierPath {
    let path = NSBezierPath()
    path.windingRule = .evenOdd
    path.append(headPath())
    path.append(eyePath())
    let frame = CGPath(rect: CGRect(x: glyphFrame.x, y: canvas - glyphFrame.y - glyphFrame.side,
                                    width: glyphFrame.side, height: glyphFrame.side), transform: nil)
    let clipped = NSBezierPath(cgPath: path.cgPath.intersection(frame, using: .evenOdd))
    clipped.windingRule = .evenOdd
    return clipped
}

/// Fills `path` (canvas coordinates) into a `size`-pixel square PNG, one color.
func glyphPNG(_ size: Int, _ hex: String) -> Data {
    let rep = NSBitmapImageRep(
        bitmapDataPlanes: nil, pixelsWide: size, pixelsHigh: size, bitsPerSample: 8, samplesPerPixel: 4,
        hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
    // Canvas (y up) to the glyph's frame in pixels (y up).
    let scale = CGFloat(size) / glyphFrame.side
    var transform = AffineTransform(translationByX: -glyphFrame.x, byY: -(canvas - glyphFrame.y - glyphFrame.side))
    transform.append(AffineTransform(scaleByX: scale, byY: scale))
    let path = glyphPath()
    path.transform(using: transform)
    color(hex).setFill()
    path.fill()
    NSGraphicsContext.restoreGraphicsState()
    return rep.representation(using: .png, properties: [:])!
}

/// Settings' toolbar logo in dark mode, in the dark app icon's colors (icon.json).
func darkLogoSVG() -> String {
    """
    <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">\
    <defs><linearGradient id="b" x1="0.3" y1="0" x2="0.7" y2="1">\
    <stop offset="0" stop-color="#1E2024"/><stop offset="1" stop-color="#08090A"/></linearGradient>\
    <linearGradient id="p" x1="0" y1="0" x2="0" y2="1">\
    <stop offset="0" stop-color="#EEF0F3"/><stop offset="1" stop-color="#AEB4BD"/></linearGradient>\
    <clipPath id="c"><rect x="1" y="1" width="22" height="22" rx="5"/></clipPath></defs>\
    <rect x="1" y="1" width="22" height="22" rx="5" fill="url(#b)"/>\
    <path clip-path="url(#c)" fill="url(#p)" fill-rule="evenodd" \
    d="\(svgPathData(pantherPath(), Frame(x: -43, y: -43, side: 1110, size: 24), digits: 2))"/></svg>

    """
}

// With a second argument (branding/overlay), also write the panther for Chromium's UI
// (DECISION-10): the New Tab page's art, the product glyph in vector icons and favicons, and the
// dark Settings logo. branding/panther-glyph.svg is the glyph for the WebUI icon set patch.
if CommandLine.arguments.count > 2 {
    let overlay = URL(fileURLWithPath: CommandLine.arguments[2])
    func write(_ data: Data, _ path: String) throws {
        let url = overlay.appendingPathComponent(path)
        try FileManager.default.createDirectory(at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
        try data.write(to: url)
    }
    func svg(_ path: NSBezierPath, _ frame: Frame, digits: Int = 1) -> String {
        """
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 \(Int(frame.size)) \(Int(frame.size))">\
        <path fill-rule="evenodd" d="\(svgPathData(path, frame, digits: digits))"/></svg>

        """
    }

    try write(Data(svg(pantherPath(), Frame()).utf8),
              "chrome/browser/resources/new_tab_page_third_party/panther.svg")

    let icon = """
        // Pantera's panther glyph (DECISION-10), generated by scripts/make-icon.swift.

        CANVAS_DIMENSIONS, 24,
        \(iconCommands(glyphPath(), glyphFrame))
        """
    for path in [
        "components/omnibox/browser/vector_icons/chrome_product.icon",
        "components/omnibox/browser/vector_icons/product_chrome_refresh_old.icon",
        "chrome/app/vector_icons/chrome_product.icon",
        "components/vector_icons/chromium/product.icon",
        "components/vector_icons/chromium/product_refresh.icon",
    ] { try write(Data(icon.utf8), path) }

    try write(Data(darkLogoSVG().utf8), "ui/webui/resources/images/chrome_logo_dark.svg")

    // Favicons of the New Tab page (gray, like Chromium's) and of Chromium's own pages (the
    // amber of scripts/recolor-favicons.swift).
    for (scale, size) in [("default_100_percent", 16), ("default_200_percent", 32)] {
        try write(glyphPNG(size, "#7A7A7A"), "chrome/app/theme/\(scale)/common/favicon_ntp.png")
        try write(glyphPNG(size, "#B37800"), "components/resources/\(scale)/chromium/favicon_product.png")
    }

    let branding = URL(fileURLWithPath: iconOutput).deletingLastPathComponent()
    try Data(svg(glyphPath(), glyphFrame, digits: 2).utf8)
        .write(to: branding.appendingPathComponent("panther-glyph.svg"))
    print("Wrote the panther's UI assets to \(overlay.path)")
}
